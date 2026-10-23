/**
  ******************************************************************************
  * @file           : app.c
  * @brief          : Application business logic (LED blink)
  ******************************************************************************
  */

#include "app.h"
#include "main.h"

#define BLINK_DELAY_MS 500

void setup(void)
{
  /* LED on PC13 is active-low: SET turns it off */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

void loop(void)
{
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  HAL_Delay(BLINK_DELAY_MS);
}
