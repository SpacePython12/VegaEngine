/*
    Vega Engine bus emulation header.

    Copyright (c) 2023 SpacePython_

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef VEGA_IO_H
#define VEGA_IO_H 1

#include "vegatypes.h"

typedef enum VegaAccessSize {
    ACCESS_BYTE = 0, // Byte access. Size 1.
    ACCESS_WORD = 1, // Word access. Size 2.
    ACCESS_LONG = 2, // Long access. Size 4.
} VegaAccessSize;

#define VEGA_AFLAG_BYTE (1 << ACCESS_BYTE)
#define VEGA_AFLAG_WORD (1 << ACCESS_WORD)
#define VEGA_AFLAG_LONG (1 << ACCESS_LONG)

typedef struct VegaCommand {
    ubitfield id : 7;
    ubitfield type : 1;
    union {
        struct {
            void * ptr;
            u32 size;
        };
        u32 datal[(sizeof(void *)/sizeof(u32)) + 1];
        u16 dataw[(sizeof(void *)/sizeof(u16)) + 2];
        u8 datab[sizeof(void *) + sizeof(u32)];
    };
} VegaCommand;

typedef struct VegaRegister {
    u8 size;
    u32 (*readCB)(VegaAccessSize); // Called when the program wants to read data FROM the register, eg. subsystem -> program.
    void (*writeCB)(u32, VegaAccessSize); // Called when the program wants to write data TO the register, eg. program -> subsystem.
    void (*cmdCB)(VegaCommand); // Called when the program wants to send a command to the register.
} VegaRegister;

#define VegaRegDef(_size, read, write, cmd) (VegaRegister){.size=_size, .readCB=read, .writeCB=write, .cmdCB=cmd}

cextern void Vega_IOInit(const VegaRegister * regs, u8 count);
cextern void Vega_IODeinit();
cextern u8 Vega_IOGetRegCount();

cextern u32 Vega_IORegRead(u8 id, VegaAccessSize size);
cextern void Vega_IORegWrite(u8 id, u32 val, VegaAccessSize size);

cextern void Vega_IORegCmd(u8 id, VegaCommand cmd);
cextern void Vega_IORegPCmd(u8 id, u8 cmdid, void * ptr, u32 size);
cextern void Vega_IORegLCmd(u8 id, u8 cmdid, u32 * data, u8 len);
cextern void Vega_IORegWCmd(u8 id, u8 cmdid, u16 * data, u8 len);
cextern void Vega_IORegBCmd(u8 id, u8 cmdid, u8 * data, u8 len);

#endif