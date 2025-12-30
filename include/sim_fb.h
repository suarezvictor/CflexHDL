// Copyright (C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>

#ifndef __SIM_FB_H__
#define __SIM_FB_H__

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

typedef struct
{
    SDL_Window* win;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
} fb_handle_t;

bool fb_init(unsigned width, unsigned height, bool vsync, fb_handle_t *handle);
void fb_update(fb_handle_t *handle, const void *buf, size_t stride_bytes);
void fb_deinit(fb_handle_t *handle);
bool fb_should_quit(void);  
void fb_screenshot_ppm(const char *filename, const uint32_t *pixels, int width, int height, size_t stride_bytes);


#ifdef __cplusplus
#define EXTERN_C extern "C" 
#else
#define EXTERN_C
#endif

EXTERN_C uint64_t SDL_GetPerformanceCounter(void);
EXTERN_C uint64_t SDL_GetPerformanceFrequency(void);
inline uint64_t higres_ticks() { return SDL_GetPerformanceCounter(); }
inline uint64_t higres_ticks_freq() { return SDL_GetPerformanceFrequency(); }

#endif //__SIM_FB_H__
