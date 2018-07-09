//--------------------------------------------------------------
// File     : cs43l22_lolevel_mp3.c
// V : 1.0 / 12.08.2013 / UB
//--------------------------------------------------------------



//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "cs43l22_lolevel_mp3.h"


//--------------------------------------------------------------
// Globale Variabeln
//--------------------------------------------------------------
DMA_InitTypeDef DMA_InitStructure;


//--------------------------------------------------------------
// Audio Codec functions
//--------------------------------------------------------------
static void Codec_GPIO_Init(void);
static void Codec_Reset(void);
static void Delay(volatile uint32_t nCount);
static void Codec_CtrlInterface_Init(void);
static uint32_t Codec_WriteRegister(uint8_t RegisterAddr, uint8_t RegisterValue);
static uint32_t Codec_ReadRegister(uint8_t RegisterAddr);
static uint32_t Codec_PauseResume(uint32_t Cmd);
static uint32_t Codec_Stop(uint32_t CodecPdwnMode);
static uint32_t Codec_Mute(uint32_t Cmd);
static uint32_t Codec_VolumeCtrl(uint8_t Volume);
static void Codec_AudioInterface_Init(uint32_t AudioFreq);
static void Audio_MAL_PauseResume(uint32_t Cmd, uint32_t Addr);


//--------------------------------------------------------------
// stellt I2S-Clock auf 135,5MHz ein
//--------------------------------------------------------------
void UB_AUDIO_SetI2SClock(void)
{
  RCC->CFGR &= ~RCC_CFGR_I2SSRC;

  /* Configure PLLI2S */
  RCC->PLLI2SCFGR = (CS43L22_PLLI2S_N << 6) | (CS43L22_PLLI2S_R << 28);

  /* Enable PLLI2S */
  RCC->CR |= ((uint32_t)RCC_CR_PLLI2SON);

  /* Wait till PLLI2S is ready */
  while((RCC->CR & RCC_CR_PLLI2SRDY) == 0)
  {
  }
}



//--------------------------------------------------------------
uint32_t EVAL_AUDIO_Init(uint8_t Volume, uint32_t AudioFreq)
{
  uint32_t counter = 0;


  /* Configure the Codec related IOs */
  Codec_GPIO_Init();

  /* Reset the Codec Registers */
  Codec_Reset();

  /* Initialize the Control interface of the Audio Codec */
  Codec_CtrlInterface_Init();

  /* Enable the DMA clock */
  RCC_AHB1PeriphClockCmd(AUDIO_I2S_DMA_CLOCK, ENABLE);
  
  /* Keep Codec powered OFF */
  counter += Codec_WriteRegister(0x02, 0x01);

  counter += Codec_WriteRegister(0x04, 0xAF); /* SPK always OFF & HP always ON */

  /* Clock configuration: Auto detection */
  counter += Codec_WriteRegister(0x05, 0x81);

  /* Set the Slave Mode and the audio Standard */
  counter += Codec_WriteRegister(0x06, CODEC_STANDARD);

  /* Set the Master volume */
  Codec_VolumeCtrl( VOLUME_CONVERT(Volume));

  /* Power on the Codec */
  counter += Codec_WriteRegister(0x02, 0x9E);

  /* Additional configuration for the CODEC. These configurations are done to reduce
     the time needed for the Codec to power off. If these configurations are removed,
     then a long delay should be added between powering off the Codec and switching
     off the I2S peripheral MCLK clock (which is the operating clock for Codec).
     If this delay is not inserted, then the codec will not shut down properly and
     it results in high noise after shut down. */

  /* Disable the analog soft ramp */
  counter += Codec_WriteRegister(0x0A, 0x00);

  /* Disable the digital soft ramp */
  counter += Codec_WriteRegister(0x0E, 0x04);

  /* Disable the limiter attack level */
  counter += Codec_WriteRegister(0x27, 0x00);
  /* Adjust Bass and Treble levels */
  counter += Codec_WriteRegister(0x1F, 0x0F);

  /* Adjust PCM volume level */
  counter += Codec_WriteRegister(0x1A, 0x0A);
  counter += Codec_WriteRegister(0x1B, 0x0A);

  /* Configure the I2S peripheral */
  Codec_AudioInterface_Init(AudioFreq);

  return(counter);
}


