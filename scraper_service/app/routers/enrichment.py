from fastapi import APIRouter
from ..models.schemas import EnrichReq


router = APIRouter()

@router.post("/data")
def enrich_data(req: EnrichReq):
    # 先返回排队态，后续接后台任务队列
    return {"task_id": "placeholder", "status": "QUEUED"}
