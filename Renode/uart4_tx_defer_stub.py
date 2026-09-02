# -*- coding: utf-8 -*-
# UART4 TX DMA 延迟转投桩（挂在 SetHookBeforePeripheralWrite 上）
# ----------------------------------------------------------------------------
# 挂载：sysbus SetHookBeforePeripheralWrite sysbus.uart4
#     "with open(r'E:/Project/Flod_Array/Renode/uart4_tx_defer_stub.py') as _f: exec _f.read()"
#
# 为什么需要这个桩：真正的钩子（uart_tx_dma_hook.py）要在 CPU 写路径上做
# 文件读取+总线搬运，一旦 IDE 的 GDB 会话（halt/单步）恰好落在钩子执行中间，
# 机器会挂死甚至进程崩溃（笔记 §12.10，"一仿真运行就崩"的真凶）。
# 本桩把写路径上的工作缩到最小：只过滤 CR3.DMAT 置位并把真正的搬运
# ScheduleAction(+1us) 转投到机器调度线程执行——GDB halt CPU 不再能
# 落在钩子内部。真机不受任何影响（这是纯仿真侧的挂载方式）。
#
# 调度回调里的两个坑（实测）：
#   1. 变量必须注入 globals()['offset'] 等——函数内直接赋值落在局部作用域，
#      文件入口读不到；
#   2. 必须 exec _f.read() in globals()——普通 exec 时文件定义的常量
#      （GPDMA1 等）落在局部作用域，文件内部函数（tx_find）按全局查找
#      会 NameError: GPDMA1。
# ----------------------------------------------------------------------------
try:
    _off = int(offset)
    _val = int(value)
except NameError:
    _off = None
    _val = 0

if _off is not None and _off == 0x08 and (_val & 0x80):
    from Antmicro.Renode.Time import TimeInterval
    from System import Action

    def _tx_go(_t, _o=_off, _v=_val, _m=machine):
        try:
            globals()['offset'] = _o
            globals()['value'] = _v
            globals()['UART_BASE'] = 0x40004C00
            with open(r'E:/Project/Flod_Array/Renode/uart_tx_dma_hook.py') as _f:
                exec _f.read() in globals()
        except Exception as _e:
            print('TX_DEFER_GO ERR: %r' % _e)

    machine.ScheduleAction(TimeInterval.FromMicroseconds(1),
                           Action[TimeInterval](_tx_go))