//--------------------------------------------------------------
void EVAL_AUDIO_Play(void)
{
  NVIC_EnableIRQ(AUDIO_I2S_DMA_IRQ);
  NVIC_SetPriority(AUDIO_I2S_DMA_IRQ, 4);

  /* Enable the I2S DMA request */
  SPI_I2S_DMACmd(CODEC_I2S, SPI_I2S_DMAReq_Tx, ENABLE);

  /* If the I2S peripheral is still not enabled, enable it */
  if ((CODEC_I2S->I2SCFGR & I2S_ENABLE_MASK) == 0)
  {
    I2S_Cmd(CODEC_I2S, ENABLE);
  }
}


//--------------------------------------------------------------
uint32_t EVAL_AUDIO_PauseResume(uint32_t Cmd)
{
  /* Call the Audio Codec Pause/Resume function */
  if (Codec_PauseResume(Cmd) != 0)
  {
    return 1;
  }
  else
  {
    /* Call the Media layer pause/resume function */
    Audio_MAL_PauseResume(Cmd, 0);

    /* Return 0 if all operations are OK */
    return 0;
  }
}


//--------------------------------------------------------------
void EVAL_AUDIO_Stop(uint32_t Option)
{
  EVAL_AUDIO_StopDMA();
  Codec_Stop(Option);
}


//--------------------------------------------------------------
void EVAL_AUDIO_StopDMA(void)
{
  /* Disable the I2S DMA Stream*/
  DMA_Cmd(AUDIO_I2S_DMA_STREAM, DISABLE);

  /* Wait the DMA Stream to be effectively disabled */
  while (DMA_GetCmdStatus(AUDIO_I2S_DMA_STREAM) != DISABLE)
  {}
}


//--------------------------------------------------------------
uint32_t EVAL_AUDIO_VolumeCtl(uint8_t Volume)
{
  /* Call the codec volume control function with converted volume value */
  return (Codec_VolumeCtrl(VOLUME_CONVERT(Volume)));
}


//--------------------------------------------------------------
void EVAL_IRQ_ENABLE(void)
{
  NVIC_EnableIRQ(AUDIO_I2S_DMA_IRQ);
}


//--------------------------------------------------------------
void EVAL_IRQ_DISABLE(void)
{
  NVIC_DisableIRQ(AUDIO_I2S_DMA_IRQ);
}


//--------------------------------------------------------------
void EVAL_SET_DMA(int next_puffer_len, int16_t *next_samples)
{
  /* Configure the DMA Stream */
  DMA_Cmd(AUDIO_I2S_DMA_STREAM, DISABLE);
  DMA_DeInit(AUDIO_I2S_DMA_STREAM);
  /* Set the parameters to be configured */
  DMA_InitStructure.DMA_Channel = AUDIO_I2S_DMA_CHANNEL;
  DMA_InitStructure.DMA_PeripheralBaseAddr = AUDIO_I2S_DMA_DREG;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)next_samples;
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_BufferSize = (uint32_t)next_puffer_len;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = AUDIO_MAL_DMA_PERIPH_DATA_SIZE;
  DMA_InitStructure.DMA_MemoryDataSize = AUDIO_MAL_DMA_MEM_DATA_SIZE;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(AUDIO_I2S_DMA_STREAM, &DMA_InitStructure);
  DMA_ITConfig(AUDIO_I2S_DMA_STREAM, DMA_IT_TC, ENABLE);

  /* Enable the I2S DMA Stream*/
  DMA_Cmd(AUDIO_I2S_DMA_STREAM, ENABLE);
}


//--------------------------------------------------------------
static void Codec_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable Reset GPIO Clock */
  RCC_AHB1PeriphClockCmd(AUDIO_RESET_GPIO_CLK,ENABLE);

  /* Audio reset pin configuration -------------------------------------------------*/
  GPIO_InitStructure.GPIO_Pin = AUDIO_RESET_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(AUDIO_RESET_GPIO, &GPIO_InitStructure);

  /* Enable I2S GPIO clocks */
  RCC_AHB1PeriphClockCmd(CODEC_I2S_GPIO_CLOCK, ENABLE);

  /* CODEC_I2S pins configuration: WS, SCK and SD pins -----------------------------*/
  GPIO_InitStructure.GPIO_Pin = CODEC_I2S_SCK_PIN | CODEC_I2S_SD_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(CODEC_I2S_GPIO, &GPIO_InitStructure);

  /* Connect pins to I2S peripheral  */
  GPIO_PinAFConfig(CODEC_I2S_WS_GPIO, CODEC_I2S_WS_PINSRC, CODEC_I2S_GPIO_AF);
  GPIO_PinAFConfig(CODEC_I2S_GPIO, CODEC_I2S_SCK_PINSRC, CODEC_I2S_GPIO_AF);

  GPIO_InitStructure.GPIO_Pin = CODEC_I2S_WS_PIN ;
  GPIO_Init(CODEC_I2S_WS_GPIO, &GPIO_InitStructure);
  GPIO_PinAFConfig(CODEC_I2S_GPIO, CODEC_I2S_SD_PINSRC, CODEC_I2S_GPIO_AF);


