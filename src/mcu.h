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
#include "mcu_interrupt.h"
#include "SDL_atomic.h"

enum {
    // INTC
    DEV_IPRA = 0xc0,
    DEV_IPRB = 0xc1,
    DEV_IPRC = 0xc2,
    DEV_IPRD = 0xc3,

    // ADC
    DEV_ADDRAH = 0x00,
    DEV_ADDRAL = 0x01,
    DEV_ADDRBH = 0x02,
    DEV_ADDRBL = 0x03,
    DEV_ADDRCH = 0x04,
    DEV_ADDRCL = 0x05,
    DEV_ADDRDH = 0x06,
    DEV_ADDRDL = 0x07,
    DEV_ADCSR = 0x08,
    DEV_ADCR = 0x09,
};

extern uint8_t dev_register[0x100];

const uint16_t sr_mask = 0x870f;
enum {
    STATUS_T = 0x8000,
    STATUS_N = 0x08,
    STATUS_Z = 0x04,
    STATUS_V = 0x02,
    STATUS_C = 0x01,
    STATUS_INT_MASK = 0x700
};

enum {
    VECTOR_RESET = 0,
    VECTOR_RESERVED1, // UNUSED
    VECTOR_INVALID_INSTRUCTION,
    VECTOR_DIVZERO,
    VECTOR_TRAP,
    VECTOR_RESERVED2, // UNUSED
    VECTOR_RESERVED3, // UNUSED
    VECTOR_RESERVED4, // UNUSED
    VECTOR_ADDRESS_ERROR,
    VECTOR_TRACE,
    VECTOR_RESERVED5, // UNUSED
    VECTOR_NMI,
    VECTOR_RESERVED6, // UNUSED
    VECTOR_RESERVED7, // UNUSED
    VECTOR_RESERVED8, // UNUSED
    VECTOR_RESERVED9, // UNUSED
    VECTOR_TRAPA_0,
    VECTOR_TRAPA_1,
    VECTOR_TRAPA_2,
    VECTOR_TRAPA_3,
    VECTOR_TRAPA_4,
    VECTOR_TRAPA_5,
    VECTOR_TRAPA_6,
    VECTOR_TRAPA_7,
    VECTOR_TRAPA_8,
    VECTOR_TRAPA_9,
    VECTOR_TRAPA_A,
    VECTOR_TRAPA_B,
    VECTOR_TRAPA_C,
    VECTOR_TRAPA_D,
    VECTOR_TRAPA_E,
    VECTOR_TRAPA_F,
    VECTOR_INTERRUPT_80,
    VECTOR_INTERRUPT_84,
    VECTOR_INTERNAL_INTERRUPT_88,
    VECTOR_INTERNAL_INTERRUPT_8C,
    VECTOR_INTERNAL_INTERRUPT_90,
    VECTOR_INTERNAL_INTERRUPT_94,
    VECTOR_INTERNAL_INTERRUPT_98,
    VECTOR_INTERNAL_INTERRUPT_9C,
    VECTOR_INTERNAL_INTERRUPT_A0,
    VECTOR_INTERNAL_INTERRUPT_A4,
    VECTOR_INTERNAL_INTERRUPT_A8,
    VECTOR_INTERNAL_INTERRUPT_AC,
    VECTOR_INTERNAL_INTERRUPT_B0,
    VECTOR_INTERNAL_INTERRUPT_B4,
    VECTOR_INTERNAL_INTERRUPT_B8,
    VECTOR_INTERNAL_INTERRUPT_BC,
    VECTOR_INTERNAL_INTERRUPT_C0,
    VECTOR_INTERNAL_INTERRUPT_C4,
    VECTOR_INTERNAL_INTERRUPT_C8,
    VECTOR_INTERNAL_INTERRUPT_CC,
    VECTOR_INTERNAL_INTERRUPT_D0,
    VECTOR_INTERNAL_INTERRUPT_D4,
    VECTOR_INTERNAL_INTERRUPT_D8,
    VECTOR_INTERNAL_INTERRUPT_DC,
    VECTOR_INTERNAL_INTERRUPT_E0,
    VECTOR_INTERNAL_INTERRUPT_E4,
    VECTOR_INTERNAL_INTERRUPT_E8,
    VECTOR_INTERNAL_INTERRUPT_EC,
    VECTOR_INTERNAL_INTERRUPT_F0,
};


struct mcu_t {
    uint16_t r[8];
    uint16_t pc;
    uint16_t sr;
    uint8_t cp, dp, ep, tp, br;
    uint8_t sleep;
    uint8_t ex_ignore;
    int32_t exception_pending;
    uint8_t interrupt_pending[INTERRUPT_SOURCE_MAX];
    uint8_t trapa_pending[16];
    uint64_t cycles;
};

extern mcu_t mcu;

void MCU_ErrorTrap(void);

uint8_t MCU_Read(uint32_t address, bool code);
uint16_t MCU_Read16(uint32_t address, bool code);
uint32_t MCU_Read32(uint32_t address, bool code);
void MCU_Write(uint32_t address, uint8_t value);
void MCU_Write16(uint32_t address, uint16_t value);

