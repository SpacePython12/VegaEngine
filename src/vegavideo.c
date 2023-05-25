#include <GL/gl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

#include "../include/vega.h"

#if RENDER_SDL
#include <SDL2/SDL.h>

static SDL_Window * win = NULL;
static SDL_Renderer * rend = NULL;
static SDL_Texture * fBuf = NULL;
#elif RENDER_GLFW
#define GL_GLEXT_PROTOTYPES 1
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>

extern const char * VegaFragShader;
extern const char * VegaVertShader;
extern const size_t VegaFragShaderSize;
extern const size_t VegaVertShaderSize;

static GLFWwindow * win = NULL;
static GLuint fragShader, vertShader, shaderProgram, paletteTexture, tileTexture, VBO;

typedef struct VertAttrib {
    struct {
        uint x;
        uint y;
        uint z;
    } screenPos;
    struct {
        uint x;
        uint y;
    } texPos;
    uint palLine;
} VertAttrib;
#else
#error "No rendering API was specified. Define a renderer macro."
#endif

typedef long double VegaTime;

static VegaVideoBackend videoBackend;
static void ** memLocs = NULL;
static VegaTime startTime = 0.0;
static VegaVideoColor * linebufs;

#define linebufget(col, prio) linebufs[(col + (prio * videoBackend.screenW))]

VegaTime Vega_VideoGetAbsTime() {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return (VegaTime)spec.tv_sec + ((VegaTime)spec.tv_nsec / 1000000000.0l);
}

static inline VegaTime Vega_VideoGetTime() {
    return Vega_VideoGetAbsTime()-startTime;
}

static inline void Vega_VideoSleep(VegaTime time) {
    nanosleep(&(struct timespec){.tv_sec = (time_t)time, .tv_nsec = (long)(time - (VegaTime)((time_t)time))}, NULL);
}

void Vega_VideoInit(VegaVideoBackend backend) {
    videoBackend = backend;
#if RENDER_SDL
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        fprintf(stderr, "SDL failed to initalize.");
        exit(0);
    }
    win = SDL_CreateWindow("Vega Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, videoBackend.screenW*3, videoBackend.screenH*3, SDL_WINDOW_OPENGL);
    rend = SDL_CreateRenderer(win, -1, (SDL_RENDERER_ACCELERATED));
    fBuf = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA5551, SDL_TEXTUREACCESS_STREAMING, videoBackend.screenW, videoBackend.screenH);
