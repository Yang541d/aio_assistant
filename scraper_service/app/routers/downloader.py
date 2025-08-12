from fastapi import APIRouter
from ..models.schemas import DownloadReq, DownloadResp
from ..services.download_service import DownloadService

router = APIRouter(prefix="/download", tags=["download"])

@router.post("/publications", response_model=DownloadResp, response_model_exclude_none=True)
def download_publications(req: DownloadReq):
    svc = DownloadService()
    return svc.plan_or_download(req)
