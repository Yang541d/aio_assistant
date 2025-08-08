from fastapi import FastAPI
from pydantic import BaseModel, AnyUrl
from typing import Optional
from scraper_service.service import ScraperService

app = FastAPI(title="Scraper Service (step 2)")
svc = ScraperService()

class CrawlListReq(BaseModel):
    name: str
    organization: Optional[str] = None

class CrawlDetailsReq(BaseModel):
    profile_url: AnyUrl

@app.get("/ping")
def ping():
    return {"status": "ok", "message": "pong"}

@app.post("/crawl_list")
def crawl_list(req: CrawlListReq):
    data = svc.crawl_list(req.name, req.organization)
    return {"status": "success", "data": data}

@app.post("/crawl_details")
def crawl_details(req: CrawlDetailsReq):
    data = svc.crawl_details(str(req.profile_url))
    return {"status": "success", "data": data}

@app.post("/crawl_list_fast")
def crawl_list_fast(req: CrawlListReq):
    data = svc.crawl_list_fast(req.name, req.organization)
    return {"status": "success", "data": data}
