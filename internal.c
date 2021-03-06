#include <SDL2/SDL.h>
#include "internal.h"
#include "sound.h"

static Win win = NULL;
static Rdr rdr;

static int pause = 0;
static int tick_count = 1;
static int draw_time = 16;
static int start_time = 0;

static int flash_time = 0;

const static double tick_len = 1000/60.0;

static Input input = {0};

static void input_event(SDL_Event e) {
    switch(e.type) {
    case SDL_KEYDOWN:
        input.keys[e.key.keysym.scancode] = 1;
        break;
    case SDL_KEYUP:
        input.keys[e.key.keysym.scancode] = 0;
        break;
    }
}

static Input input_update()
{
    Input input_copy = input;
    for(int i=0; i<SDL_NUM_SCANCODES; ++i) {
        if (input.keys[i]) input.keys[i]++;
    }
    return input_copy;
}

static int hotkey(SDL_Event e)
{
    switch(e.type) {
    case SDL_KEYDOWN:
        switch (e.key.keysym.scancode) {
        case SDL_SCANCODE_M:
            sound_toggle();
            return 1;
        case SDL_SCANCODE_P:
            pause = !pause;
            return 1;
        }
    }
    return 0;
}

void flash(int time) { flash_time = time; }
static void draw_bg()
{
    SDL_SetRenderDrawColor(rdr, 20, 30, 40, 255);
    SDL_RenderClear(rdr);

    if(flash_time) flash_time--;
    int blue = flash_time % 8 > 4 ? 200 : 0;

    SDL_SetRenderDrawColor(rdr, 0, 0, blue, 255);
    const SDL_Rect borders = {0,0,GAME_W,GAME_H};
    SDL_RenderFillRect(rdr, &borders);
}

void pause_toggle() { pause = !pause; }

void run_scene(Scene scene)
{
    int is_root = !win;
    if(is_root) {
        SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        win = SDL_CreateWindow(
            "hi", -1, -1, GAME_W*3, GAME_H*3,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        rdr = SDL_CreateRenderer(win, 0, SDL_RENDERER_PRESENTVSYNC);
        SDL_RenderSetLogicalSize(rdr, GAME_W, GAME_H);

        sound_init();
    }

    if(scene.new) scene.new(scene.data, rdr);

    SDL_Event e;
    while (1) {
        while (SDL_PollEvent(&e)) {
            if(hotkey(e)) continue;
            switch(e.type) {
            case SDL_QUIT: goto quit;
            default: input_event(e);
            }
        }
        if(!pause && !scene.update(input_update(), scene.data))
            goto quit;

        if(!start_time) start_time = SDL_GetTicks();
        int next_tick = start_time + tick_count*tick_len;
        int last_draw;
        while((last_draw = SDL_GetTicks()) < next_tick) {
            double thru = 0.5 + (last_draw - next_tick)/tick_len;
            draw_bg();
            scene.draw(scene.data, rdr, thru);
            SDL_RenderPresent(rdr);
        }
        tick_count++;
    }
quit:

    if(is_root) {
        SDL_DestroyWindow(win);
        SDL_DestroyRenderer(rdr);
        SDL_Quit();
    }

    if(scene.free) scene.free(scene.data);
}