#ifdef CODEC_MCLK_ENABLED
  /* CODEC_I2S pins configuration: MCK pin */
  GPIO_InitStructure.GPIO_Pin = CODEC_I2S_MCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(CODEC_I2S_MCK_GPIO, &GPIO_InitStructure);
  /* Connect pins to I2S peripheral  */
  GPIO_PinAFConfig(CODEC_I2S_MCK_GPIO, CODEC_I2S_MCK_PINSRC, CODEC_I2S_GPIO_AF);
#endif /* CODEC_MCLK_ENABLED */
}



//--------------------------------------------------------------
static void Codec_Reset(void)
{
  /* Power Down the codec */
  GPIO_WriteBit(AUDIO_RESET_GPIO, AUDIO_RESET_PIN, Bit_RESET);

  /* wait for a delay to insure registers erasing */
  Delay(CODEC_RESET_DELAY);

  /* Power on the codec */
  GPIO_WriteBit(AUDIO_RESET_GPIO, AUDIO_RESET_PIN, Bit_SET);
}


//--------------------------------------------------------------
static void Delay(volatile uint32_t nCount)
{
  for (; nCount != 0; nCount--);
}



//--------------------------------------------------------------
static void Codec_CtrlInterface_Init(void)
{
  // init vom I2C
  UB_I2C1_Init();
}


//--------------------------------------------------------------
static uint32_t Codec_PauseResume(uint32_t Cmd)
{
  uint32_t counter = 0;

  /* Pause the audio file playing */
  if (Cmd == AUDIO_PAUSE)
  {
    /* Mute the output first */
    counter += Codec_Mute(AUDIO_MUTE_ON);

    /* Put the Codec in Power save mode */
    counter += Codec_WriteRegister(0x02, 0x01);
  }
  else /* AUDIO_RESUME */
  {
    /* Unmute the output first */
    counter += Codec_Mute(AUDIO_MUTE_OFF);

    counter += Codec_WriteRegister(0x04, 0xAF);

    /* Exit the Power save mode */
    counter += Codec_WriteRegister(0x02, 0x9E);
  }

  return counter;
}


//--------------------------------------------------------------
static uint32_t Codec_Stop(uint32_t CodecPdwnMode)
{
  uint32_t counter = 0;

  /* Mute the output first */
  Codec_Mute(AUDIO_MUTE_ON);

  if (CodecPdwnMode == CODEC_PDWN_SW)
  {
    /* Power down the DAC and the speaker (PMDAC and PMSPK bits)*/
    counter += Codec_WriteRegister(0x02, 0x9F);
  }
  else /* CODEC_PDWN_HW */
  {
    /* Power down the DAC components */
    counter += Codec_WriteRegister(0x02, 0x9F);

    /* Wait at least 100us */
    Delay(0xFFF);

    /* Reset The pin */
    GPIO_WriteBit(AUDIO_RESET_GPIO, AUDIO_RESET_PIN, Bit_RESET);
  }

  return counter;
}


//--------------------------------------------------------------
static uint32_t Codec_Mute(uint32_t Cmd)
{
  uint32_t counter = 0;

  /* Set the Mute mode */
  if (Cmd == AUDIO_MUTE_ON)
  {
    counter += Codec_WriteRegister(0x04, 0xFF);
  }
  else /* AUDIO_MUTE_OFF Disable the Mute */
  {
    counter += Codec_WriteRegister(0x04, 0xAF);
  }

  return counter;
}



