#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TB 15
#define PATTERN_PARTS 4
#define MAX_MML_LEN 1024

// Prototypes for functions defined in mmlplay.c
void init_audio();
void close_audio();
void play_track(int track_id, const char* mml, int volume);

// Helper for SmileBasic style RND(min, max)
int rnd_range(int min, int max) {
    if (max <= min) return min;
    return min + rand() % (max - min + 1);
}

const char* S[TB][PATTERN_PARTS] = {
    {"G+4<D+8E8C+8>B8F+8D+8", "F+8<C+8>F+8B8<E8C+8>G+8<G+8", "G+8E8C+8>E8G+8<C8C+8D+8", "A8D+8A8D+8G+8G+8>A8"},
    {"G+8G+8G+8<C+8>F+8<C+8>F+8E8", "E8E8E8F+8E8>B8B8", "<A8>B8G+8<B+8>G+8<D+8>G+8C+8", "G+8D+8G+8G+8G+8F+8F+8"},
    {"C+8G+8<E8F+8G+8>B8E8A8", "<D+8A8G+8G+8C8G+8>A8G+8C+8", "G+8>F+8<G+8>F+8A8<D+8>C+8<D+8>C+8<D+8", "B8D+8B8D+8>A8<D+8>C+8<D+8>C+8<D+8"},
    {"A8<E8B8A8G+8C+8>F+8A8", "<D+8A8G+8G+8C8G+8>A8G+8C+8C+8G+8<E8F+8G+8>B8E8A8E8", "F+8<E8B8<E8>>B8<F+8>A8A+16<D+16>F+16<D+16>F+16<D+16>F+16", "<D+16>A+16<D+16>A+16<D+16>F+16<D+16>F+16<D+16>A+16"},
    {"<D+16>A+16<D+16", "C+8C+8C+8C+8C+8C+8C+8", "A8A8A8A8A8A8A8A8", "G+8A8G+8G+8G+8A8G+8G+8"},
    {">G+8<D+8E8>B8<G+8<C+8>A8E8", "G+8<D+8E8<C+8C+8>E8<C+8>E8", "E8B8<D+8>B8<C+8>E8A8E8", "C+8G+8<D+8A8E8>G+8<D+8>B8"},
    {"E8B8<D+8>B8E8<E8D+8>B8", "C+8B8<C+8D+8>B8E8D+8>B8", "G+8<D+8>G+8F+8>B8<G+8>B8G+8", "<C+8G+8B8F+8>B8<G+8>G+8G+8"},
    {"E8B8<D+8>B8E8<E8D+8>B8", "E8B8<D+8>B8E8<E8R4", "E8B8<D+8>B8E8<E8R1", ""},
    {"G+8G+8G+8<D+8>G+8<E8>G+8G+8", "G+8R8G+8<D+8>G+8G+8G+8<D+8", ">G+8<E8>G+8G+8G+8<E8>G+8<E8", "E8<E8>E8R8E8E8A+8"},
    {"E8<F+8>E8E8E8G+8E8G+8", "G+8G+8G+8<D+8>G+8R8G+8G+8", "G+8<E8>G+8<D+8>G+8G+8G+8<D+8", ">G+8<E8>G+8G+8G+8R8G+8<E8"},
    {">E8E8E8A+8E8<C+8>E8E8", "E8<E8>E8R8E8E8A+8", "E8<F+8>E8E8E8G+8E8G+8", "E8E8E8A+8E8<C+8>E8E8"},
    {"E8<E8>E8R8E8E8A+8", "E8<F+8>E8E8E8F+8E8<D+8", ">E8E8E8A+8E8<C+8>E8E8", "E8<E8>E8R8E8E8A+8"},
    {"E8<F+8>E8E8E8G+8E8G+8", "E8E8E8A+8E8<C+8>E8E8", "E8<E8>E8R8E8E8A+8", "E8<F+8>E8E16R16E16R16C+16R16E8<E8"},
    {">G+4F+16R16F+16R16F+4F+16R16", "F+16R16F+16R16G+4F+16R16F+16R16<C+8", "D+8>B8E4C+16R16C+16R16C+16R16E4", "C+16R16C+16R16C+16R16E4C+16R16C+16R16"},
    {"C+16R16E8.D+8.C+8.G+4F+16", "R16F+16R16F+16R16G+4F+16R16F+16R16F+16R16G+4", "F+16R16F+16R16F+16R16<C+8D+8>B8E4", "C+16R16C+16R16C+16R16E4C+16R16C+16+16"}
};

char compiled_bgm[TB][MAX_MML_LEN];

int main() {
    srand(time(NULL));
    init_audio();

    printf("SmileBasic 4 MML Engine Port starting...\n");

    while (1) {
        int tempo = abs(rnd_range(6, 66) - rnd_range(5, 420)) + 1;        
        // BGMSET phase: Compile the pattern library with randomized octaves
        for (int b = 0; b < TB; b++) {
            int octave = rnd_range(4, 6); 
            snprintf(compiled_bgm[b], MAX_MML_LEN, "T%dO%d%s%s%s%s", 
                     tempo, octave, S[b][0], S[b][1], S[b][2], S[b][3]);
        }

        int L = 4; // Default loop length

        // Performance loop
        for (int i = 0; i < TB; i++) {
            // Logical check from line 74
            if (i == rnd_range(1, 16)) {
                L = rnd_range(6, 8);
            }

            for (int j = 0; j < L; j++) {
                // BGMPLAY 0 (The master track)
                play_track(0, compiled_bgm[i], 47);
                
                // BGMPLAY 1-10 (The layered tracks)
                // Using rnd_range(6, TB) as per the source logic
                play_track(1, compiled_bgm[rnd_range(6, TB-1)], rnd_range(6, 90));
                play_track(2, compiled_bgm[rnd_range(6, TB-1)], rnd_range(6, 54));
                play_track(3, compiled_bgm[rnd_range(6, TB-1)], rnd_range(6, 66));
                play_track(4, compiled_bgm[rnd_range(6, TB-1)], rnd_range(6, 89));
                play_track(5, compiled_bgm[rnd_range(6, TB-1)], rnd_range(6, 97));
                play_track(6, compiled_bgm[rnd_range(6, TB-1)], rnd_range(6, 99));
                play_track(7, compiled_bgm[rnd_range(6, TB-1)], rnd_range(6, 67));
                play_track(8, compiled_bgm[rnd_range(6, TB-1)], rnd_range(6, 67));
                play_track(9, compiled_bgm[rnd_range(6, TB-1)], rnd_range(6, 47));
                play_track(10, compiled_bgm[rnd_range(6, TB-1)], rnd_range(6, 27));

                usleep(500000); // 0.5s pause between layers for visualization
            }
        }
        printf("Composition complete. Restarting...\n");
        sleep(1);
    }

    close_audio();
    return 0;
}