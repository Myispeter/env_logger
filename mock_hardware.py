import random
import time
import os
TEMP_PATH = "/tmp/virt_hw/temp_sensor"
HUMI_PATH = "/tmp/virt_hw/humi_sensor"

os.makedirs("/tmp/virt_hw",exist_ok=True)
print("Dual Virtual Sensors are running... Press Ctrl+C to stop.")

try:
    while True:
        # 模拟温度：20.0 到 30.0 之间随机波动
        current_temp=round(random.uniform(20.0,30.0),2)
        
        # 模拟湿度：40.0 到 60.0 之间随机波动
        current_humi=round(random.uniform(40.0,60.0),2)

        # 以“覆盖写入”方式打开文件，模拟寄存器更新
        with open(TEMP_PATH,"w") as f:
            f.write(str(current_temp))
        
        with open(HUMI_PATH, "w") as f:
            f.write(str(current_humi))
        
        print(f"Hardware Update: {current_temp} °C, {current_humi} %")
        time.sleep(1) # 每秒更新一次
except KeyboardInterrupt:
    print("\n Virtual Sensor stopped.")
