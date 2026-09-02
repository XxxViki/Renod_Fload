# Renode 仿真 STM32U585 开发笔记 —— 问题与解决汇总

> 本文档记录在 Flod_Array 工程上用 Renode 1.16.1 仿真 STM32U585 的完整踩坑过程，
> 供后续学习 Renode 开发参考。所有结论都经过实际验证。

---

## 1. 基础概念

### 1.1 Renode 是什么
Renode 是一个**整机级仿真器**：用文本文件描述"什么芯片、挂了哪些外设、地址在哪"，
然后加载真实固件（ELF/bin）在其上运行。它仿真的是**寄存器级行为**——固件读写寄存器，
Renode 决定返回什么。外设的行为可以由内置 C# 模型提供，也可以由 Python 脚本提供。

### 1.2 三类文件

| 文件 | 作用 | 类比 |
|------|------|------|
| `.repl` | 平台描述：声明外设类型、地址映射、中断连接 | "原理图/板卡配置" |
| `.resc` | 启动脚本：按顺序执行 monitor 命令（建机器、加载固件、开 GDB） | "开机操作流程" |
| `*.py`（pydev） | Python 打桩脚本：实现某个寄存器区域的读写行为 | "假芯片/夹具" |

### 1.3 stub（打桩）是什么
Stub = 桩。**用一段简化的假实现替换缺失的部件，让上层代码继续运行。**

为什么这里需要它：HAL 库初始化外设时的典型模式是——

```c
SET_BIT(RCC->CR, RCC_CR_HSEON);       // 写"使能位"（请求硬件开始工作）
while (READ_BIT(RCC->CR, RCC_CR_HSERDY) == 0)   // 死等"就绪位"（硬件说好了没）
{ ...超时则 Error_Handler... }
```

Renode 没有 STM32U5 的 RCC 模型，这个寄存器读回来永远是 0 → HAL 死等 → 卡死。
stub 就是一段 Python 脚本，注册在 RCC 的地址上：**固件写使能位，读就绪位时返回 1**，
模拟"硬件立即就绪"，让 HAL 顺利走完初始化流程。

> 关键认知：仿真不需要复现硬件的全部行为，只需要**让固件关心的那些位表现正确**。

---

## 2. 本工程的文件地图

```
Renode/
├── stm32u585_custom.repl     # 平台描述（CPU/NVIC/GPIO/UART/SPI/内存 + 各 stub 注册）
├── stm32u585_custom.resc     # 启动脚本（建机器→加载 ELF→串口→GDB server 3333）
├── stm32u5_rcc_stub.py       # RCC：就绪位跟随使能位（最核心的 stub）
├── stm32u5_pwr_stub.py       # PWR：VOSRDY / SVMSR.ACTVOSRDY（电压档位切换）
├── stm32u5_flash_stub.py     # FLASH 控制寄存器：写后回读（延迟校验）
├── stm32u5_adc_stub.py       # ADC：ADVREGEN 回读、ADRDY/EOC、DR 假数据
├── stm32u5_fdcan_stub.py     # FDCAN1：读写直通
└── stm32u5_dcache_stub.py    # DCACHE1：读写直通（SR 初始 0 = 不忙）
```

repl 中注册一个 stub 的写法（注意 filename 用绝对路径或相对 Renode 安装目录）：

```
rcc: Python.PythonPeripheral @ sysbus 0x46020C00
    size: 0x400
    initable: true
    filename: "D:/Xxx/Work/Flod_Array/Renode/stm32u5_rcc_stub.py"
```

---

## 3. Renode 1.16 PythonPeripheral 的正确写法（request 风格）

### 3.1 API 形态
**Renode 1.16 的 pydev 脚本不是"定义一个类"，而是"每次寄存器访问执行一遍脚本"**。
每次访问时，脚本里可以直接使用这些变量：

```python
request.IsInit    # True 表示机器初始化时的一次调用（可用来初始化状态）
request.IsRead    # 本次是读访问
request.IsWrite   # 本次是写访问
request.Offset    # 相对外设基地址的偏移（如 RCC_CR = 0x00）
request.Value     # 写：固件写入的值；读：把结果赋给它
self.NoisyLog("...")  # 打日志（受 logLevel 控制）
```

脚本顶层变量**在多次访问之间保留**（这就是状态存储的方式）。

### 3.2 最小可用模板（读写直通 / repeater）

```python
# -*- coding: utf-8 -*-
try:
    regs
except NameError:
    regs = {}                      # 首次执行时初始化状态

if request.IsWrite:
    regs[request.Offset] = request.Value
elif request.IsRead:
    request.Value = regs.get(request.Offset, 0) & 0xFFFFFFFF
```

这个"直通桩"能解决一大类问题：**HAL 写完寄存器后回读校验**（FLASH 延迟、
FDCAN 的 CCCR.INIT、DCACHE 的 CR 等）。

### 3.3 三个必须注意的坑

1. **类式写法完全无效**（本文最大教训）：写成 `class XxxStub: def Read(...)` 
   不会报错但也不会被执行，所有访问静默返回 0。必须用 request 风格。
   验证方法：monitor 里 `sysbus WriteDoubleWord <addr> 0x12345678` 再 
   `sysbus ReadDoubleWord <addr>`，读不回来就是脚本没生效。
2. **中文注释必须声明编码**：文件第一行加 `# -*- coding: utf-8 -*-`，
   否则 IronPython 直接拒绝编译（报 Non-ASCII character 错误）。
3. **状态变量要先初始化**：用上面的 `try/except NameError` 保护，
   否则第一次读访问就抛 `name 'regs' is not defined`，整个机器 Fatal error。

---

## 4. 打桩的核心设计原则

### 4.1 原则一：就绪位（RDY）跟随使能位（ON）——最重要

HAL 对振荡器/PLL 有**两种**等待：

```c
// 开启时：等 RDY 置 1
while (READ_BIT(RCC->CR, RCC_CR_PLL1RDY) == 0U) { ... }
// 关闭时：等 RDY 清 0 ！
while (READ_BIT(RCC->CR, RCC_CR_PLL1RDY) != 0U) { ... }
```

如果 stub 把 RDY **恒置 1**，第二种等待永远退不出去（本工程实际踩过：
`HAL_RCC_OscConfig` 先关 PLL 等 PLL1RDY 清零，恒置 1 导致 HAL_TIMEOUT）。

