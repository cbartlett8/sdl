#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>

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
  
  // Update SDL that the screen has changed
  SDL_UpdateRect(screen, 0,0,0,0);
  
  // pause a couple of seconds.
  SDL_Delay(3000);
  
  // free the memory allocated to the bitmap
  SDL_FreeSurface(image);
  
  printf ("Everything works!\n");
  return 0;
}
