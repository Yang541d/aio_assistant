import re
from agent.tools import index_document, ping, stats

def handle_natural_language(text: str):
    """
    最简自然语言路由：
    - "索引|index <...pdf>" -> index_document
    - 包含 "ping|心跳|健康" -> ping
    - 包含 "stats|状态|统计" -> stats
    """
    # 1) 索引
    m = re.search(r"(?:索引|index)\s+([^\s]+\.pdf)", text, re.IGNORECASE)
    if m:
        path = m.group(1)
        print(f"[路由] 识别到索引任务: {path}")
        return index_document.run(path)

    # 2) ping
    if re.search(r"(?:\bping\b|心跳|健康)", text, re.IGNORECASE):
        print("[路由] 识别到 ping")
        return ping.invoke({})   # 无参数，用 invoke({})

    # 3) stats
    if re.search(r"(?:\bstats\b|状态|统计)", text, re.IGNORECASE):
        print("[路由] 识别到 stats")
        return stats.invoke({})  # 无参数，用 invoke({})

    # 未识别
    return {"ok": False, "type": "unknown", "data": None,
            "error": {"code": "unrecognized", "message": "无法理解你的请求"}}

if __name__ == "__main__":
    while True:
        try:
            user_input = input("你: ").strip()
        except (EOFError, KeyboardInterrupt):
            break
        if user_input.lower() in {"quit", "exit"}:
            break
        print("Agent:", handle_natural_language(user_input))
