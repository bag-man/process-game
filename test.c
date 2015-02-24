#include <utmpx.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int users = 0;

int count_users() {
  setutxent();
  int num = 0;
  struct utmpx *user_info;
  while(1) {
    user_info = getutxent();
    num++;
    if(user_info == NULL) break;
  } 
  endutxent();
  return num;
}

int main(int argc, char *argv[]) {
  
  users = count_users();  
  printf("START USERS: %d\n", users);

  while(1) {
    sleep(1);
    printf("USERS: %d\n", count_users());
  }


  return 0;

}
