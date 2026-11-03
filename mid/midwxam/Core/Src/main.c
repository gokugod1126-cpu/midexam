/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typedef enum {
    AUTO_MODE = 0,
    MANUAL_MODE
} SystemState;

// 定義自動模式下的 A, B, C 階段
typedef enum {
    CYCLE_A = 0,
    CYCLE_B,
    CYCLE_C
} AutoCycle;

// 系統狀態變數
volatile SystemState current_system_state = AUTO_MODE;
volatile uint8_t manual_light_state = 0; // 0: 紅燈, 1: 綠燈

// 防彈跳變數
volatile uint32_t last_exti_time = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
void Set_Traffic_Lights(uint8_t red, uint8_t yellow, uint8_t green);
void Handle_Auto_Mode(void);
void Handle_Manual_Mode(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Set_Traffic_Lights(uint8_t red, uint8_t yellow, uint8_t green) {
    HAL_GPIO_WritePin(GPIOC, led_red_Pin, red ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, led_yellow_Pin, yellow ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, led_green_Pin, green ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// 可隨時被手動中斷打斷的延遲函式
uint8_t Interruptible_Delay(uint32_t delay_ms) {
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < delay_ms) {
        if (current_system_state == MANUAL_MODE) {
            return 1; // 被中斷打斷
        }
        HAL_Delay(10);
    }
    return 0;
}

// EXTI3 中斷服務回呼函式 (PC3 按下時切換 自動/手動 模式)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == int_Pin) {
        uint32_t now = HAL_GetTick();
        if (now - last_exti_time > 200) { // 200ms 防彈跳
            last_exti_time = now;

            if (current_system_state == AUTO_MODE) {
                current_system_state = MANUAL_MODE;
                manual_light_state = 0; // 進入手動模式時預設為紅燈
            } else {
                current_system_state = AUTO_MODE;
            }
        }
    }
}

// 自動模式處理邏輯
void Handle_Auto_Mode(void) {
    static AutoCycle current_cycle = CYCLE_A;
    uint32_t green_time = 0, red_time = 0;
    uint32_t yellow_time = 1000; // 黃燈固定 1 秒

    switch (current_cycle) {
        case CYCLE_A: green_time = 5000; red_time = 5000; break;
        case CYCLE_B: green_time = 5000; red_time = 3000; break;
        case CYCLE_C: green_time = 3000; red_time = 5000; break;
    }

    // 綠燈
    Set_Traffic_Lights(0, 0, 1);
    if (Interruptible_Delay(green_time)) return;

    // 黃燈
    Set_Traffic_Lights(0, 1, 0);
    if (Interruptible_Delay(yellow_time)) return;

    // 紅燈
    Set_Traffic_Lights(1, 0, 0);
    if (Interruptible_Delay(red_time)) return;

    // 輪替下一個循環
    switch (current_cycle) {
        case CYCLE_A: current_cycle = CYCLE_B; break;
        case CYCLE_B: current_cycle = CYCLE_C; break;
        case CYCLE_C: current_cycle = CYCLE_A; break;
    }
}

// 手動模式處理邏輯 (支援按「下後放開」才切換紅綠燈)
void Handle_Manual_Mode(void) {
    // 記錄 PC4 的前一次狀態（假設按鈕使用 Pull-up，未按為 HIGH，按下為 LOW）
    static GPIO_PinState last_pc4_state = GPIO_PIN_SET;
    GPIO_PinState current_pc4_state = HAL_GPIO_ReadPin(GPIOC, mode_Pin);

    // 偵測「上升緣」：從按下 (LOW) 變成 放開 (SET) 的那一瞬間
    if (last_pc4_state == GPIO_PIN_RESET && current_pc4_state == GPIO_PIN_SET) {
        HAL_Delay(20); // 簡易彈跳過濾
        if (HAL_GPIO_ReadPin(GPIOC, mode_Pin) == GPIO_PIN_SET) {
            // 翻轉手動燈號狀態 (0 變 1，1 變 0)
            manual_light_state = !manual_light_state;
        }
    }
    // 更新上一次的按鈕狀態
    last_pc4_state = current_pc4_state;

    // 根據目前的 manual_light_state 輸出燈號
    if (manual_light_state == 0) {
        Set_Traffic_Lights(1, 0, 0); // 紅燈
    } else {
        Set_Traffic_Lights(0, 0, 1); // 綠燈
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (current_system_state == AUTO_MODE) {
      Handle_Auto_Mode();
  } else {
      Handle_Manual_Mode();
  }
  /* USER CODE END WHILE */
}
}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, led_red_Pin|led_yellow_Pin|led_green_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : led_red_Pin led_yellow_Pin led_green_Pin */
  GPIO_InitStruct.Pin = led_red_Pin|led_yellow_Pin|led_green_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : int_Pin */
  GPIO_InitStruct.Pin = int_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(int_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LPUART1_TX_Pin LPUART1_RX_Pin */
  GPIO_InitStruct.Pin = LPUART1_TX_Pin|LPUART1_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF12_LPUART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : mode_Pin */
  GPIO_InitStruct.Pin = mode_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(mode_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