#elif RENDER_GLFW
    if (!glfwInit()) {
        fprintf(stderr, "GLFW failed to initalize.");
        exit(0);
    }
    win = glfwCreateWindow(videoBackend.screenW*3, videoBackend.screenH*3, "Vega Engine", NULL, NULL);
    glfwMakeContextCurrent(win);

    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &VegaFragShader, NULL);
    glCompileShader(fragShader);
    vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &VegaVertShader, NULL);
    glCompileShader(vertShader);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertShader);
    glAttachShader(shaderProgram, fragShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(fragShader);
    glDeleteShader(vertShader);

    glGenTextures(1, &paletteTexture);
    glBindTexture(GL_TEXTURE_2D, paletteTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, 1 << videoBackend.colorIndexDepth, 1 << videoBackend.paletteIndexDepth, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, NULL);
    
    glGenTextures(1, &tileTexture);
    glBindTexture(GL_TEXTURE_2D, tileTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 8, 8 * (1 << videoBackend.paletteIndexDepth), 0, GL_R, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_2D, 0);

    glGenBuffers(1, &VBO);

    glVertexAttribPointer(0, 3, GL_UNSIGNED_INT, GL_FALSE, 6 * sizeof(uint), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_UNSIGNED_INT, GL_FALSE, 6 * sizeof(uint), (void *)(3 * sizeof(uint)));
    glEnableVertexAttribArray(1);  
    glVertexAttribPointer(2, 1, GL_UNSIGNED_INT, GL_FALSE, 6 * sizeof(uint), (void *)(5 * sizeof(uint)));
    glEnableVertexAttribArray(2);  
#endif
    memLocs = malloc(videoBackend.memLocCount * sizeof(void *));
    for (int i = 0; i < videoBackend.memLocCount; i++) {
        memLocs[i] = calloc(videoBackend.memLocSizes[i], 1);
    }
    if (videoBackend.initCB) videoBackend.initCB();
    startTime = Vega_VideoGetAbsTime();
}
#if RENDER_SDL
void Vega_RenderLine(u32 line, volatile VegaVideoColor * pixels, volatile u32 pitch) { // TODO: Fix the nested code, please.
    volatile u32 col, tile, plane, x, y, count, prio;
    VegaVideoColor color;
    if (videoBackend.shouldBlankLine && videoBackend.shouldBlankLine(line)) {
        for (col = 0; col < videoBackend.screenW-1; col++) {
            pixels[col+(line*videoBackend.screenW)] = videoBackend.getClearColor();
        }
        return;
    }
    for (col = 0; col < videoBackend.scanW; col++) {
        if (col == videoBackend.screenW && videoBackend.hBlankCB) videoBackend.hBlankCB(line);
        else if (col < videoBackend.screenW-1) {
            for (prio = 0; prio < (1 << videoBackend.priorityIndexDepth)-1; prio++) {
                linebufget(col, prio) = 0;
            }
            for (plane = 0; plane < videoBackend.planeCount; plane++) {
                if (videoBackend.getPlaneEnabled && !videoBackend.getPlaneEnabled(plane)) continue;
                x = (col-videoBackend.getPlaneHScroll(plane, line)) % videoBackend.getPlaneHMod(plane);
                y = (line-videoBackend.getPlaneVScroll(plane, col)) % videoBackend.getPlaneVMod(plane);
                tile = videoBackend.getPlaneTileID(plane, x, y);
                color = videoBackend.getPaletteColor(videoBackend.getPlaneTilePalette(plane, x, y), videoBackend.getTileColor(tile, x & 7, y & 7));
                if ((color & 1)) linebufget(col, videoBackend.getPlaneTilePriority(plane, x, y)) = color;
            }
        }
    }
    if (videoBackend.getSpriteFirst) count = videoBackend.getSpriteFirst(); 
    else count = videoBackend.spriteCount;
    while (1) {
        if (!(videoBackend.getSpriteEnabled && !videoBackend.getSpriteEnabled(count))) {
            if (line >= videoBackend.getSpriteY(count) && line < videoBackend.getSpriteY(count)+videoBackend.getSpriteH(count)) {
                for (col = videoBackend.getSpriteX(count); col < videoBackend.getSpriteX(count)+videoBackend.getSpriteW(count); col++) {
                    x = col - videoBackend.getSpriteX(count);
                    y = line - videoBackend.getSpriteY(count);
                    color = videoBackend.getPaletteColor(videoBackend.getSpritePalette(count), videoBackend.getSpriteColor(count, x, y));
                    if (color & 1) linebufget(col, videoBackend.getSpritePriority(count)) = color;
                }
            }
        }
        if (videoBackend.getSpriteLink) {
            count = videoBackend.getSpriteLink(count);
        } else {
            count--;
        }
        if (videoBackend.getSpriteEnd) {
            if (videoBackend.getSpriteEnd(count)) break;
        } else {
            if (!count) break;
        }
    }
    for (col = 0; col < videoBackend.screenW-1; col++) {
        pixels[col+(line*videoBackend.screenW)] = videoBackend.getClearColor();
        if (videoBackend.shouldBlankPixel && !videoBackend.shouldBlankPixel(line, col)) {    
            for (prio = 0; prio < (1 << videoBackend.priorityIndexDepth)-1; prio++) {
                if ((linebufget(col, prio) & 1)) {
                    pixels[col+(line*videoBackend.screenW)] = linebufget(col, prio);
                }
            }
        }
    }
}
#elif RENDER_GLFW
void Vega_ConstructLine(u32 line) {
    GLuint size = 0;



    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
}
#endif

void Vega_VideoRun() {
    bool8 running = true;
    VegaVideoColor * pixels;
    u32 pitch, line, col, i;
    int w, h;

    VegaTime fstarttick, lstarttick, fendtick, lendtick;
    linebufs = malloc(videoBackend.screenW * (1 << videoBackend.priorityIndexDepth) * 2);
    while (running) {
        fstarttick = Vega_VideoGetTime();
#if RENDER_SDL
        SDL_QueryTexture(fBuf, NULL, NULL, &w, &h);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
            }
        }
        SDL_LockTexture(fBuf, NULL, (void **)&pixels, (int *)&pitch);
#elif RENDER_GLFW
        glfwPollEvents();
        running = glfwWindowShouldClose(win) ? true : false;
