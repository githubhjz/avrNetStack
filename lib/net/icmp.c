/*
 * icmp.c
 *
 * Copyright (c) 2012, Thomas Buck <xythobuz@me.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#define DEBUG 0

/*
 * The ICMP Checksum Algorithm is only producing valid results when sending packets.
 * Received ICMP Packets are always detected as invalid checksum...
 */
#define ICMP_CHECKSUM_DONT_CARE

#include <std.h>
#include <net/utils.h>
#include <net/icmp.h>
#include <serial.h>
#include <net/controller.h>

#ifndef DISABLE_ICMP

void (*echoHandler)(Packet *) = NULL;

#if DEBUG >= 2
char *icmpMessage(uint8_t type, uint8_t code);
#endif

// ----------------------
// |    Internal API    |
// ----------------------

#ifndef DISABLE_ICMP_CHECKSUM
uint16_t checksum(uint8_t *d, uint16_t l); // ipv4.c

uint16_t icmpChecksum(Packet *p) {
#if DEBUG >= 3
    uint16_t i;
    debugPrint("Length: ");
    debugPrint(timeToString(p->dLength));
    debugPrint(" - ");
    debugPrint(timeToString(ICMPOffset));
    debugPrint(" = ");
    debugPrint(timeToString(p->dLength - ICMPOffset));
    debugPrint("\nICMP Packet Data:\n");
    for (i = 0; i < p->dLength - ICMPOffset; i++) {
        debugPrint(hexToString(p->d[ICMPOffset + i]));
        if (i < (p->dLength - ICMPOffset - 1)) {
            debugPrint(" ");
        }
    }
    debugPrint("\n");
#endif
    return checksum(p->d + ICMPOffset, p->dLength - ICMPOffset);
}
#endif // DISABLE_ICMP_CHECKSUM

#ifndef DISABLE_ICMP_ECHO
uint8_t icmpAnswerEcho(Packet *p) {
    // Just change type to zero, recompute checksum, send.
    uint8_t i;
    uint8_t *po;
    IPv4Address target;
    uint16_t cs = 0x0000;
    for (i = 0; i < 4; i++) {
        // Get Target IP
        target[i] = p->d[MACPreambleSize + IPv4PacketSourceOffset + i];
    }
    p->d[ICMPOffset] = 0x00; // Echo Reply
    p->d[ICMPOffset + 2] = 0;
    p->d[ICMPOffset + 3] = 0; // Clear Checksum Field

    if (p->dLength > ICMPOffset + ICMPPacketSize + 4) {
        // We strip the last 4 bytes from the icmp payload
        // To be honest, i don't know why, but ping
        // is reporting a wrong total length otherwise...
        po = mrealloc(p->d, p->dLength - 4, p->dLength);
        if (po != NULL) {
            p->d = po;
        }
        p->dLength -= 4;
    }

#ifndef DISABLE_ICMP_CHECKSUM
    cs = icmpChecksum(p);
#else
    // Checksum field was not cleared...
    cs = get16Bit(p->d, ICMPOffset + 2) - 0x0800;
#endif
    p->d[ICMPOffset + 2] = (cs & 0xFF00) >> 8;
    p->d[ICMPOffset + 3] = (cs & 0x00FF);
    return ipv4SendPacket(p, target, ICMP);
}
#endif // DISABLE_ICMP_ECHO

// ----------------------
// |    External API    |
// ----------------------

void icmpInit(void) {}

// 0 success, 1 not enough mem, 2 invalid
// p freed afterwards
uint8_t icmpProcessPacket(Packet *p) {
    uint8_t type, code;
#ifndef DISABLE_ICMP_CHECKSUM
    uint16_t cs, ocs;
#endif

    type = p->d[ICMPOffset];
    code = p->d[ICMPOffset + 1];

#if DEBUG >= 2
    debugPrint(icmpMessage(type, code));
    debugPrint("\n");
#endif

#ifndef DISABLE_ICMP_CHECKSUM
    ocs = get16Bit(p->d, ICMPOffset + 2); // Store Checksum
    p->d[ICMPOffset + 2] = 0;
    p->d[ICMPOffset + 3] = 0; // Clear Checksum Field
    cs = icmpChecksum(p); // Calculate Checksum
    if (cs != ocs) {
#if DEBUG >= 1
        debugPrint("ICMP Checksum invalid: ");
        debugPrint(hexToString(cs));
        debugPrint(" != ");
        debugPrint(hexToString(ocs));
        debugPrint("\n");
#endif
#ifndef ICMP_CHECKSUM_DONT_CARE
        mfree(p->d, p->dLength);
        mfree(p, sizeof(Packet));
        return 2; // Invalid
#endif
    } else {
        debugPrint("Valid ICMP Packet!\n");
    }
#endif

    if ((type == 8) && (code == 0)) {
        // Echo request. Send reply
#ifndef DISABLE_ICMP_ECHO
        debugPrint("Sending Echo Response...\n");
        return icmpAnswerEcho(p);
#endif
    }

    if ((type == 0) && (code == 0) && (echoHandler != NULL)) {
        echoHandler(p);
        return 0; // echoHandler has to free p
    }

    mfree(p->d, p->dLength);
    mfree(p, sizeof(Packet));
    return 0;
}

