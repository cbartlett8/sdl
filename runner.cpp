// g++ -Wall -pedantic runner.cpp -lSDL -lSDL_ttf
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <fstream>
#include <list>

SDL_Surface* Init_image(std::string name);

// structure for loading sounds
typedef struct sound_s
{
  Uint8 *samples; // raw pcm sample data
  Uint32 length;   // size of the data in bytes.
} sound_t, *sound_p;

// Structure for a currently playing sound
typedef struct playing_s
{
  int active;     // 1 if the sound should be played.
  sound_p sound;  // sound data to play
  Uint32 position;   // current position in the sound buffer.
} playing_t, *playing_p;

// An array for all active sound effects.
#define MAX_PLAYING_SOUNDS 10
playing_t playing[MAX_PLAYING_SOUNDS];

// Volume. If the value gets too high it causes distrotion
#define VOLUME_PER_SOUND SDL_MIX_MAXVOLUME / 2

// This function is called by SDL when ever the sound card
// needs more samples to play. It might be called from a 
// seperate thread so BE CAREFUL!
void AudioCallback(void *user_data, Uint8 *audio, int length)
{
  // clear the audio buffer so we can mix samples into it.
  memset(audio, 0, length);
  
  // Mix in each sample/sound
  for (int i = 0; i < MAX_PLAYING_SOUNDS; i++)
  {
    if (playing[i].active)
    {
      Uint8 *sound_buf;
      Uint32 sound_len;
      
      // locate this sounds current buffer position.
      sound_buf = playing[i].sound->samples;
      sound_buf += playing[i].position;
      
      // Determine the number of samples to mix.
      if ((playing[i].position + length) > 
        playing[i].sound->length)
      {
        sound_len = playing[i].sound->length - playing[i].position;
      }
      else        
      {
        sound_len = length;
      }
      
      // Mix this sound into the stream
      SDL_MixAudio(audio, sound_buf, sound_len, VOLUME_PER_SOUND);
      
      // Update the sound buffers position
      playing[i].position += length;
      
      // Have we reached the end of the sound.
      if (playing[i].position >= playing[i].sound->length)
      {
        playing[i].active = 0;    // marking the completed sound as inactive.
      }
    }
  }
}

// LoadAndConvertSound loads a sound with SDL_LoadWAV and converts
// it to the specified sample format. Returns 0 on success and 1
// on failure.
int LoadAndConvertSound(char const *filename, SDL_AudioSpec *spec, sound_p sound)
{
  SDL_AudioCVT cvt; // format conversion structure.
  SDL_AudioSpec loaded; // format of the loaded data.
  Uint8 *new_buf;
  
  // load the WAV file in the original sample format
  if (SDL_LoadWAV(filename, &loaded, &sound->samples,
    &sound->length) == NULL)
  {
    printf("Unable to load sound: %s\n", SDL_GetError());
    return 1;
  } 
  
  // Now we build a conversion structure for convert the samples.
  if (SDL_BuildAudioCVT(&cvt, loaded.format, loaded.channels, loaded.freq, 
    spec->format, spec->channels, spec->freq) < 0)
  {
    printf("Unable to convert the sound: %s\n", SDL_GetError());
    return 1;
  }
  
  // Since converting PCM samples can result in more data we need
  // to allocate a new buffer for the converted data. 
  // Fortunately SDL_BuildAudioCVT supplied the nessary information.
  cvt.len = sound->length;
  new_buf = (Uint8 *)malloc(cvt.len * cvt.len_mult);
  if (new_buf == NULL)
  {
    printf("Sound Memory allocation failed.\n");
    SDL_FreeWAV(sound->samples);
    return 1;
  }
  
  // copy the sound samples into the new buffer
  memcpy(new_buf, sound->samples, sound->length);
  
  // perform the conversion into the new buffer
  cvt.buf = new_buf;
  if (SDL_ConvertAudio(&cvt) < 0)
  {
    printf("Audio conversion error: %s\n", SDL_GetError());
    free(new_buf);
    SDL_FreeWAV(sound->samples);
    return 1;
  }
  
  // swap the converted data for the original
  SDL_FreeWAV(sound->samples);
  sound->samples = new_buf;
  sound->length = sound->length * cvt.len_mult;
  
  // Success
  printf("'%s' was loaded and converted successfully!\n", filename);
  return 0;
}

// remove all currently playing sounds
void ClearPlayingSounds()
{
  for (int i = 0; i < MAX_PLAYING_SOUNDS; i++)
  {
    playing[i].active = 0;
  }
}

