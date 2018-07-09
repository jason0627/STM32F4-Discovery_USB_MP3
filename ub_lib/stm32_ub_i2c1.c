//--------------------------------------------------------------
// File     : stm32_ub_i2c1.c
// Datum    : 07.03.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F4
// IDE      : CooCox CoIDE 1.7.0
// Module   : GPIO, I2C 
// Funktion : I2C-LoLevel-Funktionen (I2C-1)
//
// Hinweis  : mögliche Pinbelegungen
//            I2C1 : SCL :[PB6, PB8] 
//                   SDA :[PB7, PB9]
//            externe PullUp-Widerstände an SCL+SDA notwendig
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_i2c1.h"


//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
void P_I2C1_InitI2C(void);
int16_t P_I2C1_timeout(int16_t wert);



//--------------------------------------------------------------
// Definition von I2C1
//--------------------------------------------------------------
I2C1_DEV_t I2C1DEV = {
// PORT , PIN      , Clock              , Source 
  {GPIOB,GPIO_Pin_6,RCC_AHB1Periph_GPIOB,GPIO_PinSource6}, // SCL an PB6
  {GPIOB,GPIO_Pin_9,RCC_AHB1Periph_GPIOB,GPIO_PinSource9}, // SDA an PB9
};



//--------------------------------------------------------------
// Init von I2C1
//-------------------------------------------------------------- 
void UB_I2C1_Init(void)
{
  static uint8_t init_ok=0;
  GPIO_InitTypeDef  GPIO_InitStructure;

  // initialisierung darf nur einmal gemacht werden
  if(init_ok!=0) {
    return;
  } 

  // I2C-Clock enable
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

  // Clock Enable der Pins
  RCC_AHB1PeriphClockCmd(I2C1DEV.SCL.CLK, ENABLE); 
  RCC_AHB1PeriphClockCmd(I2C1DEV.SDA.CLK, ENABLE);

  // I2C reset
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);

  // I2C Alternative-Funktions mit den IO-Pins verbinden  
  GPIO_PinAFConfig(I2C1DEV.SCL.PORT, I2C1DEV.SCL.SOURCE, GPIO_AF_I2C1); 
  GPIO_PinAFConfig(I2C1DEV.SDA.PORT, I2C1DEV.SDA.SOURCE, GPIO_AF_I2C1);

  // I2C als Alternative-Funktion als OpenDrain  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;

  // SCL-Pin
  GPIO_InitStructure.GPIO_Pin = I2C1DEV.SCL.PIN;
  GPIO_Init(I2C1DEV.SCL.PORT, &GPIO_InitStructure);
  // SDA-Pin
  GPIO_InitStructure.GPIO_Pin = I2C1DEV.SDA.PIN;
  GPIO_Init(I2C1DEV.SDA.PORT, &GPIO_InitStructure);

  // I2C init
  P_I2C1_InitI2C();

  // init Mode speichern
  init_ok=1;
}



//--------------------------------------------------------------
// Auslesen einer Adresse per I2C von einem Slave
// slave_adr => I2C-Basis-Adresse vom Slave
// adr       => Register Adresse die gelesen wird
//
// Return_wert :
//  0...255 , Bytewert der gelesen wurde
//  < 0     , Error
//--------------------------------------------------------------
int16_t UB_I2C1_ReadByte(uint8_t slave_adr, uint8_t adr)
{
  int16_t ret_wert=0;
  uint32_t timeout=I2C1_TIMEOUT;

  // Start-Sequenz
  I2C_GenerateSTART(I2C1, ENABLE);
  
  timeout=I2C1_TIMEOUT;
  while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_SB)) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-1));
  }

  // ACK disable
  I2C_AcknowledgeConfig(I2C1, DISABLE);

  // Slave-Adresse senden (write)
  I2C_Send7bitAddress(I2C1, slave_adr, I2C_Direction_Transmitter); 

  timeout=I2C1_TIMEOUT;
  while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR)) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-2));
  }  

  // ADDR-Flag löschen
  I2C1->SR2;

  timeout=I2C1_TIMEOUT;
  while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE)) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-3));
  }  

  // Adresse senden
  I2C_SendData(I2C1, adr);

  timeout=I2C1_TIMEOUT;
  while ((!I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF))) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-4));
  }

  // Start-Sequenz
  I2C_GenerateSTART(I2C1, ENABLE);

  timeout=I2C1_TIMEOUT;
  while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_SB)) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-5));
  }

  // Slave-Adresse senden (read)
  I2C_Send7bitAddress(I2C1, slave_adr, I2C_Direction_Receiver);

  timeout=I2C1_TIMEOUT;
  while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR)) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-6));
  }

  // ADDR-Flag löschen
  I2C1->SR2;

  timeout=I2C1_TIMEOUT;
  while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE)) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-7));
  } 

  // Stop-Sequenz
  I2C_GenerateSTOP(I2C1, ENABLE);

  // Daten auslesen
  ret_wert=(int16_t)(I2C_ReceiveData(I2C1));

  // ACK enable
  I2C_AcknowledgeConfig(I2C1, ENABLE);

  return(ret_wert);
}



