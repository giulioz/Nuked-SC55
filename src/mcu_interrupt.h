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

void MCU_Interrupt_SetRequest(uint32_t interrupt, uint32_t value);
void MCU_Interrupt_Exception(uint32_t exception);
void MCU_Interrupt_TRAPA(uint32_t vector);
void MCU_Interrupt_Handle(void);

enum {
    INTERRUPT_SOURCE_NMI = 0,
    INTERRUPT_SOURCE_IRQ0,
    INTERRUPT_SOURCE_WDT,
    INTERRUPT_SOURCE_OCF0,
    INTERRUPT_SOURCE_OCF1,
    INTERRUPT_SOURCE_OCF2,
    INTERRUPT_SOURCE_ISF0,
    INTERRUPT_SOURCE_ISF1,
    INTERRUPT_SOURCE_ISF2,
    INTERRUPT_SOURCE_ISF3,
    INTERRUPT_SOURCE_ISF4,
    INTERRUPT_SOURCE_ISF5,
    INTERRUPT_SOURCE_ISF6,
    INTERRUPT_SOURCE_ISF7,
    INTERRUPT_SOURCE_ISF8,
    INTERRUPT_SOURCE_ISF9,
    INTERRUPT_SOURCE_ISF10,
    INTERRUPT_SOURCE_ISF11,
    INTERRUPT_SOURCE_ISF12,
    INTERRUPT_SOURCE_ISF13,
    INTERRUPT_SOURCE_ISF14,
    INTERRUPT_SOURCE_ISF15,
    INTERRUPT_SOURCE_SCI_ERI,
    INTERRUPT_SOURCE_SCI_RXI,
    INTERRUPT_SOURCE_SCI_TXI,
    INTERRUPT_SOURCE_ADI,
    INTERRUPT_SOURCE_MAX
};

enum {
    EXCEPTION_SOURCE_ADDRESS_ERROR = 0,
    EXCEPTION_SOURCE_INVALID_INSTRUCTION,
    EXCEPTION_SOURCE_TRACE,
};
