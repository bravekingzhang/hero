# hero.py

import argparse
import converter
import server

# 设置命令行参数
parser = argparse.ArgumentParser(description='Hero Static Site Generator')
parser.add_argument('-s', '--serve', action='store_true',
                    help='Serve the site locally')
parser.add_argument('-d', '--deploy', action='store_true',
                    help='Deploy the static site')

# 解析命令行参数
args = parser.parse_args()

if args.serve:
    converter.deploy()
    # 启动本地服务器
    server.serve()

if args.deploy:
    # 生成和部署网站
    converter.deploy()
