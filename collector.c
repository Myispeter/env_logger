#include <stdio.h>      // 标准输入输出库：用于 printf() 在终端打印温度、湿度和调试信息
#include <stdlib.h>     // 标准工具库：用于 atof() 将传感器读到的字符串转换成浮点数，以及 exit() 退出程序
#include <unistd.h>     // Unix 标准函数库：提供 sleep() 延时函数，以及 read()、close() 等系统调用
#include <string.h>     // 字符串处理库：用于 memset() 初始化缓冲区，防止读取数据时出现乱码
#include <fcntl.h>      // 文件控制定义：提供 open() 函数以及 O_RDONLY（只读）等文件打开模式的常量
#include <sqlite3.h>    // SQLite3 数据库库：提供操作数据库文件的所有 API（如打开、建表、插入数据）

//定义虚拟硬件的路径和数据库名称
#define WEB_CMD_PATH "/tmp/virt_hw/web_cmd"  // 定义一个宏常量 WEB_CMD_PATH，表示 Web 控制命令文件的路径，程序将从这个文件中读取控制命令来模拟 Web 控制
#define TEMP_PATH "/tmp/virt_hw/temp_sensor"  // 定义一个宏常量 TEMP_PATH，表示温度传感器数据文件的路径，程序将从这个文件中读取温度值
#define HUMI_PATH "/tmp/virt_hw/humi_sensor"  // 定义一个宏常量 HUMI_PATH，表示湿度传感器数据文件的路径，程序将从这个文件中读取湿度值
#define FAN_CMD_PATH "/tmp/virt_hw/fan_cmd"  // 定义一个宏常量 FAN_CMD_PATH，表示风扇控制命令文件的路径，程序将向这个文件写入控制命令来模拟风扇的开关
#define TEMP_THRESHOLD 30.0  // 定义一个宏常量 TEMP_THRESHOLD，表示温度阈值，当读取到的温度超过这个值时，程序会模拟打开风扇    
#define DB_NAME "env_data.db" // 定义一个宏常量 DB_NAME，表示 SQLite 数据库文件的名称，程序将使用这个数据库文件来存储传感器数据日志

float read_sensor_value(const char* path) {  // 读取传感器值的函数，参数是传感器文件的路径
    char buffer[16];  // 定义一个小缓冲区来存储从传感器文件中读取的数据，16字节足够存储一个温度或湿度值的字符串表示（包括小数点和换行符）                       
    
    int fd = open(path, O_RDONLY);  //open()调用，fd为文件描述符，O_RDONLY表示以只读方式打开文件
    if (fd < 0) {
        return -1.0f; 
    }

  
    memset(buffer, 0, sizeof(buffer));  // 读取前先清空缓冲区，避免上次读取的残留数据干扰当前结果。

    int bytes = read(fd, buffer, sizeof(buffer) - 1); // 从传感器文件中读取数据，bytes记录实际读取的字节数，sizeof(buffer) - 1确保留出一个字节给字符串结束符'\0'。
    

    close(fd); // 读取完成后关闭文件描述符，释放系统资源。

    if (bytes > 0) {
        return atof(buffer);         // 将读取到的字符串转换成浮点数并返回，atof()函数会自动处理字符串中的数字部分，忽略非数字字符（如换行符）。
    }
    
    return -1.0f; // 如果读取失败或没有数据，返回 -1.0f 作为错误标志，调用者可以根据这个值判断是否成功读取了传感器数据。
}
int main() {
    sqlite3 *db;  // 定义一个指向 SQLite 数据库连接的指针，后续会用它来操作数据库
    char *err_msg = 0;  // 定义一个指向错误消息的指针，sqlite3_exec()函数在执行 SQL 语句时如果发生错误会将错误信息存储在这个指针指向的内存中

    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db)); // 打开数据库失败时，打印错误信息并退出程序
        return 1;   // 返回非零值表示程序异常结束，1是一个常用的错误代码，表示一般错误。
    }

    // 创建表（升级版：增加 humidity 字段）
    char *sql = "CREATE TABLE IF NOT EXISTS sensor_logs("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                "temperature REAL, "
                "humidity REAL);"; // 这里新增了湿度列
    // 这段 SQL 语句创建了一个名为 sensor_logs 的表，包含以下字段：

    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);  //%s表示以字符串的格式打印错误信息
        sqlite3_free(err_msg);
    }

    printf("Level 2 Dual-Sensor Collector Started...\n");
    printf("Threshold:%.2f°C\n", TEMP_THRESHOLD);  // %.2f 表示以小数点后两位的格式打印温度阈值

    while (1) {
        float temp = read_sensor_value(TEMP_PATH);
        float humi = read_sensor_value(HUMI_PATH);
        
        // --- 新增：读取来自网页的指令 (0:强关, 1:强开, 2:自动) ---
        int web_cmd = (int)read_sensor_value(WEB_CMD_PATH);

        if (temp >= 0 && humi >= 0) {
            FILE *fan_fp = fopen(FAN_CMD_PATH, "w");
            if (fan_fp == NULL) {
                perror("Failed to open fan command file");  //perror()函数会根据当前的 errno 值打印出一个描述错误原因的消息，这里是打开风扇命令文件失败时的错误处理
            } 
            else {
                // --- 核心逻辑升级：三路开关 ---
                if (web_cmd == 1) {
                    // 1. 网页强制开启
                    fprintf(fan_fp, "1");
                    printf("\033[1;33m[WEB] Manual Override: Fan ON\033[0m\n");
                } 
                else if (web_cmd == 0) {
                    // 2. 网页强制关闭
                    fprintf(fan_fp, "0");
                    printf("\033[1;33m[WEB] Manual Override: Fan OFF\033[0m\n");
                } 
                else {
                    // 3. 自动模式 (web_cmd == 2 或文件不存在)
                    if (temp > TEMP_THRESHOLD) {
                        fprintf(fan_fp, "1");
                        printf("\033[1;31m[AUTO] Temp %.2f > %.1f! Fan ON\033[0m\n", temp, TEMP_THRESHOLD);
                    } 
                    else {
                        fprintf(fan_fp, "0");
                        printf("[AUTO] Temp OK. Fan OFF\n");
                    }
                }
                fclose(fan_fp);
            }

            // --- 原有的数据库插入代码保持不变 ---
            char insert_sql[256];
            sprintf(insert_sql, "INSERT INTO sensor_logs (temperature, humidity) VALUES(%.2f, %.2f);", temp, humi);
            sqlite3_exec(db, insert_sql, 0, 0, &err_msg);
            printf("[DB Saved] Temp: %.2f°C | Humi: %.2f%%\n", temp, humi);

        } else {
            printf("[Warning] Hardware sensors not ready.\n");
        }

        sleep(1); 
    }

    sqlite3_close(db);
    return 0;
}
//return           发生在哪里	          目的地 (返回给谁)	           意义
//read_sensor_value  内部	             返回给 main 函数	         汇报单次任务的结果。程序继续运行。
//main              函数内部	          返回给操作系统 (Ubuntu)  	   汇报整个店经营得怎么样。程序彻底结束。


//函数       开头字母含义,             目的地（数据去哪了？）              
//printf       (无).               屏幕 (Standard Output)
//sprintf    S (String).          内存中的字符串 (char array)
//fprintf    F (File).            磁盘上的文件 (FILE pointer)