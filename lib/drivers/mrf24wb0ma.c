/*
 * mrf24wb0ma.c
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

#define DEBUG 2
// 1 --> Debug Output
// 2 --> Interactive Passphrase Prompt

#include <std.h>
#include <tasks.h>
#include <net/mac.h>
#include <net/controller.h>

#include "asynclabs/config.h"
#include "asynclabs/g2100.h"

#define INTPORT PORTD
#define INTPORTPIN PIND
#define INTPIN PD2
#define INTDDR DDRD

uint8_t ownMacAddress[6];
uint8_t shouldGetPacket = 0;

extern uint8_t *zg_buf;

// Definitions for prototypes in config.h and spi.h
char ssid[32] = {"xythobuz"}; // 32byte max
uint8_t ssid_len;
uint8_t wireless_mode = WIRELESS_MODE_INFRA;
uint8_t security_type = 3; // 0 Open, 1 WEP, 2 WPA, 3 WPA2
char security_passphrase[32] = {""}; // WPA, WPA2 Passphrase
uint8_t security_passphrase_len;
unsigned char wep_keys[52] PROGMEM = { // WEP 128-bit keys
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
uint8_t zg2100IsrEnabled; // In asynclabs spi.h

uint8_t zgInterruptOccured(void) {
    if (zg2100IsrEnabled) {
        if (INTPORTPIN & (1 << INTPIN)) {
            return 1;
        }
    }
    return 0;
}

uint8_t macInitialize(uint8_t *address) { // 0 if success, 1 on error
    uint8_t i;
    uint8_t *p;

    INTDDR &= ~(1 << INTPIN); // Interrupt PIN

    debugPrint("Initializing WiFi...");

    zg_init();

    debugPrint(" Done!\nConnecting...");

    addConditionalTask(zg_isr, zgInterruptOccured); // Emulate INT0
    do {
        zg_drv_process();
    } while (!macLinkIsUp());

    debugPrint(" Done!\n");

    p = zg_get_mac(); // Global Var. in g2100.c
    for (i = 0; i < 6; i++) {
        ownMacAddress[i] = p[i];
        address[i] = ownMacAddress[i];
    }

    return 0;
}

void macReset(void) {
    zg_chip_reset();
}

uint8_t macLinkIsUp(void) { // 0 if down, 1 if up
    if (zg_get_conn_state()) {
        return 1;
    } else {
        return 0;
    }
}

uint8_t macSendPacket(Packet *p) { // 0 on success, 1 on error
    return 0; // no way to know this?
}

uint8_t macPacketsReceived(void) { // 0 if no packet, 1 if packet ready
    if (rx_ready) {
        return 1;
    } else {
        zg_drv_process();
        if (rx_ready) {
            return 1;
        } else {
            return 0;
        }
    }
}

Packet *macGetPacket(void) { // Returns NULL on error
    uint16_t l = zg_get_rx_status();
    Packet *p = (Packet *)mmalloc(sizeof(Packet));
    if (p == NULL) {
        return NULL;
    }

    p->dLength = l;
    if (l > 0) {
        p->d = (uint8_t *)mmalloc(l * sizeof(uint8_t));
        if (p->d != NULL) {
            for (uint16_t i = 0; i < l; i++) {
                p->d[i] = zg_buf[i];
            }
        }
    } else {
        p->d = NULL;
    }
    return p;
}

uint8_t macHasInterrupt(void) {
    return macPacketsReceived();
}
