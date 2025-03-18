#include "menu.h"
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <string.h>
#include <stdlib.h>    // getenv()
#include <sys/stat.h>  // mkdir()
#include <sys/types.h> // mode_t

char HISTORY_FILE[MAX_INPUT_LENGTH];  // 存储历史文件路径

char history[HISTORY_SIZE][MAX_INPUT_LENGTH] = {""};  // 存储最近 10 条记录
int history_count = 0;
int selected_index = 0;  // 记录当前选择的索引

void setup_history_file() {
    const char *home = getenv("HOME");  // 获取 HOME 目录
    printf("HOME: %s\n", home);
    
    if (home) {
        snprintf(HISTORY_FILE, sizeof(HISTORY_FILE), "%s/.ml_gui/history.txt", home);

        // ✅ 自动创建 `~/.ml_gui` 目录
        char history_dir[MAX_INPUT_LENGTH];
        snprintf(history_dir, sizeof(history_dir), "%s/.ml_gui", home);

        struct stat st;
        if (stat(history_dir, &st) != 0) {  // 如果目录不存在
            if (mkdir(history_dir, 0700) == 0) {
                printf("Created directory: %s\n", history_dir);
            } else {
                perror("Failed to create ~/.ml_gui");
            }
        }
    } else {
        // 如果 HOME 变量不存在，回退到 /tmp
        strncpy(HISTORY_FILE, "/tmp/history.txt", sizeof(HISTORY_FILE) - 1);
    }
}

void load_history() {

    setup_history_file();  // ✅ 确保 HISTORY_FILE 正确，并创建目录

    FILE *file = fopen(HISTORY_FILE, "r");
    if (!file) return;  // 文件不存在，不报错

    history_count = 0;
    while (history_count < HISTORY_SIZE && fgets(history[history_count], MAX_INPUT_LENGTH, file)) {
        history[history_count][strcspn(history[history_count], "\n")] = '\0';  // 去掉换行符
        history_count++;
    }
    fclose(file);
}

void add_to_history(const char *new_entry) {
    if (history_count < HISTORY_SIZE) {
        strncpy(history[history_count], new_entry, MAX_INPUT_LENGTH - 1);
        history_count++;
    } else {
        // 滚动数组，移除最旧的一条
        for (int i = 1; i < HISTORY_SIZE; i++) {
            strncpy(history[i - 1], history[i], MAX_INPUT_LENGTH - 1);
        }
        strncpy(history[HISTORY_SIZE - 1], new_entry, MAX_INPUT_LENGTH - 1);
    }

    // ✅ 存入文件
    FILE *file = fopen(HISTORY_FILE, "w");
    if (!file) return;

    for (int i = 0; i < history_count; i++) {
        fprintf(file, "%s\n", history[i]);
    }
    fclose(file);
}


// 绘制菜单界面
void draw_menu(SDL_Surface *screen) {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0)); // 黑色背景

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
    if (!font) return;

    SDL_Color text_color = {255, 255, 255};  // 白色字体
    SDL_Color selected_color = {255, 255, 0};  // 选中项黄色

    for (int i = 0; i < history_count; i++) {
        SDL_Color color = (i == selected_index) ? selected_color : text_color;
        SDL_Surface *text_surface = TTF_RenderUTF8_Blended(font, history[i], color);
        
        SDL_Rect text_location = {100, 150 + i * 30, 0, 0}; // 显示在屏幕中心偏上
        SDL_BlitSurface(text_surface, NULL, screen, &text_location);
        SDL_FreeSurface(text_surface);
    }

    SDL_Flip(screen);
    TTF_CloseFont(font);
}

// 显示历史菜单，返回用户选择的记录
const char *show_history_menu(SDL_Surface *screen) {
    int menu_active = 1;
    SDL_Event event;

    while (menu_active) {
        draw_menu(screen); // 渲染菜单
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP) {
                    selected_index = (selected_index - 1 + history_count) % history_count;  // 上翻
                }
                if (event.key.keysym.sym == SDLK_DOWN) {
                    selected_index = (selected_index + 1) % history_count;  // 下翻
                }
                if (event.key.keysym.sym == SDLK_RETURN) {
                    menu_active = 0; // 选择完成
                }
                if (event.key.keysym.sym == SDLK_MENU) {
                    return NULL; // 取消选择
                }
            }
        }
        SDL_Delay(100);
    }

    // ✅ **清除菜单界面**
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));  // 黑色清屏
    SDL_Flip(screen);

    return history[selected_index];
}

