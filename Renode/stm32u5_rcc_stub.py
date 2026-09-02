# -*- coding: utf-8 -*-
# STM32U5 RCC 打桩脚本（Renode 1.16 PythonPeripheral，request 风格）
# 核心思路：就绪位(RDY)跟随使能位(ON)，与真实硬件行为一致 ——
#   HAL 开启振荡器时等 "RDY 置位"，关闭时等 "RDY 清零"，两种循环都能立即通过。
# 位定义来自 CMSIS stm32u585xx.h（RCC_CR）：
#   MSISON=0/MSISRDY=2, MSIKON=4/MSIKRDY=5, HSION=8/HSIRDY=10,
#   HSI48ON=12/HSI48RDY=13, SHSION=14/SHSIRDY=15,
#   HSEON=16/HSERDY=17, PLL1ON=24/PLL1RDY=25, PLL2ON=26/PLL2RDY=27, PLL3ON=28/PLL3RDY=29
# RCC_CFGR1 (偏移 0x1C): SW=1:0, SWS=3:2 —— 读回时 SWS 镜像 SW
# RCC_BDCR  (偏移 0xF0): LSEON=0/LSERDY=1, LSESYSEN=7/LSESYSRDY=11，
#                        U5 的 LSI 在 BDCR：LSION=26, LSIRDY=27
try:
    regs
except NameError:
    # 复位默认值：MSIS/MSIK 使能（真实芯片复位后用 MSI 作时钟源）
    regs = {0x00: 0x00000011}

if request.IsWrite:
    regs[request.Offset] = request.Value
elif request.IsRead:
    off = request.Offset
    val = regs.get(off, 0)

    if off == 0x00:      # RCC_CR: RDY 位跟随 ON 位
        val &= ~0x2A02A424
        if val & 0x00000001:
            val |= 0x00000004      # MSISRDY
        if val & 0x00000010:
            val |= 0x00000020      # MSIKRDY
        if val & 0x00000100:
            val |= 0x00000400      # HSIRDY
        if val & 0x00001000:
            val |= 0x00002000      # HSI48RDY
        if val & 0x00004000:
            val |= 0x00008000      # SHSIRDY
        if val & 0x00010000:
            val |= 0x00020000      # HSERDY
        if val & 0x01000000:
            val |= 0x02000000      # PLL1RDY
        if val & 0x04000000:
            val |= 0x08000000      # PLL2RDY
        if val & 0x10000000:
            val |= 0x20000000      # PLL3RDY
    elif off == 0x1C:    # RCC_CFGR1: SWS 跟随 SW
        val = (val & ~0xC) | ((val & 0x3) << 2)
    elif off == 0xF0:    # RCC_BDCR: LSERDY 跟随 LSEON，LSESYSRDY 跟随 LSESYSEN，LSIRDY 跟随 LSION
        val &= ~0x08000802
        if val & 0x00000001:
            val |= 0x00000002
        if val & 0x00000080:
            val |= 0x00000800
        if val & 0x04000000:
            val |= 0x08000000
    request.Value = val & 0xFFFFFFFF