#endif
#if RENDER_SDL
        if (videoBackend.frameStartCB) videoBackend.frameStartCB();
        for (line = 0; line < videoBackend.scanH; line++) {
            lstarttick = Vega_VideoGetTime();
            if (videoBackend.lineStartCB) videoBackend.lineStartCB(line);
            if (line == videoBackend.screenH && videoBackend.vBlankCB) videoBackend.vBlankCB();
            else if (line < videoBackend.screenH-1) {
                Vega_RenderLine(line, pixels, pitch);
            }
            if (videoBackend.lineEndCB) videoBackend.lineEndCB(line);
            lendtick = Vega_VideoGetTime();
            Vega_VideoSleep(((1.0l/60.0l)/(VegaTime)(videoBackend.scanH)) - (lendtick - lstarttick));
        }
        if (videoBackend.frameEndCB) videoBackend.frameEndCB();
#elif RENDER_GLFW
        glClearColor(
            (float)(videoBackend.getClearColor() >> 11) / (float)((1 << 5)-1),
            (float)(videoBackend.getClearColor() >> 6) / (float)((1 << 5)-1), 
            (float)(videoBackend.getClearColor() >> 1) / (float)((1 << 5)-1), 
            1.0
        );
        glClear(GL_COLOR_BUFFER_BIT);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        if (videoBackend.frameStartCB) videoBackend.frameStartCB();
        for (line = 0; line < videoBackend.scanH; line++) {
            lstarttick = Vega_VideoGetTime();
            if (videoBackend.lineStartCB) videoBackend.lineStartCB(line);
            if (line == videoBackend.screenH && videoBackend.vBlankCB) videoBackend.vBlankCB();
            else if (line < videoBackend.screenH) {
                Vega_ConstructLine(line);
            }
            if (videoBackend.lineEndCB) videoBackend.lineEndCB(line);
            lendtick = Vega_VideoGetTime();
            Vega_VideoSleep((lendtick - lstarttick) - (1.0l/(VegaTime)(videoBackend.scanH))/60.0l);
        }
        if (videoBackend.frameEndCB) videoBackend.frameEndCB();

#endif
#if RENDER_SDL
        SDL_UnlockTexture(fBuf);
        SDL_RenderCopy(rend, fBuf, NULL, NULL);
        SDL_RenderPresent(rend);
#elif RENDER_GLFW
        glfwSwapBuffers(win);
#endif  
        fendtick = Vega_VideoGetTime();
        // printf("\r%Lf             ", 1.0l/(fendtick-fstarttick));
        Vega_VideoSleep((fendtick - fstarttick) - 1.0l/60.0l);
    }
    free(linebufs);
}

void Vega_VideoDeinit() {
    if (videoBackend.deinitCB) videoBackend.deinitCB();
#if RENDER_SDL
    SDL_DestroyWindow(win);
    SDL_DestroyRenderer(rend);
    SDL_DestroyTexture(fBuf);
#elif RENDER_GLFW
    glfwDestroyWindow(win);
#endif
    for (int i = 0; i < videoBackend.memLocCount; i++) {
        free(memLocs[i]);
    }
    free(memLocs);
}

void Vega_VideoSetTitle(const char * name) {
#if RENDER_SDL
    SDL_SetWindowTitle(win, name);
#elif RENDER_GLFW
    glfwSetWindowTitle(win, name);
#endif
}

void * Vega_VideoGetMemLoc(u8 index) {
    return memLocs[index];
}

void Vega_VideoUpdatePaletteCache(u8 line) {
#if RENDER_GLFW
    glBindTexture(GL_TEXTURE_2D, paletteTexture);
    u16 tmp[1 << videoBackend.colorIndexDepth];
    for (u8 i = 0; i < (1 << videoBackend.colorIndexDepth); i++) {
        tmp[i] = videoBackend.getPaletteColor(line, i);
    }
    glTexSubImage2D(paletteTexture, 0, 0, line, (1 << videoBackend.colorIndexDepth), 1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, tmp);
#endif
}

void Vega_VideoUpdateTileCache(u16 tile) {
#if RENDER_GLFW
    glBindTexture(GL_TEXTURE_2D, tileTexture);
    u8 tmp[8][8];
    for (u8 y = 0; y < 8; y++) {
        for (u8 x = 0; x < 8; x++) {
            tmp[y][x] = videoBackend.getTileColor(tile, x, y);
        }
    }
    glTexSubImage2D(tileTexture, 0, 0, tile*8, 8, 8, GL_R, GL_UNSIGNED_BYTE, tmp);
#endif
}