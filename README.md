# avrNetStack 

This aims to be a very modular Networking Stack running on AVR Microcontrollers and supporting different Network Hardware (ENC28J60, MRF24WB).
Select your MCU and hardware driver in the makefile.
Compile with "make lib" to create a static library.
Compile with "make test" to create a test hex file to use with the hardware found in Hardware/avrNetStack.sch. You need Eagle 6, available for free from cadsoft.
In the future, a PCB will be designed that can act as WLAN / LAN Module for your AVR Project, in addition to this software.

## Modules

### Controller Module

Controls the operation of the whole network stack. It contains only two functions accessible by the main program, networkInit and networkHandler. The former is to be called once afer System Reset, and performs initialization of all necessary hardware and buffers, etc. The latter is to be called in the main infinite loop of the program. It performs packet receiving and handling. Also, some definitions can be uncommented in the controller.h file to deactivate parts of the stack. This could allow you to run a subset of the stack on a smaller AVR.

### MAC Module

These are the real Hardware drivers. Different MAC implementations will exist in the future, right now only the ENC28J60 is supported. This allows sending MacPackets, as well as receiving them. Received Packets are deconstructed and given to the appropriate next layer.

### ARP Module

Handles received ARP Packets, maintains an ARP Cache and gives functions of higher layers a method to obtain a MAC Address from an IP Address. If the Cache has no hit, a ARP Packet is issued, so that the higher layer can try again later.

### IPv4 Module

Handles received IPv4 Packets, supporting Packet Fragmentation as long as the MCU has enough RAM. Full received Datagrams are given to the appropriate next stack layer. Also, IPv4 Packets can be sent with this module.
