//--------------------------------------------------------------
// File     : stm32_ub_mp3Player.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __STM32F4_UB_MP3PLAYER_H
#define __STM32F4_UB_MP3PLAYER_H


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32f4xx.h"
#include "stm32_ub_cs43l22_mp3_fatfs.h"
#include "stm32_ub_button.h"


//--------------------------------------------------------------
// Verzeichniss Name in dem die MP3 liegen
// und das abgespielt werden soll
// z.B. Root      = ""
// z.B. Root/Dir  = "Dir"
//--------------------------------------------------------------
#define  MP3_DIR_NAME    ""       // Root



//--------------------------------------------------------------
// Globale Funktionen
//--------------------------------------------------------------
void UB_MP3Player_Init(void);
void UB_MP3Player_Start(void);
void UB_MP3Player_Do(void);
void UB_MP3Player_Stop(void);


//--------------------------------------------------------------
#endif // __STM32F4_UB_MP3PLAYER_H
