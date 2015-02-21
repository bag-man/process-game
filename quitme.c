#pragma GCC diagnostic ignored "-Wcpp" // sprunge.us/eNRe

// gcc quitme.c -o quitme -std=c99 -lpthread

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/shm.h> 
#include <pthread.h>
#include <sys/wait.h>

#define TIME 10
#define LIMIT 20

key_t key;
int shmid;
int *lost;

void end() {
  if(*lost == 0) {
    printf("\nGame over.\n");
    // If killall is ran, and there is no file, you get two Game Overs.
    fopen("game.over", "ab+"); 
    *lost = 1;
  }
  exit(0);
}

void *check_parent() {
  while(1) {
    if(getppid() <=1) {
      end();
    }
  }
}

void *limit() {
  sleep(LIMIT);
  printf("\nYou have won!\n");
  *lost = 1;
  end();
  return 0;
}
  

void *timer() {
  for(int i = TIME; i > 0; i--) {
    sleep(1);
    printf("%d.. ", i);
    fflush(stdout);
  }
  end();
  return 0;
}

int main(int argc, char *argv[]) {
  pid_t cpid;
  key = 42;
  shmid = shmget(key, 1, 0644 | IPC_CREAT);
  lost = shmat(shmid, (void *)0, 0);
  *lost = 0;

  cpid = fork();

  if(cpid == 0) { 

    // Child
    //printf("Parent pid %d\n", getppid());

    printf("Press Enter to reset the timer.\n");

    pthread_t pth, time, limiter;
    pthread_create(&pth, NULL, check_parent, NULL);
    pthread_create(&time, NULL, timer, NULL);
    pthread_create(&limiter, NULL, limit, NULL);
    signal(SIGTERM, end);
    
    while(1) {
      if(getchar()) {
        pthread_cancel(time);
        pthread_create(&time, NULL, timer, NULL);
      }
    }

  } else {

    // Parent 
    //printf("Child pid %d\n", cpid);

    signal(SIGINT, end);
    signal(SIGHUP, end);
    //signal(SIGTERM, end);
    wait(0);
    end();
  }

  return 0;
}
