from fastapi import APIRouter
from ..models.schemas import EnumReq, PagedPublications
from ..services.publications_service import PublicationsService


router = APIRouter()
svc = PublicationsService()

@router.post("/publications", response_model=PagedPublications, response_model_exclude_none=True)
def enumerate_publications(req: EnumReq):
    return svc.enumerate_publications(req)