正确实现（硬件真实行为的抽象）：

```python
if off == 0x00:                      # RCC_CR
    val &= ~0x2A02A424              # 先清掉所有 RDY 位
    if val & 0x00010000:            # HSEON  →
        val |= 0x00020000           #     HSERDY
    if val & 0x01000000:            # PLL1ON →
        val |= 0x02000000           #     PLL1RDY
    ...
```

STM32U5 RCC_CR 位表（CMSIS stm32u585xx.h，写 stub 时查它）：

| ON 位 | RDY 位 | 说明 |
|-------|--------|------|
| MSISON=0 | MSISRDY=2 | MSI 系统时钟 |
| MSIKON=4 | MSIKRDY=5 | MSI 内核时钟 |
| HSION=8 | HSIRDY=10 | |
| HSI48ON=12 | HSI48RDY=13 | |
| SHSION=14 | SHSIRDY=15 | |
| HSEON=16 | HSERDY=17 | 外部晶振 |
| PLL1ON=24 | PLL1RDY=25 | |
| PLL2ON=26 | PLL2RDY=27 | |
| PLL3ON=28 | PLL3RDY=29 | |

### 4.2 原则二：状态位镜像请求位
- `RCC_CFGR1` 的 **SWS（当前系统时钟）镜像 SW（目标系统时钟）**——
  HAL 切换时钟源后等 SWS 确认，镜像让它立即"切换完成"。
- `PWR_SVMSR.ACTVOS` 镜像 `PWR_VOSR.VOS`——HAL 用它判断当前电压档位。
- ADC 的 `ISR.ADRDY` 跟随 `CR.ADEN`、`ISR.EOC` 跟随 `CR.ADSTART`。

### 4.3 原则三：给主循环留活口（假数据）
ADC stub 在 ADSTART 置位后让 DR 返回半量程、EOC 置 1，主循环的
"启动转换→等完成→读结果"就能转起来。想注入特定数据：

```
(STM32U585) sysbus WriteDoubleWord 0x42028040 0x1234   # 预设 ADC1_DR
```

---

## 5. 排查工具箱（怎么定位卡死）

### 5.1 Renode monitor 常用命令

```
pause                          # 暂停
start                          # 运行
cpu0 PC                        # 看当前 PC
cpu0 IsHalted                  # CPU 是否被挂起
sysbus ReadDoubleWord 0x46020C00       # 直接读外设寄存器（验证 stub）
sysbus WriteDoubleWord 0x42028040 0x1000  # 直接写寄存器（注入数据）
sysbus WhatPeripheralIsAt 0x46020C00    # 这个地址归哪个外设模型管
sysbus LogPeripheralAccess sysbus.rcc   # 记录该外设的所有访问
logLevel 3 sysbus.rcc                  # 调日志级别
```

### 5.2 无头模式（推荐用于自动化/远程排查）
```
renode --disable-gui --plain -P 12345 stm32u585_custom.resc
```
之后任何 TCP 客户端连 127.0.0.1:12345 就是 monitor（telnet 协议）。
注意：**必须以 resc 所在目录为工作目录启动**，否则 `@相对路径` 解析失败。

### 5.3 GDB 断点调试 —— bt 的两种用法

**方法 A：CubeIDE（图形界面）**
1. 启动 Renode（GDB server 在 3333）；
2. CubeIDE 用 **GDB Hardware Debugging** 类型配置连 Renode（本工程用
   `Flod_Array Renode.launch`：initCommands = `target remote localhost:3333`）；
3. 点 Debug → 停在 main → 断点/单步/Suspend 都可用；
4. 挂起时左侧 Debug 视图显示的调用栈树 = 图形版 bt，点每层跳到对应源码行。

**方法 B：命令行（排查利器，不依赖 IDE）**
```bash
arm-none-eabi-gdb --batch \
  -ex "target remote localhost:3333" \
  -ex "bt" \
  Debug/Flod_Array.elf
```
输出解读：
```
#0  Error_Handler () at ../Core/Src/main.c:196      ← 当前停的位置（最内层）
#1  0x08000c64 in SystemClock_Config () at main.c:163  ← 是谁调进来的
#2  0x08000b94 in main () at main.c:89
```
`#0` 是栈顶（当前执行点），`#1`、`#2` 依次是调用者。Error_Handler 是全工程
共用的一个函数，光看 PC 分不出是谁跳进来的，**bt 的价值就在这里**。

配合命令：
```bash
# 地址 → 函数名/行号（PC 停在哪不知道是什么函数时）
arm-none-eabi-addr2line -e Debug/Flod_Array.elf -f -C 0x08000ca0
# 反汇编某段代码（确认某地址是哪个 bl 调用）
arm-none-eabi-objdump -d -C --start-address=0x8000bd0 --stop-address=0x8000c70 Debug/Flod_Array.elf
```
（这些工具在 CubeIDE 自带工具链里：
`C:\ST\STM32CubeIDE_*\STM32CubeIDE\plugins\*gnu-tools*\tools\bin\`）

### 5.4 排查标准流程（已被验证有效）

```
固件卡死/进 Error_Handler
  │
  ├─ GDB attach + bt → 知道挂在哪个函数哪一行
  │
  ├─ 看这一层调用栈是哪个 HAL 调用 → 打开 HAL 源码看它在等什么位
  │    （grep "return HAL_ERROR\|return HAL_TIMEOUT" 缩小范围）
  │
  ├─ monitor 读对应寄存器 → 确认读回值不对（stub 缺失/写错）
  │
  └─ 补 stub：RDY跟随ON / 镜像 / 直通，三选一 → 重启 Renode 验证
