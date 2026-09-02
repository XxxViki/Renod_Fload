# -*- coding: utf-8 -*-
# 通用 UART TX DMA 仿真钩子（挂 System bus 写前钩子：SetHookBeforePeripheralWrite）
# ----------------------------------------------------------------------------
# 【为什么 TX 不能照搬 RX 的"请求线 + AddWatchpointHook"三步模板】
#   Renode 的 UART.STM32F7_USART 模型【只有 ReceiveDmaRequest 线，没有
#   TransmitDmaRequest 线】，且 TXE 恒为 1、写 TDR(基址+0x28) 即直接发送。
#   所以 TX 方向没有"每字节请求线"可挂，必须【主动搬运】：一旦 DMA 通道使能，
#   就把内存里的整块数据搬进 TDR。好在 TXE 恒 1（无背压），可以一次性搬完。
#
# 【触发点为什么是 CR3 写】HAL_UART_Transmit_DMA 的启动顺序是：
#   先 HAL_DMAEx_List_Start_IT 写 CCR.EN（通道使能，同时使能 TCIE）
#   → 再 ATOMIC_SET_BIT(CR3, DMAT)（串口基址+0x08，bit7）。
#   所以挂在"CR3 写之前"时 CCR.EN 已置好，回读链表节点即可拿到完整配置。
#
# 【为什么用 SetHookBeforePeripheralWrite（写前）而不是写后】
#   Renode 1.16.1 的 sysbus 监控命令只提供写前/读后两种：
#     sysbus SetHookBeforePeripheralWrite <外设> "<脚本>"
#     sysbus SetHookAfterPeripheralRead   <外设> "<脚本>"
#   （SetHookAfterPeripheralWrite 是更高版本才加入的。）
#   写前钩子作用域自带 self/sysbus/machine/value/offset（已核对源码
#   BusPeripheralsHooksPythonEngine.cs），value 是即将写入的完整 CR3 值，
#   用 bit7(DMAT) 过滤：置位 = TX 启动；清零（发完回读清 DMAT）= 忽略。
#   写断点（AddWatchpointHook）的 value 恒 0（IronPython 装箱坑），不能用于
#   位过滤，所以这里必须用 bus 钩子而不是 watchpoint。
#
# 【挂载方式（每个用 TX DMA 的串口挂一条，UART_BASE 注入本串口基址）】
#   sysbus SetHookBeforePeripheralWrite sysbus.<uart>
#       "with open(r'E:/Project/Flod_Array/Renode/uart_tx_dma_hook.py') as _f: exec 'UART_BASE=0x<串口基址>' + chr(10) + _f.read()"
#   （必须 with-open：裸 open().read() 的读取器会被 GC 中途回收——坑9，
#    UART4 TX 曾因此"发几次就崩"，与 RX 钩子同款修法）
#   （脚本内部按 offset==0x08 且 DMAT 置位过滤；找不到 UART_BASE 时退化为
#    "节点 CDAR 指向非 SRAM = TX 通道"的启发式定位。）
#
# 【通道定位：扫链表节点，不是扫通道寄存器】
#   HAL_UART_Transmit_DMA 只把长度/源地址/TDR 地址写进【链表节点内存】
#   （节点布局：CTR1@+0x00 CTR2@+0x04 CBR1@+0x08 CSAR@+0x0C CDAR@+0x10），
#   List_Start_IT 只写 CLBAR/CLLR/CCR —— 通道的 CSAR/CDAR/CBR1/CTR1 要等
#   硬件"装载节点"才有值，DMA stub 不模拟装载。所以定位/配置一律读节点：
#   节点地址 = (CLBAR & 0xFFFF0000) + ((CLLR.LA) >> 2)，本串口 TX 通道的
#   判据是节点 CDAR == UART_BASE + 0x28（TDR）。方向位 CTR1.DAP(bit30)
#   也只在节点里（通道 CTR1 从未装载，按通道扫描永远找不到 TX 通道）。
#
# 【TC 中断怎么触发】
#   单次链表搬完后，固件 HAL_DMA_IRQHandler 依赖"通道 TC 中断"回调
#   UART_DMATransmitCplt（把 gState 恢复 READY + 调 HAL_UART_TxCpltCallback）。
#   但 Python 写 NVIC ISPR 只置位不通知 CPU（见 stm32u5_lpdma_stub.py 坑 6），
#   所以这里用 Miscellaneous.Button 走模型真实中断线：
#     .repl 里  tx_btn_<irq>: Miscellaneous.Button @ sysbus  ->  nvic0@<irq>
#     脚本里  machine["sysbus.tx_btn_<irq>"].PressAndRelease()
#   每个 TX 通道配一个 button（当前仅 UART4_TX=GPDMA1 CH2=IRQ31 一个）。
# ----------------------------------------------------------------------------
GPDMA1 = 0x40020000
LPDMA1 = 0x46025000

