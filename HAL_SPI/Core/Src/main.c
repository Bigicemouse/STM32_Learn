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
#define KEY_SAMPLE_INTERVAL_MS 2u // 每隔 2 ms 记录一次按键电平
#define W25Q_RECORD_ADDRESS 0x000000u // 第 0 扇区作为按键事件日志区
#define W25Q_RECORD_SECTOR_SIZE 4096u // W25Q 扇区大小
#define DISPLAY_RECORD_COUNT 10u      // OLED 显示最近 10 条记录
#define W25Q_TIMEOUT_MS 5000u        // Flash 擦写最长等待时间
#define OLED_ADDRESS 0x3Cu           // SSD1306 常用 7 位 I2C 地址
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
static uint8_t key_cur = 1;   // 当前确认的稳定按键电平
static uint8_t key_history = 0xFFu; // 最近 8 次采样记录：1=松开，0=按下
static uint32_t key_sample_tick = 0u; // 上一次按键采样时刻
static uint8_t button_records[DISPLAY_RECORD_COUNT] = {0}; // 最近 10 次按键事件
static uint8_t button_record_count = 0u;
static uint16_t flash_record_index = 0u; // Flash 中下一条日志的偏移
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
static uint8_t W25Q_TransferByte(uint8_t data);
static HAL_StatusTypeDef W25Q_WaitReady(uint32_t timeout_ms);
static uint8_t W25Q_ReadByte(uint32_t address);
static void ButtonRecord_Init(void);
static HAL_StatusTypeDef ButtonRecord_Append(uint8_t value);
static void OLED_Init(void);
static void OLED_ShowButtonRecords(void);

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
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  key_cur = (uint8_t)HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0); // 读取上电时的实际按键电平
  key_history = key_cur == GPIO_PIN_SET ? 0xFFu : 0x00u; // 用初始电平填满采样记录
  key_sample_tick = HAL_GetTick();                       // 初始化周期采样时基
  ButtonRecord_Init();
  OLED_Init();
  OLED_ShowButtonRecords();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if ((HAL_GetTick() - key_sample_tick) >= KEY_SAMPLE_INTERVAL_MS)
    {
      key_sample_tick = HAL_GetTick();
      key_history = (uint8_t)((key_history << 1) |
                              (uint8_t)HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)); // 记录最新一次采样

      if ((key_history == 0x00u) && (key_cur != GPIO_PIN_RESET)) // 最近 8 次均为低电平
      {
        key_cur = GPIO_PIN_RESET; // 综合采样结果，确认按键按下
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // LED 点亮
        (void)ButtonRecord_Append(1u);
        OLED_ShowButtonRecords(); // 按下记录 1
      }
      else if ((key_history == 0xFFu) && (key_cur != GPIO_PIN_SET)) // 最近 8 次均为高电平
      {
        key_cur = GPIO_PIN_SET; // 综合采样结果，确认按键抬起
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // LED 熄灭
        (void)ButtonRecord_Append(0u);
        OLED_ShowButtonRecords(); // 松开记录 0
      }
    }
  }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */

