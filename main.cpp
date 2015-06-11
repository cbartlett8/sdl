#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>

class mario
{
  public:
    int a;
};

bool quit = false;
int time = 0;

/*
Animate_frame is a function that will take in the start_time of a 
frame of animation and the size of the image and if enough time 
has passed, pass back the updated x coords.
So we have a clock that is always ticking every time the loop runs.
It should be 60 times a second.
*/
int Animate_frame(int start_time, int size)
{
  return 10;
}

void loop()
{
  time++;
  if (time % 60 == 0)
    printf("Time is now: %i\n", time);
  

}

mario marios[50];


int main()
{
  SDL_Surface *screen;
  SDL_Surface *image;
  SDL_Rect src, dest;
  
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    printf("Unable to init sdl\n");
    return 1;
  }
  
  // Making sure the SDL_Quit gets called at exit.
  atexit(SDL_Quit);
  
  screen = SDL_SetVideoMode(640, 480, 16, SDL_NOFRAME);
  if (screen == NULL)
  {
    printf("Uable to draw the screen with SDL.\n");
    return 1;
  }
  
  // Load the image
  image = SDL_LoadBMP("mario-8-bit.bmp");
  if (image == NULL)
    printf("We were unable to load the iamge\n");
    
  // SDL needs to know how much data to draw to the screen, so we give
  // it a rectangle to do that.
  src.x = 0;
  src.y = 0;
  src.w = image->w;
  src.h = image->h;
  
  dest.x = 0;
  dest.y = 0;
  dest.w = image->w;
  dest.h = image->h;
  
  
  // Okay now that we got a screen we lock it.
  //SDL_LockSurface(screen);
  
  // Draw bitmap to screen
  SDL_BlitSurface(image, &src, screen, &dest);
  
  // Done Drawing? Now unlock the screen
  //SDL_UnlockSurface(screen);
  
  
  // Draw a colorkeyed bmp
  int colorkey = 0;
  colorkey = SDL_MapRGB(image->format, 255, 255, 255); // white?
  SDL_SetColorKey(image, SDL_SRCCOLORKEY, colorkey);
  src.x = 0;
  src.y = 0;
  src.w = image->w;
  src.h = image->h;
  dest.x = 350;
  dest.y = 0;
  dest.w = image->w;
  dest.h = image->h;
  SDL_BlitSurface(image, &src, screen, &dest);
  
  // Update SDL that the screen has changed
  SDL_UpdateRect(screen, 0,0,0,0);
  
  while (quit != true)
  {
    loop();
  }
  
  // pause a couple of seconds.
  SDL_Delay(1000);
  
  // free the memory allocated to the bitmap
  SDL_FreeSurface(image);
  
  printf ("Everything works!\n");
  return 0;
}