# 通道内偏移（与 uart_dma_hook.py / stm32u5_lpdma_stub.py 一致）
O_CLBAR = 0x00
O_CSR = 0x10
O_CCR = 0x14
O_CBR1 = 0x48
O_CSAR = 0x4C
O_CDAR = 0x50
O_CLLR = 0x7C

CCR_EN = 0x1                 # CCR bit0
CSR_TCF = 0x100              # CSR bit8：传输完成标志
CTR1_SINC = 0x8              # CTR1 bit3：源地址递增
CTR1_DINC = 0x80000          # CTR1 bit19：目标地址递增

# 链表节点内偏移（HAL DMA_NodeTypeDef：CTR1@0 CTR2@4 CBR1@8 CSAR@0xC CDAR@0x10）
N_CTR1 = 0x00
N_CBR1 = 0x08
N_CSAR = 0x0C
N_CDAR = 0x10


def ch_base(ctrl, ch):
    """通道寄存器块基址：0x50 + ch*0x80"""
    return ctrl + 0x50 + ch * 0x80


def irq_of_ch(ctrl, ch):
    """通道号 -> IRQ 号（与 uart_dma_hook.py 的 dma_of_irq 互为反函数）"""
    if ctrl == GPDMA1:
        if ch < 8:
            return 29 + ch          # GPDMA1 CH0-7  = IRQ29-36
        return 80 + (ch - 8)        # GPDMA1 CH8-15 = IRQ80-87
    if ctrl == LPDMA1:
        return 114 + ch             # LPDMA1 CH0-3  = IRQ114-117
    return None


def node_of(sb, b):
    """通道块基址 -> 链表节点地址（读 CLBAR/CLLR，未链接返回 None）"""
    clbar = sb.ReadDoubleWord(b + O_CLBAR)
    cllr = sb.ReadDoubleWord(b + O_CLLR)
    la = (cllr >> 2) & 0x3FFF
    node = (clbar & 0xFFFF0000) + (la << 2)
    return node if node else None


def in_sram(addr):
    """地址是否落在 SRAM 区（RX 节点的 CDAR 是缓冲地址，TX 的是外设 TDR）"""
    return 0x20000000 <= addr < 0x20100000 or 0x28000000 <= addr < 0x28004000


def tx_find(sb):
    """扫描已使能(EN)通道，按链表节点找本串口的 TX 通道。
    判据（优先）：节点 CDAR == UART_BASE+0x28(TDR)；
    无 UART_BASE 时退化为：节点 CDAR 指向非 SRAM（外设）即视为 TX 通道。
    注意必须读【节点】而不是通道寄存器——通道的 CSAR/CDAR/CTR1 未装载。"""
    tdr = None
    try:
        tdr = UART_BASE + 0x28
    except NameError:
        tdr = None
    for ctrl in (GPDMA1, LPDMA1):
        for ch in range(16 if ctrl == GPDMA1 else 4):
            b = ch_base(ctrl, ch)
            if not (sb.ReadDoubleWord(b + O_CCR) & CCR_EN):
                continue
            node = node_of(sb, b)
            if node is None:
                continue
            cdar = sb.ReadDoubleWord(node + N_CDAR)
            if tdr is not None:
                if cdar == tdr:
                    return ctrl, ch, node
            elif not in_sram(cdar):
                return ctrl, ch, node
    return None, None, None


