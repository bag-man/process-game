#pragma GCC diagnostic ignored "-Wcpp" // sprunge.us/eNRe

// gcc fork.c -o fork -std=c99 -lpthread

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/shm.h> 
#include <pthread.h>
#include <sys/wait.h>

#define TIME 5

key_t key;
int shmid;
int *lost;

void end() {
  if(*lost == 0) {
    printf("\nGame over.\n");
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

void *timer() {
  int time = 5;

  for (int i = TIME; i > 0; i--) {
    sleep(1);
    printf("%d.. ", i);
    fflush(stdout);
  }
  end();
}

int main(int argc, char *argv[]) {
  pid_t cpid;
  key = 42;
  shmid = shmget(key, 1, 0644 | IPC_CREAT);
  lost = shmat(shmid, (void *)0, 0);
  *lost = 0;

  cpid = fork();
  int status;

  if(cpid == 0) { 

    // Child
    // printf("Parent pid %d\n", getppid());

    printf("Press Enter to reset the timer.\n");

    pthread_t pth, time;
    pthread_create(&pth, NULL, check_parent, NULL);
    pthread_create(&time, NULL, timer, NULL);
    
    while(1) {
      if(getchar()) {
        pthread_cancel(time);
        pthread_create(&time, NULL, timer, NULL);
      }
    }

  } else {

    // Parent 
    // printf("Child pid %d\n", cpid);

    signal(SIGINT, end);
    signal(SIGHUP, end);
    wait(0);
    end();
  }

  return 0;
}
