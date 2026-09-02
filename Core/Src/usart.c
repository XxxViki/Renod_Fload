/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
/* UART4 TX DMA（GPDMA1 CH2，UART4_TX，M2P 普通链表）—— 手写新增，CubeMX 不覆盖 */
DMA_NodeTypeDef Node_GPDMA1_Channel2;
DMA_QListTypeDef List_GPDMA1_Channel2;
DMA_HandleTypeDef handle_GPDMA1_Channel2;
/* USER CODE END 0 */

UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart4;
UART_HandleTypeDef huart5;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;
DMA_NodeTypeDef Node_LPDMA1_Channel0;
DMA_QListTypeDef List_LPDMA1_Channel0;
DMA_HandleTypeDef handle_LPDMA1_Channel0;
DMA_NodeTypeDef Node_GPDMA1_Channel1;
DMA_QListTypeDef List_GPDMA1_Channel1;
DMA_HandleTypeDef handle_GPDMA1_Channel1;
DMA_NodeTypeDef Node_GPDMA1_Channel0;
DMA_QListTypeDef List_GPDMA1_Channel0;
DMA_HandleTypeDef handle_GPDMA1_Channel0;

/* LPUART1 init function */

void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 209700;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}
/* UART4 init function */
void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_RS485Ex_Init(&huart4, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}
/* UART5 init function */
void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  huart5.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart5.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart5.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart5, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart5, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}
/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}
/* USART3 init function */

