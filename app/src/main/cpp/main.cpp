#include "4v6.h"

// global status
bool alive = false;
bool hasIP = false;

// heart beat stats
int heart_beat_recv_time;
int heart_beat_cnt;

int client_socket;
// front-end to back-end handle
int fifo_handler;
//back-end to front-end handle
int fifo_stats_handler;
// back-end to server handle
int tunnel_handler;

// network stats, collected by backend and displayed by frontend
int outlen, outtimes, inlen, intimes;
//locks for stats
pthread_mutex_t stat_out, stat_in;

void connect2Server() {
    struct sockaddr_in6 server_socket;
    bzero(&server_socket, sizeof(server_socket));
    server_socket.sin6_family = AF_INET6;
    server_socket.sin6_port = htons(SERVER_PORT);
    CHECK(inet_pton(AF_INET6, SERVER_ADDR, &server_socket.sin6_addr));
    LOGD("v6 address set\n");

    CHECK(client_socket = socket(AF_INET6, SOCK_STREAM, 0));
    LOGD("client_socket: %d\n", client_socket);
    LOGD("create client socket v6\n");

    CHECK(connect(client_socket, (const struct sockaddr *)&server_socket, sizeof(server_socket)));
    LOGD("connect to server\n");
}


void initPipe() {
    if (access(JAVA_JNI_PIPE_CTOJ_PATH, F_OK) == -1) {
        int c2jid = mknod(JAVA_JNI_PIPE_CTOJ_PATH, S_IFIFO | 0666, 0);
        LOGD("create write file (c2j) id: %d\n", c2jid);
    }
    if (access(JAVA_JNI_PIPE_JTOC_PATH, F_OK) == -1) {
        int j2cid = mknod(JAVA_JNI_PIPE_JTOC_PATH, S_IFIFO | 0666, 0);
        LOGD("create read file (j2c) id: %d\n", j2cid);
    }
    (fifo_handler = open(JAVA_JNI_PIPE_JTOC_PATH, O_RDONLY | O_CREAT | O_TRUNC | O_NONBLOCK)); //debug only
    // CHECK(fifo_handler = open(JAVA_JNI_PIPE_JTOC_PATH, O_RDONLY | O_CREAT | O_TRUNC));
    (fifo_stats_handler = open(JAVA_JNI_PIPE_CTOJ_PATH, O_RDWR|O_CREAT|O_TRUNC));
}

void* initTimer(void *foo) {
    struct Message heart_beat;
    bzero(&heart_beat, sizeof(heart_beat));
    heart_beat.type = MSG_HEARTBEAT;
    heart_beat.length = sizeof(heart_beat);
    
    char buffer[MAX_BUF_SIZE+1];
    bzero(buffer, MAX_BUF_SIZE+1);
    memcpy(buffer, &heart_beat, sizeof(struct Message));

    char fifo_buf[MAX_BUF_SIZE+1];

    while(alive) {
        int current_time = time((time_t*) NULL);
        if (current_time - heart_beat_recv_time > 60) {
            LOGE("Server timeout\n");
            exit(1);
        }
        if (heart_beat_cnt == 0) {
            int len;
            // send heart beat package
            CHECK(len = send(client_socket, buffer, 5, 0));
            if (len != 5) {
                LOGF("Send heart beat package error\n");
            }
            heart_beat_cnt = MAX_HEART_BEAT_CNT;
        }
        else {
            heart_beat_cnt--;
        }

        sprintf(fifo_buf, "%d %d %d %d%c", outlen, outtimes, inlen, intimes, '\0');
        int writelen;
        CHECK_WRITE(writelen = write(fifo_stats_handler, fifo_buf, strlen(fifo_buf) + 1));
        if (writelen < strlen(fifo_buf) + 1) {
            LOGE("write stats to fifo_stats_handler error!\n");
        }

        //clear stats
        pthread_mutex_lock(&stat_out);
        outlen = outtimes = 0;
        pthread_mutex_unlock(&stat_out);
        pthread_mutex_lock(&stat_in);
        inlen = intimes = 0;
        pthread_mutex_unlock(&stat_in);

        sleep(1);
    }

    bzero(fifo_buf, MAX_BUF_SIZE+1);
    sprintf(fifo_buf, "-1 -1 -1 -1");
    int wrt_len;
    CHECK_WRITE(wrt_len = write(fifo_stats_handler, fifo_buf, strlen(fifo_buf) + 1));
    if (wrt_len < strlen(fifo_buf) + 1) {
        LOGE("write stats to fifo_stats_handler error!\n");
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

        CHECK(len = read(tunnel_handler, buffer, MAX_BUF_SIZE));
        LOGD("Get data from frontend. Total %d bytes\n", len);
        msg.length = len + 5;
        msg.type = MSG_DATA_REQ;
        memcpy(msg.data, buffer, len);
        memcpy(buffer, &msg, sizeof(msg));

        // Send package to server
        CHECK(len = send(client_socket, buffer, sizeof(msg), 0));
        if (len != sizeof(msg)) {
            LOGE("Send data package error!\n");
        }

        pthread_mutex_lock(&stat_out);
        outlen += msg.length - 5;
        outtimes++;
        pthread_mutex_unlock(&stat_out);
    }
    LOGD("send data to server thread exited!\n");
    return NULL;
}

