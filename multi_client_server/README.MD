# 一个简单的中心化聊天应用

基于 Linux api 实现的聊天应用，支持多人聊天，支持私信，支持屏蔽，支持踢下线

## 使用方式

#### 编译

```shell
clang multi_client_server.c -o multi_client_server -lpthread
clang client.c -o client -lpthread
```

#### 运行

启动服务端

```shell
./multi_client_server 6000
```

启动客户端

```shell
./client 127.0.0.1 6000
```