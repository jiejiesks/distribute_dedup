#define MAX_FILES 100
#define MAX_SERVER_FILES 1024
#define MAX_CLIENT_FILES 100
#define MAC_LENGTH 18
#define MAX_CLIENTS 10
#define TOTALTHREAD 1
#define MD5_SIZE 16
#define READ_DATA_SIZE 1024
#define MD5_STR_LEN (MD5_SIZE * 2)

#define DEL_PATH "/home/zxj/Desktop/delpath"
#define FINGERPRINT_TABLE "/home/zxj/Desktop/fingerprint_table"

struct FileInfo
{
    char filePath[128];
    unsigned char md5[33];
};

struct FileInfo_table_server
{
    char filePath[128];
    unsigned char md5[33];
    unsigned char dev_id[MAC_LENGTH];
    int count;
};

struct FileInfo_table_client
{
    char filePath[128];
    unsigned char md5[33];
    unsigned char dev_id[MAC_LENGTH];
};

struct DelArray
{
    char filePath[128];      // 本文件的路径
    char dedupfilepath[128]; // 重复文件的路径
    char islocal[2];         // 重复文件是否在本地
};


struct Hardlink_Info
{
    char filePath[128];
    char hardlinkpath[128];
    int count;
};