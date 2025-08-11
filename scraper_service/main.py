# main.py
from fastapi import FastAPI
from pydantic import BaseModel
from typing import Optional
from scraper_service.service import ScraperService

app = FastAPI(title="Scraper Service")
svc = ScraperService()

class CrawlListReq(BaseModel):
    name: str
    organization: Optional[str] = None
    page: int = 0          # 新增
    size: int = 20         # 新增（服务端会限制到 1..20）

@app.get("/ping")
def ping():
    return {"status": "ok", "message": "pong"}

# 保留：Selenium 版
@app.post("/crawl_list")
def crawl_list(req: CrawlListReq):
    data = svc.crawl_list(req.name, req.organization)   # 不传 page/size
    return {"status": "success", "data": data}

# 直连 AMiner 版（分页）
@app.post("/crawl_list_fast")
def crawl_list_fast(req: CrawlListReq):
    data = svc.crawl_list_fast(req.name, req.organization, req.page, req.size)
    return {"status": "success", "data": data}