//--------------------------------------------------------------
static uint32_t Codec_VolumeCtrl(uint8_t Volume)
{
  uint32_t counter = 0;

  if (Volume > 0xE6)
  {
    /* Set the Master volume */
    counter += Codec_WriteRegister(0x20, Volume - 0xE7);
    counter += Codec_WriteRegister(0x21, Volume - 0xE7);
  }
  else
  {
    /* Set the Master volume */
    counter += Codec_WriteRegister(0x20, Volume + 0x19);
    counter += Codec_WriteRegister(0x21, Volume + 0x19);
  }

  return counter;
}


//--------------------------------------------------------------
static uint32_t Codec_WriteRegister(uint8_t RegisterAddr, uint8_t RegisterValue)
{
  uint32_t result = 0;
  int16_t check=0;

  check=UB_I2C1_WriteByte(CODEC_ADDRESS, RegisterAddr, RegisterValue);
  if(check==0) {
    // alles ok
    result=0;
  }
  else {
    // fehler
    result=1;
  }


#ifdef VERIFY_WRITTENDATA
  /* Verify that the data has been correctly written */
  result = (Codec_ReadRegister(RegisterAddr) == RegisterValue)? 0:1;
#endif /* VERIFY_WRITTENDATA */

  /* Return the verifying value: 0 (Passed) or 1 (Failed) */
  return result;
}


//--------------------------------------------------------------
static uint32_t Codec_ReadRegister(uint8_t RegisterAddr)
{
  uint32_t result = 0;
  int16_t check=0;

  check=UB_I2C1_ReadByte(CODEC_ADDRESS, RegisterAddr);
  if(check>=0) {
    // alles ok
    result=check;
  }
  else {
    // fehler
    result=0;
  }

  return result;
}


//--------------------------------------------------------------
static void Codec_AudioInterface_Init(uint32_t AudioFreq)
{
  I2S_InitTypeDef I2S_InitStructure;

  /* Enable the CODEC_I2S peripheral clock */
  RCC_APB1PeriphClockCmd(CODEC_I2S_CLK, ENABLE);

  /* CODEC_I2S peripheral configuration */
  SPI_I2S_DeInit(CODEC_I2S);
  I2S_InitStructure.I2S_AudioFreq = AudioFreq;
  I2S_InitStructure.I2S_Standard = I2S_STANDARD;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;

#ifdef CODEC_MCLK_ENABLED
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
#elif defined(CODEC_MCLK_DISABLED)
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
#else
#error "No selection for the MCLK output has been defined !"
#endif /* CODEC_MCLK_ENABLED */

  /* Initialize the I2S peripheral with the structure above */
  I2S_Init(CODEC_I2S, &I2S_InitStructure);

  /* The I2S peripheral will be enabled only in the EVAL_AUDIO_Play() function
       or by user functions if DMA mode not enabled */
}


//--------------------------------------------------------------
static void Audio_MAL_PauseResume(uint32_t Cmd, uint32_t Addr)
{
  /* Pause the audio file playing */
  if (Cmd == AUDIO_PAUSE)
  {
    /* Disable the I2S DMA request */
    SPI_I2S_DMACmd(CODEC_I2S, SPI_I2S_DMAReq_Tx, DISABLE);

    /* Pause the I2S DMA Stream
        Note. For the STM32F40x devices, the DMA implements a pause feature,
              by disabling the stream, all configuration is preserved and data
              transfer is paused till the next enable of the stream.
              This feature is not available on STM32F40x devices. */
    DMA_Cmd(AUDIO_I2S_DMA_STREAM, DISABLE);
  }
  else /* AUDIO_RESUME */
  {
    /* Enable the I2S DMA request */
    SPI_I2S_DMACmd(CODEC_I2S, SPI_I2S_DMAReq_Tx, ENABLE);

    /* Resume the I2S DMA Stream
        Note. For the STM32F40x devices, the DMA implements a pause feature,
              by disabling the stream, all configuration is preserved and data
              transfer is paused till the next enable of the stream.
              This feature is not available on STM32F40x devices. */
    DMA_Cmd(AUDIO_I2S_DMA_STREAM, ENABLE);

    /* If the I2S peripheral is still not enabled, enable it */
    if ((CODEC_I2S->I2SCFGR & I2S_ENABLE_MASK) == 0)
    {
      I2S_Cmd(CODEC_I2S, ENABLE);
    }
  }
}


//--------------------------------------------------------------
void Audio_MAL_I2S_IRQHandler(void)
{
  EVAL_AUDIO_TransferComplete_CallBack();
}

