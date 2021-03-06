#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#define NUM_MARIO 100
#define MAX_SPEED 6

// The struct stores the infromation for a mario.
class Mario
{
  public: 
    int x, y;   // The position
    int dx, dy; // The movement vectors
};

Mario marios[NUM_MARIO]; // an array of mario objects

SDL_Surface *screen;
SDL_Surface *mario;

// We loop through the array to mario and set each to random values.
void init_marios()
{
  for (int i = 0; i < NUM_MARIO; i++)
  {
    marios[i].x = rand() % screen->w;
    marios[i].y = rand() % screen->h;
    marios[i].dx = (rand() % (MAX_SPEED * 2)) - MAX_SPEED;
    marios[i].dy = (rand() % (MAX_SPEED * 2)) - MAX_SPEED;
  }
} 

// Moving the marios on their vector.
void move_marios()
{
  for (int i = 0; i < NUM_MARIO; i++)
  {
    // move mario by motion vector.
    marios[i].x += marios[i].dx;
    marios[i].y += marios[i].dy;
    
    // turn mario back when he hits the side of the screen.
    if (marios[i].x < 0 || marios[i].x > screen->w - 1)
    {
      marios[i].dx = -marios[i].dx;
    }
    if (marios[i].y < 0 || marios[i].y > screen->h -1)
    {
      marios[i].dy = -marios[i].dy;
    }
  }
}

void draw_marios()
{
  SDL_Rect src, dest;
  
  for (int i = 0; i < NUM_MARIO; i++)
  {
    src.x = 0;
    src.y = 0;
    src.w = mario->w;
    src.h = mario->h;
    
    // We take into account that position is in the center of mario.
    dest.x = marios[i].x - mario->w/2;
    dest.y = marios[i].y - mario->h/2;
    dest.w = mario->w;
    dest.h = mario->h;
    
    SDL_BlitSurface(mario, &src, screen, &dest);
  }
}


int main()
{
  SDL_Surface *temp;
  SDL_Surface *background;
  SDL_Rect src, dest;
  SDL_Event event;
  int frames;
  
  // Init the SDL video system
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    printf("Could not init SDL?\n");
    return 1;
  } 
  
  // make sure we clean up after SDL
  atexit(SDL_Quit);
  
  // Attempting to set screen up
  screen = SDL_SetVideoMode(640,480, 16, SDL_DOUBLEBUF);
  if (screen == NULL)
  {
    printf("SDL could not set video mode.\n");
    return 1;
  } 
  
  // load the bitmap files
  temp = SDL_LoadBMP("background.bmp");
  if (temp != NULL)
  {
    background = SDL_DisplayFormat(temp);
    if (background == NULL)
    {
      printf("background failed to load.\n");
      return 1;
    }
  }
  else
  {
    printf("ERROR: Could not load: background.bmp");
    return 1;
  }
  SDL_FreeSurface(temp);
  
  temp = SDL_LoadBMP("ttt.bmp");
  if (temp != NULL)
  {
    mario = SDL_DisplayFormat(temp);
    if   (mario == NULL)
    {
      printf("Failed to load ttt.bmp");
      return 1;
    }
  }
  else
  {
    printf("ERROR: Could not load: ttt.bmp");
  }
  SDL_FreeSurface(temp);
  
  // init the mario
  init_marios();
  
  /* animate 300 frames
  for (frames = 0; frames < 300; frames++)
  {
    // draw the background
    src.x = 0;
    src.y = 0;
    src.w = background->w;
    src.h = background->h;
    dest = src;
    
    SDL_BlitSurface(background, &src, screen, &dest);
    
    // draw the marios to the screen.
    draw_marios();
    
    // ask sdl to update the screen.
    SDL_Flip(screen);
    
    // preform the movements
    move_marios();
  }
  */
  
  // update the events action.
  while (SDL_WaitEvent(&event) != 0)
  {
    SDL_keysym keysym;
    switch(event.type)
    {
      case SDL_KEYDOWN:
        printf("Key Pressed: ");
        keysym = event.key.keysym;
        printf("SDL keysym is %i. ", keysym.sym);
        printf("(%s) ", SDL_GetKeyName(keysym.sym));
        
        // report a left shift modifier
        if (event.key.keysym.mod & KMOD_LSHIFT)
          printf("Left SHIFT is DOWN.\n");
        else
          printf("Left SHIFT is UP.\n");
          
        // Did the user press  q?
        if (keysym.sym == SDLK_q)
        {
          printf("'Q' pressed, exiting!\n");
          exit(0);
        } 
        
        break;
        
      case SDL_KEYUP:
        printf("Key released. ");
        printf("SDL keysym is %i. ", keysym.sym);
        printf("(%s) ", SDL_GetKeyName(keysym.sym));
        
        if (event.key.keysym.mod & KMOD_SHIFT)
          printf("Left Shift is down.\n");
        else 
          printf("Left Shift is UP.\n");
        break;
      
      case SDL_MOUSEMOTION:
        printf("Mouse Motion. ");
      
        // SDL Provides the current position.
        printf("New Position is (%i, %i).", event.motion.x, event.motion.y);
      
        printf("A change of (%i, %i).\n", event.motion.xrel, event.motion.yrel);
        break;
      
      case SDL_MOUSEBUTTONDOWN:
        printf("Mouse button pusheddown");
        printf("Button %i at (%i, %i)\n", 
        event.button.button, event.button.x, event.button.y);
        break;
      
      case SDL_QUIT:
        printf("Quit event. Bye!\n");
        exit(0);
    }
  }
  
  // Free up the memory we allocated.
  SDL_FreeSurface(background);
  SDL_FreeSurface(mario);
  printf("Hey we got through the program.\n");
  return 0;
} 
