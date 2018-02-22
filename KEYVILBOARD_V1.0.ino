#define SMSSERIAL Serial2

char flag_esc=0;
char TextSms[139];
char Converter = 'A';
int Pressed = 0;
 

void setup() {
  Serial.begin(115200);
  Serial3.begin(9600);
  SMSSERIAL.begin(115200);
  SMSSERIAL.println("AT+CMGF=1");
  Serial.print("ready");
}

void loop() {
  if (Pressed == 140){
  Serial.println(TextSms);
  SMSSERIAL.write("AT+CMGF=1\r\n");
  delay(200);
  SMSSERIAL.write("AT+CMGS=\"+31XXXXXXXX\"\r\n"); //phonenumber + landcode
  delay(100);
  SMSSERIAL.write(TextSms);
  delay(100);
  SMSSERIAL.write((char)26);
  delay(100);
  Serial.println("SMS Sent!");
  Pressed = 0;
  }
  
  if (Serial3.available() > 0) {
    int inByte = Serial3.read();  
    if(inByte == 27)
    {
       flag_esc = 1;
    }   
    else
    {
      if(flag_esc==1)
      {
        // Previous char was ESC - Decode all the escaped keys
        switch(inByte)
        {
            case 0x49:
              Serial.print("[PgUp]");
              Keyboard.press(KEY_PAGE_UP);  
              Keyboard.releaseAll();
              break;
            case 0x51:
              Serial.print("[PgDn]");
              Keyboard.press(KEY_PAGE_DOWN);
              Keyboard.releaseAll();
              break;    
            case 0x47:
              Serial.print("[Home]");
              Keyboard.press(KEY_HOME);
              Keyboard.releaseAll();
              break;
            case 0x4F:
              Serial.print("[End]");
              Keyboard.press(KEY_END);
              Keyboard.releaseAll();
              break;     
            case 0x52:
              Serial.print("[Ins]");
              Keyboard.press(KEY_INSERT);
              Keyboard.releaseAll();
              break;
            case 0x53:
              Serial.print("[Del]");
              Keyboard.press(KEY_DELETE);
              Keyboard.releaseAll();
              break;               
            case 0x3B:
              Serial.print("[F1]");
              Keyboard.press(KEY_F1);
              Keyboard.releaseAll();
              break;
            case 0x3C:
              Serial.print("[F2]");
              Keyboard.press(KEY_F2);
              Keyboard.releaseAll();
              break;    
            case 0x3D:
              Serial.print("[F3]");
              Keyboard.press(KEY_F3);
              Keyboard.releaseAll();
              break;
            case 0x3E:
              Serial.print("[F4]");
              Keyboard.press(KEY_F4);
              Keyboard.releaseAll();
              break;     
            case 0x3F:
              Serial.print("[F5]");
              Keyboard.press(KEY_F5);
              Keyboard.releaseAll();
              break;
            case 0x40:
              Serial.print("[F6]");
              Keyboard.press(KEY_F6);
              Keyboard.releaseAll();
              break;          
            case 0x41:
              Serial.print("[F7]");
              Keyboard.press(KEY_F7);
              Keyboard.releaseAll();
              break; 
            case 0x42:
              Serial.print("[F8]");
              Keyboard.press(KEY_F8);
              Keyboard.releaseAll();
              break; 
            case 0x43:
              Serial.print("[F9]");
              Keyboard.press(KEY_F9);
              Keyboard.releaseAll();
              break; 
            case 0x44:
              Serial.print("[F10]");
              Keyboard.press(KEY_F10);
              Keyboard.releaseAll();
              break; 
            case 0x57:
              Serial.print("[F11]");
              Keyboard.press(KEY_F11);
              Keyboard.releaseAll();
              break; 
            case 0x58:
              Serial.print("[F12]");
              Keyboard.press(KEY_F12);
              Keyboard.releaseAll();
              break;     
            case 0x48:
              Serial.print("[Up]");
              Keyboard.press(KEY_HOME);
              Keyboard.releaseAll();
              break; 
            case 0x50:
              Serial.print("[Down]");
              Keyboard.press(KEY_DOWN_ARROW);
              Keyboard.releaseAll();
              break; 
            case 0x4B:
              Serial.print("[Left]");
              Keyboard.press(KEY_LEFT_ARROW);
              Keyboard.releaseAll();
              break; 
            case 0x4D:
              Serial.print("[Right]");
              Keyboard.press(KEY_RIGHT_ARROW);
              Keyboard.releaseAll();
              break;         
            case 0x54:
              Serial.print("[Print]");
              Keyboard.press(KEY_RIGHT_GUI);
              Keyboard.releaseAll();
              break; 
            case 0x5B:
              Serial.print("[Windows]");
              Keyboard.press(KEY_LEFT_GUI);
              Keyboard.releaseAll();
              break;   
            default:
              Serial.print("[?]");
              break;            
        }
        flag_esc=0;    
      }
      else
      {  

        if(inByte==1)
        {
           Serial.print("Control-A");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_A);
           Keyboard.releaseAll();
        }  
        else if(inByte==2)
        {
           Serial.print("Control-B");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_B);
           Keyboard.releaseAll();
        }
        else if(inByte==3)
        {
           Serial.print("Control-C");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_C);
           Keyboard.releaseAll();
        }
        else if(inByte==4)
        {
           Serial.print("Control-D");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_D);
           Keyboard.releaseAll();
        }
        else if(inByte==5)
        {
           Serial.print("Control-E");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_E);
           Keyboard.releaseAll();
        }
        else if(inByte==6)
        {
           Serial.print("Control-F");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_F);
           Keyboard.releaseAll();
        }
        else if(inByte==7)
        {
           Serial.print("Control-G");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_G);
           Keyboard.releaseAll();
        }
        else if(inByte==8)
        {
           Serial.print("Backspace");
           Pressed--; //This is for that backspaced stuff is non important in the array
           Keyboard.press(KEY_BACKSPACE);
           Keyboard.releaseAll();
           delay(50);
        }
        else if(inByte==9)
        {
           Serial.print("^T");
           Converter = '^';
           TextSms[Pressed] = Converter;
           Pressed++;
           Converter = 'T';
           TextSms[Pressed] = Converter; 
           Keyboard.press(KEY_TAB);
           Keyboard.releaseAll();
        }
        // Dont decode 10 - Line Feed
        else if(inByte==11)
        {
           Serial.print("Control-K");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_K);
           Keyboard.releaseAll();
        }
        else if(inByte==12)
        {
           Serial.print("Control-L");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_L);
           Keyboard.releaseAll();
        }
        // Dont decode 13 - Carriage Return
        else if(inByte==14)
        {
           Serial.print("Control-N");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_N);
           Keyboard.releaseAll();
        }
        else if(inByte==15)
        {
           Serial.print("Control-O");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_O);
           Keyboard.releaseAll();
        }
        else if(inByte==16)
        {
           Serial.print("Control-P");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_P);
           Keyboard.releaseAll();
        }
        else if(inByte==17)
        {
           Serial.print("Control-Q");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_Q);
           Keyboard.releaseAll();
        }
        else if(inByte==18)
        {
           Serial.print("Control-R");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_R);
           Keyboard.releaseAll();
        }
        else if(inByte==19)
        {
           Serial.print("Control-S");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_S);
           Keyboard.releaseAll();
        }
        else if(inByte==20)
        {
           Serial.print("Control-T");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_T);
           Keyboard.releaseAll();
        }
        else if(inByte==21)
        {
           Serial.print("Control-U");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_U);
           Keyboard.releaseAll();
        }
        else if(inByte==22)
        {
           Serial.print("Control-V");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_V);
           Keyboard.releaseAll();
        }
        else if(inByte==23)
        {
           Serial.print("Control-W");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_W);
           Keyboard.releaseAll();
        }
        else if(inByte==24)
        {
           Serial.print("Control-X");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_X);
           Keyboard.releaseAll();
        }
        else if(inByte==25)
        {
           Serial.print("Control-Y");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_Y);
           Keyboard.releaseAll();
        }        
        else if(inByte==26)
        {
           Serial.print("Control-Z");
           Keyboard.press(KEY_LEFT_CTRL);
           Keyboard.press(KEY_Z);
           Keyboard.releaseAll();
        }

        else   
        {
          Converter = inByte;
          TextSms[Pressed] = Converter;
          delay(50);
          Pressed++;
          //Serial.print(Pressed);
          //Serial.print(Converter);
          Keyboard.press(Converter);
          Keyboard.releaseAll();
          //Serial.write(inByte);
        }  
      }
    }    
  }     
 }