void registerEchoReplyHandler(void (*func)(Packet *)) {
    echoHandler = func;
}

void sendEchoRequest(uint8_t *ip) {
    uint16_t cs;
    Packet *p = (Packet *)mmalloc(sizeof(Packet));
    if (p == NULL) {
        return;
    }
    p->dLength = ICMPOffset + ICMPPacketSize + 4;
    p->d = (uint8_t *)mmalloc(p->dLength);
    if (p->d == NULL) {
        mfree(p, sizeof(Packet));
        return;
    }
    p->d[ICMPOffset] = 8; // Type
    p->d[ICMPOffset + 1] = 0; // Code
    p->d[ICMPOffset + 2] = 0;
    p->d[ICMPOffset + 3] = 0; // Clear checksum field
    // We fill the echo id with random data...
    for (cs = 0; cs < 4; cs++) {
        p->d[ICMPOffset + 4 + cs] = (uint8_t)(rand() & 0xFF);
    }
#ifndef DISABLE_ICMP_CHECKSUM
    cs = icmpChecksum(p);
    set16Bit(p->d, ICMPOffset + 2, cs);
#endif
    ipv4SendPacket(p, ip, ICMP);
}

// ----------------------
// |    Messages API    |
// ----------------------
#if DEBUG >= 2
const char m0_0[] PROGMEM  = "Echo Reply";
const char m3_0[] PROGMEM  = "Destination network unreachable";
const char m3_1[] PROGMEM  = "Destination host unreachable";
const char m3_2[] PROGMEM  = "Destination protocol unreachable";
const char m3_3[] PROGMEM  = "Destination port unreachable";
const char m3_4[] PROGMEM  = "Fragmentation required and DF flag set";
const char m3_5[] PROGMEM  = "Source route failed";
const char m3_6[] PROGMEM  = "Destination network unknown";
const char m3_7[] PROGMEM  = "Destination host unknown";
const char m3_8[] PROGMEM  = "Source host isolated";
const char m3_9[] PROGMEM  = "Network administratively prohibited";
const char m3_10[] PROGMEM = "Host administratively prohibited";
const char m3_11[] PROGMEM = "Network unreachable for TOS";
const char m3_12[] PROGMEM = "Host unreachable for TOS";
const char m3_13[] PROGMEM = "Communication administratively prohibited";
const char m3_14[] PROGMEM = "Host Precedence Violation";
const char m3_15[] PROGMEM = "Precedence cutoff in effect";
const char m4_0[] PROGMEM  = "Source quench (congestion control)";
const char m5_0[] PROGMEM  = "Redirect Datagram for the Network";
const char m5_1[] PROGMEM  = "Redirect Datagram for the Host";
const char m5_2[] PROGMEM  = "Redirect Datagram for the TOS & Network";
const char m5_3[] PROGMEM  = "Redirect Datagram for the TOS & Host";
const char m6_0[] PROGMEM  = "Alternate Host Address";
const char m8_0[] PROGMEM  = "Echo Request";
const char m9_0[] PROGMEM  = "Router Advertisement";
const char m10_0[] PROGMEM = "Router discovery/selection/solicitation";
const char m11_0[] PROGMEM = "TTL expired in transit";
const char m11_1[] PROGMEM = "Fragment reassembly time exceeded";
const char m12_0[] PROGMEM = "Bad IP header: Pointer indicates the error";
const char m12_1[] PROGMEM = "Bad IP header: Missing a required option";
const char m12_2[] PROGMEM = "Bad IP header: Bad length";
const char m13_0[] PROGMEM = "Timestamp";
const char m14_0[] PROGMEM = "Timestamp reply";
const char m15_0[] PROGMEM = "Information request";
const char m16_0[] PROGMEM = "Information reply";
const char m17_0[] PROGMEM = "Address Mask Request";
const char m18_0[] PROGMEM = "Address Mask Reply";
const char m30_0[] PROGMEM = "Traceroute Information Request";
const char m31_0[] PROGMEM = "Datagram Conversion Error";
const char m32_0[] PROGMEM = "mobile Host Redirect";
const char m33_0[] PROGMEM = "Where-Are-You";
const char m34_0[] PROGMEM = "Here-I-Am";
const char m35_0[] PROGMEM = "Mobile Registration Request";
const char m36_0[] PROGMEM = "Mobile Registration Reply";
const char m37_0[] PROGMEM = "Domain Name Request";
const char m38_0[] PROGMEM = "Domain Name Reply";
const char m39_0[] PROGMEM = "SKIP Algorithm Discovery Protocol";
const char m40_0[] PROGMEM = "Photuris, Security failures";
const char m41_0[] PROGMEM = "ICMP for experimental mobility protocols";
const char mx_x[] PROGMEM  = "Unknown ICMP Message";

#define ret(x) return strcpy_P(buff, x)

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
