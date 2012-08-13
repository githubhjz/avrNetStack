/*
 * icmp.h
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
#ifndef _icmp_h
#define _icmp_h

#include <net/ipv4.h>

typedef struct {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint32_t restOfHeader;
} IcmpPacket;

// 0 success, 1 not enough mem, 2 invalid
// ip freed afterwards
uint8_t icmpProcessPacket(IPv4Packet *ip);

#ifndef DISABLE_ICMP_ERROR_STRINGS
void icmpRegisterMessageCallback(void (*debugOutput)(char *));
char *icmpMessage(uint8_t type, uint8_t code);
#endif

#endif