// Add a sound to the list of currently playing sounds.
// AudioCallback will start mixing this sound in to the stream
// the next time it is called
int PlaySound(sound_p sound)
{
  int i = 0;
  // find an empty slot in the array for this new sound
  for (i = 0; i < MAX_PLAYING_SOUNDS; i++)
  {
    if (playing[i].active == 0)
    {
      break;
    } 
  }
  
  // We found an empty place in the array its i.
  if (i == MAX_PLAYING_SOUNDS)
    return 1;
    
  // THe playing structures are acced by the 
  // audio callback, so we should obtain a lock BEFORE
  // we access them.
  SDL_LockAudio();
  playing[i].active = 1;
  playing[i].sound = sound;
  playing[i].position = 0;
  SDL_UnlockAudio();
  
  return 0;
}
// !------------------------------------------------------------------------------
// This is just setting up the images that will be appearing in the game.
SDL_Surface *background;
SDL_Surface *mario;
SDL_Surface *mario2;
SDL_Surface *screen;
SDL_Surface *temp;
SDL_Surface *bullet;
SDL_Surface *h_hit;
SDL_Rect src, dest;
SDL_Event event;
TTF_Font *gFont;
int quit_flag = 0;



// our loaded sounds and their formats
sound_t cannon, explosion;

#define FLOOR 100
#define SPEED 30

// Basically stolen from tysons game object. Just checks for 
// overlaps between two rectangles/squares.
static bool overlap(int ax1, int ay1, int ax2, int ay2,
                    int bx1, int by1, int bx2, int by2
                   )
{
  return !(ax2 < bx1 || ax1 > bx2 || ay1 > by2 || ay2 < by1);
}

enum Character_State
{
  RUNNING = 1,
  JUMPING = 2,
  SHOOTING = 3,
  HIT = 4,
  SLIDING = 5,
  JUMP_SHOOT = 6
}; 

struct Coords
{
  int m_x;
  int m_y;
  int m_w;
  int m_h;
  int m_max_frame;
  
  Coords(int x, int y, int w, int h, int max_frame)
  {
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    m_max_frame = max_frame;
  }
  void print()
  {
    printf("x: %i y: %i w: %i h: %i m_f: %i\n", m_x, m_y, m_w, m_h, m_max_frame);
  }
};

class Bullet
{
  public:
  int m_x;
  int m_y;
  int m_w;
  int m_h;
  int m_current_x;
  int m_current_y;
  int m_speed;
  int m_size;
  Bullet(int x, int y, int w, int h, int cur_x, int cur_y, int speed, int size)
  {
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    m_current_x = cur_x;
    m_current_y = cur_y;
    m_speed = speed;
    m_size = size;
  }
  void Print()
  {
    std::cout << "Bullet vitals: x: " << m_current_x << " y: " << m_current_y << std::endl;
  }
};

class Actor
{
  private:
    SDL_Surface *m_hit;
    Coords *c_hit;
    SDL_Surface *m_run;
    Coords *c_run;
    SDL_Surface *m_jump;
    Coords *c_jump;
    SDL_Surface *m_jump_shoot;
    Coords *c_jump_shoot;
  public:
    int m_x;
    int m_y;
    int m_w;
    int m_h;
    int m_current_frame;
    int m_max_frame;
    std::string m_sound;
    int m_hp;
    int m_score;
    Character_State m_state;
    int m_state_flag;
    int m_current_x;
    int m_current_y;
    void set_image_hit(SDL_Surface *surface)
    {
      m_hit = surface;
    }
    void set_image_run(SDL_Surface *surface)
    {
      m_run = surface;
    }
    void set_image_jump(SDL_Surface *surface)
    {
      m_jump = surface;
    }
    void set_coords_hit(int x, int y, int w, int h, int max_frame)
    {
      c_hit = new Coords(x, y, w, h, max_frame);
    }
    void set_coords_run(int x, int y, int w, int h, int max_frame)
    {
      c_run = new Coords(x, y, w, h, max_frame);
    }
    void set_coords_jump(int x, int y, int w, int h, int max_frame)
    {
      c_jump = new Coords(x, y, w, h, max_frame);
    }
    SDL_Surface *get_image_hit()
    {
      return m_hit;
    }
    SDL_Surface *get_image_run()
    {
      return m_run;
    }
    SDL_Surface *get_image_jump()
    {
      return m_jump;
    }
    SDL_Surface *get_image_jump_shoot()
    {
      return m_jump_shoot;
    }
    Coords *get_hit_coords()
    {
      return c_hit;
    }
    Coords *get_run_coords()
    {
      return c_run;
    }
    Coords *get_jump_coords()
    {
      return c_jump;
    }
    Coords *get_jump_shoot_coords()
    {
      return c_jump_shoot;
    } 
    
    Actor(int x, int y, int w, int h, int frame, int max_frame, std::string sound, int hp, int score, Character_State state, int flag, int current_x, int current_y)
    {
      m_x = x;
      m_y = y;
      m_w = w;
      m_h = h;
      m_current_frame = frame;
      m_max_frame = max_frame;
      m_sound = sound;
      m_hp = hp;
      m_score = score;
      m_state = state;
      m_state_flag = flag;
      m_current_x = current_x;
      m_current_y = current_y;
    }
    
