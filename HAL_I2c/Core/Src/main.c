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
/* SSD1306 使用 7 位地址 0x3C；HAL 接口要求地址左移 1 位后传入。 */
#define OLED_ADDRESS (0x3CU << 1)
/* 当前 OLED 的有效显示区域为 128 列、64 行。 */
#define OLED_WIDTH   128U
#define OLED_HEIGHT  64U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
/*
 * OLED 显存：SSD1306 将每 8 行像素组织成一页。
 * 128 × 64 / 8 = 1024 字节，每一位对应屏幕上的一个黑白像素。
 */
static uint8_t oled_buffer[OLED_WIDTH * OLED_HEIGHT / 8U];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
static void OLED_Init(void);
static void OLED_UpdateScreen(void);
static void OLED_Clear(void);
static void OLED_DrawDoraemon(uint8_t mouth_open, uint8_t eyes_closed);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * @brief  向 SSD1306 发送一个控制命令。
 * @param  command SSD1306 命令字节。
 * @note   首字节 0x00 表示后续字节为命令，而不是显示数据。
 */
static void OLED_WriteCommand(uint8_t command)
{
  uint8_t packet[2] = {0x00U, command};
  HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS, packet, sizeof(packet), 100U);
}

/**
 * @brief 初始化 128×64 SSD1306 OLED。
 * @note  初始化内容包括寻址方式、扫描方向、对比度、电荷泵和开屏设置。
 */
static void OLED_Init(void)
{
  static const uint8_t init_commands[] = {
      0xAE, 0x20, 0x00, 0xB0, 0xC8, 0x00, 0x10, 0x40,
      0x81, 0x7F, 0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3,
      0x00, 0xD5, 0x80, 0xD9, 0xF1, 0xDA, 0x12, 0xDB,
      0x40, 0x8D, 0x14, 0xAF};
  uint32_t index;

  /* 上电后等待 OLED 电源和控制器稳定。 */
  HAL_Delay(100U);
  for (index = 0U; index < sizeof(init_commands); index++)
  {
    OLED_WriteCommand(init_commands[index]);
  }
}

/**
 * @brief 清空软件显存，使所有像素恢复为黑色背景。
 * @note  本函数只修改 RAM，调用 OLED_UpdateScreen() 后屏幕才会更新。
 */
static void OLED_Clear(void)
{
  uint32_t index;

  for (index = 0U; index < sizeof(oled_buffer); index++)
  {
    oled_buffer[index] = 0U;
  }
}

/**
 * @brief 在软件显存中点亮一个像素。
 * @param x 像素横坐标，范围 0~127。
 * @param y 像素纵坐标，范围 0~63。
 */
static void OLED_DrawPixel(int16_t x, int16_t y)
{
  /* 越界坐标直接忽略，防止写出 oled_buffer 的有效范围。 */
  if ((x >= 0) && (x < (int16_t)OLED_WIDTH) &&
      (y >= 0) && (y < (int16_t)OLED_HEIGHT))
  {
    oled_buffer[x + ((uint16_t)y / 8U) * OLED_WIDTH] |=
        (uint8_t)(1U << ((uint16_t)y & 7U));
  }
}

/**
 * @brief 使用 Bresenham 算法在两点之间绘制直线。
 * @param x0 起点横坐标。
 * @param y0 起点纵坐标。
 * @param x1 终点横坐标。
 * @param y1 终点纵坐标。
 */
static void OLED_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t dy = -((y1 > y0) ? (y1 - y0) : (y0 - y1));
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t error = dx + dy;

  while (1)
  {
    OLED_DrawPixel(x0, y0);
    if ((x0 == x1) && (y0 == y1))
    {
      break;
    }
    if ((2 * error) >= dy)
    {
      error += dy;
      x0 += sx;
    }
    if ((2 * error) <= dx)
    {
      error += dx;
      y0 += sy;
    }
  }
}

/**
 * @brief 绘制椭圆轮廓。
 * @param center_x 椭圆中心横坐标。
 * @param center_y 椭圆中心纵坐标。
 * @param radius_x 水平方向半径。
 * @param radius_y 垂直方向半径。
 */
static void OLED_DrawEllipse(int16_t center_x, int16_t center_y,
                             int16_t radius_x, int16_t radius_y)
{
  int16_t x;
  int32_t y_squared;
  int16_t y;

  /* 逐列计算椭圆上下边界的 y 坐标，再点亮对称的两个像素。 */
  for (x = -radius_x; x <= radius_x; x++)
  {
    y_squared = (int32_t)radius_y * radius_y *
                ((int32_t)radius_x * radius_x - (int32_t)x * x) /
                ((int32_t)radius_x * radius_x);
    for (y = 0; (int32_t)y * y < y_squared; y++)
    {
    }
    OLED_DrawPixel(center_x + x, center_y + y);
    OLED_DrawPixel(center_x + x, center_y - y);
  }
}

