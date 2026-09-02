# -*- coding: utf-8 -*-
# STM32U5 ADC1/ADC2 打桩脚本（Renode 1.16 PythonPeripheral，request 风格）
# Renode 内置 STM32_ADC(F4) 模型缺少 U5 的电压调节器位 ADVREGEN (CR bit28)，
# HAL_ADC_Init 会回读校验该位导致 HAL_ERROR。
# 本 stub 关键行为：
#   CR   (0x08): ADVREGEN 跟随写入值；ADCAL(bit31) 自动清零（校准立即完成）
#   ISR  (0x00): ADRDY(bit0) 跟随 ADEN；EOC(bit2) 跟随 ADSTART（转换立即完成）
#   DR   (0x40): 返回可配置的模拟值（默认半量程，可写入 DR 预设）
# 其余寄存器读写直通。
try:
    regs
except NameError:
    regs = {}

if request.IsWrite:
    off = request.Offset
    regs[off] = request.Value
    # 也可以从外部用 monitor 写 DR 来注入模拟量：sysbus WriteDoubleWord <ADC_DR> <value>
    if off == 0x08 and (request.Value & 0x00000004):
        # ADSTART 置位后立即生成一次"转换结果"
        regs[0x40] = regs.get(0x40, 0x1800)
elif request.IsRead:
    off = request.Offset
    val = regs.get(off, 0)
    cr = regs.get(0x08, 0)
    if off == 0x08:
        val &= 0x7FFFFFFF          # ADCAL 自动清零
    elif off == 0x00:              # ISR
        val &= ~0x00000005
        if cr & 0x00000001:        # ADEN -> ADRDY
            val |= 0x00000001
        if cr & 0x00000004:        # ADSTART -> EOC
            val |= 0x00000004
    request.Value = val & 0xFFFFFFFF
