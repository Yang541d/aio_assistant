# scraper_service/config.py
import os
from dotenv import load_dotenv

# 加载 .env（放在 scraper_service/.env）
load_dotenv()

# 服务运行配置
API_HOST = os.getenv("API_HOST", "0.0.0.0")
API_PORT = int(os.getenv("API_PORT", "8000"))

MAX_PAGE_SIZE = int(os.getenv("MAX_PAGE_SIZE", "20"))

# 外部站点相关
AMINER_TOKEN = os.getenv("AMINER_TOKEN", "").strip()  # 不在这里抛错，客户端里再校验
AMINER_SEARCH_API = os.getenv(
    "AMINER_SEARCH_API",
    "https://searchtest.aminer.cn/aminer-search/search/personV2"
)
DEFAULT_AVATAR_URL = os.getenv(
    "DEFAULT_AVATAR_URL",
    "https://static.aminer.cn/default/default.jpg"
)

DATA_DIR = os.getenv("DATA_DIR", "./data")  # 下载文件保存路径