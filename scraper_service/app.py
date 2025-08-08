from flask import Flask, request, jsonify
import time
import json

# 创建一个Flask应用实例
app = Flask(__name__)

# 定义一个路由(URL)，我们的C++服务器将访问这个URL
# methods=['GET'] 表示这个接口接受HTTP GET请求
@app.route('/crawl', methods=['GET'])
def crawl_scholar():
    # 1. 从URL的查询参数中获取学者姓名
    #    例如: 访问 /crawl?name=JohnDoe, scholar_name就会是 "JohnDoe"
    scholar_name = request.args.get('name', 'default_scholar')

    print(f"--- Python服务：收到爬取任务，目标：'{scholar_name}' ---")

    # 2. 模拟一个耗时很长的爬虫任务
    print("--- Python服务：模拟爬虫工作中... (耗时5秒) ---")
    time.sleep(5)

    # 3. 模拟爬虫成功后拿到的数据
    result_data = {
        "scholar_name": scholar_name,
        "publications": [
            {"title": "A Study on Asynchronous C++", "year": 2023},
            {"title": "The Art of Database Integration", "year": 2024}
        ],
        "status": "success",
        "source": "simulated_python_scraper"
    }

    print(f"--- Python服务：爬取完成，准备返回数据 ---")

    # 4. 将Python字典转换为JSON格式，作为HTTP响应返回
    return jsonify(result_data)

# 脚本的入口
if __name__ == '__main__':
    # 启动Web服务器
    # host='0.0.0.0' 让服务器可以被外部访问（包括我们的C++程序）
    # port=5001 指定一个端口号
    app.run(host='0.0.0.0', port=5001, debug=True)
