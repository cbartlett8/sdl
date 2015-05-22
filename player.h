#ifndef PLAYER_H
#define PLAYER_H

#include "actor.h"

class Player : public Actor
{
  private:
    // The surfaces the player needed
    SDL_Surface *jump;
    SDL_Surface *shoot;
    SDL_Surface *slide;
    SDL_Surface *hit;
    
    // sounds
    
    // vitals
    int hp;
    
    // character state
    Character_State m_state;
  
  public:
    // Animate() helper function, when called will give the 
    // current frame so it can be blit.
    
};

#endif
