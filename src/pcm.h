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

// config_reg_3c
// 0: bit depth?
// 1: bit depth?
// 2: bit depth?
// 3: bit depth?
// 4: bit depth?
// 5: bit depth?
// 6: oversampling
// 7: output on/off

// config_reg_3d
// 0: reg_slots
// 1: reg_slots
// 2: reg_slots
// 3: reg_slots
// 4: reg_slots
// 5: ~WCS7/WA20
// 6: 
// 7: 

struct Ram1VoiceInfo {
    uint32_t addressEnd;
    uint32_t filterTempHPF;
    uint32_t addressLoop;
    uint32_t filterTempLPF;
    uint32_t address;
    uint32_t tempReference;
    
    uint32_t v6; // unused
    uint32_t v7; // unused
};

struct Ram1Summing {
    uint32_t prevL;
    uint32_t prevR;
    uint32_t newL;
    uint32_t osL;
    uint32_t newR;
    uint32_t osR;
    
    uint32_t v6; // unused
    uint32_t v7; // unused
};

struct Ram1Chorus {
    uint32_t addressEnd;
    uint32_t sumL;
    uint32_t addressLoop;
    uint32_t sumR;
    uint32_t address;
    
    uint32_t v5; // unused
    uint32_t v6; // unused
    uint32_t v7; // unused
};

struct Ram2VoiceInfo {
    //
    // Set by CPU

    uint16_t pitch; // pitch coarse | pitch fine
    uint16_t pan; // pan l | pan r
    uint16_t revChorSend; // reverb send | chorus send
    uint16_t volume1; // volume1 | volume1 speed
    uint16_t volume2; // volume2 | volume2 speed
    uint16_t cutoff; // cutoff | cutoff speed

    uint16_t resonanceFlags;
    // 0: irq enable
    // 1: filter mode (0:lpf, 1:hpf)
    // 8-15: resonance (inverse)

    uint16_t addrLoopFlags; // bank? | "key"?
    // 0: sub_phase addr (pitch)
    // 1: sub_phase addr (pitch)
    // 2: sub_phase addr (pitch)
    // 3: sub_phase addr (pitch)
    // 4: sub_phase addr (pitch)
    // 5: key
    // 6: b6 (ping-pong?)
    // 7: b7 (backwards?)
    // 8,9,10,11: hiaddr
    // 12,13,14,15: nibble
    
    //
    // Set by PCM

    uint16_t subPhaseState;
    // 0-13: sub_phase
    // 14: irq disable
    // 15: b15 some loop state

    uint16_t volume1TV; // volume1 tv
    uint16_t volume2TV; // volume2 tv
    uint16_t cutoffTV; // cutoff tv

    uint16_t v12; // unused
    uint16_t v13; // unused
    uint16_t v14; // unused
    uint16_t v15; // unused
};

struct pcm_t {
    Ram1VoiceInfo ram1[32];
    Ram2VoiceInfo ram2[32];

    Ram1Summing *ram1VoiceSumming = (Ram1Summing*)&ram1[30];
    Ram2VoiceInfo *ram2VoiceSumming = &ram2[30];
    Ram1Chorus *ram1VoiceChorusSum = (Ram1Chorus*)&ram1[31];
    Ram2VoiceInfo *ram2VoiceChorusSum = &ram2[31];
    
    Ram1VoiceInfo *ram1Voice31 = &ram1[31];
    Ram2VoiceInfo *ram2Voice31 = &ram2[31];

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

    bool not_first_sample;

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