    bool touches(Bullet *bullet)
    {
      // true if the bounding boxes of this and obj overlap
      return overlap(m_current_x, m_current_y, m_current_x + m_w, 
        m_current_y + m_h, bullet->m_current_x, bullet->m_current_y, 
        bullet->m_current_x + bullet->m_w, bullet->m_current_y + bullet->m_h);
    }
    /*
    bool touches(Collectible *bullet)
    {
      // true if the bounding boxes of this and obj overlap
      return overlap(m_current_x, m_current_y, m_current_x + m_w, 
        m_current_y + m_h, bullet->m_current_x, bullet->m_current_y, 
        bullet->m_current_x + bullet->m_w, bullet->m_current_y + bullet->m_h);
    }
    */
};

// Lists to deal with the variable amount of actors on screen.
std::list<Bullet*> Bullets;
std::list<Bullet*>::iterator iter;
std::list<Actor*> Actors;
std::list<Actor*>::iterator it;
std::string set = "Hello";
Actor m_char(0, 0, 20, 20, 0, 1, set, 30, 100, RUNNING, 0, 0, FLOOR);
//Bullet m_bullet(0,0,5,5,0,0, 2, 0);

/*
Filename_Grab(int level) is a function that looks in the file 
dictated by level 0 = beg, 1 = int, 2 = master and pulls out
a file at random to give to Actor_Generate(FILENAME)
*/
bool Filename_Grab(int level)
{
  std::ifstream ifs;
  char c[255];
  // Okay we are going to grab at random a text file.
  switch (level)
  {
    case 1:  
      ifs.open("random_list.txt");
  
      if (!ifs)
      {
        std::cerr << "Could not open file." << std::endl;
        return false;
      }
      
      // Okay we don't know how many lines we are going to have
      // so we need to read in all the lines. Put those lines into
      // a linked list and then randomly pick one of the lines
      // from the linked list.
      // v1 = rand() % 100 + 1   // v1 in the range of 1 to 100.
      // std::list<char *> char_arrays;
      // char_arrays.size() to get the size of the list.
      // so v1 = rand() & size_of_list + 1;
      // std::list<char*>::iterator it;
      // int i = 0;
      // for (it.char_arrays.begin(); i < v1; i++)
      // it++;
      // after that we should be at the correct place and we 
      // dereference the iterator and grab the char array.
      // cout << *it << endl;
      ifs.getline(c, 255);
      std::cout << c << std::endl;
      
      ifs.close();
      break;
    default:
      printf("Filename_grab got passed a weird value.\n");
      return false;
  }
  return true;
}

/* 
Actor_Generate(FILENAME) is a function that takes in a
text file and uses the data in the file to add actor objects
to the global actors list. 
*/ 
int Convert(char c)
{
  std::cout << "Convert c got: " << c << std::endl;
  switch(c)
  {
    case '0':
      return 0;
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    case '7':
      return 7;
    case '8':
      return 8;
    case '9':
      return 9;
    default:
      return -1;
  }
}

class Collectible
{
  public:
  int type;   // 1 for heart, 2 for coin
  int value;  // 1-100 it just depends on the type.
  SDL_Surface *s_run;
  Coords *c_run;
  SDL_Surface *s_hit;
  Coords *c_hit;
  Character_State m_state;
  
  int m_current_x;
  int m_current_y;
  
  
  void set_run(SDL_Surface *temp)
  {
    s_run = temp;
  }
  void set_hit(SDL_Surface *temp)
  {
    s_hit = temp;
  }
  SDL_Surface *get_run()
  {
    return s_run;
  }
  SDL_Surface *get_hit()
  {
    return s_hit;
  }
  void set_coords_run(int x, int y, int w, int h, int max_frame)
  {
    c_run = new Coords(x,y,w,h,max_frame);
  }
  void set_coords_hit(int x, int y, int w, int h, int max_frame)
  {
    c_hit = new Coords(x, y, w, h, max_frame);
  }
  Coords *get_coords_run()
  {
    return c_run;
  }
  Coords *get_coords_hit()
  {
    return c_hit;
  }
  void set_state(Character_State temp)
  {
    m_state = temp;
  }
  void print()
  {
    std::cout << "Collectible is holding: " << std::endl;
    std::cout << "x: " << m_current_x << " y: " << m_current_y << std::endl;
    if (s_run != NULL)
    {
      std::cout << "run: " << std::endl;
      c_run->print();
    }
    if (s_hit != NULL)
    {
      std::cout << " hit: " << std::endl;
      c_hit->print();
    }
  }
};
std::list<Collectible*> Collectibles;
std::list<Collectible*>::iterator col_iter;

