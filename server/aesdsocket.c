#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
 
#define PORT 9000
#define tmp_file "/var/tmp/aesdsocketdata"
#define BUFF_SIZE 1024
 
bool run_program = true;
int sockfd;
pthread_mutex_t shared_file_mutex = PTHREAD_MUTEX_INITIALIZER;
 
void cleanup() {
    close(sockfd);
    unlink(tmp_file);
    closelog();
}
 
// Signal handler
void end_signal(int signum) {
    syslog(LOG_INFO, "Caught signal, exiting");
    run_program = false;
}

//Timer thread 
void* timer_thread_func(void* arg) {
    while(run_program) {
        sleep(10);  // no immediate timestamp
 
        if(!run_program) break;
 
        time_t now = time(NULL);
        struct tm tm_now;
        localtime_r(&now, &tm_now);
 
        char timestamp[100];
        strftime(timestamp, sizeof(timestamp), "timestamp:%Y-%m-%d %H:%M:%S\n", &tm_now);
 
        pthread_mutex_lock(&shared_file_mutex);
        FILE *fp = fopen(tmp_file, "a");
        if(fp) {
            fputs(timestamp, fp);
            fclose(fp);
        }
        pthread_mutex_unlock(&shared_file_mutex);
    }
    return NULL;
}
 
// Client thread 
void* client_thread_func(void* arg) {
    int clientfd = *((int*)arg);
    free(arg);
 
    char buffer[BUFF_SIZE];
    ssize_t bytes_received;
    FILE *fp;
 
    while((bytes_received = recv(clientfd, buffer, BUFF_SIZE, 0)) > 0) {
        pthread_mutex_lock(&shared_file_mutex);
        fp = fopen(tmp_file, "a");
        if(fp) {
            fwrite(buffer, 1, bytes_received, fp);
            fclose(fp);
        }
        pthread_mutex_unlock(&shared_file_mutex);
 
        
        if(memchr(buffer, '\n', bytes_received)) {
            pthread_mutex_lock(&shared_file_mutex);
            fp = fopen(tmp_file, "r");
            if(fp) {
                while(fgets(buffer, BUFF_SIZE, fp)) {
                    send(clientfd, buffer, strlen(buffer), 0);
                }
                fclose(fp);
            }
            pthread_mutex_unlock(&shared_file_mutex);
        }
    }

	syslog(LOG_INFO,"Closed connection" );
 
    close(clientfd);
    return NULL;
}
 
int main(int argc, char *argv[]) {
 
    openlog("aesdsocket", 0, LOG_USER);
 
    //signal handlers
    struct sigaction sig_handler;
    memset(&sig_handler,0,sizeof(sig_handler));
    sig_handler.sa_handler = end_signal;
 
    sigaction(SIGINT,&sig_handler,NULL);
    sigaction(SIGTERM,&sig_handler,NULL);
 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) { perror("socket"); return -1; }
 
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
 
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
 
    if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); cleanup(); return -1;
    }
 
    // Daemon mode
    if(argc>1 && strcmp(argv[1],"-d")==0){
        pid_t pid = fork();
        if(pid < 0){ 
        cleanup(); 
        return -1; 
        }
        if(pid > 0) exit(0); // parent exits
 
        setsid();
        if (chdir("/") != 0) {
    		perror("chdir");
    		return -1;
		}
        close(STDIN_FILENO); 
        close(STDOUT_FILENO); 
        close(STDERR_FILENO);
    }
 
    listen(sockfd, 5);
 
    // Start timer thread
    pthread_t timer_thread;
    pthread_create(&timer_thread, NULL, timer_thread_func, NULL);
    pthread_detach(timer_thread);
 
    while(run_program){
        struct sockaddr client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int *clientfd = malloc(sizeof(int));
        *clientfd = accept(sockfd, &client_addr, &client_addr_len);
        if(*clientfd < 0){
            free(clientfd);
            if(!run_program) break;
            continue;
        }
 
        syslog(LOG_INFO,"Accepted connection from %s",client_addr.sa_data );
        pthread_t tid;
        pthread_create(&tid, NULL, client_thread_func, clientfd);
        pthread_detach(tid);
    }
 
    cleanup();
    return 0;
}