```

---

## 6. 踩坑记录总表

| # | 现象 | 根因 | 解决 |
|---|------|------|------|
| 1 | `CreateServerSocketTerminal 4444` 报 AddressAlreadyInUse | 上一个 renode 进程没退干净占着端口 | `taskkill /F /IM renode.exe` 后重启 |
| 2 | `cpu0 GetStackTrace` 报错 | Cortex-M 模型不支持该命令 | 忽略，用 GDB bt 代替 |
| 3 | repl 里 `Tag <addr> "RCC_CR" 0xFFFFFFFF` 想"强制置位" | **Tag 只给地址起日志别名，不会写值**（第 4 个参数无效） | 用 PythonPeripheral stub |
| 4 | repl 报 `Syntax error, unexpected 'f'` | `.repl` 属性里不能写 `@路径`（那是 .resc 的语法） | filename 用带引号的路径 |
| 5 | repl 报 `Could not find source file` | `.repl` 里不带盘符的相对路径按 **Renode 安装目录**解析，不是 repl 所在目录 | 写绝对路径（正斜杠 `D:/...`） |
| 6 | 构造外设时 `Non-ASCII character '\xe6'` | IronPython 对含中文的脚本要求编码声明 | 第一行加 `# -*- coding: utf-8 -*-` |
| 7 | stub 挂上了但寄存器读回全 0（写入也读不回） | Renode 1.16 pydev 是 request 脚本风格，**class 定义不会被执行** | 重写成 request 风格（见 §3） |
| 8 | 读访问直接 Fatal：`name 'regs' is not defined` | 状态变量没初始化 | `try: regs / except NameError: regs = {}` |
| 9 | ControlVoltageScaling 超时（main.c:137） | HAL 还要等 `PWR_SVMSR.ACTVOSRDY`（0x3C）并读 ACTVOS 判断档位 | PWR stub 补 SVMSR：ACTVOSRDY 置位 + ACTVOS 镜像 VOS |
| 10 | OscConfig 超时（main.c:163） | ① FLASH_ACR 写后回读校验（SVD 恒返回 0）② PLL1RDY 恒置 1 导致"等关闭"死循环 | ① FLASH 直通 stub ② RDY 跟随 ON |
| 11 | U5 的 LSI 等待 | U5 的 LSION/LSIRDY 在 **RCC_BDCR** bit26/27（不是 CSR，与 F4 不同） | BDCR stub：LSIRDY 跟随 LSION，LSERDY 跟随 LSEON |
| 12 | `HAL_ADC_Init` 失败（adc.c:61） | Renode 的 STM32_ADC(F4) 模型没有 U5 的 ADVREGEN 位（CR bit28），HAL 回读校验失败 | ADC python stub：bit28 回读 + ADRDY/EOC/DR |
| 13 | `MX_DCACHE1_Init` 失败 | DCACHE_SR 的 SVD 复位值 BUSYF=1，HAL 等"不忙"超时 | DCACHE 直通 stub（SR 初始 0） |
| 14 | `machine Reset` 后 CPU 挂起（PC=0, IsHalted） | 机器没注册 reset macro，Reset 不会真正复位 | 复位用 `pause` + `sysbus LoadELF @../Debug/Flod_Array.elf` + `start` |
| 15 | `Cannot load ELF on an unpaused machine` | LoadELF 要求先暂停 | 先 `pause` |
| 16 | CubeIDE 点 Debug 秒断 | "Flod_Array Debug" 是 **ST-LINK 配置**（连真机探针），没插探针自然失败 | 新建 GDB Hardware Debugging 配置连 localhost:3333 |
| 17 | 无头启动报 `Could not find file 'stm32u585_custom.repl'` | `.resc` 里的 `@相对路径` 按**启动 Renode 时的工作目录**解析 | 以 Renode 文件夹为 CWD 启动 |

---

## 7. "每个外设一个 stub.py" 是标准用法吗？

**机制上是官方的，规模上是被逼的。**

- Renode 官方推荐路线：用**内置 C# 外设模型**（`UART.STM32F7_USART`、
  `GPIOPort.STM32_GPIOPort`、`Timers.STM32_Timer`……本工程的 UART/GPIO/SPI/I2C/
  Timer/RNG/RTC 用的就是这些）。C# 模型行为完整、性能好。
- `Python.PythonPeripheral` + pydev 脚本是官方提供的**补充机制**，用于：
  模型缺失的寄存器区域、行为对不上的个别位、快速做实验。官方自带
  `scripts/pydev/flipflop.py`、`repeater.py` 等示例。
- 本工程 RCC/PWR/FLASH/ADC/FDCAN/DCACHE 都要打桩，是因为 **STM32U5 太新**，
  Renode（1.16）还没有 U5 专用模型，通用 F4 模型又缺 U5 特有的位。
  这在 Renode 社区是常见做法，不算 hack。
- 局限：pydev 是 IronPython 解释执行，**每次寄存器访问都跑一遍脚本**，
  高频访问的外设（如被死循环轮询的寄存器）会拖慢仿真速度。长期方案是
  写 C# 外设（编译成 dll 放进 Renode）或等官方支持。
- 想要真实的 ADC/FDCAN 报文交互（而不只是初始化通过），需要更复杂的 stub
  或 C# 模型——本工程的 stub 目标是"让固件逻辑跑起来"。

---

## 8. 日常工作流

### 8.1 启动仿真（带界面）
```
cd D:\Xxx\Work\Flod_Array\Renode
renode stm32u585_custom.resc
(STM32U585) start
```
（`i stm32u585_custom.resc` 可以在已开的 Renode 里重载，但先 `Clear` 或重启
更干净——外设注册冲突会报错。）

### 8.2 调试
1. Renode 起 GDB server（resc 已含 `machine StartGdbServer 3333`）；
2. CubeIDE 用 `Flod_Array Renode.launch` 启动调试（停在 main）；
3. 或者命令行 gdb（见 §5.3 方法 B）。

### 8.3 修改了 stub 之后
pydev 脚本在机器创建时加载，**改完必须重启 Renode**（或重新
`machine LoadPlatformDescription`）才生效。

### 8.4 看串口输出
- GUI 模式：`showAnalyzer sysbus.usart1` 弹终端窗口；
- 无头模式：printf 输出混在 console log 里；
- TCP：`emulation CreateServerSocketTerminal 4444 "uart4_tcp"` + `connector Connect`，
  外部程序连 localhost:4444 即为 UART4。

### 8.5 常用寄存器速查（本工程实际用过）
```
RCC      0x46020C00   (CR=+0x00, CFGR1=+0x1C, BDCR=+0xF0)
PWR      0x46020800   (VOSR=+0x0C, SVMSR=+0x3C)
FLASH    0x40022000   (ACR=+0x00)
ADC1     0x42028000   (ISR=+0x00, CR=+0x08, DR=+0x40)
FDCAN1   0x4000A400
DCACHE1  0x40031400
```

---

## 9. 遗留事项 / 已知限制

