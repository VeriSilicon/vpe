#ifndef NET_SENDER_H_INCLUDED
#define NET_SENDER_H_INCLUDED

#define CONFIG_NET_BUF_SIZE   (2* 1024)

#define HEART_BEAT      400
#define CLIENT_FPS      500
#define CLIENT_TIMEOUT  600
#define STATUS_PRINT    700


#define offsetof(type, member) (size_t)&(((type*)0)->member)

typedef void * NET_SENDER_HANDLE;

typedef struct message {
    int type;
    int log_level;
    int dev_id;
    int task_id;
    int  len;
    char mtext[CONFIG_NET_BUF_SIZE];
} UDP_MSG_t;
NET_SENDER_HANDLE net_sender_init(const char *server_addr, int port, int dev_id, int task_idx);
int net_sender_uninit(NET_SENDER_HANDLE      inst);
int net_sender_send_fps(NET_SENDER_HANDLE      inst, int dev_id, int task_id);

#endif /* NET_SENDER_H_INCLUDED */

