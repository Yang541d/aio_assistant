from ..models.schemas import EnumReq, PagedPublications, PublicationItem
from scraper_service import config
from ..clients.aminer_client import AminerClient

class PublicationsService:
    def __init__(self):
        self.client = AminerClient()

    def enumerate_publications(self, req: EnumReq) -> PagedPublications:
        # 提取 person_id
        person_id = self.client.extract_person_id(str(req.profile_url))
        if not person_id:
            return PagedPublications(count=0, items=[], page=req.page, size=req.size, total=0)

        if req.all:
            # 自动翻页拿全（每页不超过 MAX_PAGE_SIZE=20）
            all_items = []
            total_hint = None
            for page_chunk in self.client.iter_person_papers(
                person_id=person_id,
                start_page=0,
                size=min(max(1, req.size), config.MAX_PAGE_SIZE)
            ):
                total_hint = page_chunk.get("total", total_hint)
                
                for p in page_chunk.get("items", []):
                    urls = p.get("urls")
                    urls = urls if isinstance(urls, list) and len(urls) > 0 else None
                    all_items.append(PublicationItem(
                        id=p.get("id"),
                        title=p.get("title") or p.get("title_zh") or "",
                        year=p.get("year"),
                        urls=urls
                    ))
            return PagedPublications(
                count=len(all_items),
                items=all_items,
                page=0,
                size=len(all_items),   # 返回全量时，size 就是总返回数
                total=total_hint if total_hint is not None else len(all_items),
            )

        # 否则仅返回一页
        size = min(max(1, req.size), config.MAX_PAGE_SIZE)
        page = max(0, req.page)
        page_data = self.client.fetch_person_papers(person_id, page=page, size=size)
        items = [
            PublicationItem(id=p.get("id"),
                            title=p.get("title") or p.get("title_zh") or "",
                            year=p.get("year"),
                            urls=urls)
            for p in (page_data.get("items") or [])
        ]
        total = page_data.get("total", len(items))
        return PagedPublications(count=len(items), items=items, page=page, size=size, total=total)
