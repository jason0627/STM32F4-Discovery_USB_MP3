//--------------------------------------------------------------
// File     : stm32_ub_cs43l22_mp3_fatfs.c
// Datum    : 22.08.2013
// Version  : 1.1
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F4
// IDE      : CooCox CoIDE 1.7.0
// Module   : STM32_UB_I2C1, SPI, DMA, MISC, STM32_UB_FATFS
// Funktion : CS43L22 (2Kanal Audio-DAC)
//            zum abspielen von MP3-Files per FATFS
//            (SamplFrq vom MP3 muss 44,1 kHz sein !!)
//
// Hinweis  : benutzt I2C1 und I2S3
// 
//            I2C =>  PB9  = SDA   [SlaveAdr=0x94]
//                    PB6  = SCL
//            I2S =>  PA4  = WS
//                    PC7  = MCLK
//                    PC10 = SCK
//                    PC12 = SD
//            Reset = PD4
//            
// MP3-Decoder : Helix-MP3
//      Source : https://datatype.helixcommunity.org/Mp3dec
//      Autor  : Jon Recker (jrecker@real.com), Ken Cooke (kenc@real.com)
//
//--------------------------------------------------------------
//
// Wichtig : Compiler Optimization auf : -O3 (Most) !!
//
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_cs43l22_mp3_fatfs.h"



//--------------------------------------------------------------
// Globale Variabeln
//--------------------------------------------------------------
CS43L22_STATUS_t cs43l22_status=CS43L22_NO_INIT;
MP3_t my_mp3;
FIL myMP3File;
unsigned char mp3_buf[MP3_READ_BUF_SIZE];




//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
void P_CS43L22_InitMP3(void);
//--------------------------------------------------------------
typedef void MP3CallbackFunction(void *context,int buffer);
static MP3CallbackFunction *MP3CallbackFunctionPtr;
static void *CallbackContextPtr;
//--------------------------------------------------------------
void PlayMP3WithCallback(MP3CallbackFunction *callback_ptr,void *context);
static void MP3CallbackFkt(void *context,int buffer_nr);
void SetMP3Buffer(void *data_ptr, int sampl_cnt);
uint8_t TryMP3Buffer(void *data_ptr, int sampl_cnt);
void StartMP3Buffer(void);





//--------------------------------------------------------------
// Initialisierung vom CS43L22 (im MP3-Mode)
// Return_wert :
//  -> ERROR   , wenn Initialisierung fehlgeschlagen
//  -> SUCCESS , wenn Initialisierung ok war 
//--------------------------------------------------------------
ErrorStatus UB_CS43L22_InitMP3(void)
{
  ErrorStatus ret_wert=ERROR;
  uint32_t check;

  if(cs43l22_status==CS43L22_NO_INIT) {

    // Init vom MP3-Decoder
    my_mp3.mp3_data = MP3InitDecoder();
    if(my_mp3.mp3_data==0) {
      // nicht genügend ram frei
      my_mp3.last_err=MP3_MALLOC_ERR;
      return(ERROR);
    }

    // Clock der I2S einstellen
    UB_AUDIO_SetI2SClock();
    // Init und Check ob ok
    check=EVAL_AUDIO_Init(70, I2S_AudioFreq_44k);
    if(check==0) {
      ret_wert=SUCCESS;
      my_mp3.last_err=MP3_OK;
      cs43l22_status=CS43L22_INIT_OK;
    }
    else {
      my_mp3.last_err=MP3_INIT_ERR;
      ret_wert=ERROR;
    }
  }

  return(ret_wert);
}


//--------------------------------------------------------------
// spielt MP3-File einmal ab (per FATFS)
// Name : Filename
// Volume : [0...100]
// Return_wert :
//  ->   MP3_OK , MP3 wird abgespielt
//  -> !=MP3_OK , MP3 wird nicht abgespielt
//--------------------------------------------------------------
MP3_ERR_t UB_CS43L22_PlayMP3Single(const char* name, uint8_t Volume)
{
  MP3_ERR_t ret_wert=MP3_INIT_ERR;
  FATFS_t check_fat;
  FRESULT check_read;
  UINT bytes_read;

  if((cs43l22_status==CS43L22_INIT_OK) || (cs43l22_status==CS43L22_STOP)) {
    // File öffnen
    check_fat=UB_Fatfs_OpenFile(&myMP3File,name,F_RD);
    if(check_fat!=FATFS_OK) {
      return(MP3_FILE_ERR);
    }

    // kompletten Buffer mit Filedaten füllen
    check_read=f_read(&myMP3File, mp3_buf, MP3_READ_BUF_SIZE, &bytes_read);
    // check auf Fehler
    if((check_read!=FR_OK) || (bytes_read!=MP3_READ_BUF_SIZE)) {
      UB_Fatfs_CloseFile(&myMP3File);
      return(MP3_FILE_ERR);
    }

    cs43l22_status=CS43L22_PLAY;

    // Startwerte
    my_mp3.start_ptr=&mp3_buf[0];
    my_mp3.read_ptr=my_mp3.start_ptr;
    my_mp3.bytes_left_start=MP3_READ_BUF_SIZE;
    my_mp3.bytes_left=my_mp3.bytes_left_start;

    // nur einmal abspielen
    my_mp3.loop_flag=0;

    // Init der restlichen Variabeln
    P_CS43L22_InitMP3();

    // init vom CS43L22
    EVAL_AUDIO_Init(Volume, I2S_AudioFreq_44k);

    // MP3 starten
    PlayMP3WithCallback(MP3CallbackFkt, 0);

    ret_wert=MP3_OK;

  }

  return(ret_wert);
}


