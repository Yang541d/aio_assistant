from pydantic import BaseModel, AnyUrl, Field, HttpUrl
from typing import Optional, List
from typing_extensions import Literal
# /search/scholars
class SearchReq(BaseModel):
    name: str = Field(min_length=1)
    institution: Optional[str] = None
    page: int = 0
    size: int = 20

class CandidateItem(BaseModel):
    id: Optional[str] = None
    name: str
    institution: Optional[str] = None
    urlA: Optional[AnyUrl] = None
    avatar_url: Optional[AnyUrl] = None

class PagedCandidates(BaseModel):
    count: int
    items: List[CandidateItem]
    page: int
    size: int
    total: Optional[int] = None

# /enumerate/publications
class EnumReq(BaseModel):
    profile_url: AnyUrl
    page: int = 0
    size: int = 20
    all: bool = False # true 时自动翻页直到拿全

class PublicationItem(BaseModel):
    title: str
    year: Optional[int] = None
    id: Optional[str] = None
    urls: Optional[List[str]] = None 

class PagedPublications(BaseModel):
    count: int
    items: List[PublicationItem]
    page: int
    size: int
    total: Optional[int] = None

# /enrich/data
class EnrichReq(BaseModel):
    scholar_id: str
    publications: List[PublicationItem]

class DownloadReq(BaseModel):
    profile_url: HttpUrl
    ids: Optional[List[str]] = None
    all: bool = False
    max_items: int = 100
    prefer_pdf: bool = True                 # 新增：优先只保存 PDF
    save_html_fallback: bool = False        # 新增：拿不到 PDF 时是否保存 HTML
    dry_run: bool = True                    # 仍默认只预演，不真实下载

class DownloadItemResult(BaseModel):
    id: Optional[str] = None
    title: str
    urls: Optional[List[str]] = None
    chosen_url: Optional[str] = None
    saved_path: Optional[str] = None
    status: Literal["PLANNED","DOWNLOADED","FAILED","SKIPPED"] = "PLANNED"
    reason: Optional[str] = None

class DownloadResp(BaseModel):
    count: int
    saved: int
    failed: int
    items: List[DownloadItemResult]