bool Actor_Generate(std::string filename)
{
  // We grab the first character to dictate the type of class
  std::ifstream ifs;
  
  ifs.open(filename.c_str());
  
  if (!ifs)
  {
    std::cerr << "Could not open file: " << filename << std::endl;
    return false;
  }
  
  char c[256];
  int ia = 0;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  int max_frames = 0;
  int x_1, x_2, x_3;
  
  // Every line we are taking in is a description of what the
  // object we want to display is.
  while(ifs.getline(c, 256) && c[0] != 'E')
  {
    Collectible *temp = new Collectible();
    SDL_Surface *sur_hit;
    SDL_Surface *sur_run;
    
    ia = c[0] - '0';
    printf("Version: %i\n", ia);
    std::cout << c << std::endl;
    switch (ia)
    {
      case 3:   // This is the level for the enemies. Probably just a script and stuff
        printf("We have a three.\n");
        break;
        
      case 2:  // This is the level for the coins and collectibles.
        temp->set_state(RUNNING);
        
        // Basically here just for hit animation.
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        x = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("x: %i\n", x);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        y = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("y: %i\n", y);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        w = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("w: %i\n", w);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        h = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("h: %i\n", h);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        max_frames = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("max_frames: %i\n", max_frames);
        
        temp->set_coords_hit(0, 0, w, h, max_frames);
        // set x to m_current_x
        temp->m_current_x = x;
        // set y to m_current_y
        temp->m_current_y = y;
        
        // surface is the file
        ifs.getline(c, 256);
        sur_hit = Init_image(c);
        assert(sur_hit != NULL);
        temp->set_hit(sur_hit);
        
        temp->print();
      
      case 1:  // This is the level for like platforms.
      
        //Actor *temp = new Actor();
        // So this level takes in: x, y, w, h, Surface_name
        ifs.getline(c, 256);
        int x_1, x_2, x_3;

        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        x = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("x: %i\n", x);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        y = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("y: %i\n", y);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        w = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("w: %i\n", w);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        h = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("h: %i\n", h);
        
        ifs.getline(c, 256);
        x_1 = Convert(c[0]);
        x_2 = Convert(c[1]);
        x_3 = Convert(c[2]);
        max_frames = (x_1 * 100) + (x_2 * 10) + x_3;
        printf("max_frames: %i\n", max_frames);
        
        temp->set_coords_run(0, 0, w, h, max_frames);
        
        // set m_current_x to x 
        temp->m_current_x = x;
        // set m_current_y to y
        temp->m_current_y = y; 
        
        // surface is the file
        ifs.getline(c, 256);
        sur_run = Init_image(c);
        assert(sur_run != NULL);
        temp->set_run(sur_run);
        
        temp->print();
        Collectibles.insert(Collectibles.begin(), temp);
        break;
      default:
        printf("Unknown NUMBER ignoring.\n");
    }
    //int b = ia * 5;
    //printf("b: %i\n", b);
  }
  printf("Exiting Actor_Generate()\n");
  ifs.close();
  return true;
}


