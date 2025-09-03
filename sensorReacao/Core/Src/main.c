/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_PLAYER1_Pin GPIO_PIN_9
#define LED_PLAYER1_GPIO_Port GPIOC

#define LED_PLAYER2_Pin GPIO_PIN_8
#define LED_PLAYER2_GPIO_Port GPIOB
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t uwTick_LED_On = 0;
volatile uint32_t uwReactionTime_P1 = 0;
volatile uint32_t uwReactionTime_P2 = 0;
volatile uint32_t uwHighScore = 0xFFFFFFFF;
volatile uint8_t ubTestInProgress = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Send_UART_Message(char *message);
void StartReactionTest(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
  Send_UART_Message("Jogo Inicializado\r\n\n");
  /* USER CODE END 2 */

  while (1)
  {
      // Botão START inicia o jogo
      if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5) == GPIO_PIN_RESET)
      {
          HAL_Delay(50);
          while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5) == GPIO_PIN_RESET);
          StartReactionTest();
      }
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* TIM2 Callback: acende LED central após delay aleatório */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2 && ubTestInProgress)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET); // LED central liga

        // Reinicia TIM3 do zero
        __HAL_TIM_SET_COUNTER(&htim3, 0);
        uwTick_LED_On = 0;

        // Inicia Input Capture para Player1 (CH1) e Player2 (CH2)
        HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
        HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);
    }
}

/* TIM3 Input Capture Callback: mede tempo de reação */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3 && ubTestInProgress)
    {
        char msg[100];

        if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) // Player1
        {
            uint32_t ticks = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            uwReactionTime_P1 = ticks / 10; // converte o valor de micro segundo para ms
            HAL_TIM_IC_Stop_IT(&htim3, TIM_CHANNEL_1);
        }

        if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) // Player2
        {
            uint32_t ticks = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
            uwReactionTime_P2 = ticks / 10; // converte o valor de micro segundo para ms
            HAL_TIM_IC_Stop_IT(&htim3, TIM_CHANNEL_2);
        }

        // Se ambos já reagiram
        if (uwReactionTime_P1 && uwReactionTime_P2)
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); // LED central desliga

            // Envie o tempo dos dois players
            sprintf(msg, "Player1: %lu ms\r\n", uwReactionTime_P1);
            Send_UART_Message(msg);
            sprintf(msg, "Player2: %lu ms\r\n", uwReactionTime_P2);
            Send_UART_Message(msg);

            // Define vencedor
            if (uwReactionTime_P1 < uwReactionTime_P2)
            {
                Send_UART_Message("Vencedor: Player1\r\n");
                HAL_GPIO_WritePin(GPIOC, LED_PLAYER1_Pin, GPIO_PIN_SET); // LED P1 liga
            }
            else if (uwReactionTime_P2 < uwReactionTime_P1)
            {
                Send_UART_Message("Vencedor: Player2\r\n");
                HAL_GPIO_WritePin(GPIOB, LED_PLAYER2_Pin, GPIO_PIN_SET); // LED P2 liga
            }
            else
            {
                Send_UART_Message("Empate!\r\n");
            }

            // Atualiza o HighScore
            uint32_t menor = (uwReactionTime_P1 < uwReactionTime_P2) ? uwReactionTime_P1 : uwReactionTime_P2;
            if (menor < uwHighScore) uwHighScore = menor;
            sprintf(msg, "HighScore: %lu ms\r\n\n", uwHighScore);
            Send_UART_Message(msg);

            // Reinicia variáveis para o próximo teste
            uwReactionTime_P1 = 0;
            uwReactionTime_P2 = 0;
            ubTestInProgress = 0;
        }
    }
}

/* Função para enviar mensagem via UART */
void Send_UART_Message(char *message)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)message, strlen(message), HAL_MAX_DELAY);
}

/* Inicia o teste de reação */
void StartReactionTest(void)
{
    if (ubTestInProgress == 0)
    {
        ubTestInProgress = 1;
        Send_UART_Message("Inicio do Teste...\r\n");

        // Reseta os LEDs de vencedor
        HAL_GPIO_WritePin(GPIOC, LED_PLAYER1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, LED_PLAYER2_Pin, GPIO_PIN_RESET);

        // Delay aleatório 1-6 s em ticks do TIM2 (0,1 ms/tick)
        uint32_t random_delay_ticks = (rand() % 50000) + 10000;

        __HAL_TIM_SET_COUNTER(&htim2, 0);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, random_delay_ticks);
        HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
