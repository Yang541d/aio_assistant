from fastapi import APIRouter
from ..models.schemas import SearchReq, PagedCandidates
from ..services.discovery_service import DiscoveryService


router = APIRouter()
svc = DiscoveryService()

@router.post("/scholars", response_model=PagedCandidates)
def search_scholars(req: SearchReq):
    return svc.search_scholars(req)