// This is our main loop. 
void init_img()
{
  // Draw the background
  src.x = 0;
  src.y = 0;
  src.w = background->w;
  src.h = background->h;
  dest = src;
    
  SDL_BlitSurface(background, &src, screen, &dest);
  
  // Draw other actors / collectibles
  if (Collectibles.empty() != true)
  {
    Coords *temp;
    for (col_iter = Collectibles.begin(); col_iter != Collectibles.end(); col_iter++)
    {
      if ((*col_iter)->m_state == RUNNING)
      {
        (*col_iter)->m_current_x -= 1;
        temp = (*col_iter)->get_coords_run();
        src.x = temp->m_x;
        src.y = temp->m_y;
        src.w = temp->m_w;
        src.h = temp->m_h;
        dest.x = (*col_iter)->m_current_x;
        dest.y = (*col_iter)->m_current_y;
        dest.w = temp->m_w;
        dest.h = temp->m_h;
        SDL_BlitSurface((*col_iter)->get_run(), &src, screen, &dest);
      }
      else if ((*col_iter)->m_state == HIT)
      {
        // We don't run anymore and play the hit animation
        // Then we delete the object.
      }
      
      
      if ((*col_iter)->m_current_x < -(temp->m_w))
      {
        // We erase the object.
        printf("We are erasing the object\n");
        delete *col_iter;
        col_iter = Collectibles.erase(col_iter);
      }
    }
  }
    
  // Draw bullets
  // See if the bullets is empty?
  if (Bullets.empty() != true)
  {
    // iterate through the list 
    for (iter=Bullets.begin(); iter != Bullets.end(); iter++)
    {
      // update the bullet location
      (*iter)->m_current_x += (*iter)->m_speed;
      
      src.x = (*iter)->m_x;
      src.y = (*iter)->m_y;
      src.w = (*iter)->m_w;
      src.h = (*iter)->m_h;
      dest.x = (*iter)->m_current_x;
      dest.y = (*iter)->m_current_y;
      dest.w = (*iter)->m_w;
      dest.h = (*iter)->m_h;
      
      // draw the bullet to the screen
      SDL_BlitSurface(bullet, &src, screen, &dest);
      
      // check if we have collision 
      bool touch_check = m_char.touches(*iter);
      
      // delete the bullet if we go off the screen or touch m_char.
      if ((*iter)->m_current_x > 500 || (*iter)->m_current_x < 0 || touch_check == true)
      {
        if (touch_check == true)
        {
          m_char.m_state = HIT;
          (*iter)->Print();
        }
        delete *iter;
        iter = Bullets.erase(iter);
      }
      
      /*
      // Check for collisions between the bullet and the m_char
      if (m_char.touches(*iter) == true)
      {
        m_char.m_state = HIT;
        (*iter)->Print();
        delete *iter;
        iter = Bullets.erase(iter);
      } 
      */
    }
  }
    
    
  // Draw the main character jumping and shooting
  if (m_char.m_state == JUMP_SHOOT)
  {
    if (m_char.m_current_y > 0 && m_char.m_state_flag == 0) // going up
    {
      m_char.m_current_y -= 1;
      //printf("Going up?\n");
    }
    if (m_char.m_current_y < 5) // near the top? Start going down...
    {
      m_char.m_state_flag = 1;
      //std::cout << "GOing up?" << std::endl;
    }
    if (m_char.m_state_flag == 1) // going down
    {
      m_char.m_current_y += 1;
      //std::cout << "GOing down" << std::endl;
    }
    if (m_char.m_current_y >= FLOOR && m_char.m_state_flag == 1) // back at the bottom
    {
      m_char.m_state = RUNNING;
      m_char.m_state_flag = 0;
    }
  }
  // Draw the main character
  //printf("The current_frame: %i\n", m_char.m_current_frame);
  // calculate the movement
  if (m_char.m_state == JUMPING)
  {
    Coords *jump_coords = m_char.get_jump_coords();
    /*
    if (m_char.m_current_frame < (jump_coords->m_max_frame * SPEED))
    {
      // Display the character
      if (m_char.m_current_frame / SPEED < 1)
      {
        src.x = jump_coords->m_x;
      }
      else if (m_char.m_current_frame / SPEED < 2)
      {
        src.x = jump_coords->m_x + 64;
      }
      else if (m_char.m_current_frame / SPEED < 3)
      {
        src.x = jump_coords->m_x + 128;
      }
      else if (m_char.m_current_frame / SPEED < 4)
      {
        src.x = jump_coords->m_x + 192;
      }
      else if (m_char.m_current_frame / SPEED < 5)
      {
        src.x = jump_coords->m_x + 256;
      }
      else if (m_char.m_current_frame / SPEED < 6)
      {
        src.x = jump_coords->m_x + 320;
      } 
      //src.x = run_coords->m_x + (m_char.m_current_frame * 64);
      src.y = jump_coords->m_y;
      src.w = jump_coords->m_w;
      src.h = jump_coords->m_h;
      dest.x = m_char.m_current_x;
      dest.y = m_char.m_current_y;
      dest.w = m_char.get_image_jump()->w;
      dest.h = m_char.get_image_jump()->h;
      SDL_BlitSurface(m_char.get_image_jump(), &src, screen, &dest);
      m_char.m_current_frame += 1;
    }
    else
    {
      m_char.m_current_frame = 0;
    } 
    */
    if (m_char.m_current_y > 0 && m_char.m_state_flag == 0) // going up
    {
      m_char.m_current_y -= 1;
      if (m_char.m_current_frame < 60)
        src.x = jump_coords->m_x;
      else if (m_char.m_current_frame < 60 * 2)
        src.x = jump_coords->m_x + 64;
      else if (m_char.m_current_frame < 60 * 3)
        src.x = jump_coords->m_x + 128;
      src.y = jump_coords->m_y;
      src.w = jump_coords->m_w;
      src.h = jump_coords->m_h;
      dest.x = m_char.m_current_x;
      dest.y = m_char.m_current_y;
      dest.w = m_char.get_image_jump()->w;
      dest.h = m_char.get_image_jump()->h;
      SDL_BlitSurface(m_char.get_image_jump(), &src, screen, &dest);
      m_char.m_current_frame += 1;
    }
    if (m_char.m_current_y < 5) // near the top
    {
      m_char.m_state_flag = 1;
      m_char.m_current_frame = 0;
    }
    if (m_char.m_state_flag == 1) // going down
    {
      m_char.m_current_y += 1;
      if (m_char.m_current_frame < 60)
        src.x = jump_coords->m_x + 192;
      else if (m_char.m_current_frame < 60 * 2)
        src.x = jump_coords->m_x + 256;
      else if (m_char.m_current_frame < 60 * 3)
        src.x = jump_coords->m_x + 320;
      src.y = jump_coords->m_y;
      src.w = jump_coords->m_w;
      src.h = jump_coords->m_h;
      dest.x = m_char.m_current_x;
      dest.y = m_char.m_current_y;
      dest.w = m_char.get_image_jump()->w;
      dest.h = m_char.get_image_jump()->h;
      SDL_BlitSurface(m_char.get_image_jump(), &src, screen, &dest);
      m_char.m_current_frame += 1;
    }
    if (m_char.m_current_y >= FLOOR && m_char.m_state_flag == 1) // back on the ground.
    {
      m_char.m_state = RUNNING;
      m_char.m_state_flag = 0;
      m_char.m_current_frame = 0;
    }
  }
  
  // check if the character fired a bullet.
  if (m_char.m_state == SHOOTING)
  {
    
    // Play the animation of the character shooting the bullet.
    /*
    // Spawn the bullet
    src.x = m_bullet.m_x;
    src.y = m_bullet.m_y;
    src.w = m_bullet.m_w;
    src.h = m_bullet.m_h;
    dest.y = m_bullet.m_current_y;
    dest.w = m_bullet.m_w;
    dest.h = m_bullet.m_h;
    // move the bullet
    if (m_bullet.m_current_x < screen->w)
    {
      m_bullet.m_current_x += 1;
      dest.x = m_bullet.m_current_x;
      SDL_BlitSurface(bullet, &src, screen, &dest);
      printf("Shooting bullets.\n");
    }
    else
    {
      // despawn the bullet when it gets off screen.
      m_bullet.m_current_y = -10;
      m_bullet.m_current_x = -10;
      m_char.m_state = RUNNING;
    }
    */
    m_char.m_state = RUNNING;
  }
  
  // check if player was hit
  if (m_char.m_state == HIT)
  {
    if ((m_char.m_current_frame / 60) < 1)
    {
      //printf("hit action state frame 0\n");
      //std::cout << "Image_hit: " << m_char.get_image_hit() << std::endl;
      // Calculate the src rectangle
      src.x = 0;
      src.y = 0;
      src.w = 20;
      src.h = 20;
      dest.x = m_char.m_current_x;
      dest.y = m_char.m_current_y;
      dest.w = 20;
      dest.h = 20;
      assert(m_char.get_image_hit() != NULL);
      SDL_BlitSurface(m_char.get_image_hit(), &src, screen, &dest);
      //SDL_BlitSurface(h_hit, &src, screen, &dest);
      m_char.m_current_frame += 1;
    }
    else if ((m_char.m_current_frame / 60) < 2)
    {
      src.x = 20;
      src.y = 0;
      src.w = 20;
      src.h = 20;
      dest.x = m_char.m_current_x;
      dest.y = m_char.m_current_y;
      dest.w = 20;
      dest.h = 20;
      assert(m_char.get_image_hit() != NULL);
      SDL_BlitSurface(m_char.get_image_hit(), &src, screen, &dest);
      //SDL_BlitSurface(h_hit, &src, screen, &dest);
      m_char.m_current_frame += 1;
    }
    else if ((m_char.m_current_frame / 60) < 3)
    {
      src.x = 40;
      src.y = 0;
      src.w = 20;
      src.h = 20;
      dest.x = m_char.m_current_x;
      dest.y = m_char.m_current_y;
      dest.w = 20;
      dest.h = 20;
      assert(m_char.get_image_hit() != NULL);
      SDL_BlitSurface(m_char.get_image_hit(), &src, screen, &dest);
      //SDL_BlitSurface(h_hit, &src, screen, &dest);
      m_char.m_current_frame += 1;
    }
    else
    {
      m_char.m_current_frame = 0;
    }
  }
  
  // running
  //if (m_char.m_state == RUNNING || m_char.m_state == JUMPING || m_char.m_state == JUMP_SHOOT)
  if (m_char.m_state == RUNNING || m_char.m_state == JUMP_SHOOT)
  {
    //m_char.m_current_frame = 128;
    Coords *run_coords = m_char.get_run_coords();
    if (m_char.m_current_frame < (run_coords->m_max_frame * SPEED))
    {
      // Display the character
      if (m_char.m_current_frame / SPEED < 1)
      {
        src.x = run_coords->m_x;
      }
      else if (m_char.m_current_frame / SPEED < 2)
      {
        src.x = run_coords->m_x + 64;
      }
      else if (m_char.m_current_frame / SPEED < 3)
      {
        src.x = run_coords->m_x + 128;
      }
      else if (m_char.m_current_frame / SPEED < 4)
      {
        src.x = run_coords->m_x + 192;
      }
      //src.x = run_coords->m_x + (m_char.m_current_frame * 64);
      src.y = run_coords->m_y;
      src.w = run_coords->m_w;
      src.h = run_coords->m_h;
      dest.x = m_char.m_current_x;
      dest.y = m_char.m_current_y;
      dest.w = m_char.get_image_run()->w;
      dest.h = m_char.get_image_run()->h;
      SDL_BlitSurface(m_char.get_image_run(), &src, screen, &dest);
      m_char.m_current_frame += 1;
    }
    else
    {
      m_char.m_current_frame = 0;
    } 
    /*
    if (m_char.m_current_frame == 0)
    {
      src.x = m_char.m_x;
      src.y = m_char.m_y;
      src.w = mario->w;
      src.h = mario->h;
      dest.x = m_char.m_current_x;
      dest.y = m_char.m_current_y;
      dest.w = mario->w;
      dest.h = mario->h;
      
      SDL_BlitSurface(mario, &src, screen, &dest);
      m_char.m_current_frame += 1;
    }
    else
    {
      src.x = m_char.m_x;
      src.y = m_char.m_y;
      src.w = mario2->w;
      src.h = mario2->h;
      dest.x = m_char.m_current_x;
      dest.y = m_char.m_current_y;
      dest.w = mario2->w;
      dest.h = mario2->h;
      SDL_BlitSurface(mario2, &src, screen, &dest);
      m_char.m_current_frame = 0;
    }
    */
    
  }
  
  // Draw the HUD here as it has to be above everything else.
  SDL_Color color;
  color.r = 255;
  color.g = 165;
  color.b = 0;
  SDL_Surface *HUD;
  HUD = TTF_RenderText_Solid(gFont, "This is text", color);
  if (HUD == NULL)
  {
    std::cerr << "Could not render text in main loop." << std::endl;
  }
  src.x = 0;
  src.y = 0;
  src.w = HUD->w;
  src.h = HUD->h;
  dest.x = 0;
  dest.y = 0;
  dest.w = HUD->w;
  dest.h = HUD->h;
  SDL_BlitSurface(HUD, &src, screen, &dest);
  SDL_Flip(screen);
}

