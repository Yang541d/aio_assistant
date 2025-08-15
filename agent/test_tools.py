from agent.tools import index_document

def main():
    files = [
        "data/一种面向开源异构数据的网络安全威胁情报挖掘算法.pdf",
        "data/融合自举与语义角色标注的威胁情报实体关系抽取方法.pdf",
    ]

    for f in files:
        print(f"=== 测试文件: {f} ===")
        result = index_document.run(f)
        print(result)
        print()

if __name__ == "__main__":
    main()
