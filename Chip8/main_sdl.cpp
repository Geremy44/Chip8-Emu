#include <SDL2/SDL.h>
#include <chrono>
#include <iostream>
#include <string>

#include "chip8.h"

enum class KeyboardLayout {
    AZERTY,
    QWERTY
};

int mapKeyToChip8(SDL_Keycode key, KeyboardLayout layout) {
    if (layout == KeyboardLayout::AZERTY) {
        switch (key) {
            case SDLK_1: return 0x1;
            case SDLK_2: return 0x2;
            case SDLK_3: return 0x3;
            case SDLK_4: return 0xC;

            case SDLK_a: return 0x4;
            case SDLK_z: return 0x5;
            case SDLK_e: return 0x6;
            case SDLK_r: return 0xD;

            case SDLK_q: return 0x7;
            case SDLK_s: return 0x8;
            case SDLK_d: return 0x9;
            case SDLK_f: return 0xE;

            case SDLK_w: return 0xA;
            case SDLK_x: return 0x0;
            case SDLK_c: return 0xB;
            case SDLK_v: return 0xF;

            default: return -1;
        }
    } else {
        switch (key) {
            case SDLK_1: return 0x1;
            case SDLK_2: return 0x2;
            case SDLK_3: return 0x3;
            case SDLK_4: return 0xC;

            case SDLK_q: return 0x4;
            case SDLK_w: return 0x5;
            case SDLK_e: return 0x6;
            case SDLK_r: return 0xD;

            case SDLK_a: return 0x7;
            case SDLK_s: return 0x8;
            case SDLK_d: return 0x9;
            case SDLK_f: return 0xE;

            case SDLK_z: return 0xA;
            case SDLK_x: return 0x0;
            case SDLK_c: return 0xB;
            case SDLK_v: return 0xF;

            default: return -1;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom.ch8> [qwerty] [--debug] [--hz=500]\n";
        return 1;
    }

    std::string romPath = argv[1];
    KeyboardLayout keyboardLayout = KeyboardLayout::AZERTY;
    bool debug = false;
    int cpuHz = 500;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "qwerty") {
            keyboardLayout = KeyboardLayout::QWERTY;
        } else if (arg == "--debug") {
            debug = true;
        } else if (arg.rfind("--hz=", 0) == 0) {
            cpuHz = std::stoi(arg.substr(5));
            if (cpuHz <= 0) {
                std::cerr << "Frequence CPU invalide.\n";
                return 1;
            }
        }
    }

    const int videoWidth = 64;
    const int videoHeight = 32;
    const int scale = 12;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "Erreur SDL_Init: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        videoWidth * scale,
        videoHeight * scale,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Erreur SDL_CreateWindow: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Erreur SDL_CreateRenderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Chip8 chip8;
    chip8.setDebug(debug);

    if (!chip8.loadROM(romPath)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    using clock = std::chrono::steady_clock;
    using duration = std::chrono::duration<double>;

    const duration cpuStep(1.0 / static_cast<double>(cpuHz));
    const duration timerStep(1.0 / 60.0);

    auto nextCpuTick = clock::now();
    auto nextTimerTick = clock::now();

    bool running = true;

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool pressed = (event.type == SDL_KEYDOWN);
                int chip8Key = mapKeyToChip8(event.key.keysym.sym, keyboardLayout);

                if (chip8Key != -1) {
                    chip8.keypad[chip8Key] = pressed ? 1 : 0;
                }
            }
        }

        auto now = clock::now();

        while (now >= nextCpuTick) {
            chip8.cycle();
            nextCpuTick += std::chrono::duration_cast<clock::duration>(cpuStep);
        }

        while (now >= nextTimerTick) {
            chip8.updateTimers();
            nextTimerTick += std::chrono::duration_cast<clock::duration>(timerStep);
        }

        if (chip8.drawFlag) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

            for (int y = 0; y < videoHeight; y++) {
                for (int x = 0; x < videoWidth; x++) {
                    if (chip8.video[y * videoWidth + x] == 0xFFFFFFFF) {
                        SDL_Rect rect = {
                            x * scale,
                            y * scale,
                            scale,
                            scale
                        };
                        SDL_RenderFillRect(renderer, &rect);
                    }
                }
            }

            SDL_RenderPresent(renderer);
            chip8.drawFlag = false;
        }

        SDL_Delay(1);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}