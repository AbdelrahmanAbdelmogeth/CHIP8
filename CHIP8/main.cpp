#include "CHIP_8.h"
#include "platform.h"
#include <chrono>
#include <iostream>
#include <SDL.h>
#include <string>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <random>
#include <cstdint>
#include <random>
using std::cout;
using std::istringstream;
using std::string;

const unsigned int DISPLAY_HEIGHT = 32;
const unsigned int DISPLAY_WIDTH = 64;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << argc << "\n";
        cout << argv[0] << " " << argv[1] << " " << argv[2] << " " << argv[3] << "\n";
        std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
        std::exit(EXIT_FAILURE);
    }
    cout << argv[0] << " " << argv[1] << " " << argv[2] << " " << argv[3] << "\n";
    int videoScale = std::stoi(argv[1]);
    int cycleDelay = std::stoi(argv[2]);
    char const* romFilename = argv[3];

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Set up audio
    SDL_AudioSpec wavSpec;
    Uint32 wavLength;
    Uint8* wavBuffer;

    if (SDL_LoadWAV("beep.wav", &wavSpec, &wavBuffer, &wavLength) == NULL) {
        std::cerr << "Failed to load WAV: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);
    if (deviceId == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        return -1;
    }

    Platform platform("CHIP-8 Emulator", DISPLAY_WIDTH * videoScale, DISPLAY_HEIGHT * videoScale, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    CHIP_8 chip8;
    chip8.LoadROM(romFilename);
    int videoPitch = sizeof(chip8.Display[0][0]) * DISPLAY_WIDTH;
    auto lastCycleTime = std::chrono::high_resolution_clock::now();
    bool quit = false;

    while (!quit) {
        quit = platform.ProcessInput(chip8.keypad);
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();
        if (dt > cycleDelay) {
            lastCycleTime = currentTime;
            chip8.Cycle();
            platform.Update(chip8.Display, videoPitch);
        }
    }

    // Clean up audio
    SDL_CloseAudioDevice(deviceId);
    SDL_FreeWAV(wavBuffer);
    SDL_Quit();

    return 0;
}