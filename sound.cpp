#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

int main()
{
  SDL_Surface *screen;
  SDL_Event event;
  int quit_flag = 0;
  
  // audio format specifications
  SDL_AudioSpec desired, obtained;
  
  // our loaded sounds and their formats
  sound_t cannon, explosion;
  
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
  screen = SDL_SetVideoMode(256,256,16, 0);
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

  // Start the event loop.
  while (SDL_WaitEvent(&event) != 0 && quit_flag == 0)
  {
    SDL_keysym keysym;
    
    switch(event.type)
    {
      case SDL_KEYDOWN:
        keysym = event.key.keysym;
        
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
        
        break;
        
        case SDL_QUIT:
          printf("Quit event!\n");
          quit_flag = 1;
    } 
  }
  
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
