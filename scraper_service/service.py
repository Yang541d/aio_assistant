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
    # ……保留你已有的 crawl_list / crawl_details 不动……

    def crawl_list_fast(self, name: str, organization: Optional[str] = None) -> List[Dict[str, Any]]:
        url, params, headers, method, body = _aminer_search_api(name, organization)
        with httpx.Client(timeout=20) as client:
            r = client.post(url, params=params, headers=headers, json=body)
            r.raise_for_status()
            data = r.json()

        people = []
        data_layer = data.get("data") if isinstance(data, dict) else None
        hit_list = data_layer.get("hitList") if isinstance(data_layer, dict) else None
        if not isinstance(hit_list, list):
            # 兜底：返回结构预览，便于继续定位
            return [{
                "top_keys": list(data.keys()) if isinstance(data, dict) else "not-a-dict",
                "data_keys": list(data_layer.keys()) if isinstance(data_layer, dict) else "no-data-dict",
                "note": "hitList not found or not a list"
            }]

        def g(o, *path):
            cur = o
            for k in path:
                if isinstance(cur, dict) and k in cur:
                    cur = cur[k]
                else:
                    return None
            return cur

        def pick_name(p):
            return (
                g(p, "name")
                or g(p, "name_zh")
                or g(p, "nameZh")
                or g(p, "profile", "name")
                or g(p, "profile", "name_zh")
            )

        def pick_org(p):
            return (
                g(p, "org")
                or g(p, "affiliation")
                or g(p, "organization")
                or g(p, "profile", "org")
            )

        def pick_avatar(p):
            return (
                g(p, "avatar")
                or g(p, "avatar_url")
                or g(p, "avatarUrl")
                or g(p, "profile", "avatar")
            )

        def pick_id(p):
            return g(p, "id") or g(p, "pid") or g(p, "_id")

        for p in hit_list[:5]:  # 只取前 5 条，和页面一致
            pid = pick_id(p)
            profile_url = f"https://www.aminer.cn/profile/{pid}" if pid else None
            people.append({
                "id": pid,
                "name": pick_name(p),
                "organization": pick_org(p),
                "avatar_url": pick_avatar(p),
                "profile_url": profile_url,
                "raw_keys": list(p.keys()) if isinstance(p, dict) else None  # 便于我们继续精确化
            })

        return [{
            "count": len(hit_list),
            "items": people
        }]

