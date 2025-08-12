# app/services/download_service.py

from typing import List, Optional
import os, json, hashlib, time
from urllib.parse import urlparse
from scraper_service import config
from ..models.schemas import DownloadReq, DownloadResp, DownloadItemResult
from .publications_service import PublicationsService
from ..clients.aminer_client import AminerClient
import httpx


def _safe_slug(s: str) -> str:
    return "".join(c if c.isalnum() or c in "-._" else "_" for c in s)[:100]


def _rewrite_to_pdf_prefer(url: str) -> str:
    """对常见站点把 HTML 链接改成 PDF 直链；改不动就原样返回"""
    try:
        u = urlparse(url)
        host = u.netloc.lower()
        path = u.path

        # MDPI: https://www.mdpi.com/... -> https://www.mdpi.com/.../pdf
        if "mdpi.com" in host and not path.rstrip("/").endswith("/pdf"):
            return url.rstrip("/") + "/pdf"

        # Springer: https://link.springer.com/10.1007/... -> /content/pdf/10.1007/....pdf
        if "link.springer.com" in host:
            p = path.lstrip("/")
            if p.startswith("10."):
                return f"https://link.springer.com/content/pdf/{p}.pdf"

        # arXiv: https://arxiv.org/abs/xxxx -> https://arxiv.org/pdf/xxxx.pdf
        if "arxiv.org" in host and ("/abs/" in path):
            return url.replace("/abs/", "/pdf/") + ".pdf"

        return url
    except Exception:
        return url


def _score_url(u: str) -> int:
    """给 URL 打分：优先直接 PDF、其次常见可下 PDF 的站点；避免 crossref/api/doi 中间跳"""
    try:
        parsed = urlparse(u)
        host = parsed.netloc.lower()
        path = parsed.path.lower()

        score = 0
        # 直链 PDF
        if path.endswith(".pdf") or "/pdf" in path:
            score += 50

        # 常见易下载站点
        if any(x in host for x in ["mdpi.com", "link.springer.com", "arxiv.org", "aclanthology.org"]):
            score += 30

        # 不太适合直接下的中间站点
        if any(x in host for x in ["api.crossref.org", "doi.org"]):
            score -= 20

        # 一些常见难点（需要登录）
        if any(x in host for x in ["sciencedirect.com"]):
            score -= 10

        return score
    except Exception:
        return 0


def _choose_best_url(urls: List[str], prefer_pdf: bool) -> Optional[str]:
    if not urls:
        return None
    # 先按得分排序
    sorted_urls = sorted(urls, key=_score_url, reverse=True)
    chosen = sorted_urls[0]
    # 如果偏好 PDF，试着把选中的链接改写成更像直链 PDF
    return _rewrite_to_pdf_prefer(chosen) if prefer_pdf else chosen


