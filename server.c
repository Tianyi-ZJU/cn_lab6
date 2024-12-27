#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "def.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define SERVER_PORT 2964

typedef struct
{
    int client_sock;
    struct sockaddr_in client_addr;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg)
{
    ClientInfo *client_info = (ClientInfo *)arg;
    int sock = client_info->client_sock;
    Packet packet;

    while (recv(sock, &packet, sizeof(packet), 0) > 0)
    {
        switch (packet.type)
        {
        case REQUEST_TIME:
            // 返回时间
            time_t t;
            struct tm *tm_info;
            time(&t);
            tm_info = localtime(&t);
            strftime(packet.data, MAX_DATA_LEN, "%Y-%m-%d %H:%M:%S", tm_info);
            packet.type = RESPONSE_OK;
            send(sock, &packet, sizeof(packet), 0);
            break;
        case REQUEST_NAME:
            // 返回服务器名称
            strcpy(packet.data, "MyServer");
            packet.type = RESPONSE_OK;
            send(sock, &packet, sizeof(packet), 0);
            break;
        case REQUEST_LIST:
            // 返回客户端列表
            packet.type = RESPONSE_OK;
            pthread_mutex_lock(&client_mutex);
            if (client_count == 0)
            {
                strcpy(packet.data, "No clients connected.");
            }
            else
            {
                for (int i = 0; i < client_count; i++)
                {
                    snprintf(packet.data + strlen(packet.data), MAX_DATA_LEN - strlen(packet.data),
                             "Client %d - IP: %s, Port: %d\n", i, inet_ntoa(clients[i].client_addr.sin_addr),
                             ntohs(clients[i].client_addr.sin_port));
                }
            }

            pthread_mutex_unlock(&client_mutex);
            send(sock, &packet, sizeof(packet), 0);
            break;
        case REQUEST_MSG:
            // 转发消息
            if (packet.client_id >= 0 && packet.client_id < client_count)
            {
                ClientInfo *receiver = &clients[packet.client_id];
                packet.type = RESPONSE_FORWARD_MSG;
                send(receiver->client_sock, &packet, sizeof(packet), 0);
                packet.type = RESPONSE_OK;
                send(sock, &packet, sizeof(packet), 0);
            }
            else
            {
                packet.type = RESPONSE_ERROR;
                strcpy(packet.data, "Invalid client ID");
                send(sock, &packet, sizeof(packet), 0);
            }
            break;
        default:
            break;
        }
    }

    close(sock);
    return NULL;
}

void server_init()
{
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_sock, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        exit(1);
    }

    printf("Server is listening on port %d...\n", SERVER_PORT);

    while (1)
    {
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0)
        {
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&client_mutex);
        clients[client_count].client_sock = client_sock;
        clients[client_count].client_addr = client_addr;
        client_count++;
        pthread_mutex_unlock(&client_mutex);

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, &clients[client_count - 1]);
    }

    close(server_sock);
}

int main()
{
    server_init();
    return 0;
}
