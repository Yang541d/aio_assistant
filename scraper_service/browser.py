# browser.py
import os
from contextlib import contextmanager
from selenium import webdriver
from selenium.webdriver.chrome.options import Options

def _build_options() -> Options:
    o = Options()
    # 环境变量控制是否无头：HEADLESS=1/0
    if os.getenv("HEADLESS", "1") == "1":
        o.add_argument("--headless=new")
    o.add_argument("--disable-gpu")
    o.add_argument("--no-sandbox")
    o.add_argument("--disable-dev-shm-usage")
    o.add_argument("--window-size=1366,900")
    # 降低被识别为自动化
    o.add_argument("--disable-blink-features=AutomationControlled")
    o.add_experimental_option("excludeSwitches", ["enable-automation"])
    o.add_experimental_option("useAutomationExtension", False)
    return o

@contextmanager
def chrome():
    opts = _build_options()
    driver = webdriver.Chrome(options=opts)  # Selenium Manager 自动找驱动
    driver.set_page_load_timeout(30)
    driver.implicitly_wait(2)
    # 去掉 webdriver 痕迹（部分站点有效）
    driver.execute_cdp_cmd("Page.addScriptToEvaluateOnNewDocument", {
        "source": "Object.defineProperty(navigator,'webdriver',{get:()=>undefined});"
    })
    try:
        yield driver
    finally:
        driver.quit()

def get_html(url: str) -> str:
    with chrome() as d:
        d.get(url)
        return d.page_source
