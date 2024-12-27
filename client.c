#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "def.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 2964

int sockfd;
pthread_t recv_thread;
int connected = 0;

void *receive_data(void *arg)
{
    while (connected)
    {
        Packet packet;
        int n = recv(sockfd, &packet, sizeof(packet), 0);
        if (n <= 0)
        {
            perror("Receive failed");
            break;
        }
        // 根据数据包类型处理响应
        if (packet.type == RESPONSE_OK)
        {
            printf("Server Response: %s\n", packet.data);
        }
        else if (packet.type == RESPONSE_ERROR)
        {
            printf("Error: %s\n", packet.data);
        }
        else if (packet.type == RESPONSE_FORWARD_MSG)
        {
            printf("Message from client %d: %s\n", packet.client_id, packet.data);
        }
    }
    return NULL;
}

void connect_server()
{
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    while (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Connection failed, retrying...\n");
        sleep(1); // 重试
    }

    connected = 1;
    printf("Connected to server.\n");

    // 创建接收线程
    pthread_create(&recv_thread, NULL, receive_data, NULL);
}

void disconnect_server()
{
    close(sockfd);
    connected = 0;
    pthread_cancel(recv_thread);
    printf("Disconnected from server.\n");
}

void send_packet(Packet *packet)
{
    send(sockfd, packet, sizeof(Packet), 0);
}

void menu()
{
    while (1)
    {
        printf("\n1. Connect\n2. Disconnect\n3. Get Time\n4. Get Name\n5. Get Client List\n6. Send Message\n7. Exit\n");
        int choice;
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            if (!connected)
            {
                connect_server();
            }
            else
            {
                printf("Already connected.\n");
            }
            break;
        case 2:
            if (connected)
            {
                disconnect_server();
            }
            else
            {
                printf("Not connected.\n");
            }
            break;
        case 3:
            if (connected)
            {
                Packet packet = {0};
                packet.type = REQUEST_TIME;
                send_packet(&packet);
            }
            else
            {
                printf("Not connected.\n");
            }
            break;
        case 4:
            if (connected)
            {
                Packet packet = {0};
                packet.type = REQUEST_NAME;
                send_packet(&packet);
            }
            else
            {
                printf("Not connected.\n");
            }
            break;
        case 5:
            if (connected)
            {
                Packet packet = {0};
                packet.type = REQUEST_LIST;
                send_packet(&packet);
            }
            else
            {
                printf("Not connected.\n");
            }
            break;
        case 6:
            if (connected)
            {
                int client_id;
                char message[MAX_DATA_LEN];
                printf("Enter client id: ");
                scanf("%d", &client_id);
                printf("Enter message: ");
                scanf(" %[^\n]", message);

                Packet packet = {0};
                packet.type = REQUEST_MSG;
                packet.client_id = client_id;
                strcpy(packet.data, message);
                send_packet(&packet);
            }
            else
            {
                printf("Not connected.\n");
            }
            break;
        case 7:
            if (connected)
            {
                disconnect_server();
            }
            printf("Exiting...\n");
            return;
        default:
            printf("Invalid choice.\n");
        }
    }
}

int main()
{
    menu();
    return 0;
}
