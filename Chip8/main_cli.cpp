#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "chip8.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom.ch8> [--debug] [--hz=500]\n";
        return 1;
    }

    std::string romPath = argv[1];
    bool debug = false;
    int cpuHz = 500;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--debug") {
            debug = true;
        } else if (arg.rfind("--hz=", 0) == 0) {
            cpuHz = std::stoi(arg.substr(5));
            if (cpuHz <= 0) {
                std::cerr << "Frequence CPU invalide.\n";
                return 1;
            }
        }
    }

    Chip8 chip8;
    chip8.setDebug(debug);

    if (!chip8.loadROM(romPath)) {
        return 1;
    }

    using clock = std::chrono::steady_clock;
    using duration = std::chrono::duration<double>;

    const duration cpuStep(1.0 / static_cast<double>(cpuHz));
    const duration timerStep(1.0 / 60.0);

    auto nextCpuTick = clock::now();
    auto nextTimerTick = clock::now();

    while (true) {
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
            chip8.printDisplay();
            chip8.drawFlag = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}