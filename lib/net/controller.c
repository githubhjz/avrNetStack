/*
 * controller.c
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
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#define DEBUG 2 // 0 to receive no debug serial output

#include <std.h>
#include <time.h>
#include <serial.h>
#include <tasks.h>
#include <net/mac.h>
#include <net/arp.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/dhcp.h>
#include <net/dns.h>
#include <net/ntp.h>
#include <net/controller.h>

char buff[BUFFSIZE];
uint16_t tl = 0;

char *timeToString(time_t s) {
	return ultoa(s, buff, 10);
}

char *hexToString(uint64_t s) {
	buff[0] = '0';
	buff[1] = 'x';
	ultoa(s, (buff + 2), 16);
	return buff;
}

char *hex2ToString(uint64_t s) {
	ultoa(s, buff, 16);
	return buff;
}

void networkInit(uint8_t *mac, uint8_t *ip, uint8_t *subnet, uint8_t *gateway) {
	initSystemTimer();
	macInitialize(mac);
	debugPrint("Hardware Driver initialized...\n");
	arpInit();
	ipv4Init(ip, subnet, gateway);
	debugPrint("IPv4 initialized...\n");
#ifndef DISABLE_ICMP
	icmpInit();
	debugPrint("ICMP initialized...\n");
#endif
#ifndef DISABLE_UDP
	udpInit();
	debugPrint("UDP initialized...\n");
  #ifndef DISABLE_DHCP
	// udpRegisterHandler(&dhcpHandler, 68);
  #endif
  #ifndef DISABLE_DNS
	// udpRegisterHandler(&dnsHandler, 53);
  #endif
  #ifndef DISABLE_NTP
	// udpRegisterHandler(&ntpHandler, 123);
  #endif
#endif

	// Uncomment to enable manual polling
	// addConditionalTask((TimedTask)networkHandler, macPacketsReceived);
}

void networkInterrupt(void) {
	// Interrupts are disabled on execution of an ISR
	// and enabled when leaving the ISR
	uint8_t i;
	macSetInterrupt(0); // Don't interrupt networking with more networking
	sei(); // Enable interrupts
	i = networkHandler();
#if DEBUG >= 1
	if (i != 0xFF) {
		debugPrint("Stack: ");
		debugPrint(timeToString(i));
		debugPrint("\n");
	} else {
		debugPrint(".");
	}
#endif
	cli(); // Disable interrupts
	macClearInterruptFlags();
	macSetInterrupt(1);
}

uint8_t networkHandler(void) {
	Packet *p;
	
	while (macLinkIsUp() && (macPacketsReceived() > 0)) {
		p = macGetPacket();

		if (p == NULL) {
			debugPrint("Not enough memory to allocate Packet struct!\n");
			return 1;
		}
		if ((p->dLength == 0) || (p->dLength > 1500)) {
			debugPrint("ENC gives invalid data. Resetting...\n");
			wdt_enable(WDTO_15MS);
			while(1);
		}
		if (p->d == NULL) {
			debugPrint("Not enough memory to receive packet with ");
			debugPrint(timeToString(p->dLength));
			debugPrint(" bytes!\n");
			mfree(p, sizeof(Packet));
			return 1;
		}

#if DEBUG >= 2
		debugPrint("Packet with ");
		debugPrint(timeToString(p->dLength));
		debugPrint(" bytes Received!\n");
#endif

		tl = get16Bit(p->d, 12);
		if (tl == IPV4) {
			// IPv4 Packet
			return ipv4ProcessPacket(p);
		} else if (tl == ARP) {
			// Address Resolution Protocol Packet
			return arpProcessPacket(p);
		} else if (tl == WOL) {
			// Wake on Lan Packet
		} else if (tl == RARP) {
			// Reverse Address Resolution Protocol Packet
		} else if (tl <= 0x0600) {
			// Ethernet type I packet. typeLength = Real data length
		} else if (tl == IPV6) {
			// IPv6 Packet
		}

		// Packet unhandled, free it
		mfree(p->d, p->dLength);
		mfree(p, sizeof(Packet));
		return 42;
	}
	return 0xFF;
}

uint16_t networkLastProtocol(void) {
	return tl;
}
