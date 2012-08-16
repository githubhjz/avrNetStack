/*
 * utils.h
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
#ifndef _utils_h
#define _utils_h

#include <net/udp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/dns.h>

void freeDnsRecord(DnsRecord *dr);
void freeDnsTableEntry(DnsTableEntry *d);
void freeDnsQuestion(DnsQuestion *d);
void freeUdpPacket(UdpPacket *up);
void freeIPv4Packet(IPv4Packet *ip);
void freeIcmpPacket(IcmpPacket *ic);
uint8_t isEqual(uint8_t *d1, uint8_t *d2, uint8_t l);
uint8_t isEqualMem(uint8_t *d1, uint8_t *d2, uint8_t l);

#endif