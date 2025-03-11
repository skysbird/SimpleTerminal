#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <SDL/SDL.h>
#include "keyboard.h"

#define EVENT_DEVICE "/dev/input/event1"
#define MAX_INPUT_LENGTH 256

char input_text[MAX_INPUT_LENGTH] = "";
int cursor_position = 0;
int show_cursor = 1;
Uint32 last_cursor_toggle = 0;

SDL_Surface *screen;

void draw_input_box() {
    SDL_Rect input_box = {150, 300, 420, 50};
    SDL_FillRect(screen, &input_box, SDL_MapRGB(screen->format, 255, 255, 255));
    
    SDL_Color text_color = {0, 0, 0};
    char display_text[MAX_INPUT_LENGTH + 2];
    strcpy(display_text, input_text);
    
    if (show_cursor) {
        strcat(display_text, "|"); // 显示光标
    }
    
    draw_string(screen, display_text, 155, 315, SDL_MapRGB(screen->format, text_color.r, text_color.g, text_color.b));
    
    draw_keyboard(screen); // 重新确保绘制虚拟键盘
    SDL_Flip(screen);
}

void process_key_event(struct input_event *ev) {
    if (ev->type == EV_KEY && ev->value == 1) { // Key Pressed
        if (ev->code == KEY_ENTER) {
            printf("Entered Address: %s\n", input_text);
        } else if (ev->code == KEY_BACKSPACE && cursor_position > 0) {
            input_text[--cursor_position] = '\0';
        } else if (cursor_position < MAX_INPUT_LENGTH - 1) {
            input_text[cursor_position++] = (char)ev->code;
            input_text[cursor_position] = '\0';
        }
        draw_input_box();
    }
}

void handle_virtual_keyboard_input(SDLKey key) {
    if (key == SDLK_BACKSPACE && cursor_position > 0) { // 处理退格键
        input_text[--cursor_position] = '\0';
    } else if (key == SDLK_RETURN) { // 处理回车键
        printf("Entered Address: %s\n", input_text);
    } else if (cursor_position < MAX_INPUT_LENGTH - 1) {
        input_text[cursor_position++] = (char)key;
        input_text[cursor_position] = '\0';
    }
    draw_input_box();
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

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    screen = SDL_SetVideoMode(720, 720, 16, SDL_SWSURFACE);
    if (!screen) {
        fprintf(stderr, "SDL video mode set failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    init_keyboard(); // 初始化虚拟键盘
    draw_input_box(); // 初次绘制 UI 界面
    
    pthread_t tid;
    pthread_create(&tid, NULL, keyboard_thread, NULL);
    
    while (1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            }
            if (event.type == SDL_KEYDOWN) { // 处理 SDL 虚拟键盘输入
                handle_keyboard_event(&event);
                handle_virtual_keyboard_input(event.key.keysym.sym);
            }
        }
        
        Uint32 current_time = SDL_GetTicks();
        if (current_time > last_cursor_toggle + 500) { // 每 500ms 切换光标状态
            show_cursor = !show_cursor;
            last_cursor_toggle = current_time;
            draw_input_box();
        }
    }
    SDL_Quit();
    return 0;
}

