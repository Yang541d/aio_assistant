# service.py
import os
import httpx
from typing import Optional, List, Dict, Any

AMINER_API = "https://searchtest.aminer.cn/aminer-search/search/personV2"
def _aminer_search_api(name: str, organization: Optional[str]) -> tuple[str, dict, dict, str, dict | None]:
    """把浏览器抓到的网络包转成可复用的请求配置。"""
    # 环境变量读取 token（不要把 token 写死进代码/仓库）
    token = os.getenv("AMINER_TOKEN", "").strip()
    if not token:
        raise RuntimeError("缺少 AMINER_TOKEN 环境变量（Authorization JWT）")

    headers = {
        "Accept": "application/json, text/plain, */*",
        "Content-Type": "application/json",
        "Origin": "https://www.aminer.cn",
        "Referer": "https://www.aminer.cn/",
        "User-Agent": "Mozilla/5.0",  # 简化即可；若遇 403 再补完整 UA 和 sec-* 头
        "Authorization": token,
    }

    # 构造 body：只放我们需要的最小字段；分页先固定第一页 20 条
    searchKeyWordList = []
    if name:
        searchKeyWordList.append({
            "advanced": True,
            "keyword": name,
            "operate": "0",
            "wordType": 4,          # 4: 人名
            "segmentationWord": "true",
            "needTranslate": True
        })
    if organization:
        searchKeyWordList.append({
            "advanced": True,
            "keyword": organization,
            "operate": "0",
            "wordType": 5,          # 5: 机构
            "segmentationWord": "true",
            "needTranslate": True
        })

    body = {
        "query": "",
        "needDetails": True,
        "page": 0,
        "size": 20,
        "aggregations": [
            {"field": "h_index", "rangeList": [
                {"from": 0, "to": 10}, {"from": 10, "to": 20}, {"from": 20, "to": 30},
                {"from": 30, "to": 40}, {"from": 40, "to": 50}, {"from": 50, "to": 60},
                {"from": 60, "to": 99999}
            ], "size": 0, "type": "range"},
            {"field": "lang", "size": 10, "type": "terms"},
            {"field": "nation", "size": 10, "type": "terms"},
            {"field": "gender", "size": 10, "type": "terms"},
            {"field": "contact.position", "size": 20, "type": "terms"},
            {"field": "org_id", "size": 200, "type": "terms"}
        ],
        "filters": [],
        "searchKeyWordList": searchKeyWordList,
        "usingSemanticRetrieval": True
    }

    url = AMINER_API
    params: dict = {}
    method = "POST"
    return url, params, headers, method, body


class ScraperService:
    DEFAULT_AVATAR_URL = "https://static.aminer.cn/default/default.jpg"

    def crawl_list_fast(self, name: str, organization: Optional[str] = None) -> List[Dict[str, Any]]:
        # 1) 正确取到请求配置
        url, params, headers, method, body = _aminer_search_api(name, organization)

        # 2) 先发请求拿 JSON
        with httpx.Client(timeout=20) as client:
            r = client.post(url, params=params, headers=headers, json=body)
            r.raise_for_status()
            resp = r.json()

        # 3) 安全地取到 hitList
        data_layer = resp.get("data") if isinstance(resp, dict) else None
        hit_list = data_layer.get("hitList") if isinstance(data_layer, dict) else []
        if not isinstance(hit_list, list):
            return [{"count": 0, "items": []}]

        people = []
        for item in hit_list:
            # avatar 可能是完整URL或相对路径，做下兜底
            avatar = item.get("avatar")
            if avatar and avatar.startswith("http"):
                avatar_url = avatar
            elif avatar:
                avatar_url = f"https://static.aminer.cn/upload/avatar/{avatar}"
            else:
                avatar_url = self.DEFAULT_AVATAR_URL  # 4) 正确引用类属性

            people.append({
                "id": item.get("id"),
                "name": item.get("nameZh") or item.get("name"),
                "organization": item.get("orgZh") or item.get("org"),
                "avatar_url": avatar_url,
                "profile_url": f"https://www.aminer.cn/profile/{item.get('id')}"
            })

        return [{"count": len(people), "items": people}]



