# converter.py

import os
import markdown
from jinja2 import Environment, FileSystemLoader

posts_dir = 'posts'
templates_dir = 'templates'
output_dir = 'output'

env = Environment(loader=FileSystemLoader(templates_dir))

# 解析Markdown文件并提取元数据


def parse_post(filename):
    with open(os.path.join(posts_dir, filename), 'r', encoding='utf-8') as file:
        lines = file.readlines()

    metadata = {}
    content = []
    in_metadata_section = False

    for line in lines:
        if line.strip() == '---':  # 简单的元数据标记
            in_metadata_section = not in_metadata_section
        elif in_metadata_section:
            key, value = line.strip().split(':', 1)
            metadata[key.strip()] = value.strip()
        else:
            content.append(line)

    metadata['content'] = markdown.markdown(''.join(content))
    return metadata

# 生成文章列表和分类


def generate_site_data():
    posts = []
    post_titles = []  # 用于在模板中生成文章列表
    categories = set()

    for filename in os.listdir(posts_dir):
        if filename.endswith('.md'):
            post_data = parse_post(filename)
            posts.append(post_data)
            # 生成文章标题和URL的简化列表
            post_titles.append({
                'title': post_data['title'],
                'url': post_data['title'].replace(' ', '_') + '.html'
            })
            if 'category' in post_data:
                categories.add(post_data['category'])

    return posts, post_titles, categories


def deploy():
    posts, post_titles, categories = generate_site_data()
    os.makedirs(output_dir, exist_ok=True)
    for post in posts:
        html = env.get_template('post.html').render(
            content=post['content'],
            posts=post_titles,  # 传递文章标题列表
            categories=list(categories)
        )
        output_filename = post['title'].replace(' ', '_') + '.html'
        with open(os.path.join(output_dir, output_filename), 'w', encoding='utf-8') as file:
            file.write(html)
