# -*- coding: utf-8 -*-
# STM32U5 PWR 打桩脚本（Renode 1.16 PythonPeripheral，request 风格）
# 位定义来自 CMSIS stm32u585xx.h：
#   PWR_VOSR  (偏移 0x0C): VOSRDY=15, BOOSTRDY=14, VOS=17:16
#   PWR_SVMSR (偏移 0x3C): ACTVOSRDY=15, ACTVOS=17:16
# HAL_PWREx_ControlVoltageScaling 依次等 VOSRDY 和 SVMSR.ACTVOSRDY，
# 并用 SVMSR.ACTVOS 判断当前档位 —— 读回时强制就绪、ACTVOS 镜像 VOSR.VOS。
try:
    regs
except NameError:
    regs = {}

if request.IsWrite:
    regs[request.Offset] = request.Value
elif request.IsRead:
    off = request.Offset
    val = regs.get(off, 0)
    if off == 0x0C:      # PWR_VOSR
        val |= 0xC000    # VOSRDY | BOOSTRDY
    elif off == 0x3C:    # PWR_SVMSR
        val |= 0x8000    # ACTVOSRDY
        val |= regs.get(0x0C, 0) & 0x30000  # ACTVOS 跟随 VOSR.VOS
    request.Value = val & 0xFFFFFFFF
