#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

#include "logger.h"
static int log_fd[2] = {-1,-1};
static pid_t logger_pid = -1; 
static int sequence = 0; //log message counter
static int logger_started = 0; //boolean flag

int create_log_process() {

    if (logger_started)
        return 0;

    logger_started = 1;

    if (pipe(log_fd) < 0) {
        perror("logger: pipe failed");
        return -1;
    }

    logger_pid = fork();

    if (logger_pid < 0) {
        perror("logger: fork failed");
        return -1;
    }
    //child
    if (logger_pid == 0) {

        close(log_fd[1]);

        FILE *logfile = fopen("gateway.log", "a");
        if (!logfile) {
            perror("logger: cannot open gateway.log");
            exit(1);
        }

        char buffer[256];

        while (1) {

            int pos = 0;
            char ch;
//read byte by byte
            while (1) {
                ssize_t n = read(log_fd[0], &ch, 1);
                if (n <= 0) {
                    goto end;//parent has closed the pipe exit the logger
                }
                buffer[pos++] = ch;
                if (ch == '\0' || pos >= 255) {
                    break;//message completed
                }
            }

            time_t now = time(NULL);
            char *ts = ctime(&now);
            ts[strcspn(ts, "\n")] = '\0';

            fprintf(logfile, "%d - %s - %s\n", sequence, ts, buffer);
            fflush(logfile);
            sequence++;
        }

    end:
        close(log_fd[0]);
        fclose(logfile);
        exit(0);
    }

    close(log_fd[0]);
    return 0;
}



int write_to_log_process(char *msg){
  if(!logger_started){
    return -1; //logger not started yet
  }
  
  if(msg==NULL){
    return -1;
  }
  
  // write the message including the \0 so child knows end of string
  size_t len = strlen(msg) + 1;
  
  ssize_t written = write((log_fd[1]), msg, len);
  
  if (written < 0) {
    perror("logger: write failed");
    return -1;
  }

    return 0;
}


int end_log_process(){
  
  if(!logger_started){
    return -1;
  }
  
  //close write end so signal EOF for the child
  
  close(log_fd[1]);
  //wait for the logger child to terminate
  waitpid(logger_pid, NULL, 0);
   
  logger_started = 0;
  logger_pid = -1;
  log_fd[0] = log_fd[1] = -1;
  sequence = 0;    // sequence should restart for next session

  return 0;

}
