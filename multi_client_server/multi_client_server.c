#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048
#define NAME_LEN 32

static unsigned int cli_count = 0;
static int uid = 0;

// Client structure
typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[NAME_LEN];
    int mute; //add new field, wether the client is muted
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_trim_lf(char* arr, int length) {
  for (int i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void print_client_addr(struct sockaddr_in addr) {
    printf("%d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

// Add clients to array
void queue_add(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Remove clients to array
void queue_remove(int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->uid == uid) {
                clients[i] = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

int find_client_sockfd_by_recipient_name(char *recipient_name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]->sockfd != 0 && strcmp(clients[i]->name, recipient_name) == 0) {
            return clients[i]->sockfd;
        }
    }
    return -1;
}


int find_client_sockfd_by_uid(int uid) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]->sockfd != 0 && clients[i]->uid == uid) {
            return clients[i]->sockfd;
        }
    }
    return -1;
}

client_t * find_client_by_mute_name(char *mute_name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]->sockfd != 0 && strcmp(clients[i]->name, mute_name) == 0) {
            return clients[i];
        }
    }
    return NULL;
}

client_t * find_client_by_uid(int uid) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]->sockfd != 0 && clients[i]->uid == uid) {
            return clients[i];
        }
    }
    return NULL;
}
// 移除客户端的函数
void remove_client(int uid) {
    for(int i = 0; i < MAX_CLIENTS; ++i) {
        if(clients[i]) {
            if(clients[i]->uid == uid) {
                clients[i] = NULL;
                break;
            }
        }
    }
}

// Send message to all clients except the sender
void send_message(char *s, int uid) {
    pthread_mutex_lock(&clients_mutex);

    // 打印一下 接收到的消息和 uid
    printf("server received: %s %d\n", s, uid);
    printf("is /msg command? %d\n", strncmp(s, "/msg ", 5) == 0);
    printf("is /mute command? %d\n", strncmp(s, "/mute ", 6) == 0);
    printf("is /unmute command? %d\n", strncmp(s, "/unmute ", 8) == 0);
    printf("is /kick command? %d\n", strncmp(s, "/kick ", 6) == 0);
    // 检查是否为私密消息   /msg userB  hello
    if (strncmp(s, "/msg ", 5) == 0) {
        char recipient_name[32];
        char *message;
        char buffer[BUFFER_SIZE + 32];

        // 解析消息
        message = s + 5; // 跳过 "/msg "
        sscanf(message, "%s", recipient_name);
        message = strstr(message, " ") + 1; // 跳过接收者名字

        // 查找接收者的套接字
        int recipient_sockfd = find_client_sockfd_by_recipient_name(recipient_name);
        if (recipient_sockfd != -1) {
            // 发送私密消息给接收者
            sprintf(buffer, "[Private] %s", message);
            send(recipient_sockfd, buffer, strlen(buffer), 0);
        } else {
            // 接收者不存在，发送错误消息给发送者
            sprintf(buffer, "User %s does not exist.\\n", recipient_name);
            send(find_client_sockfd_by_uid(uid), buffer, strlen(buffer), 0);
        }
    }// /mute userB
    else if (strncmp(s, "/mute ", 6) == 0  && uid == 0) {//add new feature mute，uid = 0表示是第一个用户，就是群主
        char mute_user[32];
        char *message;
        char buffer[BUFFER_SIZE + 32];

        // 解析消息
        message = s + 6; // 跳过 "/mute "
        sscanf(message, "%s", mute_user);
        //mute client
        client_t * to_be_mute = find_client_by_mute_name(mute_user);
        if (to_be_mute) {
            to_be_mute->mute = 1;
        }
    } //  /unmute userB
    else if(strncmp(s, "/unmute ", 8) == 0 && uid == 0){
        char unmute_user[32];
        char *message;
        char buffer[BUFFER_SIZE + 32];
        // 解析消息
        message = s + 8; // 跳过 "/unmute "
        sscanf(message, "%s", unmute_user);
        //unmute client
        client_t * to_be_unmute = find_client_by_mute_name(unmute_user);
        if (to_be_unmute) {
            to_be_unmute->mute = 0;
        }
    } // /kick userB
    else if (strncmp(s, "/kick ", 6) == 0 && uid == 0) {
        char kick_user[32];
        char *message;
        char buffer[BUFFER_SIZE + 32];
        // 解析消息
        message = s + 6; // 跳过 "/kick "
        sscanf(message, "%s", kick_user);
        //kick client
        client_t * to_be_kick = find_client_by_mute_name(kick_user);
        if (to_be_kick) {
            //断开与该客户端的 socket 连接,哈哈，比较暴力哈，踢出群聊
            close(to_be_kick->sockfd);
            remove_client(to_be_kick->uid); // 从客户端列表中移除
        }
    }
    else{
        // Send message to all clients ，if client is not muted
        client_t * client = find_client_by_uid(uid);
        if(client->mute == 1){
            // 告知 客户端他已经被屏蔽了
            char buffer[BUFFER_SIZE];
            sprintf(buffer, "Your message has been blocked.\\n");
            write(client->sockfd, buffer, strlen(buffer));
        }
        else{
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clients[i]) {
                    if (clients[i]->uid != uid) {
                        if (write(clients[i]->sockfd, s, strlen(s)) < 0) {
                            perror("ERROR: write to descriptor failed");
                            break;
                        }
                    }
                }
            }
        }

    }
    pthread_mutex_unlock(&clients_mutex);
}




