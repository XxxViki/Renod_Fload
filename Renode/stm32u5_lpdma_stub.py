# -*- coding: utf-8 -*-
# STM32U5 LPDMA1 打桩脚本（Renode 1.16 PythonPeripheral，request 风格）
# ----------------------------------------------------------------------------
# 背景：Renode 没有 LPDMA1/GPDMA1 模型（STM32WBA55_GPDMA 的请求线被
#       monitoredFIFOlevel==0 拦死、链表模式是 TODO），本 stub 与
#       lpuart1_lpdma_hook.py 配合，在仿真里还原 LPDMA1 CH0 的
#       "每字节请求搬运 + 循环链表回卷 + 通道中断" 行为：
#
#   触发：lpuart1 的 ReceiveDmaRequest GPIO 线（CR3.DMAR 使能且 RDR 非空）
#         —— 该线同时接到 nvic0@114（LPDMA1_Channel0_IRQn），每字节原生挂起中断；
#         GPIO 上的 AddStateChangedHook 钩子脚本负责真正的数据搬运，并把
#         CDAR/CBR1/CSR 标志通过总线写回本 stub。
#
# 本 stub 关键行为（LPDMA1 @ 0x46025000，4 通道，通道步进 0x80，通道基址 +0x50）：
#   CLBAR/+0x00 CFCR/+0x0C CSR/+0x10 CCR/+0x14 CTR1/+0x40 CTR2/+0x44
#   CBR1/+0x48 CSAR/+0x4C CDAR/+0x50 CLLR/+0x7C；控制器级 MISR/+0x0C
#   - CSR  读：IDLEF(bit0)/TCF(bit8)/HTF(bit9) 由内部 flags 计算（真实 HW 只读）
#   - CSR  写：仅钩子使用——置位 TCF/HTF（HAL 不会写 CSR，无副作用）
#   - CFCR 写：写 1 清标志（W1C，HAL_DMA_IRQHandler 用）
#   - CCR  写：RESET(bit1)/SUSP(bit2) 为命令位写后自清（驻留会把 EN 清没，坑2）
#   - MISR 读：按各通道 TCF&&TCIE 实时计算（HAL_DMA_IRQHandler 的第一道门，
#              恒 0 会让所有 TC 中断被静默丢弃，坑10）
#   - 0x3F0/0x3F4/0x3F8：钩子的回卷缓存暂存区（CDAR/BNDT/有效标志，坑10）
#   - 其余寄存器读写直通存取（HAL_DMAEx_List_* 与钩子共同维护）
#
# ============================================================================
# 【问题总结】LPUART1 经 LPDMA1 接收仿真的全部踩坑（2026-09-02，与笔记 §12.5 同步）
# ----------------------------------------------------------------------------
# 1. 模型选型：Renode 无 GPDMA/LPDMA 模型；DMA.STM32DMA(F7 stream 型)寄存器
#    布局与 U5 完全不兼容（张冠李戴）；STM32WBA55_GPDMA 布局兼容但行为不可用
#    （EN 即整块同步搬运、请求线被 FIFOL==0 拦死、链表 TODO）
#    → 结论：request 风格 stub（本文件）+ GPIO 钩子（lpuart1_lpdma_hook.py）自建。
# 2. stub 的 CCR.RESET 位驻留：HAL 复位通道后 RESET=1 一直存着，后续
#    "CCR|=EN" 的读改写又触发"RESET 清 EN"→ EN 永远起不来
#    → RESET/SUSP 按真硬件"写后自清"处理。
# 3. 启动竞态：数据先于 CR3.DMAR 到达 → ReceiveDmaRequest 电平线错过跳变
#    → 字节滞留 RDR、RXNE 悬死 → HAL"DMA 模式"不读 RDR → 中断风暴
#    → CR3 写断点看门狗强制搬走滞留字节（钩子 on_cr3_write）。
# 4. 固件错误回调回退 Receive_IT 而 CR3.DMAR 残留 → 必然风暴
#    → ErrorCallback 一律重启 DMA 接收，绝不回退 IT。
# 5. IDLE 空闲线走不通：F7 模型的 ISR.IDLE/CR1.IDLEIE 是 WithTaggedFlag 死位
#    （写丢弃、读恒 0），HAL 置 IDLEIE 后回读仍 0，空闲分支永远进不去
#    → 帧结束改用 RTOF 接收超时（RTOR/RTOF/RTOIE 模型原生实现、真机同款），
#      固件在 LPUART1_IRQHandler 的 USER CODE 段自行处理。
# 6. NVIC 注入走不通：Python 写 ISPR 只置位不通知 CPU；反射调 OnGPIO
#    挂起被取走但处理函数不执行 → 帧结束事件必须走模型真实中断线。
# 7. TCP 终端 telnetMode=true 会先发 Telnet 协商字节，串口助手显示乱码
#    → CreateServerSocketTerminal 第 3 参数用 false（raw 字节流）。
# 8. 回显逐次累积：长度="缓冲全长-DMA计数器"是累计值不是本帧长度
#    → 固件改为读写指针追踪（rd/wr，回卷时分两段发送）。
# 9. 崩溃 "Cannot access a closed file"：exec(open().read()) 的读取器是无引用
#    临时对象，钩子嵌套重入时被 GC 中途回收 → 挂载语句改
#    with open(...) as _f: exec _f.read()（强引用+确定关闭）。
# 10. 块边界回卷卡死（累计接收=缓冲全长时）：通道 CLLR 运行中莫名变 0
#     → 节点重载判"非链表"放弃 → BNDT 卡 0、字节滞留、请求线悬高 →
#     IRQ114 风暴；且 HAL_DMA_IRQHandler 入口读 MISR(+0x0C)，本 stub 恒 0
#     → 处理函数早退、中断源头永不清 → 最终 PC 飞进数据区(CPU abort)
#     → 修复：本 stub 实现 MISR（TCF&&TCIE 置位）+ 钩子回卷缓存兜底
#       （首次节点装载成功后把 CDAR/BNDT 存到本 stub 的 0x3F0/0x3F4/0x3F8）。
#     注意：调大缓冲只是推迟触发点（累计满 1024 一样炸），不是修复。
# 11. 钩子/断点脚本注意：try/except 吞错=静默失败（引擎本可打日志）；
#     写断点作用域的 value 变量实测恒 0（IronPython 装箱问题），判断一律
#     用寄存器回读；Machine.ScheduleAction 的回调签名是 Action[TimeInterval]。
# 12. 测试侧：Write To Uart 会自动补换行；Robot 的 Evaluate 里变量带换行会
#     破坏表达式（先 strip / 用 $obj 传对象）。
# 13. 跨块帧回显被拆分：TC（块完成）中断常晚于 RTOF 处理，RxCpltCallback 里
#     回显会提前消费读指针，把跨块帧拆成两段（数据没丢，帧序被打乱）
#     → 循环模式下 RxCpltCallback 不回显，统一由 RTOF 按帧处理。
# 14. 【坑11·致命】钩子字符串绝不能"包函数再调用"（exec chr(10).join(
#     ('def _x():','  ...exec _f.read()','_x()')) 之类）：IronPython 函数内嵌
#     exec 看不到 Renode 注入的 state/address/value/self，入口的 NameError
#     分支全部落空 → 钩子静默 no-op（无任何报错日志！）→ RDR 无人读、
#     ReceiveDmaRequest 线悬高 → 通道 IRQ 风暴饿死主循环：三口零回显、
#     HAL_Delay 卡死（uwTick 停走）、PC 跑飞(0xFFFFFFA8)、GDB 拒连，全同一根因。
#     正确形式：直接 "with open(r'...') as _f: exec _f.read()"；异常防护写进
#     脚本模块级整体 try/except（uart_dma_hook.py 入口已内置），不靠外层包裹。
# 15. ServerSocketTerminal 每端口只服务一个客户端：已有连接时，后续连接被
#     无视且会把监听搞挂（再连一律 ConnectionRefused）。串口助手/PuTTY 必须
#     是该端口唯一客户端；自动化测试要复用同一条连接发收全部流量。
# ============================================================================
# ----------------------------------------------------------------------------
try:
    regs
