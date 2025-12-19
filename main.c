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
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
uint16_t Vamb = 0;             
uint32_t DC_Multiplier = 1;    

uint16_t PWM_final = 0;
uint16_t PWM_from_light = 0;
uint16_t PWM_from_pot = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN 0 */
// Hàm đọc trung bình nhiều mẫu từ 1 kênh ADC
uint16_t Read_ADC_Channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) // Lấy trung bình 16 mẫu
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
        sum += HAL_ADC_GetValue(&hadc1);
    }
    return (uint16_t)(sum / 16);
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();

  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

  // Hiệu chỉnh ADC
  HAL_ADCEx_Calibration_Start(&hadc1);

  // Đọc giá trị ánh sáng môi trường ban đầu
  Vamb = Read_ADC_Channel(ADC_CHANNEL_5);
  if (Vamb >= 4095) Vamb = 4094;  // tránh chia 0
  DC_Multiplier = htim2.Init.Period / (4096 - Vamb); 
  /* USER CODE END 2 */

  /* Infinite loop */
  while (1)
  {
    // Đọc cảm biến ánh sáng (kênh 5)
    uint16_t lightValue = Read_ADC_Channel(ADC_CHANNEL_5);

    // Đọc biến trở (kênh 1)
    uint16_t potValue = Read_ADC_Channel(ADC_CHANNEL_1);

    // PWM từ ánh sáng
    if (lightValue > Vamb)
        PWM_from_light = (uint16_t)((lightValue - Vamb) * DC_Multiplier);
    else
        PWM_from_light = 0;

    if (PWM_from_light > htim2.Init.Period)
        PWM_from_light = htim2.Init.Period;

    // PWM từ biến trở (scale 0-4095 -> 0-ARR)
    PWM_from_pot = (uint16_t)((potValue * htim2.Init.Period) / 4095);

    // PWM chung = kết hợp (tỷ lệ với biến trở)
    PWM_final = (uint16_t)(((uint32_t)PWM_from_light * PWM_from_pot) / htim2.Init.Period);

    // Xuất PWM chung ra TIM2_CH1
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, PWM_final);

    HAL_Delay(10);
  }
}
