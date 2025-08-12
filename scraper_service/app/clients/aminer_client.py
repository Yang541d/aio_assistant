from typing import Optional, Dict, Any, List, Iterable
import httpx
from scraper_service import config
import re
import time

class AminerClient:
    def _build_body(self, name: str, institution: Optional[str], page: int, size: int) -> Dict[str, Any]:
        # 归一化分页
        page = max(0, int(page))
        size = max(1, min(config.MAX_PAGE_SIZE, int(size)))

        searchKeyWordList: List[Dict[str, Any]] = []
        if name:
            searchKeyWordList.append({
                "advanced": True, "keyword": name, "operate": "0",
                "wordType": 4, "segmentationWord": "true", "needTranslate": True
            })
        if institution:
            searchKeyWordList.append({
                "advanced": True, "keyword": institution, "operate": "0",
                "wordType": 5, "segmentationWord": "true", "needTranslate": True
            })

        return {
            "query": "",
            "needDetails": True,
            "page": page,
            "size": size,
            "aggregations": [
                {"field": "h_index", "rangeList": [
                    {"from": 0, "to": 10}, {"from": 10, "to": 20}, {"from": 20, "to": 30},
                    {"from": 30, "to": 40}, {"from": 40, "to": 50}, {"from": 50, "to": 60},
                    {"from": 60, "to": 99999}], "size": 0, "type": "range"},
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

    def search(self, name: str, institution: Optional[str], page: int, size: int) -> Dict[str, Any]:
        if not config.AMINER_TOKEN:
            raise RuntimeError("缺少 AMINER_TOKEN 环境变量")

        body = self._build_body(name, institution, page, size)
        headers = {
            "Accept": "application/json, text/plain, */*",
            "Content-Type": "application/json",
            "Origin": "https://www.aminer.cn",
            "Referer": "https://www.aminer.cn/",
            "User-Agent": "Mozilla/5.0",
            "Authorization": config.AMINER_TOKEN,
        }

        with httpx.Client(timeout=20) as client:
            r = client.post(config.AMINER_SEARCH_API, headers=headers, json=body)
            r.raise_for_status()
            resp = r.json()

        data_layer = resp.get("data") if isinstance(resp, dict) else None
        hit_list = data_layer.get("hitList") if isinstance(data_layer, dict) else []
        total = data_layer.get("hitsTotal") if isinstance(data_layer, dict) else 0
        if not isinstance(hit_list, list):
            return {"count": 0, "items": [], "page": page, "size": size, "total": total}

        # 映射为统一候选结构
        items = []
        for p in hit_list:
            pid = p.get("id")
            # 头像：优先完整URL；否则按相对路径拼接；再否则默认头像
            avatar = p.get("avatar")
            if avatar and avatar.startswith("http"):
                avatar_url = avatar
            elif avatar:
                avatar_url = f"https://static.aminer.cn/upload/avatar/{avatar}"
            else:
                avatar_url = config.DEFAULT_AVATAR_URL

            items.append({
                "id": pid,
                "name": p.get("nameZh") or p.get("name"),
                "institution": p.get("orgZh") or p.get("org"),
                "urlA": f"https://www.aminer.cn/profile/{pid}" if pid else None,
                "avatar_url": avatar_url
            })

        return {"count": len(items), "items": items, "page": page, "size": size, "total": total}
    
    """与 AMiner 的 HTTP 交互"""

    # 从 profile_url 提取 person_id（备用，也可在 service 里提）
    _ID_RE = re.compile(r"/profile/([^/]+)")

    @staticmethod
    def extract_person_id(profile_url: str) -> Optional[str]:
        m = AminerClient._ID_RE.search(profile_url)
        return m.group(1) if m else None

    def _post_actions(self, actions: List[Dict[str, Any]]) -> Any:
        if not config.AMINER_TOKEN:
            raise RuntimeError("缺少 AMINER_TOKEN（.env）")

        headers = {
            "Accept": "application/json, text/plain, */*",
            "Content-Type": "application/json",
            "Origin": "https://www.aminer.cn",
            "Referer": "https://www.aminer.cn/",
            "User-Agent": "Mozilla/5.0",
            "Authorization": config.AMINER_TOKEN,
        }
        with httpx.Client(timeout=20) as client:
            r = client.post("https://apiv2.aminer.cn/n", headers=headers, json=actions)
            r.raise_for_status()
            return r.json()

    def fetch_person_papers(self, person_id: str, page: int = 0, size: int = 20) -> Dict[str, Any]:
        """取某页论文；统一返回 {'items': [...], 'total': N}，兼容 items/hitList & total/hitsTotal。"""
        size = max(1, min(size, config.MAX_PAGE_SIZE))
        payload = [{
            "action": "person.SearchPersonPaper",
            "parameters": {
                "person_id": person_id,
                "search_param": {
                    "needDetails": True,  # True 更稳定
                    "page": page,
                    "size": size,
                    "sort": [{"field": "year", "asc": False}],
                },
            },
        }]
        data = self._post_actions(payload)

        def _pack(inner: dict) -> Dict[str, Any]:
            items = inner.get("items") or inner.get("hitList") or []
            total = inner.get("total") or inner.get("hitsTotal")
            return {"items": items, "total": total}

        # 形态 A：list -> [0]['data']
        if isinstance(data, list) and data:
            return _pack(data[0].get("data") or {})

        # 形态 B：dict -> ['data'] 是 list -> [0]['data']
        if isinstance(data, dict):
            arr = data.get("data")
            if isinstance(arr, list) and arr:
                return _pack(arr[0].get("data") or {})

        return {"items": [], "total": 0}


    def iter_person_papers(self, person_id: str, start_page: int = 0, size: int = 20,
                        max_pages: int = 200, sleep_sec: float = 0.2) -> Iterable[Dict[str, Any]]:
        """自动翻页，逐页产出 {'items': [...], 'total': N, 'page': p}。遇到最后一页停止。"""
        page = max(0, start_page)
        size = max(1, min(size, config.MAX_PAGE_SIZE))
        for _ in range(max_pages):
            page_data = self.fetch_person_papers(person_id, page=page, size=size)
            items = page_data.get("items") or []
            total = page_data.get("total")
            yield {"items": items, "total": total, "page": page}
            if len(items) < size:
                break
            page += 1
            time.sleep(sleep_sec)  # 轻微限速，避免风控

