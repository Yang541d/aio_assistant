from ..models.schemas import SearchReq, PagedCandidates, CandidateItem
from ..clients.aminer_client import AminerClient
from scraper_service import config

class DiscoveryService:
    def __init__(self):
        self.client = AminerClient()

    def search_scholars(self, req: SearchReq) -> PagedCandidates:
        # 统一分页上限
        size = min(max(1, req.size), config.MAX_PAGE_SIZE)
        page = max(0, req.page)

        # 调用 AMiner 客户端
        result = self.client.search(req.name, req.institution, page, size)

        # 映射成响应模型
        items = [CandidateItem(**it) for it in result["items"]]
        return PagedCandidates(
            count=result["count"],
            items=items,
            page=result["page"],
            size=result["size"],
            total=result.get("total")
        )
