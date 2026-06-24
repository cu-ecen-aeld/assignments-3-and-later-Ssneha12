#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[]){

	openlog("Writer.c", LOG_PID, LOG_USER);
	if(argc != 3)
	{
		syslog(LOG_ERR,"Argument not specified");
		return 1;
	}

	FILE *ptr=fopen(argv[1],"w");

	 if (ptr == NULL) {
        	syslog(LOG_ERR,"FILE NOT OPENED");
		return 1;
        }
	syslog(LOG_DEBUG, "Writing  %s to  %s", argv[2], argv[1]);
	if(fputs(argv[2], ptr) <= 0){
        
		syslog(LOG_ERR,"Write failed ");
		return 1 ;

	}

        fclose(ptr);
        closelog();
	return 0;


}
