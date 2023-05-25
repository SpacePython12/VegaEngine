/*
    Vega Engine bus emulation sources.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/vega.h"

static VegaRegister * regArray;
static u8 regCount;

void Vega_IOInit(const VegaRegister * regs, u8 count) {
    regArray = malloc(sizeof(VegaRegister)*count);
    if (!regArray) perror("io register allocation");
    memcpy(regArray, regs, sizeof(VegaRegister)*count);
    regCount = count;
}

void Vega_IODeinit() {
    free(regArray);
}

u8 Vega_IOGetRegCount() {
    return regCount;
}

u32 Vega_IORegRead(u8 id, VegaAccessSize size) {
    if (id >= regCount || !regArray[id].readCB) return 0;
    else return regArray[id].readCB(size);
}

void Vega_IORegWrite(u8 id, u32 val, VegaAccessSize size) {
    if (id >= regCount || !regArray[id].writeCB) return;
    regArray[id].writeCB(val, size);
}

void Vega_IORegCmd(u8 id, VegaCommand cmd) {
    if (id >= regCount || !regArray[id].cmdCB) return;
    regArray[id].cmdCB(cmd);
}

void Vega_IORegPCmd(u8 id, u8 cmdid, void * ptr, u32 size) {
    VegaCommand cmd = (VegaCommand){.id=cmdid, .type=0, .ptr=ptr, .size=size};
    Vega_IORegCmd(id, cmd);
}

void Vega_IORegLCmd(u8 id, u8 cmdid, u32 * data, u8 len) {
    VegaCommand cmd = (VegaCommand){.id=cmdid, .type=0};
    u8 n = (sizeof(void *) + sizeof(u32)) < (len << 2) ? (sizeof(void *) + sizeof(u32)) : (len << 2);
    memcpy(cmd.datal, data, n);
    Vega_IORegCmd(id, cmd);
}

void Vega_IORegWCmd(u8 id, u8 cmdid, u16 * data, u8 len) {
    VegaCommand cmd = (VegaCommand){.id=cmdid, .type=0};
    u8 n = (sizeof(void *) + sizeof(u32)) < (len << 1) ? (sizeof(void *) + sizeof(u32)) : (len << 1);
    memcpy(cmd.dataw, data, n);
    Vega_IORegCmd(id, cmd);
}

void Vega_IORegBCmd(u8 id, u8 cmdid, u8 * data, u8 len) {
    VegaCommand cmd = (VegaCommand){.id=cmdid, .type=0};
    u8 n = (sizeof(void *) + sizeof(u32)) < (len) ? (sizeof(void *) + sizeof(u32)) : (len);
    memcpy(cmd.datab, data, n);
    Vega_IORegCmd(id, cmd);
}