//--------------------------------------------------------------
// spielt MP3-File in einer Endloosloop ab (per FATFS)
// Name : Filename
// Volume : [0...100]
// Return_wert :
//  ->   MP3_OK , MP3 wird abgespielt
//  -> !=MP3_OK , MP3 wird nicht abgespielt
//--------------------------------------------------------------
MP3_ERR_t UB_CS43L22_PlayMP3Loop(const char* name, uint8_t Volume)
{
  MP3_ERR_t ret_wert=MP3_INIT_ERR;
  FATFS_t check_fat;
  FRESULT check_read;
  UINT bytes_read;

  if((cs43l22_status==CS43L22_INIT_OK) || (cs43l22_status==CS43L22_STOP)) {
    // File öffnen
    check_fat=UB_Fatfs_OpenFile(&myMP3File,name,F_RD);
    if(check_fat!=FATFS_OK) {
      return(MP3_FILE_ERR);
    }

    // kompletten Buffer mit Filedaten füllen
    check_read=f_read(&myMP3File, mp3_buf, MP3_READ_BUF_SIZE, &bytes_read);
    // check auf Fehler
    if((check_read!=FR_OK) || (bytes_read!=MP3_READ_BUF_SIZE)) {
      UB_Fatfs_CloseFile(&myMP3File);
      return(MP3_FILE_ERR);
    }

    cs43l22_status=CS43L22_PLAY;

    // Startwerte
    my_mp3.start_ptr=&mp3_buf[0];
    my_mp3.read_ptr=my_mp3.start_ptr;
    my_mp3.bytes_left_start=MP3_READ_BUF_SIZE;
    my_mp3.bytes_left=my_mp3.bytes_left_start;
    
    // File in einer Endlosloop abspielen
    my_mp3.loop_flag=1; // Loop

    // Init der restlichen Variabeln
    P_CS43L22_InitMP3();

    // init vom CS43L22
    EVAL_AUDIO_Init(Volume, I2S_AudioFreq_44k);

    // MP3 starten
    PlayMP3WithCallback(MP3CallbackFkt, 0);

    ret_wert=MP3_OK;

  }

  return(ret_wert);
}


//--------------------------------------------------------------
// MP3_Play-Funktion
// diese Funktion muss zyklisch aufgerufen werden
// solange das MP3 abgespielt wird
//
// return_wert :
//     CS43L22_PLAY = MP3 wird noch abgespielt
//  != CS43L22_PLAY = MP3 wird nicht mehr abgespielt
//--------------------------------------------------------------
CS43L22_STATUS_t UB_CS43L22_PlayMP3Do(void)
{
  uint32_t bytes_rest;
  FRESULT check_read;
  UINT bytes_read;

  if(cs43l22_status==CS43L22_PLAY) {
    // Test ob Puffer halb geleert
    if(my_mp3.bytes_left<MP3_READ_BUF_HALF) {      
      // Daten an Puffer_Anfang kopieren
      memcpy(mp3_buf, my_mp3.read_ptr, my_mp3.bytes_left);
      my_mp3.read_ptr = mp3_buf;
      bytes_rest = MP3_READ_BUF_SIZE - my_mp3.bytes_left;
      // rest vom Puffer mit neuen Filedaten füllen
      check_read=f_read(&myMP3File, mp3_buf+my_mp3.bytes_left, bytes_rest, &bytes_read);
      // check auf Fehler (oder Fileende)
      if((check_read!=FR_OK) || (bytes_read!=bytes_rest)) {
        // test auf Play-Loop
        if(my_mp3.loop_flag==1) {
          // skip zum fileanfang
          f_lseek(&myMP3File, 0);
          // kompletten Buffer mit Filedaten füllen
          f_read(&myMP3File, mp3_buf, MP3_READ_BUF_SIZE, &bytes_read);
          // skip zum anfang
          my_mp3.ende_flag=0;
          my_mp3.read_ptr=my_mp3.start_ptr;
          my_mp3.bytes_left=my_mp3.bytes_left_start;
          my_mp3.akt_buf_nr = 0;
          my_mp3.data_ptr = my_mp3.buffer0;
        }
        else {
          // mp3 anhalten
          UB_CS43L22_StopMP3();
        }
      }
      else {
        // kein Fehler und kein Fileende
        my_mp3.bytes_left=MP3_READ_BUF_SIZE;
      }
    }
  }

  return(cs43l22_status);
}