/**
 * @brief 在显存中生成一帧哆啦 A 梦头像动画。
 * @param mouth_open 非 0 时绘制张嘴帧，0 时绘制微笑帧。
 * @param eyes_closed 非 0 时绘制闭眼帧，0 时绘制睁眼帧。
 * @note  每次绘制前会先清空显存，完成后需调用 OLED_UpdateScreen()。
 */
static void OLED_DrawDoraemon(uint8_t mouth_open, uint8_t eyes_closed)
{
  int16_t y;

  OLED_Clear();

  /* 绘制头部、面部和身体上半部分轮廓。 */
  OLED_DrawEllipse(64, 31, 29, 29);
  OLED_DrawEllipse(64, 34, 23, 21);
  OLED_DrawLine(45, 54, 39, 63);
  OLED_DrawLine(83, 54, 89, 63);
  OLED_DrawLine(48, 59, 80, 59);

  /* 根据 eyes_closed 参数绘制睁眼或闭眼状态。 */
  OLED_DrawEllipse(57, 15, 8, 10);
  OLED_DrawEllipse(71, 15, 8, 10);
  if (eyes_closed != 0U)
  {
    OLED_DrawLine(53, 16, 61, 16);
    OLED_DrawLine(67, 16, 75, 16);
  }
  else
  {
    OLED_DrawEllipse(60, 16, 1, 3);
    OLED_DrawEllipse(68, 16, 1, 3);
  }

  /* 绘制鼻子、面部中线和左右胡须。 */
  OLED_DrawEllipse(64, 26, 4, 3);
  OLED_DrawLine(64, 29, 64, 35);
  OLED_DrawLine(42, 29, 57, 33);
  OLED_DrawLine(41, 35, 57, 36);
  OLED_DrawLine(42, 41, 56, 39);
  OLED_DrawLine(86, 29, 71, 33);
  OLED_DrawLine(87, 35, 71, 36);
  OLED_DrawLine(86, 41, 72, 39);

  /* 根据 mouth_open 参数切换张嘴和微笑两种嘴型。 */
  if (mouth_open != 0U)
  {
    OLED_DrawEllipse(64, 42, 13, 10);
    OLED_DrawEllipse(64, 46, 7, 4);
    for (y = 43; y <= 50; y++)
    {
      OLED_DrawLine(55, y, 73, y);
    }
    OLED_DrawEllipse(64, 47, 7, 3);
  }
  else
  {
    OLED_DrawLine(53, 40, 57, 44);
    OLED_DrawLine(57, 44, 64, 46);
    OLED_DrawLine(64, 46, 71, 44);
    OLED_DrawLine(71, 44, 75, 40);
  }

  /* 绘制项圈和铃铛。 */
  OLED_DrawLine(46, 55, 82, 55);
  OLED_DrawEllipse(64, 58, 4, 3);
  OLED_DrawLine(64, 61, 64, 63);
}

/**
 * @brief 将 1024 字节软件显存完整刷新到 OLED。
 * @note  屏幕分为 8 页，每页包含 128 列、每列包含 8 个垂直像素。
 */
static void OLED_UpdateScreen(void)
{
  uint8_t page;
  uint8_t packet[OLED_WIDTH + 1U];
  uint16_t column;

  /* 0x40 表示后续内容为连续显示数据。 */
  packet[0] = 0x40U;
  for (page = 0U; page < 8U; page++)
  {
    /* 选择目标页，并将列地址设置到该页的第 0 列。 */
    OLED_WriteCommand((uint8_t)(0xB0U + page));
    OLED_WriteCommand(0x00U);
    OLED_WriteCommand(0x10U);
    for (column = 0U; column < OLED_WIDTH; column++)
    {
      packet[column + 1U] = oled_buffer[column + page * OLED_WIDTH];
    }
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS, packet, sizeof(packet), 100U);
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
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  /* I2C1 初始化完成后再配置 OLED 控制器。 */
  OLED_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 第 1 帧：睁眼微笑，保持 350 ms。 */
    OLED_DrawDoraemon(0U, 0U);
    OLED_UpdateScreen();
    HAL_Delay(350U);

    /* 第 2 帧：睁眼张嘴，形成嘴巴开合效果。 */
    OLED_DrawDoraemon(1U, 0U);
    OLED_UpdateScreen();
    HAL_Delay(350U);

    /* 第 3 帧：短暂闭眼 120 ms，形成眨眼效果。 */
    OLED_DrawDoraemon(0U, 1U);
    OLED_UpdateScreen();
    HAL_Delay(120U);
  }
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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
  __disable_irq(); /* 关闭全局中断 */
  while (1)        /* 死循环：程序停在错误状态，方便调试时定位问题 */
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
