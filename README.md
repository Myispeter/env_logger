# IoT Environment Logger 🌿

基于 C 和 Python 的环境监控系统。

## 功能
- **collector.c**: C 语言编写的数据采集器，负责读取传感器并控制风扇。
- **app.py**: Flask 后端，提供数据 API 和远程控制接口。
- **SQLite3**: 存储历史温湿度数据。

## 运行方法
1. 启动硬件模拟：`python3 mock_hardware.py`
2. 编译并运行采集器：`gcc collector.c -o collector -lsqlite3 && ./collector`
3. 启动网页端：`python3 app.py`