//--------------------------------------------------------------
// Pausen-Funktion
//--------------------------------------------------------------
void UB_CS43L22_PauseMP3(void)
{
  if(cs43l22_status==CS43L22_PLAY) {
    cs43l22_status=CS43L22_PAUSE;
    EVAL_AUDIO_PauseResume(AUDIO_PAUSE);
  }
}


//--------------------------------------------------------------
// Resume-Funktion
//--------------------------------------------------------------
void UB_CS43L22_ResumeMP3(void)
{
  if(cs43l22_status==CS43L22_PAUSE) {
    cs43l22_status=CS43L22_PLAY;
    EVAL_AUDIO_PauseResume(AUDIO_RESUME);
  }
}


//--------------------------------------------------------------
// Stop-Funktion
//--------------------------------------------------------------
void UB_CS43L22_StopMP3(void)
{
  if((cs43l22_status==CS43L22_PLAY) || (cs43l22_status==CS43L22_PAUSE)) {
    cs43l22_status=CS43L22_STOP;
    EVAL_AUDIO_Stop(CODEC_PDWN_HW);  // CODEC_PDWN_SW => rauschen

    my_mp3.dma_enable = 0;
    my_mp3.next_buffer_ptr = 0;

    // File schließen
    UB_Fatfs_CloseFile(&myMP3File);
  }
}


//--------------------------------------------------------------
// stellt Lautstärke ein
// Volume : [0...100]
//--------------------------------------------------------------
void UB_CS43L22_SetVolumeMP3(uint8_t Volume)
{
  EVAL_AUDIO_VolumeCtl(Volume);
}


//--------------------------------------------------------------
// letzten MP3 Fehler auslesen
// Return_wert :
//  ->   MP3_OK , kein Fehler
//  -> !=MP3_OK , Fehler
//--------------------------------------------------------------
MP3_ERR_t UB_CS43L22_GetMP3Err(void)
{
  MP3_ERR_t ret_wert;

  ret_wert=my_mp3.last_err;
  my_mp3.last_err=MP3_OK;

  return(ret_wert);
}


//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
void P_CS43L22_InitMP3(void)
{
  // Reset aller Variabeln
  my_mp3.akt_buf_nr = 0;
  my_mp3.data_ptr = my_mp3.buffer0;
  my_mp3.ende_flag=0;
  my_mp3.dma_enable=0;
  my_mp3.next_buffer_ptr = 0;
  my_mp3.next_buffer_len = 0;
  my_mp3.last_err=MP3_OK;

  MP3CallbackFunctionPtr = 0;
  CallbackContextPtr = 0;
}


//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
void PlayMP3WithCallback(MP3CallbackFunction *callback_ptr, void *context)
{
  
  EVAL_AUDIO_StopDMA();
  my_mp3.dma_enable = 0;

  EVAL_AUDIO_Play();

  MP3CallbackFunctionPtr = callback_ptr;
  CallbackContextPtr = context;
  my_mp3.akt_buf_nr = 0;

  if (MP3CallbackFunctionPtr) {
    MP3CallbackFunctionPtr(CallbackContextPtr, my_mp3.akt_buf_nr);
  }
}


