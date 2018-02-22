# KEYVILBOARD
Arduino based hardware keylogger trew SMS
Afther alot of trying it finaly works enough for me to release it as V1.0.


# CURRENT PROBLEMS
Last 2-3 Bytes are dead prob something todo with array amounts but don't want to bother with that atm.
Backspaces are single presses.
Very 0.5 second lag when it reaces 140 chars because of sms rather have 0 lagg.



# CREATING IT
HARDWARE:
Very little skill is required I'm new to this so you can do it no problem.
Lets start with the parts you will have to order. We have 3 main componets 
- SIM800L https://goo.gl/T1TNGr
- USB HOST Mini V2 FLASHED WITH USB KEYBOARD PRESET! https://goo.gl/bke8cG
- Teensy 3.2 goo.gl/C9XWSd
- Keyboard. Just any usb keyboard should work execept those with internal usb hubs.

Tools and other stuff you need 

- Some plane good silicon flexible wire. I used this goo.gl/74vvYM I recommend getting 5 diffrent colors.
- Good electric soldering iron that has small components as goal not some car soldering iron.
- SIMCARD with money on it so you can send SMS's
- USB to micro usb 2m or 3m(it's gonna act as the keyboard cable when you put the hardware inside so make sure it looks nice). For the teensy coding/keyboard reanactment.
Thats all you need now we need to start putting it all together for that I have this amazing high res paint picture
Picture: https://imgur.com/a/QWPqh Just download it and zoom in its the easiest.

Instructions:
USBHOST

Connect: USB Host GND to teensy GND 

         USB host 5V to teensy VIN(aka 5V)
         
         USB host RX to teensy TX3(PIN 8)
         
         USB host TX to teensy RX3(PIN 7)
         
SIM800L

Connect: USB Host GND to teensy GND 

         USB host 5V to teensy VIN(aka 5V)
         
         USB host RX to teensy TX2(PIN 10)
         
         USB host TX to teensy RX2(PIN 9)
         
Now connect the keyboard
Get you SIM card and make sure you turn off that you have to put in a PIN for the simcard just use a phone for this step.
Now put in the sim card in the SIM800L(the rightway!).
Put the 




# CODING IT

# THANKS TO
http://www.hobbytronics.co.uk/usb-host-mini for there hardware and part of there code.
