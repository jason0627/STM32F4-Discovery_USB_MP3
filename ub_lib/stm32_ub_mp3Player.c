//--------------------------------------------------------------
// File     : stm32_ub_mp3Player.c
// Datum    : 22.08.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F4
// IDE      : CooCox CoIDE 1.7.0
// Module   : STM32_UB_CS43L22_MP3_FATFS, Retarget_printf
//            STM32_UB_BUTTON
// Funktion : MP3-Player
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_mp3Player.h"



//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
uint32_t P_PlayMP3Nr(const char* akt_pfad, uint32_t nr);
uint32_t P_Check_File_Extension(const char *filename);


//--------------------------------------------------------------
// Globale Variabeln
//--------------------------------------------------------------
uint32_t akt_mp3_nr;


//--------------------------------------------------------------
// Init vom MP3-Player
//--------------------------------------------------------------
void UB_MP3Player_Init(void)
{
  // Init vom CS43L22
  UB_CS43L22_InitMP3();

  // Init vom Button
  UB_Button_Init();

  // aktuelle MP Nummer = 0
  akt_mp3_nr=0;
}


//--------------------------------------------------------------
// startet das abspielen des ersten MP3 File vom USB-Stick
//--------------------------------------------------------------
void UB_MP3Player_Start(void)
{
  if(UB_Fatfs_Mount(USB_0)==FATFS_OK) {
    // erstes MP3 von einem Verzeichnis abspielen
    P_PlayMP3Nr(MP3_DIR_NAME,0);
    akt_mp3_nr=0;
  }
}


//--------------------------------------------------------------
// MP3-Player
//--------------------------------------------------------------
void UB_MP3Player_Do(void)
{
  static uint32_t delay=0;
  uint32_t check;
  static uint32_t next_song=0;
  CS43L22_STATUS_t mp3_status;

  //--------------------------------
  // MP3 abspielen
  // und test ob Song noch l�uft
  //--------------------------------
  mp3_status=UB_CS43L22_PlayMP3Do();
  if(mp3_status==CS43L22_STOP) {
    // wenn Song fertig abgespielt
    // auf n�chstes Lied springen
    akt_mp3_nr++;
    check=P_PlayMP3Nr(MP3_DIR_NAME, akt_mp3_nr);
    if(check!=0) {
      // wenn nr nicht gefunden wurde
      // auf erstes Lied springen
      P_PlayMP3Nr(MP3_DIR_NAME, 0);
      akt_mp3_nr=0;
    }
  }


  //--------------------------------
  // Test auf User-Button
  //--------------------------------
  delay++;
  if(delay>10000) {
    delay=0;
    // zyklisch den Button auslesen
    if(UB_Button_Read(BTN_USER)==BTN_PRESSED) {
      if(next_song==0) {
        // auf n�chstes Lied springen
        next_song=1;
        // zuerst MP3 stoppen
        UB_CS43L22_StopMP3();
        // dann weiterschalten
        akt_mp3_nr++;
        check=P_PlayMP3Nr(MP3_DIR_NAME, akt_mp3_nr);
        if(check!=0) {
          // wenn nr nicht gefunden wurde
          // auf erstes Lied springen
          P_PlayMP3Nr(MP3_DIR_NAME, 0);
          akt_mp3_nr=0;
        }
      }
    }
    else {
      next_song=0;
    }
  }
}


//--------------------------------------------------------------
// stopt das abspielen vom MP3
//--------------------------------------------------------------
void UB_MP3Player_Stop(void)
{
  UB_Fatfs_UnMount(USB_0);
}


//--------------------------------------------------------------
// interne Funktion
// abspielen von MP3 mit der Nummer [nr] vom Pfad [akt_pfad]
// ret_wert :
//   0 = mp3 gefunden und Widergabe gestartet
//   1 = mp3 nicht gefunden bzw. Ende vom Verzeichniss erreicht
//--------------------------------------------------------------
uint32_t P_PlayMP3Nr(const char* akt_pfad, uint32_t nr)
{
  uint32_t ret_wert=0;
  FRESULT fatfs_error;
  DIR mydir;
  FILINFO myfileinfo;
  uint32_t mp3_ok=0;
  char buffer[32]; // Puffer f�r Pfad+Filename
  uint32_t akt_nr=0;

  // Verzeichniss �ffnen
  fatfs_error=f_opendir(&mydir, akt_pfad);
  if(fatfs_error==FR_OK) {
    // ein MP3 File im Verzeichniss suchen
    do {
      // ein Eintrag vom Verzeichnis lesen
      fatfs_error=f_readdir(&mydir, &myfileinfo);
      if(fatfs_error!=FR_OK) mp3_ok=1;
      if(myfileinfo.fname[0] == 0) mp3_ok=1;
      if((myfileinfo.fattrib & AM_DIR) == 0) {
        // wenn der Eintrag ein File ist und kein Verzeichnis
        sprintf(buffer, "%s/%s", akt_pfad, myfileinfo.fname);
  
        // Test ob File-Endung = .mp3
        if(P_Check_File_Extension(buffer)==1) {
          // MP3 Nummer "nr" suchen
          if(akt_nr==nr) {
            // MP3 einmal abspielen
            UB_CS43L22_PlayMP3Single(buffer,60);
            mp3_ok=2;
          }
          else {
            akt_nr++;
          }
        }
      }
    }
    while(mp3_ok==0);
  }

  if(mp3_ok==1) {
    // bei Fehler oder Ende vom Verzeichniss
    ret_wert=1;
  }

  return(ret_wert);
}


//--------------------------------------------------------------
// interne Funktion
// test ob File-Endung = ".mp3"
// return_wert : 0 = kein MP3,  1 = MP3
//--------------------------------------------------------------
uint32_t P_Check_File_Extension(const char *filename)
{
  uint32_t ret_wert=0;
  char * pch;

  pch=strrchr(filename,'.');
  if(pch==0) return(0);
  if(pch==filename) return(0);

  if (strcmp(".MP3", pch) == 0) {
    ret_wert=1;
  }
  if (strcmp(".mp3", pch) == 0) {
    ret_wert=1;
  }

  return(ret_wert);
}
