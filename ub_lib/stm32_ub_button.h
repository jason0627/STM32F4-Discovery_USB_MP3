//--------------------------------------------------------------
// File     : stm32_ub_button.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __STM32F4_UB_BUTTON_H
#define __STM32F4_UB_BUTTON_H

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"



//--------------------------------------------------------------
// Liste aller Buttons
// (keine Nummer doppelt und von 0 beginnend)
//--------------------------------------------------------------
typedef enum 
{
  BTN_USER = 0    // BTN1 auf dem STM32F4-Discovery
}BUTTON_NAME_t;

#define  BUTTON_ANZ   1 // Anzahl von Button_NAME_t


//--------------------------------------------------------------
// Status eines Buttons
//--------------------------------------------------------------
typedef enum {
  BTN_RELEASED = 0,  // Button losgelassen
  BTN_PRESSED        // Button gedrueckt
}BUTTON_STATUS_t;



//--------------------------------------------------------------
// Struktur eines Buttons
//--------------------------------------------------------------
typedef struct {
  BUTTON_NAME_t BUTTON_NAME; // Name
  GPIO_TypeDef* BUTTON_PORT; // Port
  const uint16_t BUTTON_PIN; // Pin
  const uint32_t BUTTON_CLK; // Clock
  GPIOPuPd_TypeDef BUTTON_R; // Widerstand
}BUTTON_t;


//--------------------------------------------------------------
// Globale Funktionen
//--------------------------------------------------------------
void UB_Button_Init(void);
BUTTON_STATUS_t UB_Button_Read(BUTTON_NAME_t btn_name);

//--------------------------------------------------------------
#endif // __STM32F4_UB_BUTTON_H