//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
static void MP3CallbackFkt(void *context, int buffer_nr)
{
  int offset;
  int error;
  int ok;

  // nächste sync finden
  offset = MP3FindSyncWord((unsigned char*)my_mp3.read_ptr, my_mp3.bytes_left);
  if(offset<0) {
    // kein sync gefunden
    my_mp3.last_err=MP3_SYNC_ERR;
    my_mp3.ende_flag=1;
  }
  else if(offset>0){
    // sync ok aber keine Daten
    // frameinfo auslesen
    error=MP3GetNextFrameInfo(my_mp3.mp3_data, &my_mp3.mp3_info,(unsigned char*)my_mp3.read_ptr);
    if(error==ERR_MP3_NONE) {
      // kein Fehler
      if(my_mp3.mp3_info.samprate!=44100) {
        // falsche sampelrate
        my_mp3.last_err=MP3_FRQ_ERR;
        my_mp3.ende_flag=1;
      }
    }
    else if(error==ERR_MP3_INVALID_FRAMEHEADER) {
      // bei falschen frameheader
      // den frame überspringen
      // das hier ist KEIN Fehler !!
      my_mp3.bytes_left -= offset;
      my_mp3.read_ptr += offset;
      // check ob ende vom mp3 erreicht
      if(my_mp3.bytes_left<MP3_MIN_BYTES_REMAINING) {
        my_mp3.ende_flag=1;
      }
      MP3CallbackFunctionPtr(context, buffer_nr);
    }
    else {
      // anderer Fehler
      my_mp3.last_err=MP3_FRAME_ERR;
      my_mp3.ende_flag=1;
    }
  }
  else {
    // sync ok und Daten zum abspielen
    error = MP3Decode(my_mp3.mp3_data, (unsigned char**)&my_mp3.read_ptr, &my_mp3.bytes_left, my_mp3.data_ptr, 0);
    switch(error) {
      case ERR_MP3_NONE :
        // dekodieren ok (kein Fehler)
        ok=0;
      break;
      case ERR_MP3_INDATA_UNDERFLOW :
        // ende erreicht
    	my_mp3.last_err=MP3_IDATA_ERR;
        my_mp3.ende_flag=1;
        ok=-1;
      break;
      case ERR_MP3_MAINDATA_UNDERFLOW :
        // nicht genügend Daten
        // einfach nochmal lesen
        MP3CallbackFunctionPtr(context, buffer_nr);
        my_mp3.last_err=MP3_MDATA_ERR;
        ok=-1;
      break;
      default :
        // Fehler ist aufgetreten
        // einige Bytes überspringen und nochmal lesen
    	my_mp3.last_err=MP3_DECODE_ERR;
        if(my_mp3.bytes_left>(2*MP3_BYTES_SKIP)) {
          my_mp3.bytes_left -= MP3_BYTES_SKIP;
          my_mp3.read_ptr += MP3_BYTES_SKIP;
          MP3CallbackFunctionPtr(context, buffer_nr);
        }
        else {
          // Fehler am ende
          my_mp3.ende_flag=1;
        }
        ok=-1;
      break;
    }

    if(ok==0) {
      // frameinfo auslesen
      MP3GetLastFrameInfo(my_mp3.mp3_data, &my_mp3.mp3_info);
      if(my_mp3.mp3_info.outputSamps>0) {
        // Daten aus Puffer abspielen
        SetMP3Buffer(my_mp3.data_ptr, my_mp3.mp3_info.outputSamps);
        // Puffer wechseln
        if (buffer_nr==0) {
          my_mp3.data_ptr = my_mp3.buffer0;
        } else {
          my_mp3.data_ptr = my_mp3.buffer1;
        }
      }
    }
  }

  // test ob ende erreicht
  // (kommt bei der MP3 Version normalerweise nicht vor)
  if(my_mp3.ende_flag==1) {
    UB_CS43L22_StopMP3();
  }
}


//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
void SetMP3Buffer(void *data_ptr, int sampl_cnt)
{
  while (!TryMP3Buffer(data_ptr, sampl_cnt)) {
    __asm__ volatile ("wfi");
  }
}


//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
uint8_t TryMP3Buffer(void *data_ptr, int sampl_cnt)
{
  if (my_mp3.next_buffer_ptr) {
    return 0;
  }

  EVAL_IRQ_DISABLE();

  my_mp3.next_buffer_ptr = data_ptr;
  my_mp3.next_buffer_len = sampl_cnt;

  if (my_mp3.dma_enable==0) {
    StartMP3Buffer();
  }

  EVAL_IRQ_ENABLE();

  return 1;
}


//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
void StartMP3Buffer(void)
{
  EVAL_SET_DMA(my_mp3.next_buffer_len, my_mp3.next_buffer_ptr);

  my_mp3.next_buffer_ptr = 0;
  my_mp3.akt_buf_nr = 1 - my_mp3.akt_buf_nr;
  my_mp3.dma_enable = 1;

  if (MP3CallbackFunctionPtr) {
    MP3CallbackFunctionPtr(CallbackContextPtr, my_mp3.akt_buf_nr);
  }
}


//--------------------------------------------------------------
// wird aufgerufen, wenn MP3-Buffer komplett abgespielt wurde
//--------------------------------------------------------------
void EVAL_AUDIO_TransferComplete_CallBack(void)
{
  if (DMA_GetFlagStatus(AUDIO_I2S_DMA_STREAM, AUDIO_I2S_DMA_FLAG_TC) != RESET)
  {
    DMA_ClearFlag(AUDIO_I2S_DMA_STREAM, AUDIO_I2S_DMA_FLAG_TC);

    if (my_mp3.next_buffer_ptr) {
      StartMP3Buffer();
    } else {
      my_mp3.dma_enable = 0;
    }
  }
}











