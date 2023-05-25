/*
    Vega Engine video subsystem header.

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

#ifndef VEGA_VIDEO_H
#define VEGA_VIDEO_H 1

#include "vegatypes.h"
#include "vegaio.h"

typedef u16 VegaVideoColor; // Arranged in RRRRR GGGGG BBBBB E (E = enabled/transparent)

typedef enum VegaVideoIndexDepth {
    VEGA_INDDPTH_0, // 0 bits per index, 1 possible index (this is invalid for color indexing)
    VEGA_INDDPTH_1, // 1 bit per index, 2 possible indexes
    VEGA_INDDPTH_2, // 2 bits per index, 4 possible indexes
    VEGA_INDDPTH_3, // 3 bits per index, 8 possible indexes
    VEGA_INDDPTH_4, // 4 bits per index, 16 possible indexes
    VEGA_INDDPTH_5, // 5 bits per index, 32 possible indexes
    VEGA_INDDPTH_6, // 6 bits per index, 64 possible indexes
    VEGA_INDDPTH_7, // 7 bits per index, 128 possible indexes
    VEGA_INDDPTH_8, // 8 bits per index, 256 possible indexes
} VegaVideoIndexDepth;

typedef struct VegaVideoBackend { // Constant settings and 
    u16 screenW; // The width of the internal screen buffer, in pixels.
    u16 screenH; // The height of the internal screen buffer, in pixels.
    u16 scanW; // The amount of pixels to be scanned. scanW - screenW = hblank pixels
    u16 scanH; // The amount of pixels to be scanned. scanH - screenH = vblank lines

    u8 memLocCount; // The amount of entries in the memLocSizes field.
    u32 * memLocSizes; // An array of memory area sizes to be allocated. Ex: VRAM, CRAM/CGRAM, etc.
    u8 regCount; // The amount of entries in the regs field.
    u8 * regs; // An array of register IDs the video subsystem needs access to.

    VegaVideoIndexDepth colorIndexDepth; // Bits per color index in a palette, typically stored in the tile/sprite data. (Ex: MD/Gen = VEGA_INDDPTH_4)
    VegaVideoIndexDepth paletteIndexDepth; // Bits per palette index, typically stored in layout/sprite data. (Ex: MD/Gen = VEGA_INDDPTH_2)
    VegaVideoIndexDepth priorityIndexDepth; // Bits per priority index, typically stored in layout/sprite data. (Ex: MD/Gen = VEGA_INDDPTH_1)
    u8 spriteCount; // Maximum number of sprites to render. You can limit the maximum number of sprites by always returning false in getSpriteEnabled after a certain ID.
    u8 planeCount; // Maximum number of planes to render. You can limit the maximum number of planes by always returning false in getPlaneEnabled after a certain ID.

    VegaVideoColor (*getClearColor)(); // Returns the background color.
    VegaVideoColor (*getPaletteColor)(u8 palette, u8 color); // Returns the appropriate color from the color lookup memory.
    bool8 (*getDisplayEnabled)(); // Returns false to disable rendering. If this is NULL, rendering will always occur.

    u8 (*getTileColor)(u16 tile, u8 x, u8 y); // Returns the color index at pixel coordinate (X, Y) in tile TILE.

    u16 (*getPlaneTileID)(u8 plane, u16 x, u16 y); // Returns the tile index at the pixel coordinate (X, Y) in plane PLANE.
    u8 (*getPlaneTilePriority)(u8 plane, u16 x, u16 y); // Returns the tile priority at the pixel coordinate (X, Y) in plane PLANE.
    u8 (*getPlaneTilePalette)(u8 plane, u16 x, u16 y); // Returns the tile palette at the pixel coordinate (X, Y) in plane PLANE.
    u16 (*getPlaneHScroll)(u8 plane, u16 row); // Returns the amount of pixels row ROW of plane PLANE should be scrolled horizontally.
    u16 (*getPlaneVScroll)(u8 plane, u16 col); // Returns the amount of pixels column COL of plane PLANE should be scrolled vertically.
    u16 (*getPlaneHMod)(u8 plane); // Returns the width in pixels of plane PLANE.
    u16 (*getPlaneVMod)(u8 plane); // Returns the height in pixels of plane PLANE.
    bool8 (*getPlaneEnabled)(u8 plane); // Returns true if plane PLANE should be rendered, false otherwise. If this is NULL, all planes will always be rendered.

    u8 (*getSpriteColor)(u8 sprite, u16 x, u16 y); // Returns the color index at pixel coordinate (X, Y) in sprite SPRITE.
    u8 (*getSpritePriority)(u8 sprite); // Returns the priority of sprite SPRITE.
    u8 (*getSpritePalette)(u8 sprite); // Returns the palette of sprite SPRITE.
    u16 (*getSpriteX)(u8 sprite); // Returns the x position in pixels of sprite SPRITE.
    u16 (*getSpriteY)(u8 sprite); // Returns the y position in pixels of sprite SPRITE.
    u16 (*getSpriteW)(u8 sprite); // Returns the width in pixels of sprite SPRITE.
    u16 (*getSpriteH)(u8 sprite); // Returns the height in pixels of sprite SPRITE.
    bool8 (*getSpriteEnabled)(u8 sprite); // Returns true if sprite SPRITE should be rendered, false otherwise. If this is NULL, all sprites will always be rendered.
    u8 (*getSpriteFirst)(); // Returns the first sprite to render. If NULL, rendering will always start with the last sprite.
    u8 (*getSpriteLink)(u8 sprite); // Returns the sprite to render after sprite SPRITE. If NULL, the next sprite will be the one with the next lowest ID.
    bool8 (*getSpriteEnd)(u8 sprite); // Returns true if SPRITE is the last sprite to render. If NULL, the last sprite is sprite 0.

    bool8 (*shouldBlankLine)(u32 line); // Returns true if line LINE should be filled with background color, false otherwise. If this is NULL, no lines will be blanked.
    bool8 (*shouldBlankPixel)(u32 line, u32 col); // Returns true if the pixel at (COL, LINE) should be filled with background color, false otherwise. If this is NULL, no blanking will occur.

    void (*initCB)(); // Called once to initialize the video subsytem.
    void (*deinitCB)(); // Called once to deinitialize the video subsytem.
    void (*frameStartCB)(); // Called once before the frame is drawn.
    void (*frameEndCB)(); // Called once after the frame is drawn.
    void (*vBlankCB)(); // Called once VBlank starts.
    void (*lineStartCB)(u16 line); // Called once before a line is drawn.
    void (*lineEndCB)(u16 line); // Called once after a line is drawn.
    void (*hBlankCB)(u16 line); // Called once HBlank starts.
} VegaVideoBackend;

cextern void Vega_VideoInit(VegaVideoBackend backend);
cextern void Vega_VideoRun();
cextern void Vega_VideoDeinit();

cextern void Vega_VideoSetTitle(const char * name);

#if defined(VEGA_VIDEO_BACKEND) || defined(VEGA_INTERNAL)
cextern void * Vega_VideoGetMemLoc(u8 index);
cextern bool8 Vega_VideoInHBlank();
cextern bool8 Vega_VideoInVBlank();
cextern void Vega_VideoUpdatePaletteCache();
cextern void Vega_VideoUpdatePlaneCache(u8 plane);
cextern void Vega_VideoUpdateSpriteCache(u8 sprite);
#endif

#endif