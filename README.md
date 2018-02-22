# KEYVILBOARD
Arduino based hardware keylogger trew SMS
Afther alot of trying it finaly works enough for me to release it as V1.0.

Samy Kamkar got me obsessed with hardware hacking so I decided to make this a so called hardware keylogger wich is pretty much untraceable aswell as unlimted in range as long as if you have celluar connection.


# CURRENT PROBLEMS
Last 2-3 Bytes are dead prob something todo with array amounts but don't want to bother with that atm.
Backspaces are single presses.
Very 0.5 second lag when it reaches 140 chars because of SMS rather have 0 lagg.



# CREATING IT
HARDWARE:
Very little skill is required I'm new to this so you can do it no problem.
Lets start with the parts you will have to order. We have 4 main componets 
- SIM800L https://goo.gl/T1TNGr
- USB HOST Mini V2 FLASHED WITH USB KEYBOARD PRESET! https://goo.gl/bke8cG
- Teensy 3.2 https://goo.gl/C9XWSd
- Keyboard. Just any usb keyboard should work execept those with internal usb hubs.

Tools and other stuff you need 

- Some plane good silicon flexible wire. I used this https://goo.gl/74vvYM I recommend getting 5 diffrent colors.
- Good electric soldering iron that has small components as goal not some car soldering iron.
- SIMCARD with money on it so you can send SMS's
- USB to micro usb 2m or 3m(it's gonna act as the keyboard cable when you put the hardware inside so make sure it looks nice). For the teensy coding/keyboard reanactment.

Thats all you need now we need to start putting it all together for that I have this amazing high res paint picture
Picture: https://imgur.com/a/QWPqh Just download it and zoom in its the easiest.

Instructions:
USBHOST

Connect: 
- USB Host GND to teensy GND 
- USB host 5V to teensy VIN(aka 5V)
- USB host RX to teensy TX3(PIN 8)
- USB host TX to teensy RX3(PIN 7)
         
SIM800L

Connect: 
- Connect the antenna like this http://www.circuitblocksph.com/website/image/product.template/132_c5bb48c/image
- USB Host GND to teensy GND 
- USB host 5V to teensy VIN(aka 5V)
- USB host RX to teensy TX2(PIN 10)
- USB host TX to teensy RX2(PIN 9)
         
Now connect the keyboard to the USBHOST
Get you SIM card and make sure you turn off that you have to put in a PIN for the simcard just use a phone for this step.
Now put in the sim card in the SIM800L(the rightway!).
Put the usb mirco cable in to the Teensy and connect it to your computer.

Now check for the following

- USBHOST blue led turns on static and if you press caps the led on the keyboard turns on. if not check connections. and double check if you got the preflashed USB KEYBOARD MODE.
- Wait some time around 20 seconds max and check if the sim800L has 1 static led and 1 blinking every 3 seconds your good. If it blinks every second check your sim if it works and that you have PIN disabeld (Also make sure you have the antenna connected).
- Your teensy should do nothing visble no smoke and stuff.

SOFTWARE:

Download & Install:
Arduino IDE: https://www.arduino.cc/en/Main/Software
Teensydruino: https://www.pjrc.com/teensy/teensyduino.html

Open the code and make sure your teensy shows up and if you hover above tools your preset looks like this
https://gyazo.com/925060150088bef39eb2882f32c905dc where it says port is should say Teensy or Emulated like this if you have flashed it
before as a keyboard. Make sure you change the recevieving phonenumber and the keyboard layauto matches with the keyboard it is installed on, then press upload and enjoy.

Its up to you what you gonna do with it but I DO NOT CONDONE USING THIS IN ANY ILLIGAL WAY. Make sure the other party knows of this when using it!

# QUESTIONS
EMAIL: luc@pluimakers.com ("Same as my paypal if your feeling nice ;)")
Linkedin: https://www.linkedin.com/in/luc-pluimakers-b9823b107/

# THANKS TO
hobbytronics.co.uk For there hardware and part of there code.
PJRC For creating these awesome Teensy's

# UPDATES

V1.1: Removed some garbage code and fixed some things here and there.

