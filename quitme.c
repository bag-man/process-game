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
#include "quitme.h"

#define TIME 1
#define COUNTDOWN 10
#define LIMIT 20

/* Global vars, for shared memory and display / user checks */
key_t key;
int shmid;
int *lost;
Window window;
Display* display;
int users;

int main(int argc, char *argv[]) {

  /* Define startup vars for shared memory */
  key = 42;
  shmid = shmget(key, 1, 0644 | IPC_CREAT);
  lost = shmat(shmid, (void *)0, 0);
  *lost = 0;

  /* Define startup vars for display / user checks */
  users = count_users();
  display = XOpenDisplay(NULL);
  window = has_focus();

  /* Generate random process name */
  srand(time(NULL));
  int r = rand();
  char pname[10];
  sprintf(pname, "%d", r);
  int size = strlen(argv[0]); 
  strncpy(argv[0],pname,size); 
  
  /* Create fork */
  pid_t cpid;
  cpid = fork();

  // Child fork
  if(cpid == 0) { 
    printf("Hit enter to keep the timer from reaching zero.\n");

    /* Start threads for game and parent checker */
    pthread_t parent, time, limiter;
    pthread_create(&parent, NULL, check_parent, NULL);
    pthread_create(&time, NULL, timer, NULL);
    pthread_create(&limiter, NULL, limit, NULL);
    signal(SIGTERM, end);
    
    /* If user hits enter, restart the countdown */
    while(1) {
      if(getchar()) {
        pthread_cancel(time);
        pthread_create(&time, NULL, timer, NULL);
      }
    }

  // Parent 
  } else {

    /* Start threads for window and user checks */
    pthread_t windowChecker, userChecker;
    pthread_create(&windowChecker, NULL, check_window, NULL);
    pthread_create(&userChecker, NULL, check_users, NULL);

    /* Handle signals in parent window */
    signal(SIGINT, end);
    signal(SIGHUP, end);
    signal(SIGTERM, end);
    signal(SIGTSTP, end);
    signal(SIGKILL, end);

    /* Wait for child to die (Count down gets to 0) */
    wait(0);
    end();
  }

  return 0;
}

/* Function that is called when game is lost */
void end() {
  if(*lost == 0) {
    printf("\n\nGame over.\n");
    //fopen("game.over", "ab+"); // To prove it works if stdout is lost
    *lost = 1;
  }
  exit(0);
}

Window has_focus() {
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
    if(has_focus() != window) {
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

