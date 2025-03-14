import uinput
import time

# 定义虚拟手柄按键和方向键
events = (
    uinput.BTN_A,  # A键
    uinput.ABS_HAT0X + (-1, 1, 0, 0),  # 左右方向键 (-1=左, 1=右, 0=中间)
    uinput.ABS_HAT0Y + (-1, 1, 0, 0),  # 上下方向键 (-1=上, 1=下, 0=中间)
)

# 创建虚拟 Gamepad 设备
with uinput.Device(events, name="VirtualGamepad") as device:
    print("Virtual Gamepad created. Press Ctrl+C to stop.")
#    time.sleep(10000000)

    ## 模拟按键序列
    actions = [
        (uinput.ABS_HAT0Y, 1, "DOWN"),  # 下
        (uinput.ABS_HAT0Y, 1, "DOWN"),  # 下
        (uinput.ABS_HAT0Y, 1, "DOWN"),  # 下
        (uinput.ABS_HAT0X, 1, "RIGHT"),  # 右
        (uinput.ABS_HAT0X, 1, "RIGHT"),  # 右
        (uinput.ABS_HAT0X, 1, "RIGHT"),  # 右
        (uinput.ABS_HAT0Y, 1, "UP"),  # 下
        (uinput.ABS_HAT0Y, 1, "LEFT"),  # 下
        ]
    #    (uinput.BTN_A, 1, "A"),  # A键按下
    #]

    while True:
        for event, value, name in actions:
            print(f"Pressing {name}")
            device.emit(event, value)  # 按下按键
            time.sleep(0.2)
            device.emit(event, 0)  # 松开按键
            time.sleep(0.5)
    
