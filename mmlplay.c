#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#if defined(USE_ALSA)
#include <alsa/asoundlib.h>
static snd_pcm_t *pcm_handle = NULL;
#elif defined(USE_SDL3)
#include <SDL3/SDL.h>
static SDL_AudioStream *audio_stream = NULL;
#elif defined(USE_SDL)
#include <SDL/SDL.h>
static short *sdl1_buf = NULL;
static int sdl1_len = 0;
#elif defined(USE_SDL2)
#include <SDL2/SDL.h>
static SDL_AudioDeviceID audio_device;
#elif defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
static HWAVEOUT hWaveOut = NULL;
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 44100

#if defined(USE_SDL)
void sdl1_callback(void *userdata, Uint8 *stream, int len) {
    int to_copy = (sdl1_len < len) ? sdl1_len : len;
    if (to_copy > 0 && sdl1_buf) {
        memcpy(stream, sdl1_buf, to_copy);
        sdl1_buf += (to_copy / sizeof(short));
        sdl1_len -= to_copy;
    } else {
        memset(stream, 0, len);
    }
}
#endif

void init_audio() {
#if defined(USE_ALSA)
    if (snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) exit(1);
    snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1, SAMPLE_RATE, 1, 500000);
#elif defined(USE_SDL3)
    if (!SDL_Init(SDL_INIT_AUDIO)) exit(1);
    SDL_AudioSpec spec = { SDL_AUDIO_S16LE, 1, SAMPLE_RATE };
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!audio_stream) exit(1);
    SDL_ResumeAudioStreamDevice(audio_stream);
#elif defined(USE_SDL)
    if (SDL_Init(SDL_INIT_AUDIO) < 0) exit(1);
    SDL_AudioSpec wanted;
    wanted.freq = SAMPLE_RATE;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 1;
    wanted.samples = 1024;
    wanted.callback = sdl1_callback;
    wanted.userdata = NULL;
    if (SDL_OpenAudio(&wanted, NULL) < 0) exit(1);
    SDL_PauseAudio(0);
#elif defined(USE_SDL2)
    if (SDL_Init(SDL_INIT_AUDIO) < 0) exit(1);
    SDL_AudioSpec wanted;
    memset(&wanted, 0, sizeof(wanted));
    wanted.freq = SAMPLE_RATE;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 1;
    wanted.samples = 4096;
    audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);
    if (audio_device == 0) exit(1);
    SDL_PauseAudioDevice(audio_device, 0);
#elif defined(_WIN32)
    WAVEFORMATEX wfx = {WAVE_FORMAT_PCM, 1, SAMPLE_RATE, SAMPLE_RATE * 2, 2, 16, 0};
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) exit(1);
#else
    printf("Audio initialized in DUMMY mode (no sound).\n");
#endif
}

void close_audio() {
#if defined(USE_ALSA)
    if (pcm_handle) snd_pcm_drain(pcm_handle);
    if (pcm_handle) snd_pcm_close(pcm_handle);
#elif defined(USE_SDL3)
    SDL_DestroyAudioStream(audio_stream);
    SDL_Quit();
#elif defined(USE_SDL)
    SDL_CloseAudio();
    SDL_Quit();
#elif defined(USE_SDL2)
    SDL_CloseAudioDevice(audio_device);
    SDL_Quit();
#elif defined(_WIN32)
    if (hWaveOut) {
        waveOutReset(hWaveOut);
        waveOutClose(hWaveOut);
    }
#endif
}