void CheckForInput()
{
  // Start the event loop.
  //while (SDL_WaitEvent(&event) != 0 && quit_flag == 0)
  while (SDL_PollEvent(&event) != 0 && quit_flag == 0)
  {
    SDL_keysym keysym;
    
    switch(event.type)
    {
      case SDL_KEYDOWN:
        keysym = event.key.keysym;
        
        if (keysym.sym == SDLK_h)
        {
          printf("The 'h' key was pressed.\n");
          m_char.m_state = HIT;
          m_char.m_current_frame = 0;
        } 
        
        if (keysym.sym == SDLK_a)
        {
          printf("The 'a' key was pressed.\n");
          if (m_char.m_current_y == FLOOR)
          {
            m_char.m_state = SHOOTING;
          }
          else
          {
            m_char.m_state = JUMP_SHOOT;
          }
          Bullet *temp = new Bullet(0,0,5,5,
          m_char.m_current_x + 30,
          m_char.m_current_y, 2, 0);
          //temp->m_current_x = m_char.m_current_x + 30;
          //temp->m_current_y = m_char.m_current_y;
          Bullets.insert(Bullets.begin(), temp);
        }
        
        if (keysym.sym == SDLK_q)
        {
          printf("'Q' was pressed so we exit.\n");
          quit_flag = 1;
        }
        
        if (keysym.sym == SDLK_c)
        {
          printf("'c' pressed so fire the cannon.\n");
          PlaySound(&cannon);
          Bullet *temp = new Bullet(0, 0, 5,5, m_char.m_current_x + 400,
            m_char.m_current_y, -2, 0);
          Bullets.insert(Bullets.begin(), temp);
        }
        
        if (keysym.sym == SDLK_x)
        {
          if (!Actor_Generate("./test.txt"))
            printf("Actor_Generate messed up!!!!\n");
          
        }
        
        if (keysym.sym == SDLK_g)
        {
          if (Filename_Grab(1) == false)
          {
            printf("Something bad happened in filename_grab()\n");
          }
        } 
        
        if (keysym.sym == SDLK_e)
        {
          printf("'e' explosion!!!\n");
          PlaySound(&explosion);
        }
        
        if (keysym.sym == SDLK_SPACE)
        {
          printf("'spacebar was pressed'\n"); 
          m_char.m_state = JUMPING;
        }
        
        break;
        
        case SDL_QUIT:
          printf("Quit event!\n");
          quit_flag = 1;
    } 
  }
}

