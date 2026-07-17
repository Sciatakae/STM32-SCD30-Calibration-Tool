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
#include "scd30_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include <stdio.h>     // printf
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define sensirion_hal_sleep_us sensirion_i2c_hal_sleep_usec
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// int16_t error = NO_ERROR;

 volatile uint8_t streaming = 0; // initialize the streaming flag to 0 (not streaming)
 volatile uint8_t calibration = 0; // requested ASC state (0=off, 1=on); set by UART menu
 uint8_t last_calibration = 0; // last ASC state we already sent to the sensor (edge detect)

 volatile char rxData; // one byte recieved from UART

 // flags and variables are volatile so the ISR can access them
 // ISR means Interrupt Service Routine; watches for events 
 // events mean interrupts; that means a button press is a trigger for the ISR
 
 

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int __io_putchar(int ch) // this function is used by printf to output characters to the UART
{
  uint8_t c = (uint8_t)ch; // cast the int to uint8_t to get the character value
  HAL_UART_Transmit(&huart2, &c, 1, HAL_MAX_DELAY); // transmit the character over UART2
  return ch; // return the character value
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
  MX_I2C1_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  int16_t error = NO_ERROR;
  float co2_concentration = 0.0f;
  float temperature = 0.0f;
  float humidity = 0.0f;
  int8_t serial_number[32] = {0};

  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
  printf("Welcome, SCD30 user!\r\n");
  printf(" \r\n");
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
  printf("This is a SCD30 sensor calibration tool\r\n");
  printf("\r\n");
  printf("Follow the on-screen instructions to calibrate the sensor.\r\n");
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
  printf(" \r\n");
  printf(" \r\n");
  printf("1: Start Streaming\r\n");
  printf("0: Stop Streaming\r\n");
  printf("C: Turn on ASC (Automatic Self-Calibration)\r\n");
  printf("X: Turn off ASC (Automatic Self-Calibration)\r\n");
  printf("H: Show Commands\r\n");
  printf(" \r\n");
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");

  sensirion_i2c_hal_init();

  scd30_init(SCD30_I2C_ADDR_61);
  scd30_stop_periodic_measurement();
  scd30_soft_reset();

  sensirion_hal_sleep_us(1500000);

  // ASC is menu-controlled (C/X). Do not enable it automatically at boot.
  printf("ASC is OFF at boot.\r\n");

  // Kickoff continuous measurement (needed for CO2 reads; ASC also needs this mode)
  error = scd30_start_periodic_measurement(0);
  if (error != NO_ERROR) {
    printf("error start_periodic_measurement(): %i\r\n", error);
  }

  error = scd30_read_serial_number(serial_number, 32);
  if (error != NO_ERROR) {
    printf("error read_serial_number(): %i\r\n", error);
  }

  /*
   the code above reads the serial number of the SCD30 sensor and prints it to the console.
   If there is an error during the read operation, it prints the error code instead.

   the variables are named error because the code will check for errors, find none and then overwrite the variable with a different error check
   all of the error checks are done in the same way, so the variable name is reused for each check.
  */

  // error = scd30_read_firmware_version(&major, &minor);
  // if (error != NO_ERROR) {
  //   printf("error read_firmware_version(): %i\r\n", error);
  // } else {
  //   printf("major: %u minor: %u\r\n", major, minor);
  // }

  // arm RX interrupts
  HAL_UART_Receive_IT(&huart2, (uint8_t*)&rxData, 1);
  // tells the HAL: "when one byte arrives, put that byte into rxData and call the callback function"

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // Edge detect: only talk to the sensor when the menu request CHANGES

    if (calibration != last_calibration) {
      error = scd30_activate_auto_calibration(calibration ? 1 : 0);
      if (error != NO_ERROR) {
        printf("Error setting ASC to %u: %d\r\n", calibration, error);
      } else if (calibration) {
        printf("ASC enabled.\r\n");
      } else {
        printf("ASC disabled.\r\n");
      }
      last_calibration = calibration;
    }

    sensirion_hal_sleep_us(1000000); // wait for 1000ms before reading the measurement data

    error = scd30_blocking_read_measurement_data(&co2_concentration,
                                                 &temperature, &humidity);
    if (error != NO_ERROR) {
      printf("error blocking_read_measurement_data(): %i\r\n", error);
    } else if (streaming) {
      // only print fresh data after a successful read
      printf("co2: %.2f temp: %.2f humidity: %.2f\r\n",
             co2_concentration, temperature, humidity);
      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    }

    /*
     * here im thinking about re adding another led color (in the bigger controller, so it can blink in the calibration state)
     */
  }

  /* 

   If you get -1 with errors in your output, its most likely something physical.

   Check your wiring connections 

    Vin = 3.3V
    GND = GND
    SCL = PB8
    SDA = PB9
    SEL = GND 
  
    RDY & PWM = unused

    (for I2C address 0x61)

  */

  /* USER CODE END 3 */
}

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10D19CE4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);


  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// case switch function will go here, format taken from UNT's Embedded Systems course, UART Lab

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance != USART2) {
    return;
  }

  switch (rxData) {
    case '1':
      streaming = 1;
      break;
    case '0':
      streaming = 0;
      break;
    case 'C':
    case 'c':
      calibration = 1;   // request ASC on; loop edge-detect sends the I2C command once
      break;
    case 'X':
    case 'x':
      calibration = 0;   // request ASC off; loop edge-detect sends the I2C command once
      break;
    case 'H':
    case 'h':
      printf("1: Start Streaming\r\n");
      printf("0: Stop Streaming\r\n");
      printf("C: Turn on ASC (Automatic Self-Calibration)\r\n");
      printf("X: Turn off ASC (Automatic Self-Calibration)\r\n");
      printf("H: Show Commands\r\n");
      break;
    default:
      break;
  }

  // must re-arm or you only get one key ever
  HAL_UART_Receive_IT(&huart2, (uint8_t *)&rxData, 1);
}

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
