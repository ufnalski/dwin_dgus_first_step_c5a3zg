/**
 ******************************************************************************
 * file           : main.c
 * brief          : Main program body
 *                  Calls target system initialization then loop in main.
 ******************************************************************************
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32c5xx_hal_uart.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define DWIN_UART_HANDLE mx_usart1_uart_gethandle()
#define DWIN_UART_INSTANCE HAL_UART1
#define VCP_UART_HANDLE mx_usart2_uart_gethandle()
#define VCP_UART_INSTANCE HAL_UART2
#define UART_BUFFER_SIZE 16

#define DWIN_FRAME_START 0x5AA5
#define DWIN_WRITE_CMD 0x82
#define DWIN_READ_CMD 0x83
#define LOGO_TOUCH_VP 0x6000
#define COLOR_CHANGE_VP 0x6001

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

uint8_t rxDataDwin[UART_BUFFER_SIZE];
uint8_t txDataDwin[UART_BUFFER_SIZE];
// rxVCP --> rxDWIN and txDWIN --> txVCP for debugging purposes
uint8_t rxDataVcp[UART_BUFFER_SIZE];
uint32_t rxDataSize;
volatile uint8_t dwin_data_received_flag = 0;
volatile uint8_t toggle_variable = 0;
uint8_t some_cnt;         // for debugging purposes
hal_status_t uart_status; // for debugging purposes

/* Private functions prototype -----------------------------------------------*/
void TurnMyLogoRed(void);
void TurnMyLogoOriginal(void);

/**
 * brief:  The application entry point.
 * retval: none but we specify int to comply with C99 standard
 */
int main(void) {
  /** System Init: this code placed in targets folder initializes your system.
   * It calls the initialization (and sets the initial configuration) of the
   * peripherals. You can use STM32CubeMX to generate and call this code or not
   * in this project. It also contains the HAL initialization and the initial
   * clock configuration.
   */
  if (mx_system_init() != SYSTEM_OK) {
    return (-1);
  } else {
    /*
     * You can start your application code here
     */

    HAL_GPIO_WritePin(LD2_RED_PORT, LD2_RED_PIN, HAL_GPIO_PIN_SET); // turn off
    HAL_GPIO_WritePin(LD3_BLUE_PORT, LD3_BLUE_PIN,
                      HAL_GPIO_PIN_RESET); // turn on

    HAL_UART_ReceiveToIdle_DMA(DWIN_UART_HANDLE, rxDataDwin, UART_BUFFER_SIZE);
    HAL_UART_ReceiveToIdle_DMA(VCP_UART_HANDLE, rxDataVcp, UART_BUFFER_SIZE);

    while (1) {
      if (dwin_data_received_flag == 1) {
        dwin_data_received_flag = 0;
        if (rxDataDwin[8] == 1) {
          HAL_GPIO_WritePin(LD2_RED_PORT, LD2_RED_PIN,
                            HAL_GPIO_PIN_RESET); // turn on
          HAL_GPIO_WritePin(LD3_BLUE_PORT, LD3_BLUE_PIN,
                            HAL_GPIO_PIN_SET); // turn off
          TurnMyLogoRed();
        } else {
          HAL_GPIO_WritePin(LD2_RED_PORT, LD2_RED_PIN,
                            HAL_GPIO_PIN_SET); // turn off
          HAL_GPIO_WritePin(LD3_BLUE_PORT, LD3_BLUE_PIN,
                            HAL_GPIO_PIN_RESET); // turn on
          TurnMyLogoOriginal();
        }
      }
    }
  }
} /* end main */

void TurnMyLogoRed(void) {
  txDataDwin[0] = (uint8_t)((DWIN_FRAME_START >> 8) & 0xFF);
  txDataDwin[1] = (uint8_t)(DWIN_FRAME_START & 0xFF);
  txDataDwin[2] = 0x05; // payload length
  txDataDwin[3] = DWIN_WRITE_CMD;
  txDataDwin[4] = (uint8_t)((COLOR_CHANGE_VP >> 8) & 0xFF);
  txDataDwin[5] = (uint8_t)(COLOR_CHANGE_VP & 0xFF);
  txDataDwin[6] = 0x00;
  txDataDwin[7] = 0x01; // turn logo red
  HAL_UART_Transmit_DMA(DWIN_UART_HANDLE, txDataDwin, 8);
}

void TurnMyLogoOriginal(void) {
  txDataDwin[0] = (uint8_t)((DWIN_FRAME_START >> 8) & 0xFF);
  txDataDwin[1] = (uint8_t)(DWIN_FRAME_START & 0xFF);
  txDataDwin[2] = 0x05; // payload length
  txDataDwin[3] = DWIN_WRITE_CMD;
  txDataDwin[4] = (uint8_t)((COLOR_CHANGE_VP >> 8) & 0xFF);
  txDataDwin[5] = (uint8_t)(COLOR_CHANGE_VP & 0xFF);
  txDataDwin[6] = 0x00;
  txDataDwin[7] = 0x00; // turn logo original
  HAL_UART_Transmit_DMA(DWIN_UART_HANDLE, txDataDwin, 8);
}

void HAL_UART_RxCpltCallback(hal_uart_handle_t *huart, uint32_t size_byte,
                             hal_uart_rx_event_types_t rx_event) {

  if (huart->instance == DWIN_UART_INSTANCE) {
    if ((rxDataDwin[4] == (uint8_t)((LOGO_TOUCH_VP >> 8) & 0xFF)) &&
        (rxDataDwin[5] == (uint8_t)(LOGO_TOUCH_VP & 0xFF))) {
      dwin_data_received_flag = 1;
    }
    some_cnt++;                   // debug
    rxDataDwin[size_byte] = 0xBE; // for easy frame breaking in RealTerm
    rxDataDwin[size_byte + 1] = 0xEF;
    HAL_UART_Transmit_DMA(VCP_UART_HANDLE, rxDataDwin, size_byte + 2);
    uart_status = HAL_UART_ReceiveToIdle_DMA(DWIN_UART_HANDLE, rxDataDwin,
                                             UART_BUFFER_SIZE);
  }

  if (huart->instance == VCP_UART_INSTANCE) {
    HAL_UART_Transmit_DMA(DWIN_UART_HANDLE, rxDataVcp, size_byte);
    HAL_UART_ReceiveToIdle_DMA(VCP_UART_HANDLE, rxDataVcp, UART_BUFFER_SIZE);
  }
}

void HAL_EXTI_TriggerCallback(hal_exti_handle_t *hexti,
                              hal_exti_trigger_t trigger) {
  if ((hexti->line == HAL_EXTI_LINE_13) &&
      (trigger == HAL_EXTI_TRIGGER_RISING)) {
    HAL_GPIO_TogglePin(LD1_GREEN_PORT, LD1_GREEN_PIN);
    toggle_variable ^= 1;
    if (toggle_variable == 1) {
      TurnMyLogoRed();
    } else {
      TurnMyLogoOriginal();
    }
  }
}
