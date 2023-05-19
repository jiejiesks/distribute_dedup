#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include "define.h"
#include "rw_hash.h"

int totalthread = 0;
hash_table *fp_table = NULL;

void searchDedup(int num, struct FileInfo_table_client *fileInfo_client, struct DelArray *delarray, int *delnum)
{
    // 关闭文件
    for (int i = 0; i < num; i++)
    {
        // 将fileInfo和fileInfo_server对比
        struct FileInfo_table_server *fileInfo_tmp;
        fileInfo_tmp = hash_table_get(fp_table, fileInfo_client[i].md5);
        // 如果指纹在哈希表中存在那么fileInfo_tmp就不为NULL，说明该文件重复
        if (fileInfo_tmp != NULL)
        {
            printf("test:%s\n", fileInfo_tmp->filePath);
            if (!strcmp(fileInfo_client[i].dev_id, fileInfo_tmp->dev_id))
            {
                if (strcmp(fileInfo_client[i].filePath, fileInfo_tmp->filePath))
                {
                    // 代表两个文件均在本地，且不是同一份文件
                    // /mnt/hmdfs/100/account/device_view/local/files/xxx
                    // /mnt/hmdfs/100/account/merge_view/hard/xxx
                    strncpy(delarray[(*delnum)].filePath, fileInfo_tmp->filePath, 23);
                    strcat(delarray[(*delnum)].filePath, "merge_view/hard/");
                    strcat(delarray[(*delnum)].filePath, fileInfo_tmp->md5);
                    strcpy(delarray[(*delnum)].dedupfilepath, fileInfo_client[i].filePath);
                    strcpy(delarray[(*delnum)].islocal, "1");
                    (*delnum)++;
                    fileInfo_tmp->count++;
                }
            }
            else
            {
                // 代表两个文件不同时在本地
                strncpy(delarray[(*delnum)].filePath, fileInfo_tmp->filePath, 23);
                strcat(delarray[(*delnum)].filePath, "merge_view/hard/");
                strcat(delarray[(*delnum)].filePath, fileInfo_tmp->md5);
                strcpy(delarray[(*delnum)].dedupfilepath, fileInfo_client[i].filePath);
                strcpy(delarray[(*delnum)].islocal, "1");
                (*delnum)++;
                fileInfo_tmp->count++;
            }
        }
        else
        {
            // 加入指纹库 tmp
            struct FileInfo_table_server *fileInfo_table_server = (struct FileInfo_table_server *)malloc(sizeof(struct FileInfo_table_server));
            memset(fileInfo_table_server, 0, sizeof(struct FileInfo_table_server));
            strcpy(fileInfo_table_server->filePath, fileInfo_client[i].filePath);
            strcpy(fileInfo_table_server->md5, fileInfo_client[i].md5);
            strcpy(fileInfo_table_server->dev_id, fileInfo_client[i].dev_id);
            hash_table_put(fp_table, fileInfo_table_server->md5, fileInfo_table_server);
        }
    }
}

