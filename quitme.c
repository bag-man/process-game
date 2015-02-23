#pragma GCC diagnostic ignored "-Wcpp" // sprunge.us/eNRe

// gcc quitme.c -o quitme -std=c99 -lpthread -lX11 -Wall 

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/shm.h> 
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <X11/Xutil.h>

#define TIME 10
#define LIMIT 20

key_t key;
int shmid;
int *lost;
Window window;
Display* display;

void end() {
  if(*lost == 0) {
    printf("\nGame over.\n");
    //fopen("game.over", "ab+"); // To prove it works if stdout is lost
    *lost = 1;
  }
  exit(0);
}

Window hasFocus() {
  Window w;
  int revert_to;
  XGetInputFocus(display, &w, &revert_to); // see man
  return w;
}

void *check_window() {
  while(1) {
    if(hasFocus() != window) {
      end();
    }
  }
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
    
    display = XOpenDisplay(NULL);
    window = hasFocus();
    pthread_t windowChecker;
    pthread_create(&windowChecker, NULL, check_window, NULL);

    signal(SIGINT, end);
    signal(SIGHUP, end);
    signal(SIGTERM, end);
    signal(SIGTSTP, end);
    signal(SIGKILL, end);
    wait(0);
    end();
  }

  return 0;
}