/**
  * @brief System Clock Configuration
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
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
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // 软件 I2C：PB8=SCL，PB9=SDA
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static void W25Q_Select(void)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

static void W25Q_Deselect(void)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

static uint8_t W25Q_TransferByte(uint8_t data)
{
  uint8_t received = 0u;
  (void)HAL_SPI_TransmitReceive(&hspi1, &data, &received, 1u, HAL_MAX_DELAY);
  return received;
}

static uint8_t W25Q_ReadStatus(void)
{
  uint8_t status;

  W25Q_Select();
  (void)W25Q_TransferByte(0x05u); // Read Status Register-1
  status = W25Q_TransferByte(0xFFu);
  W25Q_Deselect();
  return status;
}

static HAL_StatusTypeDef W25Q_WaitReady(uint32_t timeout_ms)
{
  uint32_t start_tick = HAL_GetTick();

  while ((W25Q_ReadStatus() & 0x01u) != 0u)
  {
    if ((HAL_GetTick() - start_tick) >= timeout_ms)
    {
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
}

static void W25Q_WriteEnable(void)
{
  W25Q_Select();
  (void)W25Q_TransferByte(0x06u); // Write Enable
  W25Q_Deselect();
}

static uint8_t W25Q_ReadByte(uint32_t address)
{
  uint8_t data;

  W25Q_Select();
  (void)W25Q_TransferByte(0x03u); // Read Data
  (void)W25Q_TransferByte((uint8_t)(address >> 16));
  (void)W25Q_TransferByte((uint8_t)(address >> 8));
  (void)W25Q_TransferByte((uint8_t)address);
  data = W25Q_TransferByte(0xFFu);
  W25Q_Deselect();
  return data;
}

static HAL_StatusTypeDef W25Q_EraseSector(uint32_t address)
{
  W25Q_WriteEnable();
  W25Q_Select();
  (void)W25Q_TransferByte(0x20u); // 4 KB Sector Erase
  (void)W25Q_TransferByte((uint8_t)(address >> 16));
  (void)W25Q_TransferByte((uint8_t)(address >> 8));
  (void)W25Q_TransferByte((uint8_t)address);
  W25Q_Deselect();
  return W25Q_WaitReady(W25Q_TIMEOUT_MS);
}

static HAL_StatusTypeDef W25Q_ProgramByte(uint32_t address, uint8_t data)
{
  W25Q_WriteEnable();
  W25Q_Select();
  (void)W25Q_TransferByte(0x02u); // Page Program
  (void)W25Q_TransferByte((uint8_t)(address >> 16));
  (void)W25Q_TransferByte((uint8_t)(address >> 8));
  (void)W25Q_TransferByte((uint8_t)address);
  (void)W25Q_TransferByte(data);
  W25Q_Deselect();
  return W25Q_WaitReady(W25Q_TIMEOUT_MS);
}

static void ButtonRecord_Push(uint8_t value)
{
  uint8_t index;

  if (button_record_count < DISPLAY_RECORD_COUNT)
  {
    button_records[button_record_count] = value;
    button_record_count++;
  }
  else
  {
    for (index = 0u; index < (DISPLAY_RECORD_COUNT - 1u); index++)
    {
      button_records[index] = button_records[index + 1u];
    }
    button_records[DISPLAY_RECORD_COUNT - 1u] = value;
  }
}

static void ButtonRecord_Init(void)
{
  uint16_t index;
  uint8_t value;

  button_record_count = 0u;
  flash_record_index = W25Q_RECORD_SECTOR_SIZE;

  for (index = 0u; index < W25Q_RECORD_SECTOR_SIZE; index++)
  {
    value = W25Q_ReadByte(W25Q_RECORD_ADDRESS + index);
    if (value == 0xFFu)
    {
      flash_record_index = index;
      break;
    }
    if ((value == 0u) || (value == 1u))
    {
      ButtonRecord_Push(value);
    }
  }
}

static HAL_StatusTypeDef ButtonRecord_Append(uint8_t value)
{
  uint32_t address;

  if (flash_record_index >= W25Q_RECORD_SECTOR_SIZE)
  {
    if (W25Q_EraseSector(W25Q_RECORD_ADDRESS) != HAL_OK)
    {
      return HAL_ERROR;
    }
    flash_record_index = 0u;
  }

  address = W25Q_RECORD_ADDRESS + flash_record_index;
  if (W25Q_ProgramByte(address, value) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (W25Q_ReadByte(address) != value)
  {
    return HAL_ERROR;
  }

  flash_record_index++;
  ButtonRecord_Push(value);
  return HAL_OK;
}

static void OLED_I2C_Delay(void)
{
  volatile uint8_t delay_count;

  for (delay_count = 0u; delay_count < 8u; delay_count++)
  {
    __NOP();
  }
}

static void OLED_I2C_Start(void)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
  OLED_I2C_Delay();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
  OLED_I2C_Delay();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
}

static void OLED_I2C_Stop(void)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
  OLED_I2C_Delay();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
  OLED_I2C_Delay();
}

static void OLED_I2C_WriteByte(uint8_t data)
{
  uint8_t bit_index;

  for (bit_index = 0u; bit_index < 8u; bit_index++)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9,
                      ((data & 0x80u) != 0u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    OLED_I2C_Delay();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    OLED_I2C_Delay();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    data <<= 1;
  }

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET); // 释放 SDA，留出 ACK 时钟
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
  OLED_I2C_Delay();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
}

static void OLED_Write(uint8_t control, uint8_t data)
{
  OLED_I2C_Start();
  OLED_I2C_WriteByte((uint8_t)(OLED_ADDRESS << 1));
  OLED_I2C_WriteByte(control);
  OLED_I2C_WriteByte(data);
  OLED_I2C_Stop();
}

static void OLED_WriteCommand(uint8_t command)
{
  OLED_Write(0x00u, command);
}

static void OLED_WriteData(uint8_t data)
{
  OLED_Write(0x40u, data);
}

static void OLED_SetPosition(uint8_t page, uint8_t column)
{
  OLED_WriteCommand((uint8_t)(0xB0u | page));
  OLED_WriteCommand((uint8_t)(0x10u | (column >> 4)));
  OLED_WriteCommand((uint8_t)(column & 0x0Fu));
}

static void OLED_Clear(void)
{
  uint8_t page;
  uint8_t column;

  for (page = 0u; page < 8u; page++)
  {
    OLED_SetPosition(page, 0u);
    for (column = 0u; column < 128u; column++)
    {
      OLED_WriteData(0x00u);
    }
  }
}

static void OLED_GetGlyph(char character, uint8_t glyph[5])
{
  static const uint8_t hex_glyphs[16][5] = {
      {0x3Eu, 0x51u, 0x49u, 0x45u, 0x3Eu}, {0x00u, 0x42u, 0x7Fu, 0x40u, 0x00u},
      {0x42u, 0x61u, 0x51u, 0x49u, 0x46u}, {0x21u, 0x41u, 0x45u, 0x4Bu, 0x31u},
      {0x18u, 0x14u, 0x12u, 0x7Fu, 0x10u}, {0x27u, 0x45u, 0x45u, 0x45u, 0x39u},
      {0x3Cu, 0x4Au, 0x49u, 0x49u, 0x30u}, {0x01u, 0x71u, 0x09u, 0x05u, 0x03u},
      {0x36u, 0x49u, 0x49u, 0x49u, 0x36u}, {0x06u, 0x49u, 0x49u, 0x29u, 0x1Eu},
      {0x7Eu, 0x11u, 0x11u, 0x11u, 0x7Eu}, {0x7Fu, 0x49u, 0x49u, 0x49u, 0x36u},
      {0x3Eu, 0x41u, 0x41u, 0x41u, 0x22u}, {0x7Fu, 0x41u, 0x41u, 0x22u, 0x1Cu},
      {0x7Fu, 0x49u, 0x49u, 0x49u, 0x41u}, {0x7Fu, 0x09u, 0x09u, 0x09u, 0x01u}};
  const uint8_t *source = 0;
  uint8_t index;

  if ((character >= '0') && (character <= '9'))
  {
    source = hex_glyphs[(uint8_t)(character - '0')];
  }
  else if ((character >= 'A') && (character <= 'F'))
  {
    source = hex_glyphs[(uint8_t)(character - 'A') + 10u];
  }
  else
  {
    static const uint8_t glyph_l[5] = {0x7Fu, 0x40u, 0x40u, 0x40u, 0x40u};
    static const uint8_t glyph_s[5] = {0x46u, 0x49u, 0x49u, 0x49u, 0x31u};
    static const uint8_t glyph_h[5] = {0x7Fu, 0x08u, 0x08u, 0x08u, 0x7Fu};
    static const uint8_t glyph_colon[5] = {0x00u, 0x36u, 0x36u, 0x00u, 0x00u};
    static const uint8_t glyph_x[5] = {0x44u, 0x28u, 0x10u, 0x28u, 0x44u};
    static const uint8_t glyph_space[5] = {0u, 0u, 0u, 0u, 0u};

    if (character == 'L') source = glyph_l;
    else if (character == 'S') source = glyph_s;
    else if (character == 'H') source = glyph_h;
    else if (character == ':') source = glyph_colon;
    else if (character == 'x') source = glyph_x;
    else source = glyph_space;
  }

  for (index = 0u; index < 5u; index++)
  {
    glyph[index] = source[index];
  }
}

static void OLED_DrawChar(char character)
{
  uint8_t glyph[5];
  uint8_t column;

  OLED_GetGlyph(character, glyph);
  for (column = 0u; column < 5u; column++)
  {
    OLED_WriteData(glyph[column]);
  }
  OLED_WriteData(0x00u);
}

static void OLED_ShowButtonRecords(void)
{
  uint8_t index;

  OLED_SetPosition(3u, 34u);
  for (index = 0u; index < DISPLAY_RECORD_COUNT; index++)
  {
    if (index < button_record_count)
    {
      OLED_DrawChar(button_records[index] != 0u ? '1' : '0');
    }
    else
    {
      OLED_DrawChar(' ');
    }
  }
}

static void OLED_Init(void)
{
  HAL_Delay(100u);
  OLED_WriteCommand(0xAEu);
  OLED_WriteCommand(0xD5u); OLED_WriteCommand(0x80u);
  OLED_WriteCommand(0xA8u); OLED_WriteCommand(0x3Fu);
  OLED_WriteCommand(0xD3u); OLED_WriteCommand(0x00u);
  OLED_WriteCommand(0x40u);
  OLED_WriteCommand(0x8Du); OLED_WriteCommand(0x14u);
  OLED_WriteCommand(0x20u); OLED_WriteCommand(0x02u);
  OLED_WriteCommand(0xA1u);
  OLED_WriteCommand(0xC8u);
  OLED_WriteCommand(0xDAu); OLED_WriteCommand(0x12u);
  OLED_WriteCommand(0x81u); OLED_WriteCommand(0xCFu);
  OLED_WriteCommand(0xD9u); OLED_WriteCommand(0xF1u);
  OLED_WriteCommand(0xDBu); OLED_WriteCommand(0x30u);
  OLED_WriteCommand(0xA4u);
  OLED_WriteCommand(0xA6u);
  OLED_WriteCommand(0xAFu);
  OLED_Clear();
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
