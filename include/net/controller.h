/*
 * controller.h
 *
 * Copyright 2012 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of avrNetStack.
 *
 * avrNetStack is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * avrNetStack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with avrNetStack.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _controller_h
#define _controller_h

#include <time.h>
#include <serial.h>
#include <net/mac.h>

// -----------------------------------
// |        Feature Selection        |
// -----------------------------------

// #define DISABLE_IPV4_FRAGMENT         // Prevent IPv4 Fragmentation
// #define DISABLE_IPV4_CHECKSUM         // Prevent IPv4 Checksum calculation
// #define DISABLE_ICMP                  // Disable complete ICMP Protocol
// #define DISABLE_ICMP_STRINGS          // Don't store ICMP Names in Flash
// #define DISABLE_ICMP_CHECKSUM         // Prevent ICMP Checksum calculation
// #define DISABLE_ICMP_ECHO             // Prevent answering to Echo Requests (Ping)
// #define DISABLE_ICMP_UDP_MSG          // Don't send ICMP Error for unhandled port.
// #define DISABLE_UDP                   // Disable the complete UDP Protocol
// #define DISABLE_UDP_CHECKSUM          // Prevent UDP Checksum calculation
// #define DISABLE_DHCP                  // Disable DHCP. Enter valid IP etc. in controller.c
// #define DISABLE_DNS                   // Disable DNS. Uses UDP, not TCP!
// #define DISABLE_DNS_STRINGS           // Disable DNS Debug Output
// #define DISABLE_DNS_DOMAIN_VALIDATION // Don't check if domains are valid
// #define DISABLE_NTP                   // Disable NTP.

// -----------------------------------
// |            RAM Usage            |
// -----------------------------------

// This times 14bytes is max. allocated RAM for ARP Cache
#define ARPMaxTableSize 8 // Should be less than 127
#define BUFFSIZE 80

// -----------------------------------
// |          External API           |
// -----------------------------------

#define DEBUG 1 // 0 to receive no debug serial output

#if DEBUG == 1
#define debugPrint(x) serialWriteString(x)
#else
#define debugPrint(x)
#endif

extern char buff[BUFFSIZE];

char *timeToString(time_t s);
char *hexToString(uint64_t s);
void networkInit(MacAddress a);
uint8_t networkHandler(void); // 0xFF if nothing to do
// 42 if unhandled protocol --> networkLastProtocol()

uint16_t networkLastProtocol(void);

#define IPV4 0x0800
#define ARP 0x0806
#define WOL 0x0842
#define RARP 0x8035
#define IPV6 0x86DD

// -----------------------------------
// |       Schematic Overview        |
// -----------------------------------
/*
 *   ---------------------------------------------
 *   | OSI |               Modules               |
 *   ---------------------------------------------
 *   ---------------------------------------------
 *   | 5-7 |   dhcp.h   |   dns.h   |   ntp.h    |
 *   ---------------------------------------------
 *   |  4  |      udp.h       |       tcp.h      |
 *   ---------------------------------------------
 *   |  3  |      ipv4.h      |      icmp.h      |
 *   ---------------------------------------------
 *   | 1+2 |   mac.h   |   arp.h   |   driver    |
 *   ---------------------------------------------
 *
 * controller.c/h is controling the operation of the IP Stack.
 */
// -----------------------------------
// |          Dependencies           |
// -----------------------------------

#ifdef DISABLE_ICMP
#define DISABLE_ICMP_STRINGS
#define DISABLE_ICMP_CHECKSUM
#define DISABLE_ICMP_ECHO
#define DISABLE_ICMP_UDP_MSG
#endif

#ifdef DISABLE_UDP
#define DISABLE_UDP_CHECKSUM
#define DISABLE_DHCP
#define DISABLE_ICMP_UDP_MSG
#define DISABLE_DNS
#define DISABLE_NTP
#endif

#endif