// monitor the fifo_handler and stop listening when receiving 999
void* stopListening(void *foo) {
    char buffer[MAX_BUF_SIZE+1];
    bzero(buffer, MAX_BUF_SIZE+1);

    while (alive) {
        int len;
        CHECK(len = read(fifo_handler, buffer, MAX_BUF_SIZE));
        if (buffer[0] == '9' && buffer[1] == '9' && buffer[2] == '9') {
            alive = false;
        }
    }
    LOGD("stop listening and exit.\n");
    return NULL;
}

void init() {
    // global variables init
    alive = true;
    hasIP = false;
    heart_beat_cnt = 0;
    heart_beat_recv_time = 0;
    outlen = outtimes = inlen = intimes = 0;

    pthread_mutex_init(&stat_out, NULL);
    pthread_mutex_init(&stat_in, NULL);

    initPipe();
    connect2Server();
}

// close all of the handler, release the socket and destory the mutex lock
void clear() {
    CHECK(close(client_socket));
    CHECK(close(tunnel_handler));
    CHECK(close(fifo_handler));
    CHECK(close(fifo_stats_handler));

    CHECK(pthread_mutex_destroy(&stat_out));
    CHECK(pthread_mutex_destroy(&stat_in));
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
    msg.length = sizeof(msg);
    msg.type = MSG_IP_REQ;
    memcpy(buffer, &msg, sizeof(msg));
    CHECK(send(client_socket, buffer, sizeof(msg), 0));

    int len;
    // parse received packages
    while(alive) {
        bzero(buffer, MAX_BUF_SIZE+1);

        CHECK(len = recv(client_socket, buffer, sizeof(struct Message), 0));
        LOGD("Recv package from server. Total %d bytes.\n", len);

        bzero(&msg, sizeof(msg));
        memcpy(&msg, buffer, sizeof(msg));
        if (!hasIP && msg.type == MSG_IP_RSB) {
            LOGD("Type: IP response. Contents: %s", msg.data);
            
            bzero(buffer, MAX_BUF_SIZE+1);
            sprintf(buffer, "%s%d", msg.data, client_socket);
            len = strlen(buffer) + 1;
            int size;
            CHECK_WRITE(size = write(fifo_handler, buffer, len));
            if (len != size) {
                LOGE("write fifo_handler error!");
                exit(1);
            }

            sleep(1);

            bzero(buffer, MAX_BUF_SIZE+1);
            CHECK(len = read(fifo_handler, buffer, MAX_BUF_SIZE));
            if(len != sizeof(int)) {
                LOGD("fifo_handler read error!");
                exit(1);
            }

            tunnel_handler = *(int *)buffer;
            tunnel_handler = ntohl(tunnel_handler);
            LOGD("Get tunnel_handler %d from frontend successfully!", tunnel_handler);

            // send to server thread;
            pthread_t send_thd, stop_thd;
            pthread_create(&send_thd, NULL, send2Server, NULL);
            pthread_create(&stop_thd, NULL, stopListening, NULL);
            // only the first IP response from server will create thread for communication
            hasIP = true;
        } else if (msg.type == MSG_DATA_RSB) {
            LOGD("Type: Data response. Len: %d Contents: %s", msg.length, msg.data);
            CHECK_WRITE(len = write(tunnel_handler, msg.data, msg.length - 5));
            if (len != msg.length -5) {
                LOGE("Send data to frontend error!\n");
            }

            pthread_mutex_lock(&stat_in);
            inlen += msg.length - 5;
            intimes ++;
            pthread_mutex_unlock(&stat_in);
        } else if (msg.type == MSG_HEARTBEAT) {
            LOGD("Type: Heart beat. Contents: %s\n", msg.data);
            heart_beat_recv_time = time((time_t *)NULL);
        } else {
            LOGD("Type: Unknown type %d. Contents: %s", msg.type, msg.data);
        }
    }

    clear();
    return 0;
}
