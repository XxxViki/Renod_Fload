# -*- coding: utf-8 -*-
# 通用 UART RX DMA 仿真钩子（挂在各串口的 ReceiveDmaRequest GPIO 线上）
# ----------------------------------------------------------------------------
# 【问题总结】本方案全部踩坑见 stm32u5_lpdma_stub.py 头部"问题总结"节（与笔记 §12.5 同步）
#
# 挂载方式（resc，每个用 DMA 收的串口各挂一条；CR3 看门狗同一段脚本）：
#   sysbus.<uart> ReceiveDmaRequest AddStateChangedHook
#       "with open(r'E:/Project/Flod_Array/Renode/uart_dma_hook.py') as _f: exec _f.read()"
#   sysbus AddWatchpointHook <uart_base+8> DoubleWord Write
#       "with open(r'E:/Project/Flod_Array/Renode/uart_dma_hook.py') as _f: exec _f.read()"
#
# 通用性：通道由 GPIO 端点接的 NVIC 中断号反推（GPDMA1: IRQ29-36=CH0-7、
# IRQ80-87=CH8-15；LPDMA1: IRQ114-117=CH0-3），源地址直接读通道 CSAR
# （它存的就是该串口 RDR 的地址）——一份脚本服务所有串口/通道。
#
# 每字节流程：钩子读通道寄存器 → 读 CSAR（RDR）弹出一字节（清 RXNE）→
#   写入 CDAR（缓冲）→ BNDT-1 → 块完成置 TCF 并回卷（节点重载 + 缓存兜底）。
# 帧结束事件：模型原生 RTOF（接收超时），固件在各 UART IRQ 的 USER 段处理。
# 每字节中断无需脚本：ReceiveDmaRequest 线在 repl 里直连 nvic@<通道IRQ>。
# ----------------------------------------------------------------------------
GPDMA1 = 0x40020000
LPDMA1 = 0x46025000


def dma_of_irq(irq):
    """IRQ 号 → (控制器基址, 通道号)"""
    if 29 <= irq <= 36:
        return (GPDMA1, irq - 29)          # GPDMA1 CH0-7
    if 80 <= irq <= 87:
        return (GPDMA1, irq - 80 + 8)      # GPDMA1 CH8-15
    if 114 <= irq <= 117:
        return (LPDMA1, irq - 114)         # LPDMA1 CH0-3
    return (None, None)


def ch_base(ctrl, ch):
    """通道寄存器块基址"""
    return ctrl + 0x50 + ch * 0x80

# 通道内偏移
O_CLBAR = 0x00
O_CFCR = 0x0C
O_CSR = 0x10
O_CCR = 0x14
O_CTR1 = 0x40
O_CTR2 = 0x44
O_CBR1 = 0x48
O_CSAR = 0x4C
O_CDAR = 0x50
O_CLLR = 0x7C


def scr(ctrl, ch):
    """回卷缓存暂存区（借 stub 闲置寄存器区，每通道 3 字：CDAR/BNDT/有效）"""
    b = ctrl + 0x300 + ch * 12
    return (b, b + 4, b + 8)


def dma_reload(sb, ctrl, ch):
    """块完成后按链表节点镜像回卷通道寄存器（单节点循环=回到起点）"""
    b = ch_base(ctrl, ch)
    cllr = sb.ReadDoubleWord(b + O_CLLR)
    la = (cllr >> 2) & 0x3FFF
    ut = ((cllr >> 31) & 1) | ((cllr >> 30) & 1) | ((cllr >> 29) & 1) \
       | ((cllr >> 28) & 1) | ((cllr >> 27) & 1) | ((cllr >> 16) & 1)
    if la == 0 and ut == 0:
        return False    # 未用链表（普通模式）：不回卷
    clbar = sb.ReadDoubleWord(b + O_CLBAR)
    node = (clbar & 0xFFFF0000) + (la << 2)
    # 节点镜像 LinkRegisters[8]：CTR1@0 CTR2@4 CBR1@8 CSAR@0xC CDAR@0x10 (CTR3/CBR2) CLLR@0x1C
    if (cllr >> 31) & 1:
        sb.WriteDoubleWord(b + O_CTR1, sb.ReadDoubleWord(node + 0x00))
    if (cllr >> 30) & 1:
        sb.WriteDoubleWord(b + O_CTR2, sb.ReadDoubleWord(node + 0x04))
    if (cllr >> 29) & 1:
        sb.WriteDoubleWord(b + O_CBR1, sb.ReadDoubleWord(node + 0x08))
    if (cllr >> 28) & 1:
        sb.WriteDoubleWord(b + O_CSAR, sb.ReadDoubleWord(node + 0x0C))
    if (cllr >> 27) & 1:
        sb.WriteDoubleWord(b + O_CDAR, sb.ReadDoubleWord(node + 0x10))
    if (cllr >> 16) & 1:
        sb.WriteDoubleWord(b + O_CLLR, sb.ReadDoubleWord(node + 0x1C))
    return True


