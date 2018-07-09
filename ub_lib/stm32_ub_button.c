//--------------------------------------------------------------
// File     : stm32_ub_button.c
// Datum    : 09.02.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F4
// IDE      : CooCox CoIDE 1.7.0
// Module   : GPIO
// Funktion : Button Funktionen
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_button.h"


//--------------------------------------------------------------
// Definition aller Buttons
// Reihenfolge wie bei BUTTON_NAME_t
//
// Widerstand : [GPIO_PuPd_NOPULL,GPIO_PuPd_UP,GPIO_PuPd_DOWN]
//--------------------------------------------------------------
BUTTON_t BUTTON[] = {
  // Name    ,PORT , PIN       , CLOCK              ,Widerstand
  {BTN_USER  ,GPIOA,GPIO_Pin_0 ,RCC_AHB1Periph_GPIOA,GPIO_PuPd_NOPULL},  // PA0=User-Button auf dem Discovery-Board
};


//--------------------------------------------------------------
// Init aller Buttons
//--------------------------------------------------------------
void UB_Button_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;  
  BUTTON_NAME_t btn_name;
  
  for(btn_name=0;btn_name<BUTTON_ANZ;btn_name++) {
    // Clock Enable
    RCC_AHB1PeriphClockCmd(BUTTON[btn_name].BUTTON_CLK, ENABLE);
  
    // Config als Digital-Eingang
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = BUTTON[btn_name].BUTTON_R;
    GPIO_InitStructure.GPIO_Pin = BUTTON[btn_name].BUTTON_PIN;
    GPIO_Init(BUTTON[btn_name].BUTTON_PORT, &GPIO_InitStructure);
  }
}

//--------------------------------------------------------------
// Status von einem Button auslesen
// Return Wert :
//  -> wenn Button losgelassen = BTN_RELEASED
//  -> wenn Button gedrueckt   = BTN_PRESSED
//--------------------------------------------------------------
BUTTON_STATUS_t UB_Button_Read(BUTTON_NAME_t btn_name)
{
  uint32_t wert;

  wert=GPIO_ReadInputDataBit(BUTTON[btn_name].BUTTON_PORT, BUTTON[btn_name].BUTTON_PIN);
  if(wert==Bit_RESET) {
	  return(BTN_RELEASED);
  }
  else {
	  return(BTN_PRESSED);
  }
} 
