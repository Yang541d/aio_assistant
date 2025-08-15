from langchain_core.tools import tool
from typing import Dict, Any
from .transport import Transport

HOST, PORT = "127.0.0.1", 8080

def _normalize(resp: Dict[str, Any], type_: str) -> Dict[str, Any]:
    """把服务端的 {status,message} 或 {ok,type,data,error} 映射到统一结构。"""
    if isinstance(resp, dict) and "ok" in resp:
        return resp
    status = resp.get("status", "")
    if status in ("task_received", "ok", "success"):
        return {"ok": True, "type": type_, "data": resp, "error": None}
    return {"ok": False, "type": type_, "data": None,
            "error": {"code": status or "error", "message": resp.get("message")}}

@tool("index_document", return_direct=False)
def index_document(path: str) -> Dict[str, Any]:
    """提交单个文档的索引任务。参数: path (文件路径)"""
    t = Transport(HOST, PORT).connect()
    raw = t.call("index_document", {"path": path})
    return _normalize(raw, "index_document")

@tool("ping", return_direct=False)
def ping() -> Dict[str, Any]:
    """健康检查：向 C++ 服务发送 ping 指令。"""
    t = Transport(HOST, PORT).connect()
    raw = t.call("ping")
    return _normalize(raw, "ping")

@tool("stats", return_direct=False)
def stats() -> Dict[str, Any]:
    """获取服务器端统计信息。"""
    t = Transport(HOST, PORT).connect()
    raw = t.call("stats")
    return _normalize(raw, "stats")
