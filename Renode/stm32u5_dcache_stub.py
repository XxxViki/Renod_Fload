# -*- coding: utf-8 -*-
# STM32U5 DCACHE1 打桩脚本（Renode 1.16 PythonPeripheral，request 风格）
# 读写直通（repeater）：HAL_DCACHE_Init/Enable 会读 DCACHE_SR 的
# BUSYF 等待"不忙"、回读 CR 确认，直通存储即可全部通过（SR 初始 0 = 不忙）。
try:
    regs
except NameError:
    regs = {}

if request.IsWrite:
    regs[request.Offset] = request.Value
elif request.IsRead:
    request.Value = regs.get(request.Offset, 0) & 0xFFFFFFFF
