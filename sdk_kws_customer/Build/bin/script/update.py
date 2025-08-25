import argparse
import requests

HOST = "https://cdn.bootcdn.net/ajax/libs/jquery/3.6.0/jquery.min.js"
SAVE_PATH = "./test_download_{version_name}.js"

def download_file(url, filename=None):
    if filename is None:
        filename = url.split('/')[-1]
    
    # 发送GET请求
    response = requests.get(url, stream=True)
    # 检查请求是否成功
    response.raise_for_status()
    
    # 以二进制写入模式打开文件
    with open(filename, 'wb') as f:
        for chunk in response.iter_content(chunk_size=8192):
            f.write(chunk)
    
    print(f"文件已下载: {filename}")

if __name__ == "__main__":
    # 创建解析器对象
    parser = argparse.ArgumentParser(description='本脚本用于下载更新SDK程序')
    subparsers = parser.add_subparsers(dest='command', help='可用的命令')

    download_parser = subparsers.add_parser('download', help='下载SDK程序')
    download_parser.add_argument('-v', '--download_version', required=False, help='下载指定版本，不指定则为最新')

    # 解析参数
    args = parser.parse_args()

    if args.command == 'download':
        # 下载测试
        version_name = (
            "latest"
            if not args.download_version else 
            args.download_version
        )
        download_file(
            url=HOST.format(version_name=version_name), 
            filename=SAVE_PATH.format(version_name=version_name),
        )
    else:
        parser.print_help()