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

char input_text[MAX_INPUT_LENGTH] = "";
int cursor_position = 0;
int show_cursor = 1;
Uint32 last_cursor_toggle = 0;

SDL_Surface *screen;
TTF_Font *font = NULL;

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
    } else if (key == SDLK_RETURN) {
        printf("Entered Address: %s\n", input_text);
    } else if (key != SDLK_UP && key != SDLK_DOWN && key != SDLK_LEFT && key != SDLK_RIGHT) {
        if (cursor_position < MAX_INPUT_LENGTH - 1) {
            input_text[cursor_position++] = (char)key;
            input_text[cursor_position] = '\0';
        }
    }
    draw_input_box();
}


void process_key_event(struct input_event *ev) {
	    printf("Event Type: %d\n", ev->type);          // ????
    printf("Event Code: %d\n", ev->code);          // ??
    printf("Event Value: %d\n", ev->value);        // ??????????
    //if (ev->type == EV_KEY && ev->value == 1) { // Key Pressed
    //    if (ev->code == KEY_ENTER) {
    //        printf("Entered Address: %s\n", input_text);
    //    } else if (ev->code == KEY_BACKSPACE && cursor_position > 0) {
    //        input_text[--cursor_position] = '\0';
    //    } else if (cursor_position < MAX_INPUT_LENGTH - 1) {
    //        input_text[cursor_position++] = (char)ev->code;
    //        input_text[cursor_position] = '\0';
    //    }
    //    draw_input_box();
    //}
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

    while (1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            }
            if (event.type == SDL_KEYDOWN) {
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
