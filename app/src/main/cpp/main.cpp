#include "4v6.h"

// global status
bool alive = false;
bool dead = true;
bool getIP = false;

// heart beat stats
int heart_beat_recv_time;
int heart_beat_cnt;

int client_socket;
// front-end to back-end handle
int des_pipe;
// back-end to front-end ip handle
int ip_pipe;
//back-end to front-end stats handle
int stats_pipe;
// back-end to server handle
int vpn_tunnel;

// network stats, collected by backend and displayed by frontend
int out_len, out_times, in_len, in_times;
//locks for stats
pthread_mutex_t out_stats, in_stats;

void connect2Server() {
    struct sockaddr_in6 server_socket;
    bzero(&server_socket, sizeof(server_socket));
    server_socket.sin6_family = AF_INET6;
    server_socket.sin6_port = htons(SERVER_PORT);
    CHK(inet_pton(AF_INET6, SERVER_ADDR, &server_socket.sin6_addr));
    LOGD("v6 address set\n");

    CHK(client_socket = socket(AF_INET6, SOCK_STREAM, 0));

    LOGD("client_socket: %d\n", client_socket);
    LOGD("create client socket v6\n");

    int status;
    while ((status = connect(client_socket, (const struct sockaddr *)&server_socket, sizeof(server_socket))) == -1)
        sleep(1);
    LOGD("connect to server\n");
}

void connect2ServerV4()
{
    struct sockaddr_in server_socket;
    bzero(&server_socket, sizeof(server_socket));
    server_socket.sin_family = AF_INET;
    server_socket.sin_port = htons(SERVER_PORT);
    CHK(inet_pton(AF_INET, SERVER_V4_ADDR, &server_socket.sin_addr));
    LOGD("v4 address set\n");

    CHK(client_socket = socket(AF_INET, SOCK_STREAM, 0));
    LOGD("client_socket: %d\n", client_socket);
    LOGD("create client socket v4\n");

    CHK(connect(client_socket, (const struct sockaddr *)&server_socket, sizeof(server_socket)));
    LOGD("connect to server v4\n");
}

void initPipe() {
    if (access(JNI_STATS_PIPE_PATH, F_OK) == -1) {
        int stats_id = mknod(JNI_STATS_PIPE_PATH, S_IFIFO | 0666, 0);
        LOGD("create write file (stats) id: %d\n", stats_id);
    }
    if (access(JNI_IP_PIPE_PATH, F_OK) == -1) {
        int ip_id = mknod(JNI_IP_PIPE_PATH, S_IFIFO | 0666, 0);
        LOGD("create read file (ip) id: %d\n", ip_id);
    }
    if (access(JNI_DES_PIPE_PATH, F_OK) == -1) {
        int des_id = mknod(JNI_DES_PIPE_PATH, S_IFIFO | 0666, 0);
        LOGD("create read file (des) id: %d\n", des_id);
    }
    //    CHK(ip_pipe = open(JNI_IP_PIPE_PATH, O_RDONLY | O_CREAT | O_TRUNC | O_NONBLOCK));

    CHK(ip_pipe = open(JNI_IP_PIPE_PATH, O_RDWR | O_CREAT | O_TRUNC)); //debug only
    LOGD("ip_pipe ready, %d\n", ip_pipe);
    CHK(close(ip_pipe));

    CHK(stats_pipe = open(JNI_STATS_PIPE_PATH, O_RDWR | O_CREAT | O_TRUNC));
    LOGD("stats_pipe ready, %d\n", stats_pipe);
    CHK(close(stats_pipe));

    CHK(des_pipe = open(JNI_DES_PIPE_PATH, O_RDWR | O_CREAT | O_TRUNC));
    LOGD("des_pipe ready, %d\n", des_pipe);
    CHK(close(des_pipe));
}

int readPipe(char* buffer) {
    CHK(des_pipe = open(JNI_DES_PIPE_PATH, O_RDONLY));
    int len = read(des_pipe, buffer, MAX_BUF_SIZE);
    CHK(close(des_pipe));
    return len;
}

int writePipe(int type, char *buffer, int length) {
    if (type == WRITE_IP) {
        CHK(ip_pipe = open(JNI_IP_PIPE_PATH, O_RDWR)); //debug only
        CHK(safe_write(ip_pipe, buffer, length));
        CHK(close(ip_pipe));
    } else {
        CHK(stats_pipe = open(JNI_STATS_PIPE_PATH, O_RDWR));
        CHK(safe_write(stats_pipe, buffer, length));
        CHK(close(stats_pipe));
}
    return length;
}