inline uint32_t MCU_GetAddress(uint8_t page, uint16_t address) {
    return (page << 16) + address;
}

inline uint8_t MCU_ReadCode(void) {
    return MCU_Read(MCU_GetAddress(mcu.cp, mcu.pc), true);
}

inline uint8_t MCU_ReadCodeAdvance(void) {
    uint8_t ret = MCU_ReadCode();
    mcu.pc++;
    return ret;
}

inline void MCU_SetRegisterByte(uint8_t reg, uint8_t val)
{
    mcu.r[reg] = val;
}

inline uint32_t MCU_GetVectorAddress(uint32_t vector)
{
    return MCU_Read32(vector * 4, true);
}

inline uint32_t MCU_GetPageForRegister(uint32_t reg)
{
    if (reg >= 6)
        return mcu.tp;
    else if (reg >= 4)
        return mcu.ep;
    return mcu.dp;
}

inline void MCU_ControlRegisterWrite(uint32_t reg, uint32_t siz, uint32_t data)
{
    if (siz)
    {
        if (reg == 0)
        {
            mcu.sr = data;
            mcu.sr &= sr_mask;
        }
        else if (reg == 5) // FIXME: undocumented
        {
            // printf("fixme 3\n");
            mcu.dp = data & 0xff;
            // printf("%02x%04x: set dp w %02x\n", mcu.cp, mcu.pc, data);
        }
        else if (reg == 4) // FIXME: undocumented
        {
            // printf("fixme 4 %04x\n", data);
            mcu.ep = data & 0xff;
        }
        else if (reg == 3) // FIXME: undocumented
        {
            // printf("fixme 5\n");
            mcu.br = data & 0xff;
        }
        else if (reg == 1) // FIXME: undocumented
        {
            // printf("fixme 6\n");
            mcu.sr = data;
        }
        else
        {
            MCU_ErrorTrap();
        }
    }
    else
    {
        if (reg == 1)
        {
            mcu.sr &= ~0xff;
            mcu.sr |= data & 0xff;
            mcu.sr &= sr_mask;
        }
        else if (reg == 3)
        {
            mcu.br = data;
        }
        else if (reg == 4)
        {
            mcu.ep = data;
        }
        else if (reg == 5)
        {
            mcu.dp = data;
            // printf("%02x%04x: set dp b %02x\n", mcu.cp, mcu.pc, mcu.dp);
        }
        else if (reg == 7)
        {
            mcu.tp = data;
        }
        else
        {
            MCU_ErrorTrap();
        }
    }
}

inline uint32_t MCU_ControlRegisterRead(uint32_t reg, uint32_t siz)
{
    uint32_t ret = 0;
    if (siz)
    {
        if (reg == 0)
        {
            // printf("fixme 7\n");
            ret = mcu.sr & sr_mask;
        }
        else if (reg == 5) // FIXME: undocumented
        {
            // printf("fixme 8 %02x\n", mcu.dp);
            ret = mcu.dp | (mcu.dp << 8);
        }
        else if (reg == 4) // FIXME: undocumented
        {
            // printf("fixme 9\n");
            ret = mcu.ep | (mcu.ep << 8);
        }
        else if (reg == 3) // FIXME: undocumented
        {
            // printf("fixme 10\n");
            ret = mcu.br | (mcu.br << 8);
        }
        else if (reg == 1) // FIXME: undocumented
        {
            // printf("fixme 11\n");
            ret = mcu.sr;
        }
        else
        {
            MCU_ErrorTrap();
        }
        ret &= 0xffff;
    }
    else
    {
        if (reg == 1)
        {
            ret = mcu.sr & sr_mask;
        }
        else if (reg == 3)
        {
            ret = mcu.br;
        }
        else if (reg == 4)
        {
            ret = mcu.ep;
        }
        else if (reg == 5)
        {
            ret = mcu.dp;
        }
        else if (reg == 7)
        {
            ret = mcu.tp;
        }
        else
        {
            MCU_ErrorTrap();
        }
        ret &= 0xff;
    }
    return ret;
}

inline void MCU_SetStatus(uint32_t condition, uint32_t mask)
{
    if (condition)
        mcu.sr |= mask;
    else
        mcu.sr &= ~mask;
}

inline void MCU_PushStack(uint16_t data)
{
    if (mcu.r[7] & 1)
        MCU_Interrupt_Exception(EXCEPTION_SOURCE_ADDRESS_ERROR);
    mcu.r[7] -= 2;
    MCU_Write16(mcu.tp << 16 | mcu.r[7], data);
}

inline uint16_t MCU_PopStack(void)
{
    uint16_t ret;
    if (mcu.r[7] & 1)
        MCU_Interrupt_Exception(EXCEPTION_SOURCE_ADDRESS_ERROR);
    ret = MCU_Read16(mcu.tp << 16 | mcu.r[7], true);
    mcu.r[7] += 2;
    return ret;
}


void MCU_Reset(void);
