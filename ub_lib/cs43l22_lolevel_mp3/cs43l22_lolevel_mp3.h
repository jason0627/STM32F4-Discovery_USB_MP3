//--------------------------------------------------------------
// File     : stm32_ub_cs43l22_lolevel_mp3.h
//--------------------------------------------------------------


#ifndef __STM32F4_UB_CS43L22_LOLEVEL_MP3_H
#define __STM32F4_UB_CS43L22_LOLEVEL_MP3_H

//--------------------------------------------------------------
// includes
//--------------------------------------------------------------
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32_ub_i2c1.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_dma.h"
#include "misc.h"


//--------------------------------------------------------------
// OPTIONAL Configuration defines parameters
//--------------------------------------------------------------

#define I2S_ENABLE_MASK                 0x0400
#define CODEC_RESET_DELAY               0x4FFF


//--------------------------------------------------------------
// I2S-Clock Defines :
// HSE-Clock = 8 MHz, PLL_M = 8
// I2S_VCO = (HSE / PLL_M) * PLLI2S_N => 271 MHz
// I2S_CLK = I2S_VCO / PLLI2S_R => 135,5 MHz
//--------------------------------------------------------------
#define CS43L22_PLLI2S_N   271
#define CS43L22_PLLI2S_R   2

//--------------------------------------------------------------
#define  CODEC_STANDARD                0x04
#define  I2S_STANDARD                  I2S_Standard_Phillips





//--------------------------------------------------------------
#define CODEC_MCLK_ENABLED
// #define CODEC_MCLK_DISABLED


//--------------------------------------------------------------
#define VERIFY_WRITTENDATA



//--------------------------------------------------------------
// Hardware Configuration defines parameters
//--------------------------------------------------------------



//--------------------------------------------------------------
// Reset-Pin vom CS43L22
// [Reset = PD4]
//--------------------------------------------------------------
#define AUDIO_RESET_GPIO_CLK           RCC_AHB1Periph_GPIOD
#define AUDIO_RESET_PIN                GPIO_Pin_4
#define AUDIO_RESET_GPIO               GPIOD



//--------------------------------------------------------------
// I2S-Verbindung vom CS43L22
// I2S per SPI3
// [WS = PA4, SCK = PC10, SD = PC12, MCK = PC7]
//--------------------------------------------------------------
#define CODEC_I2S                      SPI3
#define CODEC_I2S_CLK                  RCC_APB1Periph_SPI3
#define CODEC_I2S_ADDRESS              0x40003C0C
#define CODEC_I2S_GPIO_AF              GPIO_AF_SPI3
#define CODEC_I2S_IRQ                  SPI3_IRQn
#define CODEC_I2S_GPIO_CLOCK           (RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA)
#define CODEC_I2S_WS_PIN               GPIO_Pin_4
#define CODEC_I2S_SCK_PIN              GPIO_Pin_10
#define CODEC_I2S_SD_PIN               GPIO_Pin_12
#define CODEC_I2S_MCK_PIN              GPIO_Pin_7
#define CODEC_I2S_WS_PINSRC            GPIO_PinSource4
#define CODEC_I2S_SCK_PINSRC           GPIO_PinSource10
#define CODEC_I2S_SD_PINSRC            GPIO_PinSource12
#define CODEC_I2S_MCK_PINSRC           GPIO_PinSource7
#define CODEC_I2S_GPIO                 GPIOC
#define CODEC_I2S_WS_GPIO              GPIOA
#define CODEC_I2S_MCK_GPIO             GPIOC
#define Audio_I2S_IRQHandler           SPI3_IRQHandler

#define AUDIO_MAL_DMA_PERIPH_DATA_SIZE DMA_PeripheralDataSize_HalfWord
#define AUDIO_MAL_DMA_MEM_DATA_SIZE    DMA_MemoryDataSize_HalfWord


//--------------------------------------------------------------
// DMA per SPI3_TX
// [DMA1, Stream7, Channel0]
//--------------------------------------------------------------
#define AUDIO_I2S_DMA_CLOCK            RCC_AHB1Periph_DMA1
#define AUDIO_I2S_DMA_STREAM           DMA1_Stream7
#define AUDIO_I2S_DMA_DREG             CODEC_I2S_ADDRESS
#define AUDIO_I2S_DMA_CHANNEL          DMA_Channel_0
#define AUDIO_I2S_DMA_IRQ              DMA1_Stream7_IRQn
#define AUDIO_I2S_DMA_FLAG_TC          DMA_FLAG_TCIF7
#define AUDIO_I2S_DMA_FLAG_HT          DMA_FLAG_HTIF7
#define AUDIO_I2S_DMA_FLAG_FE          DMA_FLAG_FEIF7
#define AUDIO_I2S_DMA_FLAG_TE          DMA_FLAG_TEIF7
#define AUDIO_I2S_DMA_FLAG_DME         DMA_FLAG_DMEIF7

#define Audio_MAL_I2S_IRQHandler       DMA1_Stream7_IRQHandler



//--------------------------------------------------------------
// I2C-Slave-Adresse vom CS43L22
//--------------------------------------------------------------
#define CODEC_ADDRESS                   0x94


//--------------------------------------------------------------
// Audio Codec User defines
//--------------------------------------------------------------

#define AUDIO_PAUSE                   0
#define AUDIO_RESUME                  1

#define CODEC_PDWN_HW                 1
#define CODEC_PDWN_SW                 2

#define AUDIO_MUTE_ON                 1
#define AUDIO_MUTE_OFF                0



//--------------------------------------------------------------
#define VOLUME_CONVERT(x)    ((Volume > 100)? 100:((uint8_t)((Volume * 255) / 100)))


//--------------------------------------------------------------
void UB_AUDIO_SetI2SClock(void);
uint32_t EVAL_AUDIO_Init(uint8_t Volume, uint32_t AudioFreq);
void EVAL_AUDIO_Play(void);
uint32_t EVAL_AUDIO_PauseResume(uint32_t Cmd);
void EVAL_AUDIO_Stop(uint32_t Option);
void EVAL_AUDIO_StopDMA(void);
uint32_t EVAL_AUDIO_VolumeCtl(uint8_t Volume);
void EVAL_IRQ_ENABLE(void);
void EVAL_IRQ_DISABLE(void);
void EVAL_SET_DMA(int next_puffer_len, int16_t *next_samples);


//--------------------------------------------------------------
void EVAL_AUDIO_TransferComplete_CallBack(void);


 
#endif // __STM32F4_UB_CS43L22_LOLEVEL_MP3_H