void* initTimer(void *foo) {
    struct Message heart_beat;
    bzero(&heart_beat, sizeof(heart_beat));
    heart_beat.type = MSG_HEARTBEAT;
    heart_beat.length = 5;
    
    char buffer[MAX_BUF_SIZE+1];
    bzero(buffer, MAX_BUF_SIZE+1);
    memcpy(buffer, &heart_beat, 5);

    char stats_buf[MAX_BUF_SIZE+1];

    int prev_out_len = 0, prev_in_len = 0;

    while(alive) {
        int current_time = time((time_t*) NULL);
        if (current_time - heart_beat_recv_time > 60) {
            LOGE("Server timeout\n");
            // heart_beat_recv_time = current_time;
            exit(1);
        }
        if (heart_beat_cnt == 0) {
            int len;
            // send heart beat package
            CHK(len = safe_send(client_socket, buffer, 5));
            if (len != 5) {
                LOGF("Send heart beat package error\n");
            } else {
                LOGD("Send heart beat package");
            }
            heart_beat_cnt = MAX_HEART_BEAT_CNT;
        }
        else {
            heart_beat_cnt--;
        }

        sprintf(stats_buf, "%d %d %d %d %d%c", out_len, out_times, in_len, in_times, 54, '\0');

        // monitor the net speed
        static int cnt = 0;
        if (cnt % 5 == 0)
            LOGD("out speed: %2f KB/s, in speed: %2f Kb/s", (float)(out_len-prev_out_len)/1000.0, (in_len-prev_in_len)/1000.0);
        cnt ++;

        int write_len;

        CHK_WRITE(write_len = writePipe(WRITE_STATS, stats_buf, strlen(stats_buf)));
        if (write_len != strlen(stats_buf)) {
            LOGE("write stats to stats_pipe error!\n");
        }

        //clear stats
        pthread_mutex_lock(&out_stats);
        out_len = out_times = 0;
        pthread_mutex_unlock(&out_stats);
        pthread_mutex_lock(&in_stats);
        in_len = in_times = 0;
        pthread_mutex_unlock(&in_stats);

        sleep(1);
    }

    while (!dead) { usleep(10000); }

    bzero(stats_buf, MAX_BUF_SIZE+1);
    sprintf(stats_buf, "-1 -1 -1 -1 -1%c", '\0');
    int wrt_len;
    CHK_WRITE(wrt_len = writePipe(WRITE_STATS, stats_buf, strlen(stats_buf)));
    if (wrt_len != strlen(stats_buf)) {
        LOGE("write stats to stats_pipe error!\n");
    }
    LOGD("timer thread exit\n");
    return NULL;
}

void* send2Server(void *foo) {
    char buffer[MAX_BUF_SIZE+1];

    int len;
    struct Message msg;


    while (alive) {
        bzero(buffer, MAX_BUF_SIZE+1);
        bzero(&msg, sizeof(msg));

        while ((len = read(vpn_tunnel, buffer, MAX_BUF_SIZE)) < 0){
            usleep(1000);
        };
//         LOGD("Get data from frontend. Total %d bytes\n", len);
        msg.length = len + 5;
        msg.type = MSG_DATA_REQ;
        memcpy(msg.data, buffer, len);
        bzero(buffer, MAX_BUF_SIZE+1);
        memcpy(buffer, &msg, msg.length);

        // Send package to server
        CHK(len = safe_send(client_socket, buffer, msg.length));
        if (len != msg.length) {
            LOGE("Send data package error!\n");
        } else {
            //  LOGD("Send data package to server, content: %d, %d\n", *(int*)buffer, int(*(buffer+4)));
        }

        pthread_mutex_lock(&out_stats);
        out_len += msg.length - 5;
        out_times++;
        pthread_mutex_unlock(&out_stats);
    }
    LOGD("send data to server thread exited!\n");
    return NULL;
}

// monitor the ip_pipe and stop listening when receiving 999
void* stopListening(void *foo) {
    char buffer[MAX_BUF_SIZE+1];
    bzero(buffer, MAX_BUF_SIZE+1);

    while (alive) {
        int len;
        CHK(len = readPipe(buffer));
        if (buffer[0] == 'q' && buffer[1] == '1' && buffer[2] == 'j') {
            alive = false;
            dead = true;
            heart_beat_recv_time = time((time_t *)NULL);
            LOGD("alive is false");
        }
    }
    LOGD("stop listening and exit.\n");
    return NULL;
}