void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  DMA_NodeConfTypeDef nodeconfig= {0};
  DMA_NodeConfTypeDef NodeConfig= {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(uartHandle->Instance==LPUART1)
  {
  /* USER CODE BEGIN LPUART1_MspInit 0 */

  /* USER CODE END LPUART1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
    PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK3;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* LPUART1 clock enable */
    __HAL_RCC_LPUART1_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**LPUART1 GPIO Configuration
    PC0     ------> LPUART1_RX
    PC1     ------> LPUART1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* LPUART1 DMA Init */
    /* LPDMA1_REQUEST_LPUART1_RX Init */
    nodeconfig.NodeType = DMA_LPDMA_LINEAR_NODE;
    nodeconfig.Init.Request = LPDMA1_REQUEST_LPUART1_RX;
    nodeconfig.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    nodeconfig.Init.Direction = DMA_PERIPH_TO_MEMORY;
    nodeconfig.Init.SrcInc = DMA_SINC_FIXED;
    nodeconfig.Init.DestInc = DMA_DINC_FIXED;
    nodeconfig.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    nodeconfig.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    nodeconfig.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    nodeconfig.Init.Mode = DMA_NORMAL;
    nodeconfig.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    nodeconfig.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    if (HAL_DMAEx_List_BuildNode(&nodeconfig, &Node_LPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    if (HAL_DMAEx_List_InsertNode(&List_LPDMA1_Channel0, NULL, &Node_LPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    if (HAL_DMAEx_List_SetCircularMode(&List_LPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    handle_LPDMA1_Channel0.Instance = LPDMA1_Channel0;
    handle_LPDMA1_Channel0.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_LPDMA1_Channel0.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    handle_LPDMA1_Channel0.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_LPDMA1_Channel0.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;
    if (HAL_DMAEx_List_Init(&handle_LPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    if (HAL_DMAEx_List_LinkQ(&handle_LPDMA1_Channel0, &List_LPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle, hdmarx, handle_LPDMA1_Channel0);

    if (HAL_DMA_ConfigChannelAttributes(&handle_LPDMA1_Channel0, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* LPUART1 interrupt Init */
    HAL_NVIC_SetPriority(LPUART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(LPUART1_IRQn);
  /* USER CODE BEGIN LPUART1_MspInit 1 */

  /* USER CODE END LPUART1_MspInit 1 */
  }
  else if(uartHandle->Instance==UART4)
  {
  /* USER CODE BEGIN UART4_MspInit 0 */

  /* USER CODE END UART4_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_UART4;
    PeriphClkInit.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* UART4 clock enable */
    __HAL_RCC_UART4_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**UART4 GPIO Configuration
    PA15 (JTDI)     ------> UART4_DE
    PC10     ------> UART4_TX
    PC11     ------> UART4_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* UART4 DMA Init */
    /* GPDMA1_REQUEST_UART4_RX Init */
    NodeConfig.NodeType = DMA_GPDMA_LINEAR_NODE;
    NodeConfig.Init.Request = GPDMA1_REQUEST_UART4_RX;
    NodeConfig.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    NodeConfig.Init.Direction = DMA_PERIPH_TO_MEMORY;
    NodeConfig.Init.SrcInc = DMA_SINC_FIXED;
    NodeConfig.Init.DestInc = DMA_DINC_FIXED;
    NodeConfig.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    NodeConfig.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    NodeConfig.Init.SrcBurstLength = 1;
    NodeConfig.Init.DestBurstLength = 1;
    NodeConfig.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
    NodeConfig.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    NodeConfig.Init.Mode = DMA_NORMAL;
    NodeConfig.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    NodeConfig.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    NodeConfig.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    if (HAL_DMAEx_List_BuildNode(&NodeConfig, &Node_GPDMA1_Channel1) != HAL_OK)
    {
      Error_Handler();
    }

    if (HAL_DMAEx_List_InsertNode(&List_GPDMA1_Channel1, NULL, &Node_GPDMA1_Channel1) != HAL_OK)
    {
      Error_Handler();
    }

    if (HAL_DMAEx_List_SetCircularMode(&List_GPDMA1_Channel1) != HAL_OK)
    {
      Error_Handler();
    }

    handle_GPDMA1_Channel1.Instance = GPDMA1_Channel1;
    handle_GPDMA1_Channel1.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel1.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    handle_GPDMA1_Channel1.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    handle_GPDMA1_Channel1.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel1.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;
    if (HAL_DMAEx_List_Init(&handle_GPDMA1_Channel1) != HAL_OK)
    {
      Error_Handler();
    }

    if (HAL_DMAEx_List_LinkQ(&handle_GPDMA1_Channel1, &List_GPDMA1_Channel1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle, hdmarx, handle_GPDMA1_Channel1);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel1, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* UART4 interrupt Init */
    HAL_NVIC_SetPriority(UART4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(UART4_IRQn);
  /* USER CODE BEGIN UART4_MspInit 1 */
    /* ---- UART4 TX DMA（GPDMA1 CH2，UART4_TX，M2P 普通/单次链表）----
     * RX 用循环链表（上面 CubeMX 已配 GPDMA1 CH1），TX 用单次链表：
     * 发送完成进 HAL_UART_TxCpltCallback，链表不循环（发完即停）。
     * RS485 方向控制：DE=PA15 配 AF8_UART4，由硬件自动拉高/拉低，
     * 无需软件管理（HAL_RS485Ex_Init 已配好断言/去断言时序）。 */
    NodeConfig.NodeType = DMA_GPDMA_LINEAR_NODE;
    NodeConfig.Init.Request = GPDMA1_REQUEST_UART4_TX;
    NodeConfig.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    NodeConfig.Init.Direction = DMA_MEMORY_TO_PERIPH;
    NodeConfig.Init.SrcInc = DMA_SINC_INCREMENTED;
    NodeConfig.Init.DestInc = DMA_DINC_FIXED;
    NodeConfig.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    NodeConfig.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    NodeConfig.Init.SrcBurstLength = 1;
    NodeConfig.Init.DestBurstLength = 1;
    NodeConfig.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
    NodeConfig.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    NodeConfig.Init.Mode = DMA_NORMAL;
    NodeConfig.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    NodeConfig.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    NodeConfig.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    if (HAL_DMAEx_List_BuildNode(&NodeConfig, &Node_GPDMA1_Channel2) != HAL_OK)
    {
      Error_Handler();
    }
    if (HAL_DMAEx_List_InsertNode(&List_GPDMA1_Channel2, NULL, &Node_GPDMA1_Channel2) != HAL_OK)
    {
      Error_Handler();
    }

    handle_GPDMA1_Channel2.Instance = GPDMA1_Channel2;
    handle_GPDMA1_Channel2.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel2.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    handle_GPDMA1_Channel2.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    handle_GPDMA1_Channel2.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel2.InitLinkedList.LinkedListMode = DMA_LINKEDLIST;  /* 单次，非循环 */
    if (HAL_DMAEx_List_Init(&handle_GPDMA1_Channel2) != HAL_OK)
    {
      Error_Handler();
    }
    if (HAL_DMAEx_List_LinkQ(&handle_GPDMA1_Channel2, &List_GPDMA1_Channel2) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle, hdmatx, handle_GPDMA1_Channel2);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel2, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }
  /* USER CODE END UART4_MspInit 1 */
  }
  else if(uartHandle->Instance==UART5)
  {
  /* USER CODE BEGIN UART5_MspInit 0 */

  /* USER CODE END UART5_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_UART5;
    PeriphClkInit.Uart5ClockSelection = RCC_UART5CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* UART5 clock enable */
    __HAL_RCC_UART5_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**UART5 GPIO Configuration
    PC12     ------> UART5_TX
    PD2     ------> UART5_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN UART5_MspInit 1 */

  /* USER CODE END UART5_MspInit 1 */
  }
  else if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA10     ------> USART1_RX
    PB6     ------> USART1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
    PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PA5     ------> USART3_RX
    PB10     ------> USART3_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART3 DMA Init */
    /* GPDMA1_REQUEST_USART3_RX Init */
    NodeConfig.NodeType = DMA_GPDMA_LINEAR_NODE;
    NodeConfig.Init.Request = GPDMA1_REQUEST_USART3_RX;
    NodeConfig.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    NodeConfig.Init.Direction = DMA_PERIPH_TO_MEMORY;
    NodeConfig.Init.SrcInc = DMA_SINC_FIXED;
    NodeConfig.Init.DestInc = DMA_DINC_FIXED;
    NodeConfig.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    NodeConfig.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    NodeConfig.Init.SrcBurstLength = 1;
    NodeConfig.Init.DestBurstLength = 1;
    NodeConfig.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
    NodeConfig.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    NodeConfig.Init.Mode = DMA_NORMAL;
    NodeConfig.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    NodeConfig.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    NodeConfig.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    if (HAL_DMAEx_List_BuildNode(&NodeConfig, &Node_GPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    if (HAL_DMAEx_List_InsertNode(&List_GPDMA1_Channel0, NULL, &Node_GPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    if (HAL_DMAEx_List_SetCircularMode(&List_GPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    handle_GPDMA1_Channel0.Instance = GPDMA1_Channel0;
    handle_GPDMA1_Channel0.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel0.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    handle_GPDMA1_Channel0.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    handle_GPDMA1_Channel0.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel0.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;
    if (HAL_DMAEx_List_Init(&handle_GPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    if (HAL_DMAEx_List_LinkQ(&handle_GPDMA1_Channel0, &List_GPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle, hdmarx, handle_GPDMA1_Channel0);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel0, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspInit 1 */
    /* USART3 全局中断使能（RTOF 接收超时=帧结束，走 USART3_IRQHandler） */
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==LPUART1)
  {
  /* USER CODE BEGIN LPUART1_MspDeInit 0 */

  /* USER CODE END LPUART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_LPUART1_CLK_DISABLE();

    /**LPUART1 GPIO Configuration
    PC0     ------> LPUART1_RX
    PC1     ------> LPUART1_TX
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0|GPIO_PIN_1);

    /* LPUART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* LPUART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(LPUART1_IRQn);
  /* USER CODE BEGIN LPUART1_MspDeInit 1 */

  /* USER CODE END LPUART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==UART4)
  {
  /* USER CODE BEGIN UART4_MspDeInit 0 */

  /* USER CODE END UART4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART4_CLK_DISABLE();

    /**UART4 GPIO Configuration
    PA15 (JTDI)     ------> UART4_DE
    PC10     ------> UART4_TX
    PC11     ------> UART4_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11);

    /* UART4 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* UART4 interrupt Deinit */
    HAL_NVIC_DisableIRQ(UART4_IRQn);
  /* USER CODE BEGIN UART4_MspDeInit 1 */

  /* USER CODE END UART4_MspDeInit 1 */
  }
  else if(uartHandle->Instance==UART5)
  {
  /* USER CODE BEGIN UART5_MspDeInit 0 */

  /* USER CODE END UART5_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART5_CLK_DISABLE();

    /**UART5 GPIO Configuration
    PC12     ------> UART5_TX
    PD2     ------> UART5_RX
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_12);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);

  /* USER CODE BEGIN UART5_MspDeInit 1 */

  /* USER CODE END UART5_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA10     ------> USART1_RX
    PB6     ------> USART1_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_10);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration
    PA5     ------> USART3_RX
    PB10     ------> USART3_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

    /* USART3 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void process_Host_data()
{

}

#define RX_BUF_SIZE 1
uint8_t rx_byte = 0; // 单字节接收缓冲区（IT 路径备用）

/* ---------------------------------------------------------------------------
 * LPUART1 DMA 接收（LPDMA1 CH0 循环链表）
 * 缓冲必须放 SRAM4（0x28000000）：U5 的 LPDMA 只能访问 SRAM4，
 * 由链接脚本 .sram4_section 收集（见 STM32U585RITXQ_FLASH.ld）
 * Renode 仿真说明：stm32u5_lpdma_stub.py + lpuart1_lpdma_hook.py 还原
 * 每字节搬运/链表回卷/通道中断；仿真中 IDLE 事件不会产生，只有块完成(TC)回调
 * ------------------------------------------------------------------------- */
#define LPUART1_DMA_RX_BUF_SIZE 1024
__attribute__((section(".sram4_buffer"), aligned(4)))
uint8_t lpuart1_dma_rx_buf[LPUART1_DMA_RX_BUF_SIZE];

/* ---------------------------------------------------------------------------
 * USART3 DMA 接收（GPDMA1 CH0 循环链表）
 * GPDMA1 可访问全部 SRAM，缓冲放普通 RAM 即可（无 LPDMA 的 SRAM4 限制）
 * ------------------------------------------------------------------------- */
#define USART3_DMA_RX_BUF_SIZE 256
uint8_t usart3_dma_rx_buf[USART3_DMA_RX_BUF_SIZE];

/* ---------------------------------------------------------------------------
 * UART4 DMA 接收（GPDMA1 CH1 循环链表）
 * RS485 半双工：DE=PA15（AF8_UART4）硬件自动方向控制，收发互斥由总线仲裁
 * ------------------------------------------------------------------------- */
#define UART4_DMA_RX_BUF_SIZE 256
uint8_t uart4_dma_rx_buf[UART4_DMA_RX_BUF_SIZE];

/**
  * @brief  帧到达处理（通用）：回显自上次读取位置以来的新数据（读写指针追踪）
  * @param  huart: 串口句柄   buf/buf_size: 该串口的 DMA 接收缓冲
  * @note   由 RTOF（接收超时=帧结束）触发；真机与 Renode 仿真行为一致。
  *         不能用"缓冲全长-计数器"当本次长度——那是自通道启动/回卷以来的累计值，
  *         会导致旧帧被反复回显（每次多一帧）。写指针回卷（wr < rd）时分两段发。
  */
/* 仿真排障观察点（真机无副作用）：帧处理调用次数 / 最近一次 TX DMA 返回码 */
volatile uint32_t g_uart4_frame_calls = 0;
volatile uint32_t g_uart4_tx_dma_ret = 0xFFFFFFFF;

void uart_frame_handler(UART_HandleTypeDef *huart, uint8_t *buf, uint16_t buf_size)
{
    static uint16_t rd_lpuart1 = 0, rd_usart3 = 0, rd_uart4 = 0;   /* 各串口的读位置 */
    uint16_t *rd;
    if (huart->Instance == LPUART1)      rd = &rd_lpuart1;
    else if (huart->Instance == UART4)   rd = &rd_uart4;
    else                                 rd = &rd_usart3;
    uint16_t wr = (uint16_t)(buf_size - __HAL_DMA_GET_COUNTER(huart->hdmarx));
    if (wr == *rd)
    {
        return;                /* 无新数据（如整块刚被 TC 处理完） */
    }
    /* 回显出口：UART4 走 TX DMA（GPDMA1 CH2，发送完成进 HAL_UART_TxCpltCallback），
     * 其余串口维持阻塞发送。TX 通道忙（HAL_BUSY，RS485 半双工并发）时回退阻塞，
     * 保证数据必达；写指针回卷的两段帧也退回阻塞（单 TX 通道一次只能发一段）。 */
    if (wr > *rd)
    {
        if (huart->Instance == UART4)
        {
            g_uart4_frame_calls++;
            g_uart4_tx_dma_ret = (uint32_t)uart4_transmit_dma(&buf[*rd], (uint16_t)(wr - *rd));
            if (g_uart4_tx_dma_ret != (uint32_t)HAL_OK)
            {
                HAL_UART_Transmit(huart, &buf[*rd], (uint16_t)(wr - *rd), 100);
            }
        }
        else
        {
            HAL_UART_Transmit(huart, &buf[*rd], (uint16_t)(wr - *rd), 100);
        }
    }
    else
    {
        /* 写指针已回卷：尾段 + 头段 */
        HAL_UART_Transmit(huart, &buf[*rd], (uint16_t)(buf_size - *rd), 100);
        HAL_UART_Transmit(huart, buf, wr, 100);
    }
    *rd = wr;
}

/* 兼容旧调用名（LPUART1_IRQHandler 等处使用） */
void lpuart1_frame_handler(void)
{
    uart_frame_handler(&hlpuart1, lpuart1_dma_rx_buf, LPUART1_DMA_RX_BUF_SIZE);
}

/* USART3（GPDMA1 CH0）帧到达：stm32u5xx_it.c 的 USART3_IRQHandler 调用 */
void usart3_frame_handler(void)
{
    uart_frame_handler(&huart3, usart3_dma_rx_buf, USART3_DMA_RX_BUF_SIZE);
}

/* UART4（GPDMA1 CH1）帧到达：stm32u5xx_it.c 的 UART4_IRQHandler 调用 */
void uart4_frame_handler(void)
{
    uart_frame_handler(&huart4, uart4_dma_rx_buf, UART4_DMA_RX_BUF_SIZE);
}

/**
  * @brief  整块完成回调（缓冲收满，DMA 循环链表自动回卷继续收）
  * @note   循环模式下这里不做回显：TC 中断常晚于 RTOF 处理，若在此消费
  *         读指针会把跨块帧拆成两段回显。统一由 RTOF 按帧处理（帧结束后
  *         约 2.2ms 必然触发，rd/wr 的回卷分支本就覆盖跨块帧）。
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == LPUART1)
    {
        if (huart->hdmarx != NULL && huart->hdmarx->State == HAL_DMA_STATE_BUSY)
        {
            /* DMA 循环模式：块边界不回显，等 RTOF 统一按帧处理 */
        }
        else
        {
            // IT 路径备用：单字节回显 + 重启
            HAL_UART_Transmit(huart, &rx_byte, 1, 10);
            HAL_UART_Receive_IT(huart, &rx_byte, 1);
        }
    }
}

/**
  * @brief  初始化完成后启动串口接收（DMA 循环链表 + RTOF 接收超时帧结束）
  * @note   帧结束用 RTOF 而非 IDLE：Renode 的 F7 模型不实现空闲线（IDLE/IDLEIE
  *         是死位），但完整实现了 RTOR/RTOF/RTOIE；真机 LPUART1 两者都支持
  */
void Start_UART_Receive(void)
{
    // LPDMA1 CH0 已由 MX_LPUART1_UART_Init 配好循环链表（hdmarx->Mode=DMA_LINKEDLIST_CIRCULAR）
    hlpuart1.Instance->RTOR = 32u;                  // 32 bit 时间无数据 => 帧结束
    __HAL_UART_ENABLE_IT(&hlpuart1, UART_IT_RTO);   // RTOF 中断使能
    HAL_UART_Receive_DMA(&hlpuart1, lpuart1_dma_rx_buf, LPUART1_DMA_RX_BUF_SIZE);

    // GPDMA1 CH0 已由 MX_USART3_UART_Init 配好循环链表（USART3_RX，hdmarx->Mode=DMA_LINKEDLIST_CIRCULAR）
    huart3.Instance->RTOR = 32u;                    // 32 bit 时间无数据 => 帧结束
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RTO);     // RTOF 中断使能
    HAL_UART_Receive_DMA(&huart3, usart3_dma_rx_buf, USART3_DMA_RX_BUF_SIZE);

    // GPDMA1 CH1 已由 MX_UART4_Init 配好循环链表（UART4_RX，hdmarx->Mode=DMA_LINKEDLIST_CIRCULAR）
    huart4.Instance->RTOR = 32u;                    // 32 bit 时间无数据 => 帧结束
    __HAL_UART_ENABLE_IT(&huart4, UART_IT_RTO);     // RTOF 中断使能
    HAL_UART_Receive_DMA(&huart4, uart4_dma_rx_buf, UART4_DMA_RX_BUF_SIZE);
}

// 如果串口出错（如溢出 ORE），在错误回调中重启 DMA 接收。
// 注意：不要回退到 Receive_IT——此时 CR3.DMAR 仍置位，HAL 的中断分支会认为
// "字节归 DMA 管"而不读 RDR，RXNE 清不掉会造成中断风暴（Renode/真机同理）
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == LPUART1)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UART_Receive_DMA(&hlpuart1, lpuart1_dma_rx_buf, LPUART1_DMA_RX_BUF_SIZE);
    }
    else if (huart->Instance == USART3)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UART_Receive_DMA(&huart3, usart3_dma_rx_buf, USART3_DMA_RX_BUF_SIZE);
    }
    else if (huart->Instance == UART4)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UART_Receive_DMA(&huart4, uart4_dma_rx_buf, UART4_DMA_RX_BUF_SIZE);
    }
}

/**
  * @brief  UART4 TX DMA 发送封装（GPDMA1 CH2，单次链表）
  * @param  pData: 发送缓冲   Size: 字节数
  * @note   RS485 DE 由硬件自动管理；发送完成进 HAL_UART_TxCpltCallback。
  *         同一时刻只允许一路发送（半双工），gState==BUSY_TX 时返回 HAL_BUSY。
  */
HAL_StatusTypeDef uart4_transmit_dma(const uint8_t *pData, uint16_t Size)
{
    HAL_StatusTypeDef ret = HAL_UART_Transmit_DMA(&huart4, pData, Size);
    /* Renode 仿真兼容：HAL 内部用 ATOMIC_SET_BIT(CR3, DMAT)（LDREX/STREX 独占写）
     * 启动 TX，仿真钩子在 STREX 飞行中嵌套搬运会污染这次写，导致 CR3.DMAR 陪葬。
     * 真机上 DMAR 常置，此行是无操作；仿真里把 CR3 修回正确状态。
     * 注意必须保持 bit7=0，避免重复触发 TX 钩子搬运分支。 */
    if (ret == HAL_OK)
    {
        SET_BIT(huart4.Instance->CR3, USART_CR3_DMAR);
    }
    return ret;
}

/**
  * @brief  UART4 发送完成回调：GPDMA1 CH2 单次链表跑完触发
  * @note   这里可置应用层标志（如 tx_done），供上层查询后发起下一帧
  */
volatile uint32_t g_uart4_tx_dma_cplt = 0;    /* TX DMA 完成次数（测试/诊断观察点） */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART4)
    {
        g_uart4_tx_dma_cplt++;               /* 仅 TX DMA 路径会走到这里 */
        /* RS485 发送完成：DE 已由硬件拉低，总线回到接收态 */
        /* 应用层可在此置标志/触发下一帧，例如：g_uart4_tx_done = 1; */
    }
}
/* USER CODE END 1 */
