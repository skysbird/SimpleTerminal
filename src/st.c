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

int running = 1;
char input_text[MAX_INPUT_LENGTH] = "skysbird.synology.me";
int cursor_position = strlen("skysbird.synology.me")+1;
// char input_text[MAX_INPUT_LENGTH] = "";
// int cursor_position = 0;

int show_cursor = 1;
Uint32 last_cursor_toggle = 0;

SDL_Surface *screen;
TTF_Font *font = NULL;


#include <ctype.h>


#define MAX_LOG_LINES 10
#define LOG_LINE_WIDTH 50  // 每行最多字符
char log_lines[MAX_LOG_LINES][LOG_LINE_WIDTH + 1];
int log_line_count = 0;

void draw_moonlight_output(const char *text) {
    // 清除日志区域，确保没有残留
    SDL_Rect output_box = {50, 250, 720, 200};
    SDL_FillRect(screen, &output_box, SDL_MapRGB(screen->format, 0, 0, 0));

    // 处理文本换行
    char *line = strtok(text, "\n");
    while (line != NULL) {
        if (log_line_count < MAX_LOG_LINES) {
            strncpy(log_lines[log_line_count], line, LOG_LINE_WIDTH);
            log_lines[log_line_count][LOG_LINE_WIDTH] = '\0';
            log_line_count++;
        } else {
            for (int i = 1; i < MAX_LOG_LINES; i++) {
                strncpy(log_lines[i - 1], log_lines[i], LOG_LINE_WIDTH);
            }
            strncpy(log_lines[MAX_LOG_LINES - 1], line, LOG_LINE_WIDTH);
        }
        line = strtok(NULL, "\n");
    }

    // 重新绘制日志
    SDL_Color text_color = {255, 255, 255};
    for (int i = 0; i < log_line_count-1; i++) {
        draw_text(log_lines[i], 55, 260 + (i * 20), text_color);
    }

    SDL_Flip(screen);
}





void reset_sdl_input() {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_InitSubSystem(SDL_INIT_VIDEO);
}

void adjust_volume(int volume_change) {
    char command[128];
    if (volume_change > 0) {
        snprintf(command, sizeof(command), "amixer set 'lineout volume' %d+", volume_change);
    } else {
        snprintf(command, sizeof(command), "amixer set 'lineout volume' %d-", -volume_change);
    }    
    system(command);
    printf("Volume changed.\n");
    printf(command);
    printf("\n");
}


void start_moonlight_streaming() {
    if (cursor_position == 0) return; // 如果输入为空，则不启动

    printf("Starting Moonlight stream for: %s\n", input_text);

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) { // 子进程
        close(pipe_fd[0]); // 关闭子进程的 pipe 读端
        dup2(pipe_fd[1], STDOUT_FILENO); // 重定向 stdout
        dup2(pipe_fd[1], STDERR_FILENO); // 重定向 stderr
        close(pipe_fd[1]); // 关闭子进程的写端

        setsid();
        execl("/usr/bin/moonlight", "moonlight", "stream", "-width", "720", "-height", "720",
              "-platform", "sdl", "-mapping", "/mnt/vendor/deep/ppsspp/assets/gamecontrollerdb.txt",
              "-app", "Steam", "-windowed", "-quitappafter", input_text, NULL);

        perror("execl failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // 父进程
        close(pipe_fd[1]); // 关闭父进程的 pipe 写端

        char buffer[256];
        int bytes_read;
        while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer); // 终端输出
            draw_moonlight_output(buffer); // 渲染到 SDL 界面
        }

        close(pipe_fd[0]); // 关闭 pipe 读端
        waitpid(pid, NULL, 0); // 等待 Moonlight 退出
        printf("Moonlight exited, now returning to SDL window.\n");

        //SDL_Quit();
        //SDL_Init(SDL_INIT_VIDEO);
    } else {
        perror("fork failed");
    }
}

void start_moonlight_streaming2() {
    if (cursor_position == 0) return; // 如果输入为空，则不启动

    printf("Starting Moonlight stream for: %s\n", input_text);
    // SDL_WM_GrabInput(SDL_GRAB_OFF);
    // SDL_ShowCursor(SDL_ENABLE);
    // SDL_QuitSubSystem(SDL_INIT_VIDEO);  // 释放 SDL 持有的键盘/鼠标输入


    pid_t pid = fork();
if (pid == 0) { // 子进程
    setsid();
    execl("/usr/bin/moonlight", "moonlight", "stream", "-width", "720", "-height", "720", "-platform", "sdl", "-mapping", "/mnt/vendor/deep/ppsspp/assets/gamecontrollerdb.txt","-app", "Steam", "-windowed","-quitappafter", input_text, NULL);

    perror("execl failed");
    exit(EXIT_FAILURE);
    // 构建命令字符串
    // char command[1024];
    // snprintf(command, sizeof(command),
    //          "nohup /usr/bin/moonlight stream -width 720 -height 720 -platform sdl -mapping /mnt/vendor/deep/ppsspp/assets/gamecontrollerdb.txt -app Steam -windowed \"%s\" &",
    //          input_text);

    // // 执行 system 命令
    // system(command);

    // printf("Moonlight started.\n");

} else if (pid > 0) { // 父进程
    printf("Moonlight started with PID: %d\n", pid);
    int status;
    waitpid(pid, &status, 0); // 等待 Moonlight 退出
    printf("Moonlight exited, now returning to SDL window.\n");

//    reset_sdl_input();
// 重新获取 SDL 输入焦点
        // SDL_WM_GrabInput(SDL_GRAB_ON);
        // SDL_ShowCursor(SDL_DISABLE);
        // SDL_InitSubSystem(SDL_INIT_VIDEO);

        // 重新初始化 SDL 窗口（可选）
        SDL_Quit();
        SDL_Init(SDL_INIT_VIDEO);

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
    SDL_Rect input_box = {150, 200, 420, 50}; // 调整输入框位置，使其上移
    SDL_FillRect(screen, &input_box, SDL_MapRGB(screen->format, 255, 255, 255));

    SDL_Color text_color = {0, 0, 0};
    char display_text[MAX_INPUT_LENGTH + 20];
    snprintf(display_text, sizeof(display_text), "Domain(IP): %s%s", input_text, show_cursor ? "|" : "");

    draw_text(display_text, 155, 215, text_color);

    draw_keyboard(screen);
    SDL_Flip(screen);
}

int menu_pressed = 0;
int start_pressed = 0;

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

	if (ev->code == 311) {
		start_pressed = 1;
	}

	if (ev->code == 312) {
		menu_pressed = 1;
	}

        if (menu_pressed && start_pressed) {
            printf("Menu + Start pressed, exiting application.\n");
            running = 0;
        }

        
        if (ev->code == 114) {
            //down
            adjust_volume(-1);
        }

        if (ev->code == 115) {
            //up
            adjust_volume(1);
        }
        
    } 

    if (ev->type == EV_KEY && ev->value == 0) {
	    menu_pressed = 0;
	    start_pressed = 0;
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
