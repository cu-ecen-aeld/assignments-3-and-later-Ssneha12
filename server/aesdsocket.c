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
 
#define PORT 9000
#define tmp_file "/var/tmp/aesdsocketdata"
 
 bool run_program = true;
 char *buff;
 int sockfd;
 
void cleanup() {

    free(buff);//free buffer 
    //remove tmp file
    if (unlink(tmp_file) != 0) {
		perror("Failed to remove file");
    }
    //close connections 
    close(sockfd);
    closelog();
}
 
 
void end_signal(int signum) {
    syslog(LOG_INFO, "Caught signal, exiting");
    run_program = false;
    return ;
}
 
int main(int argc, char *argv[]) {
 
    
    openlog("aesdsocket", 0, LOG_USER);// Setup syslog 

 
    //Setup signal handlers 
    struct sigaction sig_handler ;
    memset(&sig_handler,0,sizeof(sig_handler));
    sig_handler.sa_handler = end_signal;
    
    if(sigaction(SIGINT,&sig_handler,NULL) !=0){
        perror("Sigaction SIGINT Failed");
        return -1;
    }
    
    if(sigaction(SIGTERM, &sig_handler,NULL) !=0){
       perror("Sigaction SIGTERM Failed");
       return -1;
    }
 
    
    int buff_size = 1024; // Dynamic Allocatation for buffer
    buff = malloc(buff_size);
    if (buff == NULL) {
        return -1;
    }
 
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }
    // Set socket options to allow reuse of address and port
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        return -1;
    }
 
	struct sockaddr_in addr;
	memset(&addr,0, sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(PORT);
	addr.sin_addr.s_addr=INADDR_ANY;
	
   // bind the socket to the address and port
   
	if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        cleanup();
        return -1;
    }
 
    if(argc>1 && strcmp(argv[1],"-d") == 0){
        //enter daemon mode
        pid_t pid = fork();
        //check if fork failed
        if(pid == -1){
            cleanup();
            return -1;
        }
        else if(pid == 0){
            //child process
            int setsid_result = setsid();
            if(setsid_result == -1){
                cleanup();
                return -1;
            }
            chdir("/");
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }
        else{
            //parent process
            //exit parent process
            exit(0);
        }
    }

 
    // Listen for incoming connections
    int result = listen(sockfd, 5);
    if (result < 0) {
        cleanup();
        return -1;
    }
 
    while(run_program){
 
        // Accept a connection
        struct sockaddr client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int clientfd = accept(sockfd, &client_addr, &client_addr_len);
        if (clientfd < 0) {
        	if (run_program ==false){
	        	break;
			}		
			continue;
        }
 
        syslog(LOG_INFO,"Accepted connection from %s",client_addr.sa_data);
 
        //open aesdsocketdata file and write to it
        FILE *transfered_data_file = fopen(tmp_file, "a+");
        if (transfered_data_file == NULL) {
            cleanup();
            return -1;
        }
        ssize_t bytes_received = 0;
        while((bytes_received = recv(clientfd, buff, buff_size, 0)) > 0) {
            fwrite(buff, 1, bytes_received, transfered_data_file);
             // Search for newline in the received chunk
            if (memchr(buff, '\n', bytes_received)) {
                break;
            }
        }
 
        fflush(transfered_data_file);
        fseek(transfered_data_file, 0, SEEK_SET);
 
        memset(buff, 0, buff_size);
 
        while (fgets(buff, buff_size, transfered_data_file) != NULL) {
            send(clientfd, buff, strlen(buff), 0);
        }
        fclose(transfered_data_file);

        syslog(LOG_INFO, "Closing connection from %s", client_addr.sa_data);
        close(clientfd);
 
    }
 
    cleanup();
 
    return 0;
}