SDL_Surface* Init_image(std::string name)
{
  SDL_Surface *surface;
  // load the bitmap files
  temp = SDL_LoadBMP(name.c_str());
  if (temp != NULL)
  {
    surface = SDL_DisplayFormat(temp);
    if (surface == NULL)
    {
      printf("background failed to load.\n");
      return NULL;
    }
  }
  else
  {
    std::cout << "ERROR: Could not load image: " << name << std::endl;
    return NULL;
  }
  SDL_FreeSurface(temp);
  return surface;
}

int main()
{    
  // audio format specifications
  SDL_AudioSpec desired, obtained;
  
  // Init SDL's video and audio subsystems. Have to have both.
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
  {
    printf("Unable to initialize SDL: %s\n", SDL_GetError());
    return 1;
  }
  
  // make sure SDL_Quit gets called when the program exits
  atexit(SDL_Quit);
  
  // also need to call close audio before we exit.
  atexit(SDL_CloseAudio);
  
  // Set window manager name and icon.
  SDL_Surface *icon = Init_image("./image/icon.bmp");  // needs to be 32x32pix
  //Uint32 colorkey = SDL_MapRGB(icon->format, 255,0,255);
  //SDL_SetColorKey(icon, SDL_SRCCOLORKEY, colorkey);
  SDL_WM_SetIcon(icon, NULL);
  SDL_WM_SetCaption("JellyRoll?", NULL);
  
  // attempt to set video mode.
  screen = SDL_SetVideoMode(640,480, 16, SDL_DOUBLEBUF);
  if (screen == NULL)
  {
    printf("Unable to set video mode: %s\n", SDL_GetError());
    return 1;
  }
  
  // load the font and error out of program if fail.
  if (TTF_Init() < 0)
  {
    std::cerr << "ERROR: Could not initialize the ttf_font." << std::endl;
    return 1;
  }
  gFont = TTF_OpenFont("./lazy.ttf", 10);
  if (gFont == NULL)
  {
    std::cerr << "ERROR: Could not open font." << std::endl;
    return 1;
  }
  
  
  
  // open the audio device. Hopefully we can get the desired 
  // but obtained will have the actual values.
  desired.freq = 44100;       //output rate
  desired.format = AUDIO_S16; // request signed 16-bit values
  desired.samples = 4096;     // arbitrary value.
  desired.channels = 2;
  desired.callback = AudioCallback;
  desired.userdata = NULL;
  if (SDL_OpenAudio(&desired, &obtained) < 0)
  {
    printf("Unable to open audio device: %s\n", SDL_GetError());
    return 1;
  }
  
  // load our sound files and convert them to the sound cards format
  if (LoadAndConvertSound("cannon.wav", &obtained, &cannon) != 0)
  {
    printf("Unable to load sound\n");
    return 1;
  }
  
  // load our sound files and convert them to the sound cards format
  if (LoadAndConvertSound("explosion.wav", &obtained, &explosion) != 0)
  {
    printf("Unable to load sound\n");
    return 1;
  }
  
  // clear the list of playing sounds.
  ClearPlayingSounds();
  
  // Start the audio.
  SDL_PauseAudio(0);
  
  printf("Press 'Q' to quit. C and E to play sounds.\n");
  
  // load background
  background = Init_image("background.bmp");
  std::cout << "background: " << background << std::endl; 
  // load character 1
  mario = Init_image("ttt.bmp");
  std::cout << "mario: " << mario << std::endl; 
  // Load me the character2.
  mario2 = Init_image("ttu.bmp");
  std::cout << "mario2: " << mario2 << std::endl; 
  // Load me the bullet.
  bullet = Init_image("bullet.bmp");
  std::cout << "bullet: " << bullet << std::endl;
  // load the m_char hit image
  SDL_Surface *hit = Init_image("ttv.bmp");
  std::cout << "hit: " << hit << std::endl;
  m_char.set_image_hit(hit);
  h_hit = Init_image("ttv.bmp");
  SDL_Surface *run = Init_image("./image/run.bmp");
  m_char.set_image_run(run);
  m_char.set_coords_run(0, 0, 64, 64, 4);
  SDL_Surface *jump = Init_image("./image/jump.bmp");
  m_char.set_image_jump(jump);
  m_char.set_coords_jump(0,0,64,64,6);
  
  
  //int blah;
  //std::cin >> blah;
  
  // This is the main game loop.
  while (quit_flag == 0)
  {
    init_img();
    CheckForInput();
  }
  printf("Out of main while loop\n");
  
  // Free our background and character
  SDL_FreeSurface(background);
  SDL_FreeSurface(mario);
  SDL_FreeSurface(mario2);
  SDL_FreeSurface(bullet);
  // pause and lock the sound system so we can safely 
  // delete our sound data
  SDL_PauseAudio(1);
  SDL_LockAudio();
  
  // free our sounds before we exit
  free(cannon.samples);
  free(explosion.samples);
  
  // unlock because now we are safe
  SDL_UnlockAudio();
  
  return 0;
}
