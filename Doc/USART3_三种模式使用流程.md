# USART3 串口使用流程（阻塞 / 中断 / DMA 三种模式）

> 适用：STM32U585（本工程 Flod_Array），HAL 库。
> 目的：整理 USART3 三种收发模式的完整落地步骤，覆盖涉及的配置项、函数、回调与常见坑，供学习与复用。

---

## 0. 硬件资源速查

| 项目 | 值 |
|---|---|
| 外设 | USART3 |
| 引脚 | PA5 = RX，PB10 = TX，复用 AF7 |
| 时钟源 | PCLK1（`RCC_USART3CLKSOURCE_PCLK1`） |
| 默认参数 | 115200 / 8N1 / 无流控 / 无校验 |
| 接收 DMA | GPDMA1 Channel0，请求 `GPDMA1_REQUEST_USART3_RX` |
| 串口中断 | `USART3_IRQn` |
| DMA 完成中断 | `GPDMA1_Channel0_IRQn`（IRQ29） |
| 句柄 | `huart3`（UART）、`handle_GPDMA1_Channel0`（DMA） |
| 接收缓冲 | `usart3_dma_rx_buf[256]`（DMA 模式用） |

---

## 1. 共同基础：初始化（三种模式都需要，一次性）

不管用哪种模式，USART3 都得先完成「外设初始化 + 引脚复用 + 时钟」，这部分完全一样。

### 1.1 CubeMX 侧配置（图形化，生成代码前）

1. `Connectivity → USART3`：Mode 选 `Asynchronous`，波特率 115200，字长 8bit，停止位 1，无校验、无流控。
2. 引脚自动落到 `PA5(USART3_RX)`、`PB10(USART3_TX)`，AF7 由 CubeMX 自动分配。
3. **中断模式**需勾选 `NVIC Settings → USART3 global interrupt`。
4. **DMA 模式**需在 `DMA Settings` 里 Add 一条 `USART3_RX`，方向 Peripheral→Memory，模式 Circular（循环）。

### 1.2 生成的代码（已落地，无需手写）

`Core/Src/usart.c`：

```c
/* 句柄声明 */
UART_HandleTypeDef huart3;
DMA_NodeTypeDef   Node_GPDMA1_Channel0;
DMA_QListTypeDef  List_GPDMA1_Channel0;
DMA_HandleTypeDef handle_GPDMA1_Channel0;

/* 参数初始化：115200 8N1 */
void MX_USART3_UART_Init(void)
{
    huart3.Instance                    = USART3;
    huart3.Init.BaudRate               = 115200;
    huart3.Init.WordLength             = UART_WORDLENGTH_8B;
    huart3.Init.StopBits               = UART_STOPBITS_1;
    huart3.Init.Parity                 = UART_PARITY_NONE;
    huart3.Init.Mode                   = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling           = UART_OVERSAMPLING_16;
    huart3.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    huart3.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
    huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart3) != HAL_OK) { Error_Handler(); }
    /* 其余为 FIFO 阈值配置（U5 特有），略 */
}
```

`HAL_UART_MspInit()` 的 `USART3` 分支做三件事：

```c
else if (uartHandle->Instance == USART3)
{
    /* ① 时钟 */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
    PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
    __HAL_RCC_USART3_CLK_ENABLE();

    /* ② GPIO：PA5=RX / PB10=TX，AF7 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin       = GPIO_PIN_5;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_10;   // PB10=TX，其余同上
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* ③（可选，仅 DMA 模式）GPDMA1 CH0 循环链表 —— 见第 4 节 */

    /* ④（仅中断/DMA 模式）NVIC */
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}
```

### 1.3 落地 checklist

- [ ] CubeMX 使能 USART3 + 引脚正确（PA5/PB10, AF7）
- [ ] `MX_USART3_UART_Init()` 在 `main()` 里被调用
- [ ] 时钟、GPIO 复用配置正确
- [ ] 想要回显/打印时，重定向 `printf` 或封装 `UART_SendString()`

---

## 2. 阻塞模式（轮询）

**原理**：CPU 全程等待传输完成，简单但占用 CPU，适合调试打印、低频短报文。

### 2.1 额外配置

- **无需** NVIC、**无需** 回调、**无需** DMA。
- 只需要第 1 节的共同基础即可。