- TIM1 个别寄存器（CCMR1/BDTR）未仿真，WARNING 可忽略（初始化不校验）；
- UART 的 PRESC（偏移 0x2C）未仿真，WARNING 可忽略；
- ADC 只有假数据（半量程），需要真实数据流时扩展 adc stub 的 DR 注入；
- FDCAN 只做了寄存器直通，报文收发不仿真；
- `machine Reset` 不可用（无 reset macro），重启固件用 pause+LoadELF+start。

## 10. 补充：UART 打印不显示的排查（2026-08-30 追加）

**现象**：主循环里 `HAL_UART_Transmit_IT(&hlpuart1, ...)` 和 `(&huart1, ...)`
各打印一次，Renode 的 analyzer 窗口毫无输出。

**排查过程**（可复用的方法）：
1. 采样 PC → 在 `HAL_Delay`，说明主循环在跑，不是卡死；
2. 读 LPUART1 CR1 = 0xE4E6 → TXEIE 已置位，固件配置正确；
3. 读 SR = 0x8D → TXE=1（发送器空闲，真实硬件此刻必触发中断）；
4. analyzer 零字符 → 结论：**Renode 的 STM32F7_USART 模型不产生 TXE 中断**，
   `HAL_UART_Transmit_IT` 依赖中断逐字节推送，一个字节都发不出。

**结论**：仿真阶段用阻塞 API：
```c
HAL_UART_Transmit(&huart1, data, sizeof(data) - 1, 100);
```
阻塞发送 = 直接写 TDR，模型完整支持；IT/DMA 方式（Transmit_IT/Receive_IT/DMA）
在当前平台不可用（GPDMA 无模型）。真机部署时再换回 IT/DMA。

**附带提醒**：
- 重新编译后 Renode 不会自动加载新 ELF，需 `pause` → 
  `sysbus LoadELF @../Debug/Flod_Array.elf` → `start`；
- 观察某个 UART 前确认 resc 里有对应的 `showAnalyzer sysbus.xxx`。

## 11. 重大更正：UART 中断发送其实可用，真凶是 NVIC 注册（2026-08-30 深夜追加）

§10 的结论"Renode 不产生 TXE 中断"**是错的**，向你道歉。深挖后的真相：

### 真正的根因
repl 里 NVIC 用了这种写法：
```
nvic0: IRQControllers.NVIC @ {
    sysbus new Bus.BusPointRegistration { address: 0xe000e000; cpu: cpu0 }
}
```
实测它只映射了 SysTick 一小块区域——**ISER(+0x100)/ISPR(+0x200) 根本不在映射里**
（monitor 访问 0xE000E204 报 "non existing peripheral"）。
后果链条：
1. 固件 `HAL_NVIC_EnableIRQ` 写 ISER → 写入无效 → 外设中断从未使能；
2. UART 模型明明把 TXE 中断线拉高了（`sysbus.usart1 IRQ IsSet` 为 True），
   NVIC 却收不到 → `HAL_UART_Transmit_IT` 死等第一个 TXE 中断 → 一个字节都发不出。

### 修复（已改入 stm32u585_custom.repl）
官方 stm32f4/f746 平台的写法：
```
nvic0: IRQControllers.NVIC @ sysbus 0xE000E000
    systickFrequency: 160000000
    IRQ -> cpu0@0
```
修复后全新实例实测：`HAL_UART_Transmit_IT` 在 usart1 和 lpuart1 上都正常工作
（GDB 查证 huart1.gState=32/READY、TxXferCount=0，40+ 轮循环反复收发）。

### 经验教训（排查方法层面）
- "模型不支持"这种结论必须做**链路逐级验证**：外设中断线状态（IRQ IsSet）
  → NVIC 挂起寄存器（ISPR）→ CPU 是否进入 ISR，哪一环断了修哪一环；
- `WhatPeripheralIsAt` 对 point registration 可能返回空，不可全信；
  用 monitor 读写 + 看日志里 "non existing peripheral" 告警更可靠；
- SysTick 能用 ≠ NVIC 整个映射正确（这次就是被 HAL_Delay 正常这个假象骗了）。

### 顺带：串口打印自动换行
UART 就是裸字节流，analyzer 不会替你换行，在字符串里加 `\r\n` 即可：
```c
uint8_t data[] = "send uart\r\n";
```

## 12. LPUART1 经 LPDMA1 循环链表接收：Python 自实现 DMA 仿真（2026-09-02）

§9 里"GPDMA 无模型"的结论部分过时：**v1.16.1 二进制里其实有 `DMA.STM32WBA55_GPDMA`**
（与 U5 GPDMA/LPDMA 同代 IP，通道寄存器布局逐字节一致，已实测可实例化），
但它的行为有三个硬伤，RX 不可用：
- EN 置位即同步整块搬运（RX 使能时刻 RDR 是空的 → 搬一缓冲垃圾并立即 TC）；
- 硬件请求线被 `TryTriggerTransfer` 入口的 `monitoredFIFOlevel==0` 拦死（FIFOL 无人写）；
- 链表模式 CLLR 只存不跟（源码标 TODO）。

### 方案：stub + GPIO 钩子 + NVIC 直连（已全部验证通过）

```
TCP/终端字节 → F7模型RDR → ReceiveDmaRequest 线 ─┬→ nvic0@114（每字节原生挂起通道中断）
                                                 └→ AddStateChangedHook 钩子(IronPython)
                                                     读通道寄存器→读RDR弹一字节→写CDAR(SRAM4)
                                                     →BNDT-1→块完成置TCF并按CLLR回卷节点
```

文件与接线：
- `stm32u5_lpdma_stub.py`（0x46025000）：寄存器存储 + CSR/CFCR 标志 + CCR 的 RESET/SUSP
  命令位写后自清（**驻留 RESET 位会把后续 EN 写又清掉，EN 永远起不来——踩过**）；
- `lpuart1_lpdma_hook.py`：逐字节搬运 + 链表回卷 + 空闲线仿真；
- repl：`lpuart1 ReceiveDmaRequest -> nvic0@114`；resc：挂钩子 + ICR 写断点（清 IDLE）。

空闲线仿真（`ReceiveToIdle` 依赖）：每字节后 `Machine.ScheduleAction`(1ms) 检查
CDAR 是否停滞，是则置 ISR.IDLE（F7 模型里是可写的 tagged 位）+ 写 NVIC ISPR2 挂起
LPUART1_IRQn(66)；HAL 清 ICR 时由断点同步清 ISR.IDLE。