def dma_wrap(sb, ctrl, ch):
    """块完成：置 TCF + 节点重载；重载无效时用首次装载的缓存兜底。
    背景：实测固件运行中通道 CLLR 会莫名变 0（节点重载判'非链表'放弃），
    导致 BNDT 卡 0、字节滞留 RDR、请求线悬高、通道 IRQ 风暴直至固件跑飞。"""
    b = ch_base(ctrl, ch)
    s_cd, s_bn, s_v = scr(ctrl, ch)
    sb.WriteDoubleWord(b + O_CSR, 1 << 8)               # TCF
    dma_reload(sb, ctrl, ch)
    if (sb.ReadDoubleWord(b + O_CBR1) & 0xFFFF) == 0 \
       and sb.ReadDoubleWord(s_v) == 1:
        sb.WriteDoubleWord(b + O_CBR1, sb.ReadDoubleWord(s_bn))
        sb.WriteDoubleWord(b + O_CDAR, sb.ReadDoubleWord(s_cd))


def dma_serve(sb, ctrl, ch):
    """搬运一个字节 + 维护通道状态（核心逻辑，只依赖 sysbus）"""
    b = ch_base(ctrl, ch)
    ccr = sb.ReadDoubleWord(b + O_CCR)
    if not (ccr & 0x1):              # EN=0：DMA 未启动，字节留给固件 IT/轮询
        return
    cbr1 = sb.ReadDoubleWord(b + O_CBR1)
    bndt = cbr1 & 0xFFFF
    s_cd, s_bn, s_v = scr(ctrl, ch)
    if bndt == 0:                    # 未装载/回卷后：节点重载 + 缓存兜底
        dma_reload(sb, ctrl, ch)
        if (sb.ReadDoubleWord(b + O_CBR1) & 0xFFFF) == 0 \
           and sb.ReadDoubleWord(s_v) == 1:
            sb.WriteDoubleWord(b + O_CBR1, sb.ReadDoubleWord(s_bn))
            sb.WriteDoubleWord(b + O_CDAR, sb.ReadDoubleWord(s_cd))
        cbr1 = sb.ReadDoubleWord(b + O_CBR1)
        bndt = cbr1 & 0xFFFF
        if bndt == 0:
            return
        if sb.ReadDoubleWord(s_v) == 0:   # 首次装载成功：缓存供回卷兜底
            sb.WriteDoubleWord(s_cd, sb.ReadDoubleWord(b + O_CDAR))
            sb.WriteDoubleWord(s_bn, bndt)
            sb.WriteDoubleWord(s_v, 1)
    rdr = sb.ReadDoubleWord(b + O_CSAR) & 0xFFFFFFFF     # 源=CSAR（该串口 RDR）
    data = sb.ReadDoubleWord(rdr) & 0xFF                 # 读 RDR：弹出字节+清 RXNE
    cdar = sb.ReadDoubleWord(b + O_CDAR)
    sb.WriteByte(cdar, data)
    newb = (bndt - 1) & 0xFFFF
    sb.WriteDoubleWord(b + O_CBR1, (cbr1 & 0xFFFF0000) | newb)
    sb.WriteDoubleWord(b + O_CDAR, (cdar + 1) & 0xFFFFFFFF)
    if newb == 0:                    # 块完成：置 TCF + 回卷（含缓存兜底）
        dma_wrap(sb, ctrl, ch)


def dma_machine(gpio):
    """反射取 machine（GPIOPythonEngine 作用域只有 self/state；NVIC.machine 私有）"""
    recv = gpio.Endpoints[0].Receiver
    from System.Reflection import BindingFlags
    fi = recv.GetType().GetField('machine',
                                 BindingFlags.NonPublic | BindingFlags.Instance)
    return fi.GetValue(recv)


def dma_on_gpio(gpio):
    """GPIO 请求线触发（每字节正常路径）：按端点中断号定位控制器/通道"""
    ep = gpio.Endpoints[0]
    ctrl, ch = dma_of_irq(int(ep.Number))   # 端点接的 NVIC 输入号 = 通道 IRQ
    if ctrl is None:
        return
    dma_serve(dma_machine(gpio).SystemBus, ctrl, ch)


def dma_on_cr3_write(sb, uart_base):
    """CR3 写看门狗：DMA 已使能且该串口 RDR 有滞留字节（启动竞态——数据先于
    DMAR 到达，请求线错过跳变）时强制搬走，避免 RXNE 悬死引发中断风暴。
    通道定位：扫描两个控制器的通道，找 CSAR==该串口 RDR 且 EN 的。"""
    rdr = uart_base + 0x24
    for ctrl in (GPDMA1, LPDMA1):
        for ch in range(16 if ctrl == GPDMA1 else 4):
            b = ch_base(ctrl, ch)
            if sb.ReadDoubleWord(b + O_CSAR) == rdr \
               and (sb.ReadDoubleWord(b + O_CCR) & 0x1) \
               and (sb.ReadDoubleWord(uart_base + 0x1C) & 0x20):   # RXNE
                dma_serve(sb, ctrl, ch)
                return


# ---- 入口：按作用域自动分发 ----
# GPIO 钩子作用域有 state/self(GPIO)；写断点作用域有 address/value/self(sysbus)
try:
    _gpio_state = state
except NameError:
    _gpio_state = False
if _gpio_state:
    dma_on_gpio(self)
try:
    _wp_value = value
except NameError:
    _wp_value = None
if _wp_value is not None:
    _wp_addr = int(address)
    if (_wp_addr & 0xFFF) == 0x008:        # 只认 CR3（串口基址+8）
        dma_on_cr3_write(self, _wp_addr - 8)
