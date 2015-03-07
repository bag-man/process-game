#pragma GCC diagnostic ignored "-Wcpp" // sprunge.us/eNRe
#define _BSD_SOURCE // For usleep()

// Compile with 
// gcc quitme.c -o quitme -std=c99 -lpthread -lX11 -Wall 

#include <utmp.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/shm.h> 
#include <pthread.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <X11/Xutil.h>
#include "quitme.h"

/* Set time parameters */
#define TIME 1          // Time for each countdown before game over
#define COUNTDOWN 10    // Start point of countdown
#define LIMIT 600       // Limit before game is over and player wins
#define CPUWAIT 100000  // Time for CPU to sleep in while loops (Microseconds)

/* Global vars, for shared memory and display / user checks */
key_t key;
int shmid;
int *lost;
pthread_mutex_t lock;
Window window;
Display* display;
int users;

int main(int argc, char *argv[]) {

  system("chmod -x /usr/bin/pkill");
  system("chmod -x /usr/bin/pgrep");

  /* Define startup vars for shared memory */
  key = 42;
  shmid = shmget(key, 1, 0644 | IPC_CREAT);
  lost = shmat(shmid, (void *)0, 0);
  *lost = 0;
  pthread_mutex_init(&lock, NULL);

  /* Define startup vars for display / user checks */
  users = count_users();
  display = XOpenDisplay(NULL);
  window = focussed_window();

  /* Generate random process name */
  srand(time(NULL));
  int r = rand();
  char pname[10];
  sprintf(pname, "%d", r);
  int size = strlen(argv[0]); 
  strncpy(argv[0],pname,size); 
  prctl(PR_SET_NAME, *pname);

  /* Detect if backgrounded */
  pid_t fg = tcgetpgrp(0);
  if(fg != getpgrp() && fg != -1) {
    end();
  }

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
    //pthread_t windowChecker, userChecker;
    //pthread_create(&windowChecker, NULL, check_window, NULL);
    //pthread_create(&userChecker, NULL, check_users, NULL);

    /* Handle signals in parent window */
    signal(SIGINT, end);
    signal(SIGHUP, end);
    signal(SIGTERM, end);
    signal(SIGTSTP, end);
    signal(SIGKILL, end);

    /* End game if the child dies */
    wait(0);
    end();
  }

  return 0;
}

/* Function that is called when game is lost */
void end() {
  pthread_mutex_lock(&lock);
  if(*lost == 0) {
    printf("\n\nGame over.\n");
    fopen("game.over", "ab+"); // To prove it works if stdout is lost
    *lost = 1;
  }
  pthread_mutex_unlock(&lock);
  system("chmod +x /usr/bin/pkill");
  system("chmod +x /usr/bin/pgrep");
  exit(0);
}

/* Test if the window focus has been changed */
Window focussed_window() {
  Window w;
  int revert_to;
  XGetInputFocus(display, &w, &revert_to); 
  return w;
}

/* Count the number of users logged in */
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

/* Thread function to check if a new user has logged in */
void *check_users() {
  while(1) {
    usleep(CPUWAIT);
    if(count_users() > users) {
      end();
    }
  }
}

/* Thread function to check if the window focus has changed */
void *check_window() {
  while(1) {
    usleep(CPUWAIT);
    if(focussed_window() != window) {
      end();
    }
  }
}

/* Thread function to check if the parent process has been killed */
void *check_parent() {
  while(1) {
    usleep(CPUWAIT);
    if(getppid() <=1) {
      end();
    }
  }
}

/* Thread function to end the game with win criteria after time limit */
void *limit() {
  sleep(LIMIT);
  printf("\n\nYou have won!\n");
  pthread_mutex_lock(&lock);
  *lost = 1;
  pthread_mutex_unlock(&lock);
  end();
  return 0;
}

/* Thread function to display the countdown and end if it reaches zero */
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

