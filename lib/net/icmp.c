/*
 * icmp.c
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

#include <net/icmp.h>

#ifndef DISABLE_ICMP

#ifndef DISABLE_ICMP_STRINGS
void (*debugOutputHandler)(char *) = NULL;
#endif

// ----------------------
// |    Internal API    |
// ----------------------

#define print(x, y) debugOutputHandler(icmpMessage(x, y))
void freeIPv4Packet(IPv4Packet *ip); // Defined in ipv4.c

#ifndef DISABLE_ICMP_CHECKSUM
uint16_t checksumIcmp(IcmpPacket *ic) {
	uint32_t a = 0;
	uint16_t i;
	a += ((uint16_t)ic->type << 8) | ic->code;
	// a += ic->checksum;
	a += (ic->restOfHeader & 0xFF000000) >> 16;
	a += (ic->restOfHeader & 0x00FF0000) >> 16;
	a += (ic->restOfHeader & 0x0000FFFF);
	for (i = 0; i < ic->dLength; i += 2) {
		a += ((ic->data[i] << 8) | ic->data[i + 1]);
	}
	// a now 16bit sum
	a = (a & 0x0000FFFF) + ((a & 0xFFFF0000) > 16); // 1's complement 16bit sum
	return (uint16_t)~a; // 1's complement of 1's complement 16bit sum
}
#endif

IcmpPacket *ipv4PacketToIcmpPacket(IPv4Packet *ip) {
	uint16_t i;
	IcmpPacket *ic = (IcmpPacket *)malloc(sizeof(IcmpPacket));
	if (ic == NULL) {
		return NULL;
	}
	ic->type = ip->data[0];
	ic->code = ip->data[1];
	ic->checksum = ip->data[3];
	ic->checksum |= (ip->data[2] << 8);
	ic->restOfHeader = ip->data[7];
	ic->restOfHeader |= ((uint32_t)ip->data[6] << 8);
	ic->restOfHeader |= ((uint32_t)ip->data[5] << 16);
	ic->restOfHeader |= ((uint32_t)ip->data[4] << 24);
	if (ip->dLength > 8) {
		// There is more data following...
		ic->data = (uint8_t *)malloc(ip->dLength - 8);
		if (ic->data == NULL) {
			free(ic);
			return NULL;
		}
		for (i = 0; i < (ip->dLength - 8); i++) {
			ic->data[i] = ip->data[8 + i];
		}
		ic->dLength = ip->dLength - 8;
	} else {
		ic->dLength = 0;
	}
	return ic;
}

void freeIcmpPacket(IcmpPacket *ic) {
	if (ic != NULL) {
		if (ic->data != NULL) {
			free(ic->data);
		}
		free(ic);
	}
}

// ----------------------
// |    External API    |
// ----------------------

void icmpInit(void) {}

// 0 success, 1 not enough mem, 2 invalid
// ip freed afterwards
uint8_t icmpProcessPacket(IPv4Packet *ip) {
#ifndef DISABLE_ICMP_CHECKSUM
	uint16_t cs;
#endif
	IcmpPacket *ic = ipv4PacketToIcmpPacket(ip);
	freeIPv4Packet(ip);
	if (ic == NULL) {
		return 1; // Not enough RAM
	}
#ifndef DISABLE_ICMP_CHECKSUM
	cs = checksumIcmp(ic);
	if (cs != ic->checksum) {
		freeIcmpPacket(ic);
		return 2; // Invalid checksum
	}
#endif

	// Handle ICMP Packet
#ifndef DISABLE_ICMP_ECHO
	if (ic->type == 8) {
		// Echo Request
		ic->type = 0; // Now Echo Reply
#ifndef DISABLE_ICMP_CHECKSUM
		cs = checksumIcmp(ic);
		ic->checksum = cs;
#endif
		return icmpSendPacket(ic);
	}
#endif // DISABLE_ICMP_ECHO
	
	freeIcmpPacket(ic);
	return 0;
}

uint8_t icmpSendPacket(IcmpPacket *ic) {
	return 1;
}

// ----------------------
// |    Messages API    |
// ----------------------
#ifndef DISABLE_ICMP_STRINGS
#include <avr/pgmspace.h>

void icmpRegisterMessageCallback(void (*debugOutput)(char *)) {
	debugOutputHandler = debugOutput;
}

char m0_0[] PROGMEM  = "Echo Reply";
char m3_0[] PROGMEM  = "Destination network unreachable";
char m3_1[] PROGMEM  = "Destination host unreachable";
char m3_2[] PROGMEM  = "Destination protocol unreachable";
char m3_3[] PROGMEM  = "Destination port unreachable";
char m3_4[] PROGMEM  = "Fragmentation required and DF flag set";
char m3_5[] PROGMEM  = "Source route failed";
char m3_6[] PROGMEM  = "Destination network unknown";
char m3_7[] PROGMEM  = "Destination host unknown";
char m3_8[] PROGMEM  = "Source host isolated";
char m3_9[] PROGMEM  = "Network administratively prohibited";
char m3_10[] PROGMEM = "Host administratively prohibited";
char m3_11[] PROGMEM = "Network unreachable for TOS";
char m3_12[] PROGMEM = "Host unreachable for TOS";
char m3_13[] PROGMEM = "Communication administratively prohibited";
char m3_14[] PROGMEM = "Host Precedence Violation";
char m3_15[] PROGMEM = "Precedence cutoff in effect";
char m4_0[] PROGMEM  = "Source quench (congestion control)";
char m5_0[] PROGMEM  = "Redirect Datagram for the Network";
char m5_1[] PROGMEM  = "Redirect Datagram for the Host";
char m5_2[] PROGMEM  = "Redirect Datagram for the TOS & Network";
char m5_3[] PROGMEM  = "Redirect Datagram for the TOS & Host";
char m6_0[] PROGMEM  = "Alternate Host Address";
char m8_0[] PROGMEM  = "Echo Request";
char m9_0[] PROGMEM  = "Router Advertisement";
char m10_0[] PROGMEM = "Router discovery/selection/solicitation";
char m11_0[] PROGMEM = "TTL expired in transit";
char m11_1[] PROGMEM = "Fragment reassembly time exceeded";
char m12_0[] PROGMEM = "Bad IP header: Pointer indicates the error";
char m12_1[] PROGMEM = "Bad IP header: Missing a required option";
char m12_2[] PROGMEM = "Bad IP header: Bad length";
char m13_0[] PROGMEM = "Timestamp";
char m14_0[] PROGMEM = "Timestamp reply";
char m15_0[] PROGMEM = "Information request";
char m16_0[] PROGMEM = "Information reply";
char m17_0[] PROGMEM = "Address Mask Request";
char m18_0[] PROGMEM = "Address Mask Reply";
char m30_0[] PROGMEM = "Traceroute Information Request";
char m31_0[] PROGMEM = "Datagram Conversion Error";
char m32_0[] PROGMEM = "mobile Host Redirect";
char m33_0[] PROGMEM = "Where-Are-You";
char m34_0[] PROGMEM = "Here-I-Am";
char m35_0[] PROGMEM = "Mobile Registration Request";
char m36_0[] PROGMEM = "Mobile Registration Reply";
char m37_0[] PROGMEM = "Domain Name Request";
char m38_0[] PROGMEM = "Domain Name Reply";
char m39_0[] PROGMEM = "SKIP Algorithm Discovery Protocol";
char m40_0[] PROGMEM = "Photuris, Security failures";
char m41_0[] PROGMEM = "ICMP for experimental mobility protocols";
char mx_x[] PROGMEM  = "Unknown ICMP Message";

char buffer[45];

#define ret(x) return strcpy_P(buffer, (PGM_P)pgm_read_word(&x))

char *icmpMessage(uint8_t type, uint8_t code) {
	if (type == 0) {
		ret(m0_0);
	} else if (type == 3) {
		switch(code) {
			case 0:	ret(m3_0);
			case 1: ret(m3_1);
			case 2: ret(m3_2);
			case 3: ret(m3_3);
			case 4: ret(m3_4);
			case 5: ret(m3_5);
			case 6: ret(m3_6);
			case 7: ret(m3_7);
			case 8: ret(m3_8);
			case 9: ret(m3_9);
			case 10: ret(m3_10);
			case 11: ret(m3_11);
			case 12: ret(m3_12);
			case 13: ret(m3_13);
			case 14: ret(m3_14);
			case 15: ret(m3_15);
			default: ret(mx_x);
		}
	} else if (type == 4) {
		ret(m4_0);
	} else if (type == 5) {
		switch(code) {
			case 0: ret(m5_0);
			case 1: ret(m5_1);
			case 2: ret(m5_2);
			case 3: ret(m5_3);
			default: ret(mx_x);
		}
	} else if (type == 6) {
		ret(m6_0);
	} else if (type == 8) {
		ret(m8_0);
	} else if (type == 9) {
		ret(m9_0);
	} else if (type == 10) {
		ret(m10_0);
	} else if (type == 11) {
		switch(code) {
			case 0: ret(m11_0);
			case 1: ret(m11_1);
			default: ret(mx_x);
		}
	} else if (type == 12) {
		switch(code) {
			case 0: ret(m12_0);
			case 1: ret(m12_1);
			case 2: ret(m12_2);
			default: ret(mx_x);
		}
	} else if (type == 13) {
		ret(m13_0);
	} else if (type == 14) {
		ret(m14_0);
	} else if (type == 15) {
		ret(m15_0);
	} else if (type == 16) {
		ret(m16_0);
	} else if (type == 17) {
		ret(m17_0);
	} else if (type == 18) {
		ret(m18_0);
	} else if (type == 30) {
		ret(m30_0);
	} else if (type == 31) {
		ret(m31_0);
	} else if (type == 32) {
		ret(m32_0);
	} else if (type == 33) {
		ret(m33_0);
	} else if (type == 34) {
		ret(m34_0);
	} else if (type == 35) {
		ret(m35_0);
	} else if (type == 36) {
		ret(m36_0);
	} else if (type == 37) {
		ret(m37_0);
	} else if (type == 38) {
		ret(m38_0);
	} else if (type == 39) {
		ret(m39_0);
	} else if (type == 40) {
		ret(m40_0);
	} else if (type == 41) {
		ret(m41_0);
	} else {
		ret(mx_x);
	}
	return NULL;
}
#endif

#endif // DISABLE_ICMP