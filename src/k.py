import time
import evdev
from evdev import UInput, ecodes as e
from Xlib import X, display

# 创建 uinput 设备
ui = UInput()

# 连接 Xvfb 显示
d = display.Display()
root = d.screen().root
root.change_attributes(event_mask=X.KeyPressMask | X.KeyReleaseMask)

# Xvfb Keycode 到 Linux evdev 键位映射
key_map = {
    111: e.KEY_UP,    # X Keycode for Up Arrow
    116: e.KEY_DOWN,  # X Keycode for Down Arrow
    113: e.KEY_LEFT,  # X Keycode for Left Arrow
    114: e.KEY_RIGHT, # X Keycode for Right Arrow
}

print("开始监听 Xvfb 输入，并转发到 /dev/input/event11")

while True:
    event = d.next_event()
    
    if event.type == X.KeyPress or event.type == X.KeyRelease:
        keycode = event.detail
        if keycode in key_map:
            evdev_key = key_map[keycode]
            state = 1 if event.type == X.KeyPress else 0
            ui.write(e.EV_KEY, evdev_key, state)
            ui.syn()

