//--------------------------------------------------------------
// File     : stm32_ub_cs43l22_mp3_fatfs.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __STM32F4_UB_CS43L22_MP3_FATFS_H
#define __STM32F4_UB_CS43L22_MP3_FATFS_H


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32f4xx.h"
#include "cs43l22_lolevel_mp3.h"
#include "mp3dec.h"
#include "stm32_ub_fatfs.h"
#include <string.h>  // wegen "memcpy"




//--------------------------------------------------------------
// interne Enum
//--------------------------------------------------------------
typedef enum {
  CS43L22_NO_INIT = 0,
  CS43L22_INIT_OK,
  CS43L22_PLAY,
  CS43L22_PAUSE,
  CS43L22_STOP
}CS43L22_STATUS_t;


//--------------------------------------------------------------
// größe vom Puffer (nicht ändern)
//--------------------------------------------------------------
#define   MP3_BUFFER_SIZE   4096 // 4kByte

//--------------------------------------------------------------
// Fehlerliste
//--------------------------------------------------------------
typedef enum {
  MP3_OK = 0,       // kein Fehler
  MP3_INIT_ERR,     // Fehler beim init
  MP3_MALLOC_ERR,   // nicht genug RAM frei
  MP3_FILE_ERR,     // File Fehler
  MP3_SYNC_ERR,     // kein Sync gefunden
  MP3_FRQ_ERR,      // Fehler bei Samplefrq
  MP3_FRAME_ERR,    // sonstiger Frame fehler
  MP3_IDATA_ERR,    // INDATA_UNDERFLOW
  MP3_MDATA_ERR,    // MAINDATA_UNDERFLOW
  MP3_DECODE_ERR    // sonstige Dekoder fehler
}MP3_ERR_t;


//--------------------------------------------------------------
// interne Struktur
//--------------------------------------------------------------
typedef struct {
  HMP3Decoder mp3_data;
  MP3FrameInfo mp3_info;
  const uint8_t *start_ptr;
  const uint8_t *read_ptr;
  int bytes_left_start;
  int bytes_left;
  int16_t *data_ptr;
  int16_t buffer0[MP3_BUFFER_SIZE];
  int16_t buffer1[MP3_BUFFER_SIZE];
  uint8_t akt_buf_nr;
  uint8_t ende_flag;
  uint8_t dma_enable;
  int16_t *next_buffer_ptr;
  int next_buffer_len;
  uint8_t loop_flag;
  MP3_ERR_t last_err;
}MP3_t;


//--------------------------------------------------------------
// HELIX MP3 Defines (nicht ändern !!)
//--------------------------------------------------------------
#define MP3_MIN_BYTES_REMAINING   100   // ende vom mp3 file
#define MP3_BYTES_SKIP            50    // bytes üeberspringen


//--------------------------------------------------------------
// Lese Puffer Größe (NICHT ÄNDERN !!)
// wenn Puffer zu klein -> Fehler bei einigen MP3's
//--------------------------------------------------------------
#define  MP3_READ_BUF_SIZE    MP3_BUFFER_SIZE * 4   // 4k*4 = 16 kByte
#define  MP3_READ_BUF_HALF    MP3_READ_BUF_SIZE / 2 // halber Puffer


//--------------------------------------------------------------
// Globale Funktionen
//--------------------------------------------------------------
ErrorStatus UB_CS43L22_InitMP3(void);
MP3_ERR_t UB_CS43L22_PlayMP3Single(const char* name, uint8_t Volume);
MP3_ERR_t UB_CS43L22_PlayMP3Loop(const char* name, uint8_t Volume);
CS43L22_STATUS_t UB_CS43L22_PlayMP3Do(void);
void UB_CS43L22_StopMP3(void);
void UB_CS43L22_PauseMP3(void);
void UB_CS43L22_ResumeMP3(void);
void UB_CS43L22_SetVolumeMP3(uint8_t Volume);
MP3_ERR_t UB_CS43L22_GetMP3Err(void);




//--------------------------------------------------------------
#endif // __STM32F4_UB_CS43L22_MP3_FATFS_H
