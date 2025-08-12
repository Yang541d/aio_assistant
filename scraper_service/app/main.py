from fastapi import FastAPI
from .routers.discovery import router as discovery_router
from .routers.enumeration import router as enumeration_router
from .routers.enrichment import router as enrichment_router
from .routers.downloader import router as downloader_router

app = FastAPI(title="Scholar Scraper API")
app.include_router(discovery_router, prefix="/search", tags=["discovery"])
app.include_router(enumeration_router, prefix="/enumerate", tags=["enumeration"])
app.include_router(enrichment_router, prefix="/enrich", tags=["enrichment"])
app.include_router(downloader_router)