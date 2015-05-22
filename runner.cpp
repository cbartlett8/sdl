#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <iostream>

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
int LoadAndConvertSound(char *filename, SDL_AudioSpec *spec, sound_p sound)
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

// This is just setting up the images that will be appearing in the game.
SDL_Surface *background;
SDL_Surface *mario;
SDL_Surface *mario2;
SDL_Surface *screen;
SDL_Surface *temp;
SDL_Surface *bullet;
SDL_Rect src, dest;
SDL_Event event;
int quit_flag = 0;
// our loaded sounds and their formats
sound_t cannon, explosion;


enum Character_State
{
  RUNNING = 1,
  JUMPING = 2,
  SHOOTING = 3,
  HIT = 4,
  SLIDING = 5
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
  Bullet(int x, int y, int w, int h, int cur_x, int cur_y)
  {
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    m_current_x = cur_x;
    m_current_y = cur_y;
  }
};

class Actor
{
  private:
    SDL_Surface *m_hit;
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
    SDL_Surface *get_image_hit()
    {
      return m_hit;
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
};

std::string set = "Hello";
Actor m_char(0, 0, 32, 32, 0, 1, set, 30, 100, RUNNING, 0, 0, 100);
Bullet m_bullet(0,0,5,5,0,0);

void init_img()
{
  // Draw the background
    src.x = 0;
    src.y = 0;
    src.w = background->w;
    src.h = background->h;
    dest = src;
    
    SDL_BlitSurface(background, &src, screen, &dest);
  
  // Draw the main character
  //printf("The current_frame: %i\n", m_char.m_current_frame);
  // calculate the movement
  if (m_char.m_state == JUMPING)
  {
    if (m_char.m_current_y > 0 && m_char.m_state_flag == 0)
    {
      m_char.m_current_y -= 1;
    }
    if (m_char.m_current_y < 5)
    {
      m_char.m_state_flag = 1;
    }
    if (m_char.m_state_flag == 1)
    {
      m_char.m_current_y += 1;
    }
    if (m_char.m_current_y >= 100 && m_char.m_state_flag == 1)
    {
      m_char.m_state = RUNNING;
      m_char.m_state_flag = 0;
    }
  }
  
  // check if the character fired a bullet.
  if (m_char.m_state == SHOOTING)
  {
    // Play the animation of the character shooting the bullet.
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
  }
  
  // check if player was hit
  if (m_char.m_state == HIT)
  {
    //printf("In hit action state\n");
    if (m_char.m_current_frame == 0)
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
      m_char.m_current_frame += 1;
    }
    else if (m_char.m_current_frame == 1)
    {
      src.x = (m_char.m_current_frame + 1) * 20;
      src.y = 0;
      src.w = 20;
      src.h = 20;
      dest.x = m_char.m_current_x;
      dest.y = m_char.m_current_y;
      dest.w = 20;
      dest.h = 20;
      assert(m_char.get_image_hit() != NULL);
      SDL_BlitSurface(m_char.get_image_hit(), &src, screen, &dest);
      //m_char.m_current_frame += 1;
    }
    else if (m_char.m_current_frame == 2)
    {
      src.x = (m_char.m_current_frame + 1) * 20;
      src.y = 0;
      src.w = 20;
      src.h = 20;
      dest.x = m_char.m_current_x;
      dest.y = m_char.m_current_y;
      dest.w = 20;
      dest.h = 20;
      assert(m_char.get_image_hit() != NULL);
      SDL_BlitSurface(m_char.get_image_hit(), &src, screen, &dest);
      //m_char.m_current_frame += 1;
    }
    else
    {
      m_char.m_current_frame = 0;
    }
  }
  
  // check the frame.
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
          m_char.m_state = SHOOTING;
          m_bullet.m_current_x = m_char.m_current_x;
          m_bullet.m_current_y = m_char.m_current_y;
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
    printf("ERROR: Could not load: background.bmp\n");
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
  
  // attempt toset video mode.
  screen = SDL_SetVideoMode(640,480, 16, SDL_DOUBLEBUF);
  if (screen == NULL)
  {
    printf("Unable to set video mode: %s\n", SDL_GetError());
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
  SDL_Surface *hit = Init_image("./image/hit.bmp");
  std::cout << "hit: " << hit << std::endl;
  m_char.set_image_hit(hit);
  
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
