#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include "md5c.h"
#include "define.h"

struct FileInfo fileInfo[MAX_FILES];
struct DelArray delarray[MAX_FILES];
struct Hardlink_Info hard_info[MAX_SERVER_FILES];
int linknum;
int num;
int delnum;

// 判断硬链接是否存在，如果存在返回1，不存在返回0
int isFileExists(const char *path)
{
    struct stat st;
    int res = stat(path, &st);
    if (res == -1)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

// 扫描目录，并把文件内容哈希
void *scanDir(const char *dirPath)
{
    printf("jinrumulu\n");
    DIR *dir = NULL;
    struct dirent *entry;
    struct stat st;

    dir = opendir(dirPath);
    if (dir == NULL)
    {
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }
        char fileFullPath[128];
        snprintf(fileFullPath, 128, "%s/%s", dirPath, entry->d_name);
        if (entry->d_type & DT_DIR)
        {
            scanDir(fileFullPath);
        }
        else
        {
            snprintf(fileInfo[num].filePath, 128, "%s", fileFullPath);
            printf("%s\n", fileInfo[num].filePath);
            int fd;
            int ret;
            unsigned char md5_value[MD5_SIZE];
            fd = open(fileFullPath, O_RDONLY);
            MD5_CTX md5;
            MD5Init(&md5);
            unsigned char data[READ_DATA_SIZE];
            while (1)
            {
                ret = read(fd, data, READ_DATA_SIZE);
                if (-1 == ret)
                {
                    perror("read");
                    return -1;
                }

                MD5Update(&md5, data, ret);

                if (0 == ret || ret < READ_DATA_SIZE)
                {
                    break;
                }
            }

            close(fd);
            MD5Final(md5_value, &md5);
            for (int i = 0; i < 16; i++)
            {
                snprintf(fileInfo[num].md5 + i * 2, 2 + 1, "%02x", md5_value[i]);
            }
            fileInfo[num].md5[MD5_STR_LEN] = '\0'; // add end
            num++;
        }
    }
    closedir(dir);
    return NULL;
}

int main()
{
    const char *dir = "/mnt/hmdfs/100/account/device_view/local";
    num = 0;
    struct ifreq ifreq;
    unsigned char mac[MAC_LENGTH];
    int status;
    status = system("/data/fscryptctl add_key /data/service/el2/100/hmdfs/account/ < /data/crypttable");
    if (status != 0)
    {
        printf("add key success\n");
    }
    pthread_t thread;
    pthread_create(&thread, NULL, &scanDir, dir);
    pthread_join(thread, NULL);
    printf("%d\n", num);
    for (int i = 0; i < num; i++)
    {
        printf("FileFullPath: %s \nMD5: %s\n", fileInfo[i].filePath, fileInfo[i].md5);
    }

    // 建立socket连接
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        exit(-1);
    }
    strcpy(ifreq.ifr_name, "eth0");
    if (ioctl(fd, SIOCGIFHWADDR, &ifreq) < 0)
    {
        close(fd);
        perror("ioctl");
        return -1;
    }

    // 打印mac地址
    snprintf(mac, MAC_LENGTH, "%02X:%02X:%02X:%02X:%02X:%02X", (unsigned char)ifreq.ifr_hwaddr.sa_data[0], (unsigned char)ifreq.ifr_hwaddr.sa_data[1], (unsigned char)ifreq.ifr_hwaddr.sa_data[2], (unsigned char)ifreq.ifr_hwaddr.sa_data[3], (unsigned char)ifreq.ifr_hwaddr.sa_data[4], (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
    printf("binMAC:%s\n", mac);

    // 2. 连接服务器
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, IP_ADDRESS, &serveraddr.sin_addr.s_addr);
    serveraddr.sin_port = htons(PORT);
    int ret = connect(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (ret == -1)
    {
        perror("connect");
        exit(-1);
    }

    char recvBuf[1024] = {0}; // 发送
    char sendBuf[1024] = {0}; // 接收
    int len = 0;
    int flag = 1;
    // 3.通信
    while (1)
    {
        memset(sendBuf, 0, 1024);
        memcpy(sendBuf, &num, sizeof(num));
        send(fd, sendBuf, sizeof(sendBuf), 0);
        memset(sendBuf, 0, 1024);
        memcpy(sendBuf, mac, sizeof(mac));
        send(fd, sendBuf, sizeof(sendBuf), 0);
        for (int i = 0; i < num; i++)
        {
            memset(sendBuf, 0, 1024);
            memcpy(sendBuf, &fileInfo[i], sizeof(fileInfo[i]));
            send(fd, sendBuf, sizeof(sendBuf), 0);
        }
        // accept delpath table and save
        memset(recvBuf, 0, 1024);
        recv(fd, recvBuf, sizeof(recvBuf), 0);
        memcpy(&delnum, recvBuf, sizeof(delnum));
        for (int i = 0; i < delnum; i++)
        {
            memset(recvBuf, 0, 1024);
            recv(fd, recvBuf, sizeof(recvBuf), 0);
            memcpy(&delarray[i], recvBuf, sizeof(delarray[i]));
            printf("path:%s \ndelpath:%s \nislocal:%s \n", delarray[i].filePath, delarray[i].dedupfilepath, delarray[i].islocal);
            // 将delpath文件删除
            unlink(delarray[i].dedupfilepath);
        }

        // 将新增的delarray结构体数组的方式写入文件
        FILE *p = fopen(DEL_PATH, "w");
        // 打开失败直接退出
        if (p == NULL)
        {
            printf("open fail");
            return 0;
        }
        int j = 0;
        // 将结构体写出到文件中
        while (j < delnum)
        {
            printf("write %d successfully\n", j);
            fwrite(&delarray[j], sizeof(struct DelArray), 1, p);
            j++;
        }
        fclose(p);

        // accept hardlink table and save
        memset(recvBuf, 0, 1024);
        recv(fd, recvBuf, sizeof(recvBuf), 0);
        memcpy(&linknum, recvBuf, sizeof(linknum));
        for (int i = 0; i < linknum; i++)
        {
            memset(recvBuf, 0, 1024);
            recv(fd, recvBuf, sizeof(recvBuf), 0);
            memcpy(&hard_info[i], recvBuf, sizeof(hard_info[i]));
            printf("filepath:%s\n", hard_info[i].filePath);
            printf("hardlinkpath:%s\n", hard_info[i].hardlinkpath);
            printf("count:%d\n", hard_info[i].count);

            if (isFileExists(hard_info[i].hardlinkpath))
            {
                printf("exist\n");
                if (hard_info[i].count == 0)
                {
                    unlink(hard_info[i].hardlinkpath);
                }
            }
            else
            {
                printf("not exist\n");
                if (hard_info[i].count != 0)
                {
                    link(hard_info[i].filePath, hard_info[i].hardlinkpath);
                }
            }
        }

        memset(sendBuf, 0, 1024);
        memcpy(sendBuf, &flag, sizeof(flag));
        send(fd, sendBuf, sizeof(sendBuf), 0);
        sleep(1);
        break;
    }

    close(fd);

    return 0;
}
