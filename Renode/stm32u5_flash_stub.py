# -*- coding: utf-8 -*-
# STM32U5 FLASH 控制寄存器打桩脚本（Renode 1.16 PythonPeripheral，request 风格）
# 读写直通（repeater）：回读返回最后一次写入的值。
# 用途：RCC_SetFlashLatencyFromMSIRange / __HAL_FLASH_SET_LATENCY 写完
# FLASH_ACR (偏移 0x00) 后会回读校验 LATENCY 位，没有这个 stub 时
# SVD 恒返回 0，导致 HAL_RCC_OscConfig / HAL_RCC_ClockConfig 返回 HAL_ERROR。
try:
    regs
except NameError:
    regs = {}

if request.IsWrite:
    regs[request.Offset] = request.Value
elif request.IsRead:
    request.Value = regs.get(request.Offset, 0) & 0xFFFFFFFF
