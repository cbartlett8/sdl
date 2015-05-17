#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>

#include <SDL/SDL_thread.h>

static int counter = 0;
SDL_mutex *counter_mutex;

// The three threads will run until the flag is changed.
static int exit_flag = 0;

// This function is a thread entry point.
int ThreadEntryPoint(void *data)
{
  char *threadname;
  
  // Anything can be passed as thread data. We will use it as a thread name
  threadname = (char *) data;
  
  // loop until main() set the exit flag.
  while (exit_flag == 0)
  {
    printf("This is %s! ", threadname);
    
    // Get a lock on the counter variable
    SDL_LockMutex(counter_mutex);
    
    // We are safe to change the counter.
    printf("The counter is currently at: %i\n", counter);
    counter++;
    
    // release the lock
    SDL_UnlockMutex(counter_mutex);
    
    // Delay for a random amount of time.
    SDL_Delay(rand() % 3000);
  }
  
  printf("Thread %s is now exiting.\n", threadname);
  return 0;
}

int main()
{
  SDL_Thread *thread1, *thread2, *thread3;
  char *num1 = "HELLO";
  char *num2 = "SOUND";
  char *num3 = "PHYICS";
  
  // create a mutex to protect the counter.
  counter_mutex = SDL_CreateMutex();
  
  printf("Ctrl-C to exit this program.\n");
  
  // Creating three threads and their name is their data.
  thread1 = SDL_CreateThread(ThreadEntryPoint, num1);
  thread2 = SDL_CreateThread(ThreadEntryPoint, num2);
  thread3 = SDL_CreateThread(ThreadEntryPoint, num3);
  
  // We let the threads run until the counter gets to 20.
  while (counter < 20)
    SDL_Delay(1000);
    
  // Now we signal the threads to exit
  exit_flag = 1;
  printf("exit_flag has been sent by main().\n");
  
  // Give some time for the threads to exit.
  SDL_Delay(3000);
  
  // Destroy the counter mutex.
  SDL_DestroyMutex(counter_mutex);
  
  return 0;
}
