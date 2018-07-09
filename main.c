//--------------------------------------------------------------
// File     : main.c
// Datum    : 22.08.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F4
// IDE      : CooCox CoIDE 1.7.0
// Module   : CMSIS_BOOT, M4_CMSIS_CORE
// Funktion : MP3 Player
//            >Ein USB-Stick kann per USB-OTG-Buchse
//               an die CPU angeschlossen werden
//               (USB-Host Memory-Controller)
//            >Im Root-Verzeichniss vom Stick können
//               normale MP3-Files liegen
//            >Die CPU spielt die MP3 per Audio-DAC
//               vom Discovery-Board (CS43L22)
//            >Mit dem User-Button kann zum nächsten MP3
//               gesprungen werden
//
// Hinweis  : Diese zwei Files muessen auf 8MHz stehen
//              "cmsis_boot/stm32f4xx.h"
//              "cmsis_boot/system_stm32f4xx.c"
//
// Portpins :
//              PA8   -> USB_OTG_SOF (wird nicht benutzt)
//              PA9   -> USB_OTG_VBUS
//              PA10  -> USB_OTG_ID
//              PA11  -> USB_OTG_DM
//              PA12  -> USB_OTG_DP
//
//              I2C =>  PB9  = SDA   [SlaveAdr=0x94]
//                      PB6  = SCL
//              I2S =>  PA4  = WS
//                      PC7  = MCLK
//                      PC10 = SCK
//                      PC12 = SD
//              Reset = PD4
//
//              PA0   -> User-Button
//
// Wichtig : Compiler Optimization auf : -O3 (Most) !!
//--------------------------------------------------------------

#include "main.h"
#include "stm32_ub_usb_msc_host.h"
#include "stm32_ub_mp3Player.h"


//--------------------------------------------------------------
int main(void)
{
  uint32_t akt_mode=0;

  SystemInit(); // Quarz Einstellungen aktivieren

  // Init vom USB-MSC-Host
  UB_USB_MSC_HOST_Init();

  // Init vom MP3-Player
  UB_MP3Player_Init();

  while(1)
  {
    // Test ob USB-Stick angeschlossen ist
    if(UB_USB_MSC_HOST_Do()==USB_MSC_DEV_CONNECTED) {
      if(akt_mode==0) {
        akt_mode=1;
        UB_MP3Player_Start(); // Start vom MP3-Player
      }
      else {
        UB_MP3Player_Do(); // MP3 abspielen
      }
    }
    else {
      if(akt_mode==1) {
        akt_mode=0;
        UB_MP3Player_Stop(); // Stop vom MP3-Player
      }
    }
  }
}




