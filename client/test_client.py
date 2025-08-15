import socket
import json
import struct

def send_message(sock, message_json):
    """编码并发送一条消息"""
    # 1. 将JSON对象序列化为UTF-8编码的字符串
    message_body = json.dumps(message_json).encode('utf-8')

    # 2. 获取消息体的长度
    body_length = len(message_body)

    # 3. 将长度打包成一个4字节的网络字节序（大端序）的二进制数据
    #    '>I' 表示大端序的无符号整数 (4字节)
    header = struct.pack('>I', body_length)

    # 4. 先发送消息头，再发送消息体
    print(f"客户端: 准备发送消息头 (长度: {body_length})")
    sock.sendall(header)
    print(f"客户端: 准备发送消息体: {message_json}")
    sock.sendall(message_body)

def receive_message(sock):
    """接收并解码一条消息"""
    # 1. 接收4字节的消息头
    header_data = sock.recv(4)
    if not header_data:
        return None

    # 2. 解包消息头，获取消息体长度
    body_length = struct.unpack('>I', header_data)[0]
    print(f"客户端: 收到消息头，期望的消息体长度: {body_length}")

    # 3. 接收指定长度的消息体
    #    需要循环接收，确保接收完整
    body_data = b''
    while len(body_data) < body_length:
        packet = sock.recv(body_length - len(body_data))
        if not packet:
            return None
        body_data += packet

    # 4. 将消息体解码为JSON对象
    message_json = json.loads(body_data.decode('utf-8'))
    print(f"客户端: 成功收到并解析响应JSON。")
    return message_json

def main():
    host = '127.0.0.1'
    port = 8080

    try:
        # 创建一个TCP socket并连接到服务器
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((host, port))
            print(f"客户端: 成功连接到服务器 {host}:{port}")

            # 准备要发送的 "index_document" 指令
            test_message = {
                "command": "index_document",
                "payload": {
                    "path": "data/一种面向开源异构数据的网络安全威胁情报挖掘算法.pdf"
                }
            }

            # 发送消息
            send_message(sock, test_message)

            # 接收响应
            response = receive_message(sock)

            if response:
                print("\n--- 服务器响应 ---")
                # 使用json.dumps美化打印
                print(json.dumps(response, indent=4, ensure_ascii=False))
                print("--------------------")

    except ConnectionRefusedError:
        print(f"[错误] 连接被拒绝。请确认C++服务器正在运行于 {host}:{port}")
    except Exception as e:
        print(f"[错误] 发生异常: {e}")

if __name__ == "__main__":
    main()