# avrNetStack 

This aims to be a very modular Networking Stack running on AVR Microcontrollers and supporting different Network Hardware (ENC28J60, MRF24WB).
Select your MCU and hardware driver in the makefile.
Compile with "make lib" to create a static library.
Compile with "make test" to create a test hex file to use with the hardware found in Hardware/avrNetStack.sch. You need Eagle 6, available for free from cadsoft.
In the future, a PCB will be designed that can act as WLAN / LAN Module for your AVR Project, in addition to this software.

## Software Overview

### Hardware Libraries

avrNetStack includes UART, SPI and Timer libs aswell as a basic task switcher and scheduler.
The UART lib uses FIFO Buffers for receiving and transmitting interrupt driven. Change the Buffer size in 'include/serial.h', if you want. For debugging, you can run the serial library in a blocking mode.
The Time lib supports 16MHz and 20MHz on a small selection of hardware devices. If you get compile errors after changing the target plattform in the makefile, you have to extend these libraries to support your target.
If you want to use this library with your own software, don't include another UART library. Use the functions from serial.h!
You need to call scheduler() and tasks() in you main-loop and also enable interrupts to use the Networking Stack. Both are completely dynamic, so you can use them for your application logic, too.
The std module is a wrapper for the libc memory allocation functions. It is used to keep track of memory allocations, for debugging purposes.

### Debug Output

Every software modules debug output can be individually turned off or on. Just set the "#define DEBUG" at the start of the line to '0' or '1'. To add debug output, use debugPrint() to print. If you need some more code to generate your output, put it in a "#if DEBUG == 1 ... #endif" block.
debug.h also includes a custom assert implementation.

### Controller Module

Controls the operation of the whole network stack. It contains only one function for the main program, networkInit. It is to be called once afer System Reset and performs initialization of all necessary hardware and buffers, etc. Also, some definitions can be uncommented in the controller.h file to deactivate parts of the stack. This could allow you to run a subset of the stack on a smaller AVR.

### MAC Module

These are the Network Hardware drivers. Different MAC implementations will exist in the future, right now only the ENC28J60 is supported. This allows sending Ethernet Packets, as well as receiving them. Received Packets are given to the appropriate next layer by the controller.

### ARP Module

Handles received ARP Packets, maintains an ARP Cache and gives functions of higher layers a method to obtain a MAC Address from an IP Address. If the Cache has no hit, a ARP Packet is issued, so that the higher layer can try again later.

### IPv4 Module

Handles received IPv4 Packets. Received valid Datagrams are given to the appropriate next stack layer. Also, IPv4 Packets can be transmitted with this module. It buffers outgoing IPv4 Packets to get the target MAC from the ARP Module.

### ICMP Module

Handles received ICMP Packets. Can answer and send out echo requests. A single handler for echo replies can be registered.

### UDP Module

Handles the really simple User Datagram Protocol. A handler for every port can be registered and UDP packets can be transmitted.

### NTP Module

Simple NTP (SNTP) Client. Will update systemTime automagically to1350074042 the current unix timestamp some time after networkInit.
