#ifndef MENU_H
#define MENU_H

#include <SDL/SDL.h>

#define HISTORY_SIZE 10
#define MAX_INPUT_LENGTH 256


extern char history[HISTORY_SIZE][MAX_INPUT_LENGTH];
extern int history_count;

void add_to_history(const char *new_entry);  // 添加历史记录
const char *show_history_menu(SDL_Surface *screen);  // 显示历史菜单

#endif

