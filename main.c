#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <math.h>

#include "grid.h"
#include "block.h"

extern int height;
extern int width;

Block* b;
int level = 0;
char** grid;
struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI };

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef struct information{
  Block* b;
  int level;
  char** grid;

  int height;
  int width;
  int score;
  int lines;
  char* next_blocks;

} info;

void processInput(char input, int* soft_drop);
int waitMillis(int ms);
void* gravity();

int main(){

  srand(time(NULL));
  pthread_t gravity_thread;


  int score = 0;
  int lines = 0;
  int soft_drop = 0;
  grid = createGrid(height, width);
  char* next_blocks = initializeNextBlocks();

  b = spawnBlock(next_blocks);
  char input = ' ';
  void* ptr = &input;
  pthread_create(&gravity_thread, NULL, gravity, NULL);

  printGrid(height, width, grid, b);

  do{
    system("clear");
    printf("Score: %d\nLevel: %d\nLines: %d\n", score, level, lines);
    printf("Next blocks: %c, %c, %c\n", next_blocks[0], next_blocks[1], next_blocks[2]);
    printGrid(height, width, grid, b);


    system("stty raw");
    if(poll(&mypoll, 1, 1000 - level * 50)){
      scanf("%c", &input);
    }
    system("stty cooked");


    processInput(input, &soft_drop);
    moveBlock(b, input, grid, &score, &soft_drop);
    input = 'k';

    if(blockHasCrashed(b)){
      clearLines(height, width, grid, &score, &level);
      free(b);

      b = spawnBlock(next_blocks);
    }

  }while(!playerLost(height, width, grid));

  printf("\nYou lost!\n");

  free(next_blocks);
  freeGrid(height, grid);
  return 0;

}

void processInput(char input, int* soft_drop){
  switch(input){
    case 'q':
      system("stty cooked");
      exit(0);
    case 's':
      *soft_drop = 1;
      pthread_cond_signal(&cond);
      break;
    }
}

int waitMillis(int ms)
{
    int n;
    struct timeval tv;
    struct timespec ts;

    gettimeofday(&tv, NULL);
    ts.tv_sec = time(NULL) + ms / 1000;
    ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (ms % 1000);
    ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
    ts.tv_nsec %= (1000 * 1000 * 1000);

    pthread_mutex_lock(&mutex);
    n = pthread_cond_timedwait(&cond, &mutex, &ts);
    pthread_mutex_unlock(&mutex);

    /*pthread_mutex_lock(&mutex);
    int rc = 0;
    while (!predicate && rc == 0)
      rc = pthread_cond_timedwait(&cond, &mutex, &ts);
    pthread_mutex_unlock(&mutex);*/

    /*if (rc == 0) {
        return 0;
    } else if (rc == ETIMEDOUT) {
        return 1;
    }*/

    if (n == 0) {
        return 0;
    } else if (n == ETIMEDOUT) {
        return 1;
    }
}

void* gravity(){

  char input = 's';
  int dummy_score = 0;
  int soft_drop = 0;
  while(1){
    do{
      if(waitMillis(1000 - level * 50)){
        moveBlock(b, input, grid, &dummy_score, &soft_drop);
      }
    }while(!blockHasCrashed(b));
  }
}

info* getBlockInfo(Block* b, int level, char** grid){
  info* block_info = malloc(sizeof(info));
  block_info->b = b;
  block_info->level = level;
  block_info->grid = grid;

  return block_info;
}