void init() {
    // global variables init
    alive = true;
    dead = false;
    getIP = false;
    heart_beat_cnt = 0;
    heart_beat_recv_time = 0;
    out_len = out_times = in_len = in_times = 0;

    pthread_mutex_init(&out_stats, NULL);
    pthread_mutex_init(&in_stats, NULL);

    initPipe();
    connect2Server();
    //  connect2ServerV4();
}

// close all of the handler, release the socket and destory the mutex lock
void clear() {
    CHK(close(client_socket));
    CHK(close(vpn_tunnel));
    CHK(pthread_mutex_destroy(&out_stats));
    CHK(pthread_mutex_destroy(&in_stats));
}

int main() {
    init();
    char buffer[MAX_BUF_SIZE+1];
    bzero(buffer, MAX_BUF_SIZE + 1);

    // heart beat thread
    pthread_t heart_beat_thd;
    heart_beat_recv_time = time((time_t *)NULL);
    pthread_create(&heart_beat_thd, NULL, initTimer, NULL);

    // require IP from server
    struct Message msg;
    bzero(&msg, sizeof(msg));
    msg.length = 5;
    msg.type = MSG_IP_REQ;
    memcpy(buffer, &msg, msg.length);
    CHK(safe_send(client_socket, buffer, msg.length));

    int len;
    int msg_len;
    // parse received packages
    while(alive) {
        bzero(buffer, MAX_BUF_SIZE+1);

        CHK(len = safe_read(client_socket, buffer, 4));
        if (len != 4) {
            LOGD("Recv package hdr error");
        }
        msg_len = *(int *)buffer;
        CHK(len = safe_read(client_socket, buffer + 4, msg_len - 4));
        // LOGD("Recv package from server. Should %d bytes, get Total %d bytes.\n", msg_len - 4, len);

        bzero(&msg, sizeof(msg));
        memcpy(&msg, buffer, sizeof(msg));
        if (!getIP && msg.type == MSG_IP_RSB) {
            LOGD("Type: IP response. Contents: %s\n", msg.data);
            
            bzero(buffer, MAX_BUF_SIZE+1);
#ifndef OUR_SERVER
            sprintf(buffer, "%s%d ", msg.data, client_socket);
#else
            sprintf(buffer, "%s %d ", msg.data, client_socket);
#endif
            len = int(strlen(buffer) + 1);
            int size;

            CHK_WRITE(size = writePipe(WRITE_IP, buffer, len));
            if (len != size) {
                LOGE("write ip_pipe error!\n");
            }

            sleep(1);

            bzero(buffer, MAX_BUF_SIZE+1);
            while((len = readPipe(buffer)) <= 0){
                sleep(1);
                LOGD("read len = %d, content: %d, waiting for handler...\n", len, *(int *)buffer);
            };

            vpn_tunnel = int(*(char *)buffer);
            LOGD("Get vpn_tunnel %d from frontend successfully!", vpn_tunnel);
            // vpn_tunnel = ntohl(vpn_tunnel);

            // send to server thread;
            pthread_t send_thd, stop_thd;
            pthread_create(&send_thd, NULL, send2Server, NULL);
            pthread_create(&stop_thd, NULL, stopListening, NULL);
            // only the first IP response from server will create thread for communication
            getIP = true;
        } else if (msg.type == MSG_DATA_RSB) {
            // LOGD("Type: Data response.");
            CHK_WRITE(len = safe_write(vpn_tunnel, msg.data, msg.length - 5));
            if (len != msg.length - 5) {
                LOGE("Send data to frontend error!\n");
            }

            pthread_mutex_lock(&in_stats);
            in_len += msg.length - 5;
            in_times ++;
            pthread_mutex_unlock(&in_stats);
        } else if (msg.type == MSG_HEARTBEAT) {
            LOGD("Type: Heart beat.\n");
            heart_beat_recv_time = int(time((time_t *)NULL));
        } else {
            // LOGD("Type: Unknown type %d. Contents: %s\n", msg.type, msg.data);
            LOGD("Type: Unknown type %d.\n", msg.type);
        }
    }

    dead = true;

    clear();
    return 0;
}
