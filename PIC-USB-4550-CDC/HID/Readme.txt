USB Keyboard/Mouse Combination Device

Overview:

This example firmware is based on Microchip's USB Support Firmware.  
The purpose of this example is to provide a model for making a USB combination device.  
This mainly entails having the USB descriptors set up correctly.  
File usb_ch9.c contains the mouse and keyboard descriptors in the correct configuration.
The main routine of this example moves the cursor in a circle and sends keyboard 
keycodes for the message "Microchip USB" over and over again.

Some advice:

Before running this firmware open a blank text document.  Make sure that when you run 
the device/emulator that you are able to give focus to the document.  The cursor will 
be moving in a circle so make sure you have plenty of room to click on the editor window.  
This will allow you to see the message "Microchip USB" without inadvertently having 
the message type over an existing document.
  