except NameError:
    regs = {}
try:
    dma_flags
except NameError:
    dma_flags = {}

# 通道内偏移 → 名称（相对通道基址 +0x50 + ch*0x80）
CH_OFF = {0x00: 'CLBAR', 0x0C: 'CFCR', 0x10: 'CSR', 0x14: 'CCR',
          0x40: 'CTR1', 0x44: 'CTR2', 0x48: 'CBR1', 0x4C: 'CSAR',
          0x50: 'CDAR', 0x7C: 'CLLR'}


def ch_of(off):
    # 返回 (ch, 通道内偏移)；不在任何通道范围则 (None, off)
    if off >= 0x50:
        ch = (off - 0x50) // 0x80
        inner = (off - 0x50) % 0x80
        if ch < 16 and inner in CH_OFF:
            return ch, inner
    return None, off


def flags(ch):
    if ch not in dma_flags:
        dma_flags[ch] = {'tc': False, 'ht': False, 'idle': False}
    return dma_flags[ch]


if request.IsWrite:
    off = request.Offset
    ch, inner = ch_of(off)
    if ch is not None and inner == 0x0C:            # CFCR：W1C
        f = flags(ch)
        if request.Value & (1 << 8):
            f['tc'] = False
        if request.Value & (1 << 9):
            f['ht'] = False
        if request.Value & 0x1:
            f['idle'] = False
    elif ch is not None and inner == 0x10:          # CSR：钩子置标志（仿真内部接口）
        f = flags(ch)
        if request.Value & (1 << 8):
            f['tc'] = True
        if request.Value & (1 << 9):
            f['ht'] = True
        if request.Value & 0x1:
            f['idle'] = True
    elif ch is not None and inner == 0x14:          # CCR：RESET/SUSP 为命令位，
        v = request.Value & 0xFFFFFFF9              # 真硬件写后自清，不驻留
        if request.Value & 0x2:                     # RESET：通道复位清 EN
            v &= ~0x1
        regs[off] = v
    else:                                           # 其余直通存
        regs[off] = request.Value & 0xFFFFFFFF

elif request.IsRead:
    off = request.Offset
    ch, inner = ch_of(off)
    val = regs.get(off, 0)
    if off == 0x0C:                                  # MISR：通道"使能且激活"的标志摘要
        val = 0                                      # （HAL_DMA_IRQHandler 入口先查它，
        for c in range(16):                          #   为 0 直接返回——之前恒 0 导致
            f = flags(c)                             #   TC 中断被静默丢弃）
            ccr = regs.get(0x50 + c * 0x80 + 0x14, 0)
            if f['tc'] and (ccr & 0x100):
                val |= (1 << c)
    elif ch is not None and inner == 0x10:           # CSR：实时计算标志
        f = flags(ch)
        val = 0
        if f['idle']:
            val |= 0x1
        if f['tc']:
            val |= (1 << 8)
        if f['ht']:
            val |= (1 << 9)
        # IDLEF(bit0) TCF(bit8) HTF(bit9) DTEF(bit10) ... TOF(bit14)
    request.Value = val & 0xFFFFFFFF