### 2.2 涉及函数

| 函数 | 作用 |
|---|---|
| `HAL_UART_Transmit(&huart3, buf, len, timeout)` | 阻塞发送，直到发完或超时 |
| `HAL_UART_Receive(&huart3, buf, len, timeout)` | 阻塞接收，直到收满 `len` 字节或超时 |
| `HAL_UART_GetState(&huart3)` | 查询当前状态 |

### 2.3 落地示例

```c
uint8_t tx[] = "hello usart3\r\n";
uint8_t rx[16] = {0};

/* 发送：发完 14 字节或超时 100ms 返回 */
HAL_UART_Transmit(&huart3, tx, sizeof(tx) - 1, 100);

/* 接收：收满 16 字节或超时 200ms 返回，实际收到多少看返回值 */
HAL_StatusTypeDef st = HAL_UART_Receive(&huart3, rx, sizeof(rx), 200);
```

### 2.4 不定长帧的变通

阻塞模式不知道对方发多少字节，两种变通：

```c
/* 变通一：逐字节收，凑满一帧（用超时判断帧尾） */
uint8_t b; int i = 0;
while (HAL_UART_Receive(&huart3, &b, 1, 10) == HAL_OK) { rx[i++] = b; }

/* 变通二：固定协议长度，直接一次收 N 字节 */
HAL_UART_Receive(&huart3, rx, 8, 200);   // 协议规定每帧 8 字节
```

### 2.5 落地 checklist

- [ ] 只用 `HAL_UART_Transmit` / `HAL_UART_Receive`
- [ ] 注意 `timeout` 单位是 ms，且是**整体超时**（不是每字节）
- [ ] 阻塞函数不能在中断里调用（会卡死）

---

## 3. 中断模式（IT）

**原理**：启动接收后 CPU 不等待，每收满约定字节数触发一次中断，在回调里处理。适合中低吞吐、需即时响应。

### 3.1 额外配置（比阻塞模式多 3 处）

1. **NVIC 使能** `USART3_IRQn`（第 1.2 节 ④，已落地）。
2. **中断服务函数** `USART3_IRQHandler()`，内部必须调 `HAL_UART_IRQHandler(&huart3)`（已落地，`Core/Src/stm32u5xx_it.c:270`）。
3. **回调函数** `HAL_UART_RxCpltCallback` / `HAL_UART_TxCpltCallback`，在 `usart.c` 的 USER CODE 段实现。

### 3.2 涉及函数

| 函数 | 作用 |
|---|---|
| `HAL_UART_Transmit_IT(&huart3, buf, len)` | 非阻塞发送，发完进 `TxCpltCallback` |
| `HAL_UART_Receive_IT(&huart3, buf, len)` | 非阻塞接收，收满 `len` 进 `RxCpltCallback` |
| `HAL_UART_IRQHandler(&huart3)` | 在 `USART3_IRQHandler` 里调用（HAL 内部处理） |
| `__HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE)` | 使能 IDLE 中断（真机，判帧结束） |
| `__HAL_UART_CLEAR_IDLEFLAG(&huart3)` | 清 IDLE 标志 |

### 3.3 落地示例（逐字节接收）

```c
/* usart.c USER CODE 段 */
uint8_t usart3_rx_byte = 0;

/* 启动：收 1 字节就进中断 */
HAL_UART_Receive_IT(&huart3, &usart3_rx_byte, 1);

/* 收满回调：处理这 1 字节后，立刻续收下一字节 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        /* 处理 usart3_rx_byte：存环形缓冲 / 拼帧 / 回显 ... */
        HAL_UART_Transmit(huart, &usart3_rx_byte, 1, 10);   // 回显

        /* 关键：续收，否则中断链会断 */
        HAL_UART_Receive_IT(&huart3, &usart3_rx_byte, 1);
    }
}

/* 发送完成回调（用 Transmit_IT 时才需要） */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) { /* 置发送完成标志 */ }
}
```

`USART3_IRQHandler`（已落地，`stm32u5xx_it.c:270`）：

```c
void USART3_IRQHandler(void)
{
    /* USER CODE 段：可在此处理 IDLE（帧结束） */
    HAL_UART_IRQHandler(&huart3);   // 必须调用
}
```

