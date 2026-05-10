/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
volatile uint8_t rx_byte = 0;           // ✅ 1 байт для приёма команды
volatile uint8_t cmd_received = 0;      // ✅ Флаг: команда получена

// ✅ Колбэк: вызывается при завершении приёма 1 байта
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    if(huart->Instance == USART1){
        cmd_received = 1;  // Сигнал главному циклу
        // ⚠️ Переармивание делаем в main(), а не здесь!
    }
}

// ✅ Чтение из Flash (безопасное)
void read_flash_safe(uint8_t *buffer, uint32_t address, uint32_t size){
    // Проверяем, что адрес в пределах Flash
    if(address >= 0x08000000 && address + size <= 0x08000000 + 64*1024){ // 64KB Flash для F103C8
        for(uint32_t i = 0; i < size; ++i){
            buffer[i] = *(__IO uint8_t*)(address + i);
        }
    } else {
        // Заполняем нулями если адрес неверный
        for(uint32_t i = 0; i < size; ++i) buffer[i] = 0;
    }
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */
void Debug_LED_Toggle(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void Debug_LED_Toggle(void){
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */

  // ✅ Тестовое мигание при старте (проверка, что код работает)
  for(int i = 0; i < 3; i++){
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
      HAL_Delay(200);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
      HAL_Delay(200);
  }

  // ✅ Запуск приёма первой команды
  HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_byte, 1);

  uint8_t ack_response = 0x22;  // Ответ для рукопожатия
  uint8_t tx_buffer[64];        // Буфер для передачи
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    if(cmd_received == 1){
        cmd_received = 0;  // ✅ Сброс флага

        // ✅ СРАЗУ переармиваем приём следующей команды (КРИТИЧНО!)
        HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_byte, 1);

        // ✅ Отладка: мигнём светодиодом при получении любой команды
        Debug_LED_Toggle();

        // === Обработка команды 0x01 ===
        if(rx_byte == 0x01){

            // ✅ Мигнём ещё раз для визуального подтверждения
            Debug_LED_Toggle();

            // 1. Отправляем подтверждение (0x22)
            if(HAL_UART_Transmit(&huart1, &ack_response, 1, 100) != HAL_OK){
                // Обработка ошибки передачи (опционально)
                Debug_LED_Toggle(); // Дополнительная индикация ошибки
            }
            HAL_Delay(10);  // Небольшая пауза между пакетами

            // 2. Читаем данные из Flash (или замените на свои данные)
            read_flash_safe(tx_buffer, 0x08000000, 64);

            // 3. Отправляем буфер
            if(HAL_UART_Transmit(&huart1, tx_buffer, 64, 500) != HAL_OK){
                Debug_LED_Toggle(); // Индикация ошибки
            }

            // 4. Финальное мигание после успешной отправки
            Debug_LED_Toggle();
            HAL_Delay(50);
            Debug_LED_Toggle();
        }
        // === Можно добавить другие команды ===
        // else if(rx_byte == 0x02){ ... }
    }

    // Фоновая задача: периодическое мигание "жив ли контроллер"
    static uint32_t last_heartbeat = 0;
    if(HAL_GetTick() - last_heartbeat > 1000){
        last_heartbeat = HAL_GetTick();
        // Слабое мигание раз в секунду (не мешает основной логике)
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);  // Если есть светодиод на PC13
    }

    /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();  // ✅ Обязательно для PB2!

  // LED на PC13 (если есть)
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  // ✅ LED на PB2 (основной индикатор)
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // ✅ Начальное состояние: светодиод выключен
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
}

/**
  * @brief  Error Handler
  */
void Error_Handler(void)
{
  __disable_irq();
  // ✅ Индикация ошибки: быстрое мигание
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
    HAL_Delay(100);
  }
}