钩子取 machine 的方式：`gpio.Endpoints[0].Receiver`（NVIC）反射私有字段 `machine`
（GPIOPythonEngine 作用域只有 self/state，无 machine）。

### 固件侧配套（已在真机语义下编译验证）
- 链接脚本加 `.sram4_section`（**必须放在 .bss 之前**，否则 `*(.bss*)` 先吞走 Node/List）：
  Node_LPDMA1_Channel0@0x28000000、List@0x28000024、缓冲@0x2800003C 全进 SRAM4；
- `Start_UART_Receive` 改 `HAL_UARTEx_ReceiveToIdle_DMA`（UART HAL 看 `hdmarx->Mode`
  == DMA_LINKEDLIST_CIRCULAR 自动走循环回调路径，无需 hack Init.Mode）；
- 回调用 `HAL_UARTEx_RxEventCallback(huart, Size)` 回显。

### 已验证（renode-test 全过）
- `lpdma_chain_test.robot`：线性搬运、循环链表回卷（BNDT 归零→节点重载→TCF→CFCR 清除）；
- `lpdma_fw_integration.robot`：真固件 12 字节 → SRAM4 缓冲内容正确、BNDT 64→52、
  IDLE 事件产生且被 HAL 正确消费。注意测试要先轮询 CR3.DMAR 再发数（否则字节在
  DMAR 置位前到达会滞留 RDR，BufferState 不再翻转导致请求线永不触发——竞态坑）。

### 已知限制（盲区）
- 半传输 HTF 不产生（HAL 使能了 HTIE 也收不到半满事件）；
- DTE/ULE/USE/TO 等错误通道不仿真；
- 空闲判定是固定 1ms 虚拟时间，不是按波特率算的位时间；
- Renode 不检查"LPDMA 只能访问 SRAM4"的总线限制——缓冲错放 SRAM1 的真机
  HardFault 在仿真里发现不了；
- `Write To Uart` 关键字会自动补换行（BNDT 按发 1 字节多算）。

### §12.1 中断风暴事故与启动竞态看门狗（2026-09-02 追加）

**现象**：串口助手一发数据，Renode"卡死"（CPU 在 HAL_UART_IRQHandler 里
无限重入：ISR 显示 RXNE=1，CR1 显示 RXNEIE=1，CR3 显示 DMAR=1）。

**根因链**：
1. 数据在固件置 CR3.DMAR **之前**到达 → `ReceiveDmaRequest` 线（电平信号，
   仅在 BufferState 翻转时重评估）错过这次跳变 → 字节滞留 RDR；
2. 后续字节造成 ORE → HAL 错误回调把 ReceptionType 降级 → 旧版回调回退到
   `HAL_UART_Receive_IT`（开 RXNEIE）；
3. HAL 中断分支看到 DMAR=1 →"字节归 DMA 管"不读 RDR → RXNE 永远清不掉 → 风暴。

**修复（两侧）**：
- 仿真：CR3 写断点看门狗（resc 里 `AddWatchpointHook 0x46002408 ...`）——
  DMAR 置位瞬间若 RXNE=1，钩子强制搬运滞留字节（`lpdma_on_cr3_write`，
  钩子脚本按作用域变量 state/value 自动分发入口）；
- 固件：`HAL_UART_ErrorCallback` 一律重启 `ReceiveToIdle_DMA`，
  **绝不回退 Receive_IT**（DMAR 残留时必风暴，真机同理）。

**验证**：`lpdma_race_test.robot`（启动瞬间注入字节 + 无风暴 + 帧仍正确）
与 `lpdma_fw_integration.robot` 均通过。

### §12.2 回显不通的终极排查与最终方案（2026-09-02 追加）

**现象**：数据能进 SRAM4 缓冲，但永远没有回显（`lpdma_tcp_repro.robot` 用真实
TCP socket 复现）。逐级二分（monitor 直写 TDR 能到 socket → 终端/模型 TX 正常
→ 固件回显回调从未执行 → 空闲事件从未发生）。

**三个依次排除的死路（都做了实证）**：
1. **ISR.IDLE / CR1.IDLEIE 是 `WithTaggedFlag` 死位**——写被丢弃、读恒 0。
   HAL 置了 IDLEIE 后回读仍是 0，`HAL_UART_IRQHandler` 的空闲分支永远进不去；
   Python 写 ISR.IDLE 同样无效。（"Unhandled bits" 告警≠已存储，之前误判了。）
2. **NVIC 中断注入不触发 CPU 响应**——Python 写 ISPR3 只置挂起位；反射调
   `nvic.OnGPIO(114, True)` 挂起会被取走但 HAL_DMA_IRQHandler 未执行
   （CSR.TCF 一直悬着），且长跑会拖死仿真（601s 超时）。
3. 写断点作用域的 `value` 变量实测恒 0（IronPython 装箱问题）——看门狗判断
   一律改用寄存器回读。

**最终方案（三套测试全过：TCP 回显/启动竞态/回归）**：
- **帧结束改用 RTOF（接收超时）**：F7 模型完整实现了 RTOR/RTOF/RTOIE 的真语义
  （每次收字节取消并重排 32bit 超时 → 置 RTOF → 拉 UART 中断线）。
  固件 `Start_UART_Receive`：`RTOR=32; __HAL_UART_ENABLE_IT(UART_IT_RTO);
  HAL_UART_Receive_DMA(...)`（不用 ReceiveToIdle）；`LPUART1_IRQHandler` 的
  USER CODE 段处理 RTOF（清标志 + 按计数器算长度 + 回显）。
  **这是真机也成立的正规设计**——LPUART1 支持 RTOF，上机不用改代码。
- 钩子只剩纯搬运 + 链表回卷 + CR3 看门狗（`lpuart1_lpdma_hook.py` 已精简）。
- TCP 终端注意 `CreateServerSocketTerminal` 第三个参数是 **telnetMode**，
  串口助手直连用 `false`（true 会先发 Telnet 协商字节，部分工具显示乱码）。
- 帧长判定：RTOF 处理里 `n = 缓冲全长 - __HAL_DMA_GET_COUNTER(hdmarx)`；
  整块完成时计数器已回卷，n==0 取全长。

### §12.3 两个实战缺陷修复（2026-09-02 追加）