def tx_run(sb, ctrl, ch, node):
    """TX 搬运主流程：读节点配置 -> 逐字节搬内存->TDR -> 置 TCF -> 清 EN。

    真实硬件里每字节要等 TXE 置位（外设请求），但模型 TXE 恒为 1，写 TDR 即
    发送，所以无背压、可一次性把整块搬完。写 TDR 必须用 32 位访问（模型寄存器
    是 DoubleWord 型，字节写会走总线读改写路径、误读 RDR 清 RXNE）。"""
    b = ch_base(ctrl, ch)
    ctr1 = sb.ReadDoubleWord(node + N_CTR1)
    cbr1 = sb.ReadDoubleWord(node + N_CBR1)
    bndt = cbr1 & 0xFFFF
    if bndt == 0:
        return
    sar = sb.ReadDoubleWord(node + N_CSAR)
    dar = sb.ReadDoubleWord(node + N_CDAR)
    sinc = 1 if (ctr1 & CTR1_SINC) else 0
    dinc = 1 if (ctr1 & CTR1_DINC) else 0
    for _ in range(bndt):
        sb.WriteDoubleWord(dar, sb.ReadByte(sar))   # 内存(节点CSAR) -> TDR(节点CDAR)
        sar += sinc
        dar += dinc
    # 通道寄存器同步到完成状态（供 HAL/调试观察）
    sb.WriteDoubleWord(b + O_CBR1, cbr1 & 0xFFFF0000)      # BNDT 清 0
    sb.WriteDoubleWord(b + O_CSAR, sar & 0xFFFFFFFF)
    sb.WriteDoubleWord(b + O_CDAR, dar & 0xFFFFFFFF)
    sb.WriteDoubleWord(b + O_CSR, CSR_TCF)                 # 置 TCF(bit8)
    # 真实硬件在装载/执行节点时会把节点的 CLLR 字段写回通道 CLLR：单节点（末端）
    # 的 CLLR=0。HAL_DMA_IRQHandler 对链表模式要求 "CLLR==0 且 CBR1==0" 才把
    # hdma->State 恢复 READY；stub 不模拟这一步会让 State 卡 BUSY，导致下一帧
    # HAL_DMAEx_List_Start_IT 返回 HAL_ERROR。
    sb.WriteDoubleWord(b + O_CLLR, sb.ReadDoubleWord(node + 0x1C))
    ccr = sb.ReadDoubleWord(b + O_CCR)
    sb.WriteDoubleWord(b + O_CCR, ccr & ~CCR_EN)           # 单次传输完自动禁能
    return


def get_peripheral(machine, name):
    """多路 fallback 拿外设（machine 索引器 / GetPeripheralByName / ...）"""
    for getter in (
        lambda: machine[name],
        lambda: machine.GetPeripheralByName(name),
        lambda: machine.TryGetByName(name),
        lambda: machine.SystemBus.GetPeripheralByName(name),
    ):
        try:
            p = getter()
            if p is not None:
                return p
        except Exception:
            continue
    return None


def tx_trigger_tc(machine, ctrl, ch):
    """用 button 走模型真实中断线，触发 TX 通道的 TC 中断"""
    irq = irq_of_ch(ctrl, ch)
    if irq is None:
        return
    btn = get_peripheral(machine, "sysbus.tx_btn_%d" % irq)
    if btn is None:
        return
    try:
        btn.PressAndRelease()
    except Exception:
        pass


# ---- 入口：bus 写前钩子作用域有 self/sysbus/machine/value/offset ----
try:
    _off = int(offset)
    _val = int(value)
except NameError:
    _off = None
    _val = 0
# 只认 CR3 写（offset==0x08）且 DMAT(bit7) 置位：TX DMA 启动瞬间。
# （RX 启动写 DMAR=bit6 不匹配；发完 HAL 清 DMAT 时 bit7=0 也不匹配。）
if _off is not None and _off == 0x08 and (_val & 0x80):
    _ctrl, _ch, _node = tx_find(sysbus)
    if _ctrl is not None:
        tx_run(sysbus, _ctrl, _ch, _node)
        try:
            _nobtn = TX_NO_BUTTON     # 调试开关：挂载脚本注入 True 则不触发 TC
        except NameError:
            _nobtn = False
        if not _nobtn:
            tx_trigger_tc(machine, _ctrl, _ch)