// Handle all communication with the client
void *handle_client(void *arg) {
    char buff_out[BUFFER_SIZE];
    char name[NAME_LEN];
    int leave_flag = 0;

    cli_count++;
    client_t *cli = (client_t *)arg;

    // Name
    if (recv(cli->sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) < 2 || strlen(name) >= NAME_LEN-1) {
        printf("Enter the name correctly\n");
        leave_flag = 1;
    } else {
        strcpy(cli->name, name);
        sprintf(buff_out, "%s has joined\n", cli->name);
        printf("%s", buff_out);
        send_message(buff_out, cli->uid);
    }

    bzero(buff_out, BUFFER_SIZE);

    while (1) {
        if (leave_flag) {
            break;
        }

        int receive = recv(cli->sockfd, buff_out, BUFFER_SIZE, 0);
        if (receive > 0) {
            if (strlen(buff_out) > 0) {
                send_message(buff_out, cli->uid);

                str_trim_lf(buff_out, strlen(buff_out));
                printf("%s -> %s\n", buff_out, cli->name);
            }
        } else if (receive == 0 || strcmp(buff_out, "exit") == 0) {
            sprintf(buff_out, "%s has left\n", cli->name);
            printf("%s", buff_out);
            send_message(buff_out, cli->uid);
            leave_flag = 1;
        } else {
            printf("ERROR: -1\n");
            leave_flag = 1;
        }

        bzero(buff_out, BUFFER_SIZE);
    }

    close(cli->sockfd);
    queue_remove(cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
            printf("Usage: %s <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int port = atoi(argv[1]);
  int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  pthread_t tid;

  // Socket settings
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  // Signals
  signal(SIGPIPE, SIG_IGN);

  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
    perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
  }

  // Binding
  if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR: Bind failed");
    return EXIT_FAILURE;
  }

  // Listening
  if (listen(listenfd, 10) < 0) {
    perror("ERROR: Listen failed");
    return EXIT_FAILURE;
  }

  printf("<[ SERVER STARTED ]>\n");

  // Accept clients
  while (1) {
    socklen_t clilen = sizeof(cli_addr);
    connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

    // Check if max clients is reached
    if ((cli_count + 1) == MAX_CLIENTS) {
      printf("Max clients reached. Rejected: ");
      print_client_addr(cli_addr);
      printf(":%d\n", cli_addr.sin_port);
      close(connfd);
      continue;
    }

    // Client settings
    client_t *cli = (client_t *)malloc(sizeof(client_t));
    cli->address = cli_addr;
    cli->sockfd = connfd;
    cli->uid = uid++;

    // Add client to the queue and fork thread
    queue_add(cli);
    pthread_create(&tid, NULL, &handle_client, (void*)cli);

    // Reduce CPU usage
    sleep(1);
  }

  return EXIT_SUCCESS;
}