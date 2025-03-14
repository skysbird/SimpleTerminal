#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include "keyboard.h"

#define EVENT_DEVICE "/dev/input/event1"
#define MAX_INPUT_LENGTH 256

char input_text[MAX_INPUT_LENGTH] = "skysbird.synology.me";
int cursor_position = strlen("skysbird.synology.me")+1;
// char input_text[MAX_INPUT_LENGTH] = "";
// int cursor_position = 0;

int show_cursor = 1;
Uint32 last_cursor_toggle = 0;

SDL_Surface *screen;
TTF_Font *font = NULL;


void reset_sdl_input() {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_InitSubSystem(SDL_INIT_VIDEO);
}

void start_moonlight_streaming() {
    if (cursor_position == 0) return; // 如果输入为空，则不启动

    printf("Starting Moonlight stream for: %s\n", input_text);

    pid_t pid = fork();
if (pid == 0) { // 子进程
    setsid();
    execl("/usr/bin/moonlight", "moonlight", "stream", "-width", "720", "-height", "720", "-platform", "sdl", "-app", "Steam", "-windowed", input_text, NULL);

    perror("execl failed");
    exit(EXIT_FAILURE);
} else if (pid > 0) { // 父进程
    printf("Moonlight started with PID: %d\n", pid);
    int status;
    waitpid(pid, &status, 0); // 等待 Moonlight 退出
    printf("Moonlight exited, now returning to SDL window.\n");

//    reset_sdl_input();

} else {
    perror("fork failed");
}


}

void draw_text(const char *text, int x, int y, SDL_Color color) {
    if (!font) return;

    SDL_Surface *text_surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!text_surface) {
        fprintf(stderr, "TTF_RenderUTF8_Blended failed: %s\n", TTF_GetError());
        return;
    }

    SDL_Rect text_location = {x, y, 0, 0};
    SDL_BlitSurface(text_surface, NULL, screen, &text_location);
    SDL_FreeSurface(text_surface);
}

void draw_input_box() {
    SDL_Rect input_box = {150, 300, 420, 50};
    SDL_FillRect(screen, &input_box, SDL_MapRGB(screen->format, 255, 255, 255));
    
    SDL_Color text_color = {0, 0, 0};
    char display_text[MAX_INPUT_LENGTH + 20];
    snprintf(display_text, sizeof(display_text), "Domain(IP): %s%s", input_text, show_cursor ? "|" : "");

    draw_text(display_text, 155, 315, text_color);
    
    draw_keyboard(screen);
    SDL_Flip(screen);
}

void handle_virtual_keyboard_input(SDLKey key) {
    if (key == SDLK_BACKSPACE && cursor_position > 0) {
        input_text[--cursor_position] = '\0';
    } else if (key == 311) {
        start_moonlight_streaming(); // 启动 Moonlight
    } else if (key!= SDLK_RETURN  && key != SDLK_UP && key != SDLK_DOWN && key != SDLK_LEFT && key != SDLK_RIGHT) {
        if (cursor_position < MAX_INPUT_LENGTH - 1) {
            input_text[cursor_position++] = (char)key;
            input_text[cursor_position] = '\0';
        }
    }
    draw_input_box();
}

#define BTN_SOUTH 304

void process_key_event(struct input_event *ev) {
    printf("Event Type: %d\n", ev->type);          // ????
    printf("Event Code: %d\n", ev->code);          // ??
    printf("Event Value: %d\n", ev->value);        
    SDL_Event sdl_event;

    //sdl_event.type = SDL_KEYDOWN;
    //sdl_event.key.keysym.sym = SDLK_1;
    //SDL_PushEvent(&sdl_event);

    if (ev->type == EV_KEY && ev->value == 1) { // Key Pressed
        if (ev->code == BTN_SOUTH) {
            // A 按键按下，转换为 SDL 事件
            sdl_event.type = SDL_KEYDOWN;
            sdl_event.key.keysym.sym = SDLK_RETURN;
            sdl_event.key.state = SDL_PRESSED;
            SDL_PushEvent(&sdl_event);
        }
        if (ev->code == 311 || ev->code == 312) {
            sdl_event.type = SDL_KEYDOWN;
            sdl_event.key.keysym.sym = ev->code;
            sdl_event.key.state = SDL_PRESSED;
            SDL_PushEvent(&sdl_event);
        }
        
    }

    if (ev->type == EV_ABS) {
        if (ev->code == ABS_HAT0Y) {
            // D-Pad 上下方向
            sdl_event.type = SDL_KEYDOWN;
	    sdl_event.key.state = SDL_PRESSED;
            if (ev->value == -1) { // 上
                sdl_event.key.keysym.sym = SDLK_UP;
            } else if (ev->value == 1) { // 下
                sdl_event.key.keysym.sym = SDLK_DOWN;
            } else { // 松开
                sdl_event.type = SDL_KEYUP;
                sdl_event.key.keysym.sym = SDLK_UP; // 重置方向键
            }
            SDL_PushEvent(&sdl_event);
        } else if (ev->code == ABS_HAT0X) {
            // D-Pad 左右方向
            sdl_event.type = SDL_KEYDOWN;
            sdl_event.key.state = SDL_PRESSED;

            if (ev->value == -1) { // 左
                sdl_event.key.keysym.sym = SDLK_LEFT;
            } else if (ev->value == 1) { // 右
                sdl_event.key.keysym.sym = SDLK_RIGHT;
            } else { // 松开
                sdl_event.type = SDL_KEYUP;
                sdl_event.key.keysym.sym = SDLK_LEFT; // 重置方向键
            }
            SDL_PushEvent(&sdl_event);
        }
    }

}

void *keyboard_thread(void *arg) {
    (void)arg;
    int fd = open(EVENT_DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open input device");
        return NULL;
    }
    struct input_event ev;
    while (read(fd, &ev, sizeof(struct input_event)) > 0) {
        process_key_event(&ev);
    }
    close(fd);
    return NULL;
}

int running = 1;

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

//    SDL_WM_SetCaption("Moonlight_Window", NULL);


    if (TTF_Init() < 0) {
        fprintf(stderr, "Unable to initialize SDL_ttf: %s\n", TTF_GetError());
        return EXIT_FAILURE;
    }

    font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
    if (!font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        return EXIT_FAILURE;
    }

    screen = SDL_SetVideoMode(720, 720, 16, SDL_SWSURFACE);
    if (!screen) {
        fprintf(stderr, "SDL video mode set failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    draw_input_box();

    pthread_t tid;
    pthread_create(&tid, NULL, keyboard_thread, NULL);

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.key.keysym.sym == 0) {
                continue;
            }            
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == 312) {
                    running = 0;
                }
                handle_virtual_keyboard_input(event.key.keysym.sym);
	        handle_keyboard_event(&event);
            }
        }
        Uint32 current_time = SDL_GetTicks();
        if (current_time > last_cursor_toggle + 500) {
            show_cursor = !show_cursor;
            last_cursor_toggle = current_time;
            draw_input_box();
        }
    }

    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
