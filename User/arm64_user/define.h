#define MAX_FILES 1024
#define MAX_SERVER_FILES 1024
#define MAX_CLIENT_FILES 1024
#define MAC_LENGTH 18
#define MAX_CLIENTS 10
#define TOTALTHREAD 1
#define MD5_SIZE 16
#define READ_DATA_SIZE 1024
#define MD5_STR_LEN (MD5_SIZE * 2)
#define FILE_PATH_MAX 128
#define PORT 9999
#define IP_ADDRESS "192.168.5.1"

#define DEL_PATH "/data/delpath"
#define FINGERPRINT_TABLE "/data/fingerprint_table"

struct FileInfo
{
    char filePath[FILE_PATH_MAX];
    unsigned char md5[MD5_STR_LEN+1];
};

struct FileInfo_table_server
{
    char filePath[FILE_PATH_MAX];
    unsigned char md5[MD5_STR_LEN+1];
    unsigned char dev_id[MAC_LENGTH];
    int count;
};

struct FileInfo_table_client
{
    char filePath[FILE_PATH_MAX];
    unsigned char md5[MD5_STR_LEN+1];
    unsigned char dev_id[MAC_LENGTH];
};

struct DelArray
{
    char filePath[FILE_PATH_MAX];      // 本文件的路径
    char dedupfilepath[FILE_PATH_MAX]; // 重复文件的路径
    char islocal[2];         // 重复文件是否在本地
};


struct Hardlink_Info
{
    char filePath[FILE_PATH_MAX];
    char hardlinkpath[FILE_PATH_MAX];
    int count;
};