void play_note(char note, int sharp, int octave, int length, int tempo, int volume) {
    // Calculate duration based on MML rules: (240 / tempo) / length
    // This assumes 4/4 time where a whole note is 4 beats.
    double duration = (240.0 / (double)tempo) / (double)length;
    int num_samples = (int)(duration * SAMPLE_RATE);
    short *buffer = calloc(num_samples, sizeof(short));
    
    if (toupper(note) == 'R') {
        printf("  [REST]  Dur: %4.3fs\n", duration);
#if defined(USE_ALSA)
        snd_pcm_writei(pcm_handle, buffer, num_samples);
#elif defined(USE_SDL3)
        SDL_PutAudioStreamData(audio_stream, buffer, num_samples * sizeof(short));
        SDL_Delay((Uint32)(duration * 1000));
#elif defined(USE_SDL)
        sdl1_buf = buffer;
        sdl1_len = num_samples * sizeof(short);
        while(sdl1_len > 0) SDL_Delay(10);
#elif defined(USE_SDL2)
        SDL_QueueAudio(audio_device, buffer, num_samples * sizeof(short));
        SDL_Delay((Uint32)(duration * 1000));
#elif defined(_WIN32)
        Sleep((DWORD)(duration * 1000));
#endif
        free(buffer);
        return;
    }

    // Map note to semitones from C
    int note_map[] = {9, 11, 0, 2, 4, 5, 7}; // A, B, C, D, E, F, G
    int semitone = note_map[toupper(note) - 'A'];
    semitone += sharp;
    
    // Frequency relative to A4 (440Hz)
    // Formula: f = 440 * 2^((n - 69) / 12) where n is MIDI key number
    // MIDI C4 is 60. A4 is 69.
    int midi_note = (octave + 1) * 12 + semitone;
    double freq = 440.0 * pow(2.0, (double)(midi_note - 69) / 12.0);
    int amplitude = (int)((volume / 127.0) * 32767.0 * 0.5);

    printf("  [NOTE]  %c%s Oct: %d Len: %2d Freq: %7.2fHz Vol: %d\n", 
           toupper(note), (sharp > 0 ? "+" : (sharp < 0 ? "-" : " ")), octave, length, freq, volume);

    for (int i = 0; i < num_samples; i++) {
        buffer[i] = (short)(amplitude * sin(2.0 * M_PI * freq * i / SAMPLE_RATE));
    }

#if defined(USE_ALSA)
    snd_pcm_writei(pcm_handle, buffer, num_samples);
#elif defined(USE_SDL3)
    SDL_PutAudioStreamData(audio_stream, buffer, num_samples * sizeof(short));
    SDL_Delay((Uint32)(duration * 1000));
#elif defined(USE_SDL)
    sdl1_buf = buffer;
    sdl1_len = num_samples * sizeof(short);
    while(sdl1_len > 0) SDL_Delay(1);
#elif defined(USE_SDL2)
    SDL_QueueAudio(audio_device, buffer, num_samples * sizeof(short));
    SDL_Delay((Uint32)(duration * 1000));
#elif defined(_WIN32)
    WAVEHDR hdr = { (LPSTR)buffer, num_samples * sizeof(short), 0, 0, 0, 0, NULL, 0 };
    waveOutPrepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
    waveOutWrite(hWaveOut, &hdr, sizeof(WAVEHDR));
    while (!(hdr.dwFlags & WHDR_DONE)) { struct timespec ts = {0, 1000000}; nanosleep(&ts, NULL); }
    waveOutUnprepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
#else
    // Dummy fallback: just simulate time
    usleep((useconds_t)(duration * 1000000.0));
#endif

    free(buffer);
}

void parse_and_play_mml(const char* mml, int volume) {
    int octave = 4;
    int tempo = 120;
    int default_length = 4;
    const char* p = mml;

    while (*p) {
        char cmd = toupper(*p);
        if (cmd == 'T') {
            p++; tempo = 0;
            while (isdigit(*p)) tempo = tempo * 10 + (*p++ - '0');
            if (tempo == 0) tempo = 120;
        } else if (cmd == 'O') {
            p++; if (isdigit(*p)) octave = (*p++ - '0');
        } else if (cmd == '<') {
            octave--; p++;
        } else if (cmd == '>') {
            octave++; p++;
        } else if (cmd == 'L') {
            p++; default_length = 0;
            while (isdigit(*p)) default_length = default_length * 10 + (*p++ - '0');
        } else if ((cmd >= 'A' && cmd <= 'G') || cmd == 'R') {
            char note = cmd; p++;
            int sharp = 0;
            if (*p == '+') { sharp = 1; p++; }
            else if (*p == '#') { sharp = 1; p++; }
            else if (*p == '-') { sharp = -1; p++; }
            
            int length = 0;
            while (isdigit(*p)) length = length * 10 + (*p++ - '0');
            if (length == 0) length = default_length;
            
            // Skip dotting for now in simple terminal output
            if (*p == '.') p++;
            
            play_note(note, sharp, octave, length, tempo, volume);
        } else {
            p++;
        }
    }
}

void play_track(int track_id, const char* mml, int volume) {
    printf("[Track %2d] [Vol %2d] MML: %s\n", track_id, volume, mml);
    parse_and_play_mml(mml, volume);
}

#ifdef STANDALONE_PLAYER
int main(int argc, char** argv) {
    FILE* input = stdin;
    if (argc > 1 && strcmp(argv[1], "-") != 0) {
        input = fopen(argv[1], "r");
        if (!input) { perror("Error opening file"); return 1; }
    }
    init_audio();
    printf("MML Standalone Player ready.\n");
    char line[4096];
    while (fgets(line, sizeof(line), input)) {
        char* mml_ptr = strstr(line, "MML: ");
        if (mml_ptr) {
            mml_ptr += 5;
            char* nl = strchr(mml_ptr, '\n'); 
            if (nl) *nl = '\0';
            printf("\n>>> Parsing Track: %s\n", mml_ptr);
            parse_and_play_mml(mml_ptr, 100);
        } else if (strstr(line, "--- Sequence Step")) {
            printf("\n%s", line);
        }
    }
    close_audio();
    return 0;
}
#endif