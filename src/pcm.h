/*
 * Copyright (C) 2021, 2024 nukeykt
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#include <stdint.h>

struct Ram1VoiceInfo {
    uint32_t addressEnd;
    uint32_t v1;
    uint32_t addressLoop;
    uint32_t v3;
    uint32_t address;
    uint32_t v5;
    
    uint32_t v6; // unused
    uint32_t v7; // unused
};

struct Ram2VoiceInfo {
    // Set by CPU
    uint16_t pitch; // pitch?
    uint16_t pan; // pan
    uint16_t revChorSend; // reverb send | chorus send
    uint16_t v3; // loop mode/playing? | ??
    uint16_t volume; // volume?
    uint16_t cutoff; // cutoff freq
    uint16_t resonance; // resonance (inverse) | filter mode
    uint16_t v7;
    
    // Set by PCM
    uint16_t v8;
    uint16_t v9; // v3 tv?
    uint16_t volumeTV; // volume tv
    uint16_t cutoffTV; // cutoff tv

    uint16_t v12; // unused
    uint16_t v13; // unused
    uint16_t v14; // unused
    uint16_t v15; // unused
};

struct pcm_t {
    Ram1VoiceInfo ram1[32];
    Ram2VoiceInfo ram2[32];
    uint32_t select_channel;
    uint32_t voice_mask;
    uint32_t voice_mask_pending;
    uint32_t voice_mask_updating;
    uint32_t write_latch;
    uint32_t wave_read_address;
    uint8_t wave_byte_latch;
    uint32_t read_latch;
    uint8_t config_reg_3c; // SC55:c3 JV880:c0
    uint8_t config_reg_3d;
    uint32_t irq_channel;
    uint32_t irq_assert;

    // if the first sample has been rendered already
    uint32_t nfs;

    uint32_t tv_counter;

    uint64_t cycles;

    uint16_t eram[0x4000];

    int accum_l;
    int accum_r;
    int rcsum[2];
};

extern pcm_t pcm;
extern uint8_t waverom1[];
extern uint8_t waverom2[];
extern uint8_t waverom3[];
extern uint8_t waverom_exp[];

void PCM_Write(uint32_t address, uint8_t data);
uint8_t PCM_Read(uint32_t address);
void PCM_Reset(void);
void PCM_Update(uint64_t cycles);