class DownloadService:
    def __init__(self):
        self.pub_svc = PublicationsService()
        self.aminer = AminerClient()

    def _download_one(
        self,
        url: str,
        out_dir: str,
        prefer_pdf: bool,
        save_html_fallback: bool,
        timeout: float = 20.0,
    ) -> tuple[str, str, int, Optional[str]]:
        """
        返回: (status, saved_path, size_bytes, reason)
        status: DOWNLOADED / SKIPPED / FAILED
        """
        os.makedirs(out_dir, exist_ok=True)

        # 如果偏好 PDF，先尝试把 URL 改写成更像 PDF 的直链
        request_url = _rewrite_to_pdf_prefer(url) if prefer_pdf else url

        try:
            with httpx.Client(follow_redirects=True, timeout=timeout,
                              headers={"User-Agent": "Mozilla/5.0"}) as client:
                # HEAD 试探（有些站点不支持，失败就忽略）
                looks_like_pdf = request_url.lower().endswith(".pdf")
                try:
                    hr = client.head(request_url)
                    ctype_head = (hr.headers.get("Content-Type") or "").lower()
                    if "application/pdf" in ctype_head:
                        looks_like_pdf = True
                except Exception:
                    pass

                # 正确的 httpx 流式下载写法
                with client.stream("GET", request_url) as r:
                    final_url = str(r.url)
                    ctype = (r.headers.get("Content-Type") or "").lower()
                    is_pdf = ("application/pdf" in ctype) or looks_like_pdf

                    # 非 PDF 且不保存 HTML 兜底 -> 跳过
                    if (not is_pdf) and (not save_html_fallback):
                        return ("SKIPPED", "", 0, f"non-pdf content-type: {ctype or 'unknown'}")

                    # 选择文件名
                    fname = "paper.pdf" if is_pdf else "page.html"
                    fpath = os.path.join(out_dir, fname)

                    h = hashlib.sha256()
                    size = 0
                    with open(fpath, "wb") as f:
                        for chunk in r.iter_bytes():
                            if not chunk:
                                continue
                            f.write(chunk)
                            h.update(chunk)
                            size += len(chunk)

            # 落地 meta.json
            meta = {
                "source_url": url,
                "request_url": request_url,
                "final_url": final_url,
                "content_type": ("application/pdf" if is_pdf else (ctype or "text/html")),
                "saved_file": fname,
                "size_bytes": size,
                "sha256": h.hexdigest(),
            }
            with open(os.path.join(out_dir, "meta.json"), "w", encoding="utf-8") as mf:
                json.dump(meta, mf, ensure_ascii=False, indent=2)

            return ("DOWNLOADED", fpath, size, None)

        except httpx.HTTPError as e:
            return ("FAILED", "", 0, f"http error: {e}")
        except Exception as e:
            return ("FAILED", "", 0, f"error: {e}")

    def plan_or_download(self, req: DownloadReq) -> DownloadResp:
        # 1) enumerate 清单（最多取 max_items，但每页不超过 20）
        enum = self.pub_svc.enumerate_publications(
            type("EnumReqShim", (object,), {
                "profile_url": req.profile_url,
                "page": 0,
                "size": min(20, req.max_items),
                "all": req.all
            })()
        )

        # 2) 过滤 ids / 截断
        items = enum.items
        if req.ids:
            keep = set(req.ids)
            items = [p for p in items if (p.id or "") in keep]
        items = items[: req.max_items]

        # person_id 用来组织存储目录
        person_id = self.aminer.extract_person_id(str(req.profile_url)) or "unknown_person"
        base_dir = os.path.join(config.DATA_DIR, _safe_slug(person_id))
        os.makedirs(base_dir, exist_ok=True)

        planned: List[DownloadItemResult] = []
        saved = 0
        failed = 0

        for p in items:
            urls = p.urls or []
            chosen = _choose_best_url(urls, req.prefer_pdf) if urls else None

            # dry-run：只列计划或无 URL
            if req.dry_run or not chosen:
                planned.append(DownloadItemResult(
                    id=p.id,
                    title=p.title,
                    urls=urls or None,
                    chosen_url=chosen,
                    status="PLANNED" if req.dry_run else "SKIPPED",
                    reason="dry_run preview" if req.dry_run else ("no url" if not chosen else "not downloaded"),
                ))
                continue

            # 真下载（串行 + 轻限速）
            out_dir = os.path.join(base_dir, _safe_slug(p.id or p.title))
            status, saved_path, size_bytes, reason = self._download_one(
                url=chosen,
                out_dir=out_dir,
                prefer_pdf=req.prefer_pdf,
                save_html_fallback=req.save_html_fallback,
            )
            if status == "DOWNLOADED":
                saved += 1
            elif status == "FAILED":
                failed += 1

            planned.append(DownloadItemResult(
                id=p.id,
                title=p.title,
                urls=urls or None,
                chosen_url=chosen,
                saved_path=saved_path or None,
                status=status,
                reason=reason
            ))

            time.sleep(0.2)  # 轻微限速，避免触发风控

        return DownloadResp(
            count=len(planned),
            saved=saved,
            failed=failed,
            items=planned
        )
