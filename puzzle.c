#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MILES (10)
#define DONG (11)
#define KLEAR (12)
#define ONION (13)

#define IDEA "AI for Dogs"

// author: nanxis2

// Here are the public Function declarations of the puzzle box
// You will need these (plus system and C library calls)
// to solve this puzzle box.
// pipe,dup2, using pthread and signal handling may be useful

void open_box(char *mesg); // puzzle 1
// There is no function call required for puzzle 2.
// Puzzle 2 starts as soon as puzzle 1 is finished, so be prepared
void enact_sneaky_plans();      // puzzle 3
void *smash_artwork(void *arg); // puzzle 4
void finish(char *mesg);        // puzzle 4 (closing credits)

int main() {
  int pipe_fd[2];
  pipe(pipe_fd);
  dup2(pipe_fd[0], ONION);
  write(pipe_fd[1], IDEA, strlen(IDEA));
  close(pipe_fd[1]);
  signal(SIGQUIT, enact_sneaky_plans);
  open_box("secret");
  pthread_t threads[100];
  for (int i = 0; i < 100; i++) {
    pthread_create(&threads[i], NULL, smash_artwork, (void *)0x341);
  }
  for (int i = 0; i < 100; i++) {
    void *ret;
    pthread_join(threads[i], &ret);
    if (ret != NULL) {
      finish((char *)ret);
    }
  }
}