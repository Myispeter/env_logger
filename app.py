from flask import Flask, render_template, jsonify, request
import sqlite3
import pandas as pd
import os

app = Flask(__name__)
DB_NAME = "env_data.db"
# 网页指令文件路径：0=强制关，1=强制开，2=自动模式
WEB_CMD_PATH = "/tmp/virt_hw/web_cmd"

# 确保存放指令的目录存在
os.makedirs(os.path.dirname(WEB_CMD_PATH), exist_ok=True)

def get_latest_data():
    """从数据库读取最新的 20 条记录"""
    try:
        conn = sqlite3.connect(DB_NAME)
        # 我们按时间倒序取 20 条，然后再正序排列，这样图表才是从左往右画
        query = "SELECT timestamp, temperature, humidity FROM sensor_logs ORDER BY id DESC LIMIT 20"
        df = pd.read_sql_query(query, conn)
        conn.close()
        # 将数据翻转回正序（符合时间流向）
        return df.iloc[::-1].to_dict(orient='records')
    except Exception as e:
        print(f"Database error: {e}")
        return []

@app.route('/')
def index():
    """主页面"""
    return render_template('index.html')

@app.route('/api/data')
def api_data():
    """给 Chart.js 提供实时数据"""
    data = get_latest_data()
    return jsonify(data)

@app.route('/api/control')
def control_fan():
    """
    处理控制指令：
    state=1 -> 强制开启
    state=0 -> 强制关闭
    state=2 -> 恢复自动（由 C 程序阈值控制）
    """
    state = request.args.get('state')
    
    if state in ['0', '1', '2']:
        with open(WEB_CMD_PATH, "w") as f:
            f.write(state)
        
        mode_text = { "0": "Force OFF", "1": "Force ON", "2": "Auto Mode" }
        return jsonify({
            "status": "success", 
            "current_mode": mode_text[state]
        })
    
    return jsonify({"status": "error", "msg": "Invalid Command"}), 400

if __name__ == '__main__':
    # 初始化：默认设为“自动模式 (2)”
    if not os.path.exists(WEB_CMD_PATH):
        with open(WEB_CMD_PATH, "w") as f:
            f.write("2")
            
    app.run(host='0.0.0.0', port=5000, debug=True)