**缺陷1：回显逐次累积（每次多回显一帧）**
根因：`n = 缓冲全长 - __HAL_DMA_GET_COUNTER()` 算的是**自通道启动/回卷以来的
累计字节数**，不是本帧长度——第 N 次发送会把缓冲里前 N 帧全部重发。
修复：`lpuart1_frame_handler()` 改为**读写指针追踪**（static rd，wr 由计数器换算，
wr<rd 时分两段发送处理回卷），真机同样适用。

**缺陷2：收一段时间后 Fatal "Cannot access a closed file" 崩溃**
根因：钩子引导语句 `exec(open(文件).read())` 里 StreamReader 是无引用临时对象，
嵌套重入（读 RDR 清空缓冲→请求线落下→钩子在自身内部再次触发）触发 GC 时把
**正在读的流中途回收**。`globals().setdefault` 缓存编译对象也无效（该引擎每次
执行的 globals 不持久）。
修复：挂载语句改为 `with open(路径) as _f: exec _f.read()`——with 变量保持强
引用 + 确定关闭，根除 GC 竞态（resc 与全部 robot 测试已同步更新）。

**回归**：`lpdma_stress_test.robot`（同帧二进制数据连发 6 次→恰好回显 6 帧、
无累积无崩溃、BNDT 正确）+ TCP/竞态/集成三套全部通过。

### §12.4 块边界回卷卡死与 1024 缓冲的真相（2026-09-02 追加）

**现象**：64 字节缓冲时，累计收到第 64 字节（循环块边界）后：回显停止、
偶发单字节回显、最终 `CPU abort PC=0x646E6572`（PC 飞进数据区）。
把 `LPUART1_DMA_RX_BUF_SIZE` 调到 1024 后"一直正常"。

**为什么调大缓冲有效（但不是修复）**：故障点=累计接收字节数到达"缓冲全长"
那一刻的**循环链表回卷**。1024 只是把触发条件推迟到累计 1024 字节——
真机上没问题（硬件自己回卷），仿真里到了 1024 一样会炸。

**卡死链条（实证）**：回卷时钩子的节点重载读通道 CLLR 得 0（CLLR 在运行中
莫名变 0——CPU 只写过两次合法值、stub 作用域从未重置，来源未最终定位）→
判"非链表"放弃回卷 → BNDT 卡 0、CDAR 停在缓冲末尾 → 新字节滞留 RDR、
ReceiveDmaRequest 线悬高 → IRQ114 反复重入；`HAL_DMA_IRQHandler` 第一行读
**MISR（LPDMA1+0x0C 全局中断摘要）**，stub 恒返回 0 → 处理函数直接返回、
中断源头永不清除 → 风暴直至固件状态损坏、PC 跑飞。

**修复（两处，均已回归）**：
1. stub 实现 MISR（通道 TCF&&TCIE 时置位）——`HAL_DMA_IRQHandler` 的
   入口检查能通过，TC 中断正常走完回调链；
2. 钩子回卷加**缓存兜底**：首次从节点装载成功后，把 (CDAR,BNDT) 存进
   stub 闲置寄存器区（0x3F0/0x3F4/0x3F8），节点重载失败时直接恢复——
   不再依赖运行中可能变 0 的 CLLR。

**验证**：`lpdma_stress_test.robot` 发 160 帧×7 字节=1120 字节（跨过 1024
边界），160 帧全部恰好回显一次、回卷后 BNDT=1024-96 正确、无卡死无崩溃；
TCP/竞态/集成三套回归通过。

**教训**：CPU abort 到 0x646E6572（ASCII "rene"）这类"飞进数据"的 PC，
先怀疑中断风暴把回调指针/栈搅坏，而不是直接查那段地址。

### §12.5 问题总结（全历程索引，2026-09-02；stub 脚本头部有同步副本）

| # | 现象 | 根因 | 修复 |
|---|------|------|------|
| 1 | 找不到可用的 DMA 模型 | Renode 无 GPDMA/LPDMA；F7 的 STM32DMA 布局不兼容；STM32WBA55_GPDMA 布局兼容但 EN 即整块搬运、请求线被 FIFOL==0 拦死、链表 TODO | request 风格 stub + GPIO 钩子自建数据通路 |
| 2 | CCR.EN 永远置不上 | stub 把 RESET 位当普通位驻留，后续 EN 读改写又被"RESET 清 EN"抵消 | RESET/SUSP 按真硬件写后自清 |
| 3 | 中断风暴（HAL_UART_IRQHandler 无限重入） | 数据先于 CR3.DMAR 到达，请求线错过跳变，RXNE 悬死 | CR3 写断点看门狗强制搬走滞留字节 |
| 4 | 错误后必风暴 | ErrorCallback 回退 Receive_IT 而 DMAR 残留 | 错误回调一律重启 DMA 接收 |
| 5 | ReceiveToIdle 永不触发回显 | F7 模型 IDLE/IDLEIE 是 WithTaggedFlag 死位（写丢弃读恒 0） | 帧结束改 RTOF 接收超时（模型原生、真机同款） |
| 6 | Python 挂不起中断 | ISPR 只置位不通知 CPU；OnGPIO 注入无响应 | 帧结束走模型真实中断线（RTOF→UART IRQ） |
| 7 | 串口助手连 4445 显示乱码 | CreateServerSocketTerminal 第 3 参是 telnetMode=true | 改 false（raw 字节流） |
| 8 | 回显逐次累积（每次多一帧） | 长度"全长-计数器"是累计值非本帧长度 | 读写指针追踪（rd/wr + 回卷分段） |
| 9 | 收一会儿 Fatal "Cannot access a closed file" | exec(open().read()) 读取器是无引用临时对象，嵌套重入时被 GC 中途回收 | with open(...) as _f: exec _f.read() |
| 10 | 累计收满缓冲全长后卡死、PC 飞进数据区 | CLLR 运行中变 0→回卷放弃→BNDT 卡 0→字节滞留→请求线悬高→IRQ114 风暴；且 MISR 恒 0 让 HAL_DMA_IRQHandler 早退 | stub 实现 MISR + 钩子回卷缓存兜底（stub 0x3F0 区）；调大缓冲只是推迟不是修复 |
| 11 | 脚本静默失败 / 判断失效 | try/except 吞错；断点作用域 value 恒 0（IronPython 装箱） | 不吞错；判断一律寄存器回读；ScheduleAction 回调是 Action[TimeInterval] |
| 12 | robot 测试莫名失败 | Write To Uart 自动补换行；Evaluate 变量带换行破坏表达式 | 断言按 N+1 字节算；$obj 传对象、先 strip |