void *client_handler(void *arg)
{
    int cfd = *(int *)arg;
    int linknum = 0;
    int num = 0;
    int delnum = 0;
    struct FileInfo fileInfo[MAX_FILES];
    struct DelArray delarray[MAX_FILES];
    struct Hardlink_Info hard_info[MAX_SERVER_FILES];
    char recvBuf[1024] = {0};
    char sendBuf[1024] = {0};
    int len = 0;
    int flag = 0;
    while (1)
    {
        unsigned char binMAC[MAC_LENGTH];
        memset(recvBuf, 0, 1024);
        recv(cfd, recvBuf, sizeof(recvBuf), 0);
        memcpy(&num, recvBuf, sizeof(num));
        memset(recvBuf, 0, 1024);
        // 接收到num个Fileinfo结构体，后续通过malloc分配num个Fileinfo和fileInfo_client结构体
        struct FileInfo *fileInfo = (struct FileInfo *)malloc(num * sizeof(struct FileInfo));
        memset(fileInfo, 0, num * sizeof(struct FileInfo));
        struct FileInfo_table_client *fileInfo_client = (struct FileInfo_table_client *)malloc(num * sizeof(struct FileInfo_table_client));
        memset(fileInfo_client, 0, num * sizeof(struct FileInfo_table_client));

        recv(cfd, recvBuf, sizeof(recvBuf), 0);
        memcpy(binMAC, recvBuf, sizeof(binMAC));
        for (int i = 0; i < num; i++)
        {
            memset(recvBuf, 0, 1024);
            recv(cfd, recvBuf, sizeof(recvBuf), 0);
            memcpy(&fileInfo[i], recvBuf, sizeof(fileInfo[i]));
            strcpy(fileInfo_client[i].filePath, fileInfo[i].filePath);
            strcpy(fileInfo_client[i].md5, fileInfo[i].md5);
            strcpy(fileInfo_client[i].dev_id, binMAC);
            printf("FileFullPath: %s \nMD5: %s\ndev_id:%s\n", fileInfo_client[i].filePath, fileInfo_client[i].md5, fileInfo_client[i].dev_id);
        }
        // 查询指纹表
        searchDedup(num, fileInfo_client, delarray, &delnum);
        for (int i = 0; i < delnum; i++)
        {
            printf("path:%s \ndelpath:%s \nislocal:%s \n", delarray[i].filePath, delarray[i].dedupfilepath, delarray[i].islocal);
        }
        memset(sendBuf, 0, 1024);
        memcpy(sendBuf, &delnum, sizeof(delnum));
        send(cfd, sendBuf, sizeof(sendBuf), 0);
        for (int i = 0; i < delnum; i++)
        {
            memset(sendBuf, 0, 1024);
            memcpy(sendBuf, &delarray[i], sizeof(delarray[i]));
            send(cfd, sendBuf, sizeof(sendBuf), 0);
        }
        totalthread++;

        // 设置一个屏障，等待所有线程均执行到这一步
        while (1)
        {
            if (totalthread == TOTALTHREAD)
            {
                printf("we are at totalthread: %d\n", totalthread);
                // 遍历哈希表
                for (int i = 0; i < HASH_SIZE; i++)
                {
                    if (fp_table->buckets[i] != NULL)
                    {
                        hash_node *current = fp_table->buckets[i];
                        
                        while (current != NULL)
                        {
                            struct FileInfo_table_server *FileInfo_table_server = current->value;
                            if (!strcmp(FileInfo_table_server->dev_id, binMAC))
                            {
                                // source
                                strcat(hard_info[linknum].filePath, "/data/service/el2/100/hmdfs/account/files/");
                                strcat(hard_info[linknum].filePath, (FileInfo_table_server->filePath) + 47);
                                // hardlinkpath
                                strcat(hard_info[linknum].hardlinkpath, "/data/service/el2/100/hmdfs/account/hard/");
                                strcat(hard_info[linknum].hardlinkpath, FileInfo_table_server->md5);
                                // nlink
                                hard_info[linknum].count = FileInfo_table_server->count;
                                linknum++;
                            }
                            current = current->next;
                        }
                    }
                }

                memset(sendBuf, 0, 1024);
                memcpy(sendBuf, &linknum, sizeof(linknum));
                send(cfd, sendBuf, sizeof(sendBuf), 0);
                printf("send successfully\n");
                for (int i = 0; i < linknum; i++)
                {
                    memset(sendBuf, 0, 1024);
                    memcpy(sendBuf, &hard_info[i], sizeof(hard_info[i]));
                    send(cfd, sendBuf, sizeof(sendBuf), 0);
                }

                break;
            }
        }

        memset(recvBuf, 0, 1024);
        recv(cfd, recvBuf, sizeof(recvBuf), 0);
        memcpy(&flag, recvBuf, sizeof(flag));
        //释放malloc分配的空间
        free(fileInfo);
        free(fileInfo_client);
        break;
    }
}

int main()
{
    // 1.创建socket（用于监听的套接字）
    pthread_t threads[MAX_CLIENTS] = {0};
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket");
        exit(-1);
    }

    // 2.绑定
    struct sockaddr_in sadrr;
    sadrr.sin_family = AF_INET; // 确定协议
    // 主机字节序转为网络字节序，绑定本机IP，可以使用命令ifconfig来查询，本机的话也可以使用127.0.0.1
    //  inet_pton(AF_INET,"127.0.0.1",sadrr.sin_addr.s_addr);      //确定IP
    // 如果要做一个服务器的话可以这么写
    sadrr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0代表任意地址
    sadrr.sin_port = htons(PORT);       // 确定端口，并且转为网络字节序
    int ret = bind(lfd, (struct sockaddr *)&sadrr, sizeof(sadrr));
    if (ret == -1)
    {
        perror("bind");
        exit(-1);
    }

    // 3.监听
    ret = listen(lfd, 10); // 允许排队的个数10
    if (ret == -1)
    {
        perror("listen");
        exit(-1);
    }

    // 4.接收客户端连接
    struct sockaddr_in clientaddr;
    socklen_t len1 = sizeof(clientaddr);
    int thread_num = 0;
    //加载指纹表
    fp_table = load_hash_table(FINGERPRINT_TABLE);
    while (1)
    {
        if (thread_num == TOTALTHREAD)
        {
            break;
        }
        int cfd = accept(lfd, (struct sockaddr *)&clientaddr, &len1);
        if (cfd == -1)
        {
            perror("accept");
            exit(-1);
        }
        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, &cfd);
        // 将线程ID添加到数组中以供稍后清理
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (threads[i] == 0)
            {
                threads[i] = tid;
                break;
            }
        }
        thread_num++;
    }
    // 等待所有客户端连接的线程退出,而所有的子线程都会在客户端发送flag=1的消息后退出，代表客户端已经接收完所有的消息
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (threads[i] != 0)
        {
            pthread_join(threads[i], NULL);
        }
    }
    //服务结束，持久化指纹表，然后将内存中的指纹表销毁
    save_hash_table(fp_table,FINGERPRINT_TABLE);
    destroy_hash_table(fp_table);
    close(lfd);
    return 0;
}