### 3.4 不定长帧：加 IDLE 中断（真机）

IT 模式下判「一帧结束」用 IDLE 空闲线中断：

```c
void USART3_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart3);
        /* 一帧结束：取已收字节数、置标志，交给主循环处理 */
    }
    HAL_UART_IRQHandler(&huart3);
}
```

### 3.5 落地 checklist

- [ ] NVIC 使能 `USART3_IRQn`
- [ ] `USART3_IRQHandler` 内调 `HAL_UART_IRQHandler`
- [ ] 实现 `HAL_UART_RxCpltCallback`，且**处理完必须重新调用 `HAL_UART_Receive_IT` 续收**
- [ ] 不定长帧：使能 IDLE 中断并在 IRQ 里判帧结束

> 本项目现状：NVIC、IRQHandler 已就绪，`RxCpltCallback` 目前只写了 LPUART1 分支，USART3 分支需补（见 3.3）。

---

## 4. DMA 模式

**原理**：DMA 硬件自动把 USART3 的 RDR 搬运到内存，CPU 几乎不参与，适合大批量、高吞吐收发。

### 4.1 标准流程（通用 STM32）

**额外配置（比中断模式再多 2 处）：**

1. **DMA 通道**：GPDMA1 CH0，请求 `GPDMA1_REQUEST_USART3_RX`，P→M，字节宽，循环模式（第 1.2 节 ③，已落地）。
2. **两个中断**：
   - `USART3_IRQHandler`（帧结束/错误）；
   - `GPDMA1_Channel0_IRQHandler` → `HAL_DMA_IRQHandler(&handle_GPDMA1_Channel0)`（handler 已落地，`stm32u5xx_it.c:214`）。
3. **NVIC 使能两个 IRQ**：`USART3_IRQn`（已使能）+ `GPDMA1_Channel0_IRQn`（**当前未使能**，定长收满回调需补）。

**涉及函数：**

| 函数 | 作用 |
|---|---|
| `HAL_UART_Receive_DMA(&huart3, buf, len)` | 启动 DMA 接收，收满 `len` 进 `RxCpltCallback` |
| `HAL_UART_Transmit_DMA(&huart3, buf, len)` | 启动 DMA 发送，发完进 `TxCpltCallback` |
| `HAL_UARTEx_ReceiveToIdle_DMA(&huart3, buf, len)` | 收满或 IDLE 均触发 `RxEventCallback`（不定长） |
| `HAL_UART_DMAStop(&huart3)` / `HAL_UART_DMAPause(&huart3)` | 停止 / 暂停 DMA |
| `__HAL_DMA_GET_COUNTER(huart->hdmarx)` | 读剩余计数器，反算已收字节数 |

**标准落地示例（定长 + 循环）：**

```c
uint8_t dma_rx[256];

void Start_Rx(void)
{
    HAL_UART_Receive_DMA(&huart3, dma_rx, sizeof(dma_rx));   // 收满 256 回调
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) { /* 处理整块 dma_rx */ }
}
```

### 4.2 本项目实际流程（GPDMA1 CH0 循环链表 + RTOF 帧结束）

> 与标准差异：Renode 仿真里 USART 模型**不实现 IDLE**，所以本项目改用 **RTOF（接收超时）** 判帧结束——真机同样支持。GPDMA1 CH0 配成**循环链表**持续收，帧处理由 USART3 的 RTOF 中断驱动，不依赖 DMA 的 TC 中断。

已落地的代码路径：