//--------------------------------------------------------------
// Beschreiben einer Adresse per I2C von einem Slave
// slave_adr => I2C-Basis-Adresse vom Slave
// adr       => Register Adresse die beschrieben wird
// wert      => Bytewert der geschrieben wird
//
// Return_wert :
//    0   , Ok
//  < 0   , Error
//--------------------------------------------------------------
int16_t UB_I2C1_WriteByte(uint8_t slave_adr, uint8_t adr, uint8_t wert)
{
  int16_t ret_wert=0;
  uint32_t timeout=I2C1_TIMEOUT;

  // Start-Sequenz
  I2C_GenerateSTART(I2C1, ENABLE); 

  timeout=I2C1_TIMEOUT;
  while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_SB)) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-1));
  } 

  // Slave-Adresse senden (write)
  I2C_Send7bitAddress(I2C1, slave_adr, I2C_Direction_Transmitter);

  timeout=I2C1_TIMEOUT;
  while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR)) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-2));
  }  

  // ADDR-Flag löschen
  I2C1->SR2;

  timeout=I2C1_TIMEOUT;
  while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE)) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-3));
  }

  // Adresse senden
  I2C_SendData(I2C1, adr);

  timeout=I2C1_TIMEOUT;
  while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE)) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-4));
  }

  // Daten senden
  I2C_SendData(I2C1, wert);

  timeout=I2C1_TIMEOUT;
  while ((!I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF))) {
    if(timeout!=0) timeout--; else return(P_I2C1_timeout(-5));
  }

  // Stop-Sequenz
  I2C_GenerateSTOP(I2C1, ENABLE);

  ret_wert=0; // alles ok

  return(ret_wert);
}


//--------------------------------------------------------------
// kleine Pause (ohne Timer)
//--------------------------------------------------------------
void UB_I2C1_Delay(volatile uint32_t nCount)
{
  while(nCount--)
  {
  }
}


//--------------------------------------------------------------
// interne Funktion
// Init der I2C-Schnittstelle
//--------------------------------------------------------------
void P_I2C1_InitI2C(void)
{
  I2C_InitTypeDef  I2C_InitStructure;

  // I2C-Konfiguration
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0x00;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C1_CLOCK_FRQ;

  // I2C enable
  I2C_Cmd(I2C1, ENABLE);

  // Init Struktur
  I2C_Init(I2C1, &I2C_InitStructure);
}

//--------------------------------------------------------------
// interne Funktion
// wird bei einem Timeout aufgerufen
// Stop, Reset und reinit der I2C-Schnittstelle
//--------------------------------------------------------------
int16_t P_I2C1_timeout(int16_t wert)
{
  int16_t ret_wert=wert;

  // Stop und Reset
  I2C_GenerateSTOP(I2C1, ENABLE);
  I2C_SoftwareResetCmd(I2C1, ENABLE);
  I2C_SoftwareResetCmd(I2C1, DISABLE);

  // I2C deinit
  I2C_DeInit(I2C1);
  // I2C init
  P_I2C1_InitI2C();
    
  return(ret_wert);
}