**排障方法论沉淀**：①"模型不支持"类结论必须链路逐级验证（中断线→NVIC→CPU 进 ISR），
哪环断修哪环；② CPU 飞进数据区（如 PC=0x646E6572）先怀疑中断风暴搅坏状态；
③ 二分法定位（monitor 直写寄存器/直写 TDR 隔离固件与仿真层）；④ SRAM 留痕
计数器/轨迹区是无日志 Python 钩子的最佳取证手段；⑤ 每修一坑都固化成 robot
回归测试（chain/集成/竞态/压力四套），改完必跑。

### §12.6 补遗：跨块帧回显拆分（2026-09-02，stub 总结第 13 条已同步）

**现象**：64 字节缓冲连发 160 帧（跨 17 次回卷），回显流里凑不齐完整帧
（150/160），但 BNDT 校验证明接收侧一字节不丢。

**根因**：TC（块完成）中断常被推迟到 RTOF 回显之后才处理；此时
`HAL_UART_RxCpltCallback` 里的 frame_handler 会把"跨块帧已到达的前半段"
提前回显并推进读指针，该帧剩余部分随后由 RTOF 补发——字节都在，
但一帧被拆成两段、且可能与下一帧交错，帧序被打乱。

**修复**：循环模式下 `HAL_UART_RxCpltCallback` 不再回显，块边界完全交给
RTOF（帧结束约 2.2ms 后必然触发，rd/wr 的回卷分支本就支持跨块帧整帧处理）。

**验证**：160 帧×7 字节=1120 字节（缓冲 1024，跨边界），CNT=160 恰好、
BNDT=1024-96=928 正确；TCP/竞态/集成三套回归通过。四套 robot 测试的
BNDT 断言已改为缓冲大小自适应（从节点镜像 0x28000008 读全量）。

### §12.7 多串口 GPDMA1 接入与 resc 重组（2026-09-02）

**USART3（电机）RX 上 GPDMA1 CH0（IRQ29）**，钩子已通用化：
- `uart_dma_hook.py`（原 lpuart1_lpdma_hook.py 改名扩展）：通道由 GPIO 端点的
  NVIC 输入号反推（GPDMA1 IRQ29-36=CH0-7、80-87=CH8-15；LPDMA1 114-117），
  源地址直接读通道 CSAR——一份脚本服务所有串口；
- stub 扩到 16 通道（GPDMA1 实例 @0x40020000 size 0x1000，与 LPDMA1 同脚本）；
- 固件：GPDMA1 无 SRAM4 限制，缓冲放普通 RAM（usart3_dma_rx_buf@0x20000824），
  `uart_frame_handler()` 泛化（按 Instance 分读位置）。
- 注意端点属性名是 `ep.Number`（GPIOEndpoint 的目的编号）。

**resc 重组规范**（用户要求）：§1 机器/固件 §2 观察窗口集中 §3 TCP 终端集中
§4 按串口分块（说明在前、收发成对、钩子/看门狗紧随）§5 DMA 速查与新外设模板
§6 调试工具 §7 启动；重复长命令用 `$DMA_RX_HOOK` 变量消重——**resc 的
$变量可传给任何 Monitor 命令**（含钩子挂载），已实测。

**坑14**：`Write To Uart` 只收字符串（传 bytes 报 InvalidCastException）；
本版本实测**不**自动补换行（之前按 +1 字节算的断言要按实际字节数核）。

**验证**：usart3_dma_test（USART3 三帧进 RAM 缓冲 + BNDT 精确）+ LPUART1
五套回归全部通过。

### §12.8 坑9 复发：TX 钩子的挂载串（2026-09-02）

**现象**：UART4 用 TX DMA 回显，发几次 Renode 崩（Cannot access a closed file，
栈顶在 BusPeripheralsHooksPythonEngine —— SetHookBeforePeripheralWrite 的
CPU 写路径上）。

**根因**：坑9 的裸 `open().read()` 模式当时只修了 RX 钩子，TX 钩子挂载串
（`$DMA_TX_HOOK` 的前身）仍是裸模式；且写前钩子挂在**每次 uart4 寄存器写**
上，触发频率极高，GC 中途回收读取器必崩。另一个中间版本的 def+try/except
变体还吞掉了所有脚本错误（坑11），且被 robot 的 `|` 续行符切碎导致参数错乱。

**修复（统一模板，三处同步）**：RX/TX 钩子挂载一律用标准 with-open 单行：
  RX: "with open(r'.../uart_dma_hook.py') as _f: exec _f.read()"
  TX: "with open(r'.../uart_tx_dma_hook.py') as _f: exec 'UART_BASE=0x<基址>' + chr(10) + _f.read()"
（TX 的 offset==0x08/DMAT 过滤在 uart_tx_dma_hook.py 内部做，外层不再包
def/try——引擎本身会捕获并打印脚本错误，不需要也不应该自己吞。）

**重要**：钩子挂载串在 resc 执行时就被固化为引擎的脚本文本，**改完 resc 必须
重启 Renode 才生效**（RX 钩子因每次执行都重读文件可自愈，TX 的挂载串不能）。
已改处：stm32u585_custom.resc（$DMA_TX_HOOK）、uart4_tx_dma_test.robot、
uart_tx_dma_hook.py 头部模板。uart4_tx_dma_test 两连跑通过。

### §12.9 三口 TCP 双向验收 + 压力全绿（2026-09-02）

**验收标准**（用户目标）：4445(LPUART1)/4446(USART3)/4444(UART4) 三口
串口助手收发全部正常 + 压力测试。

**结果**：`three_port_acceptance.robot` 全过——完整 resc 钩子环境 +
三个真实 TCP 客户端：25s 空闲（PC 采样健康）→ 三口各发 PING 帧全部回显 →
三口各 20 帧二进制压力全部恰好回显（COUNTS 20/20/20）→ 终态 PC 健康。

**排障中的两个关键教训**：
1. 测试 socket 的端口必须与被测实例严格对应：一度 sed 没替换到
   `localhost', 4445`（引号后逗号），测试流量全部打到了**用户正在运行的
   实例**上，造成"我的环境坏掉了"的假象。测试固定用 1444x 独立端口段。