```c
/* ① MspInit：GPDMA1 CH0 循环链表（DMA_LINKEDLIST_CIRCULAR）已配好
 *    请求 GPDMA1_REQUEST_USART3_RX，P→M，字节宽 */

/* ② 启动接收（usart.c: Start_UART_Receive） */
void Start_UART_Receive(void)
{
    huart3.Instance->RTOR = 32u;                 // 32 bit 时间无数据 = 一帧结束
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RTO);  // 使能 RTOF 中断
    HAL_UART_Receive_DMA(&huart3, usart3_dma_rx_buf, USART3_DMA_RX_BUF_SIZE);
}

/* ③ 帧结束（stm32u5xx_it.c: USART3_IRQHandler） */
void USART3_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_RTOF))
    {
        __HAL_UART_CLEAR_FLAG(&huart3, UART_CLEAR_RTOF);
        usart3_frame_handler();   // 按 DMA 计数器算帧长，回显并更新读指针
    }
    HAL_UART_IRQHandler(&huart3);
}

/* ④ 帧处理（usart.c: uart_frame_handler，通用 LPUART1/USART3）
 *    长度 = 缓冲大小 - __HAL_DMA_GET_COUNTER(huart->hdmarx)，
 *    用读写指针追踪，处理回卷时尾段+头段两段 */

/* ⑤ 错误恢复（usart.c: HAL_UART_ErrorCallback 的 USART3 分支） */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);   // 清溢出
        HAL_UART_Receive_DMA(&huart3, usart3_dma_rx_buf, USART3_DMA_RX_BUF_SIZE);
    }
}
```

### 4.3 落地 checklist

- [ ] GPDMA1 CH0 已配置（请求、方向、宽度、循环）
- [ ] `USART3_IRQHandler` 已就位（RTOF/IDLE 判帧）
- [ ] `GPDMA1_Channel0_IRQHandler` → `HAL_DMA_IRQHandler`
- [ ] NVIC：`USART3_IRQn` 已使能；**定长收满回调需额外使能 `GPDMA1_Channel0_IRQn`**
- [ ] 启动 `HAL_UART_Receive_DMA`（循环链表下收满自动回卷）
- [ ] 实现 `HAL_UART_RxCpltCallback` / `HAL_UART_ErrorCallback`
- [ ] 用 `__HAL_DMA_GET_COUNTER` 算实际收了多少，别用「缓冲全长」当本次帧长

---

## 5. 三种模式对比

| 维度 | 阻塞 | 中断 IT | DMA |
|---|---|---|---|
| CPU 占用 | 全程等待 | 每字节一次中断 | 几乎不占用 |
| 代码复杂度 | 最低 | 中 | 最高 |
| 实时性 | 差 | 好 | 最好（大批量） |
| 需要 NVIC | 否 | `USART3_IRQn` | `USART3_IRQn`（定长回调再加 `GPDMA1_Channel0_IRQn`） |
| 需要回调 | 否 | Rx/TxCplt | Rx/TxCplt / Error |
| 帧结束方式 | 超时 / 逐字节 | IDLE（真机） | IDLE 或本项目 RTOF |
| 适用场景 | 调试打印、低频短报文 | 中等吞吐、即时响应 | 大批量、高吞吐 |
| 本项目现状 | ✅ 即用 | ⚠️ 需补 RxCplt 的 USART3 分支 | ✅ 已实现（RTOF 帧结束） |

---

## 6. 常见坑

1. **续收中断链会断**：`RxCpltCallback` 里处理完必须重新调 `HAL_UART_Receive_IT`/`HAL_UART_Receive_DMA`，否则只收一次就停。
2. **DMAR 位陷阱**：DMA 模式出错后**不要回退到 `Receive_IT`**——此时 `CR3.DMAR` 仍置位，HAL 的中断分支会认为「字节归 DMA 管」而不读 RDR，`RXNE` 清不掉 → 中断风暴。错误恢复应继续用 `Receive_DMA`（本项目 `HAL_UART_ErrorCallback` 已按此处理）。
3. **IDLE vs RTOF**：真机两者都有；本项目因 Renode 不实现 IDLE，统一用 RTOF。真机移植时若要 IDLE，用 `HAL_UARTEx_ReceiveToIdle_DMA` 或自定义 IDLE 中断。
4. **定时器单位**：阻塞函数 `timeout` 是**整体超时**（ms），不是每字节超时。
5. **`__HAL_DMA_GET_COUNTER` 含义**：是「剩余未搬运字节数」，且循环模式下是相对当前节点/回卷的累计值，跨块帧会回卷，要用读/写指针两段处理（本项目 `uart_frame_handler` 已处理）。
6. **NVIC 优先级**：多路串口并发时，注意 `SetPriority` 与抢占优先级配置，避免互相打断。
7. **GPDMA1_Channel0_IRQn 未使能**：当前定长收满回调（`RxCpltCallback` 经 TC 触发）不会进来，需 `HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn)`；本项目因用 RTOF 帧结束，此中断非必需。
