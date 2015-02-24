#pragma GCC diagnostic ignored "-Wcpp" // sprunge.us/eNRe

// gcc quitme.c -o quitme -std=gnu99 -lpthread -lX11 -Wall 

#include <utmp.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/shm.h> 
#include <pthread.h>
#include <sys/wait.h>
#include <X11/Xutil.h>

#define TIME 1
#define COUNTDOWN 10
#define LIMIT 20

key_t key;
int shmid;
int *lost;
Window window;
Display* display;
int users;

void end() {
  if(*lost == 0) {
    printf("\n\nGame over.\n");
    //fopen("game.over", "ab+"); // To prove it works if stdout is lost
    *lost = 1;
  }
  exit(0);
}

Window hasFocus() {
  Window w;
  int revert_to;
  XGetInputFocus(display, &w, &revert_to); 
  return w;
}

int count_users() {
  FILE *ufp;
  ufp = fopen(_PATH_UTMP, "r");
  int num = 0;
  struct utmp usr;
  while(fread((char *)&usr, sizeof(usr), 1, ufp) == 1) {
    if(*usr.ut_name && *usr.ut_line && *usr.ut_line != '~') {
      num++; 
    }
  }
  fclose(ufp);
  return num;
}

void *check_users() {
  while(1) {
    sleep(1);
    if(count_users() > users) {
      end();
    }
  }
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
  printf("\n\nYou have won!\n");
  *lost = 1;
  end();
  return 0;
}

void *timer() {
  printf("\n");
  for(int i = COUNTDOWN; i > 0; i--) {
    sleep(TIME);
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
  users = count_users();

  srand(time(NULL));
  int r = rand();
  char pname[10];
  sprintf(pname, "%d", r);
  int size = strlen(argv[0]); 
  strncpy(argv[0],pname,size); 
  
  cpid = fork();

  if(cpid == 0) { 

    // Child
    //printf("Parent pid %d\n", getppid());
    printf("Hit enter to keep the timer from reaching zero.\n");

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

    pthread_t windowChecker, userChecker;
    pthread_create(&windowChecker, NULL, check_window, NULL);
    pthread_create(&userChecker, NULL, check_users, NULL);

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