2. 用户实例上的 CPU abort(0x646E6572) 是 §12.8 的旧 TX 钩子在跑——
   **改完 resc/钩子必须重启 Renode**，且旧实例不关会占住 444x 端口，
   新实例在 CreateServerSocketTerminal 处直接报错中止（更迷惑）。

**测试清单**（E:/Project/Flod_Array/Renode/）：
- three_port_acceptance.robot —— 目标验收+压力（三口双向 20 帧）
- threeport_diag.robot —— 三口单帧诊断（寄存器/缓冲/回显全 dump）

### §12.10 真凶：GDB 服务 + IDE 自动重连（2026-09-02）

**现象**："一仿真运行就崩"——完整 resc 环境（analyzer×4 + TCP×3 + 全部钩子
+ StartGdbServer）下，Renode **进程级死亡**（无 Fatal 栈，~20s），CPU 此前
PC 采样全部健康。

**定位**：二分环境——去掉 `machine StartGdbServer 3333` 后同环境一次全绿
（analyzer/TCP/钩子/30s 空闲/三口流量回显全过）。机理：GDB 服务开启后，
本机 IDE（CubeIDE 调试会话的自动重连）接入 3333，与 CPU 线程上执行的
Python 写前钩子相互作用，进程崩溃。

**处置**：resc 中 StartGdbServer 默认注释掉（需要 IDE 联调时打开，且仿真
期间关闭 IDE 的活动调试会话）。复现/验证测试：full_env_repro_test.robot。

### §12.10 GDB + TX DMA 共存问题（终版修复，2026-09-02）

**现象**：用户必须用 GDB 联调（CubeIDE/3333），但引入 UART4 TX DMA 后
"一仿真运行就崩"——Renode 进程级死亡（~20s，无 Fatal 栈）。之前三串口
纯中断模式时 GDB 一直正常。

**根因**：TX 写前钩子（SetHookBeforePeripheralWrite）的 Python（文件读取+
总线搬运）在 **CPU 写路径内联执行**；IDE 的 GDB 会话（halt/单步/断点）
恰好落在钩子中间时机器挂死甚至进程崩溃。可控复现：脚本化 GDB 客户端周期
halt(\x03)+continue，直连钩子下 UART4 回显归零/挂死；去掉 GDB 则全绿。

**修复：延迟转投桩 uart4_tx_defer_stub.py**（resc 的 $DMA_TX_HOOK 已指向它）：
- 写路径上只留最小过滤（CR3.DMAT 置位判定，零文件 I/O）+ ScheduleAction(+1us)
  把真正搬运转投到机器调度线程——GDB halt CPU 不再落在钩子内部；
- GDB 服务恢复常开（resc 已取消注释 StartGdbServer 3333）。

**延迟回调的两个新坑（记入 stub 头部）**：
- 变量注入必须 globals()['offset']=...（函数内赋值落局部，文件入口读不到）；
- 必须 exec src in globals()——普通 exec 时文件定义的常量（GPDMA1）落局部，
  文件内部函数按全局查找报 NameError: GPDMA1（异常文本写 SRAM 取证定位）。

**验证**：gdb_deferred_test.robot（GDB halt/continue 风暴 ×60 + 双口各 10 帧
二进制 → lpuart1=10/uart4=10，机器存活）；three_port_acceptance 与
uart4_tx_dma_test（已同步延迟桩）回归通过。GDB 客户端复现脚本也在该测试里
（模拟 IDE 行为：\x03 + $c#63 循环）。

### §12.11 坑11：钩子串包函数 → 全部 RX 钩子静默失效（2026-09-02 终版）

**现象**：串口助手对 4445/4446/4444 全部零回显；换新实例、重启都一样；
连上 CubeIDE 后"串口打印几个数据就跑飞"、"卡在 HAL_Delay 不出来"，
GDB 端口（3333）查询无响应甚至拒连。

**定位过程**：
1. 对用户活实例实测（python socket 直连 4445/4446 + GDB `?` 包）：三口零回显
   + GDB 无响应，复现成功；
2. 用**逐字复刻的真实 resc**（仅端口改 1444x、GDB 改 13333）headless 复现：
   启动正常（PC 在 flash、uwTick 走、CR3.DMAR=0x40、通道 EN=1），发 3 帧后
   **PC=0xFFFFFFA8**，三口零回显；
3. 对照实验：同一环境再挂一条**旧直连形式**钩子 → 回显立刻出现 → 锁定
   resc 里的 `$DMA_RX_HOOK` 新形式。

**根因**：为做异常防护，$DMA_RX_HOOK 曾改成
`exec chr(10).join(('def _rxh():', '  try:', '    with open(...) as _f:',
'      exec _f.read()', '  except: pass', '_rxh()'))`。
**IronPython 函数内嵌 exec 看不到 Renode 注入的 state/address/value/self**
——uart_dma_hook.py 入口的 `try: state / except NameError` 分支全部落空，
钩子静默 no-op（不报任何错，日志无线索）。钩子死后无人读 RDR：
- ReceiveDmaRequest 线悬高 → 通道 IRQ（114/29/30）风暴 → 主循环饿死 →
  **HAL_Delay 卡死（uwTick 停走）**；
- 中断不断重入 + 固件跑飞（**PC=0xFFFFFFA8**）；
- GDB 服务线程随之无响应 → 拒连。
一个根因串起全部症状。

**修复**（两处，缺一不可）：
- resc 恢复直连形式 `$DMA_RX_HOOK = "with open(r'...uart_dma_hook.py') as _f: exec _f.read()"`；
- 异常防护移入 uart_dma_hook.py 入口（模块级整体 try/except: pass）——
  作用域可见性与异常防护从此解耦。

**坑12（同日）**：ServerSocketTerminal 每端口只服务一个客户端——已有连接时
后续连接被无视并把监听搞挂（再连 ConnectionRefused）。排障时用探针脚本连过
用户活实例的 4445/4446 会把它搞坏；自动化测试必须复用同一条连接。

**验证**（repro2.robot + gdb_full_test.robot，均 include 真实 resc）：
- repro2：单帧×3 口 + 连发 6×3 + 压力 50×3（共 399B 收 399B 回显）全 OK，
  压力后 PC=0x0800CDD6、uwTick 持续走；
- gdb_full_test：GDB halt/continue 风暴 ×50 下三口各 10 帧 → 10/10/10，
  PC 正常。
