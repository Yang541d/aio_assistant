import argparse
from agent.tools import index_document

def cmd_index(args):
    result = index_document.run(args.path)
    # 简洁输出（ok / error）
    ok = result.get("ok", False)
    if ok:
        data = result.get("data") or {}
        print("[OK]", data.get("message") or result)
    else:
        err = (result.get("error") or {}).get("message") or result
        print("[ERR]", err)

def build_parser():
    p = argparse.ArgumentParser(prog="agent", description="Minimal Agent CLI")
    sub = p.add_subparsers(dest="cmd", required=True)

    p_index = sub.add_parser("index", help="提交单个文档的索引任务")
    p_index.add_argument("path", help="PDF 文件路径")
    p_index.set_defaults(func=cmd_index)

    return p

def main():
    parser = build_parser()
    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()
