import json, socket, struct
from typing import Any, Dict, Optional

class Transport:
    """
    最小传输层：负责 TCP 连接 + 4B 大端长度前缀 + JSON 编解码。
    用法：
        t = Transport("127.0.0.1", 8080).connect()
        rsp = t.call("ping")
    """
    def __init__(self, host: str, port: int, timeout: float = 3.0) -> None:
        self.host, self.port, self.timeout = host, port, timeout
        self._sock: Optional[socket.socket] = None

    def connect(self) -> "Transport":
        s = socket.create_connection((self.host, self.port), timeout=self.timeout)
        s.settimeout(self.timeout)
        self._sock = s
        return self

    def _send_frame(self, obj: Dict[str, Any]) -> None:
        assert self._sock is not None, "call connect() first"
        data = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        hdr = struct.pack(">I", len(data))
        self._sock.sendall(hdr + data)

    def _recv_frame(self) -> Dict[str, Any]:
        assert self._sock is not None, "call connect() first"
        # 读 4 字节长度
        hdr = self._sock.recv(4)
        if len(hdr) != 4:
            raise ConnectionError("short header")
        (n,) = struct.unpack(">I", hdr)
        # 读 n 字节正文
        buf = bytearray()
        while len(buf) < n:
            chunk = self._sock.recv(n - len(buf))
            if not chunk:
                raise ConnectionError("short body")
            buf += chunk
        return json.loads(buf.decode("utf-8"))

    def call(self, command: str, payload: Optional[Dict[str, Any]] = None, req_id: Optional[str] = None) -> Dict[str, Any]:
        """发送一条 {command, payload, id?} 请求，返回响应 dict。"""
        req = {"command": command}
        if payload is not None:
            req["payload"] = payload
        if req_id is not None:
            req["id"] = req_id
        self._send_frame(req)
        return self._recv_frame()

