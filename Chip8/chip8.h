#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <string>

class Chip8 {
public:
    Chip8();

    void initialize();
    bool loadROM(const std::string& filename);

    void cycle();
    void updateTimers();

    void printDisplay() const;
    void setDebug(bool enabled);

    uint8_t keypad[16];
    uint32_t video[64 * 32];
    bool drawFlag;

private:
    uint8_t memory[4096];
    uint8_t V[16];
    uint16_t I;
    uint16_t pc;
    uint16_t opcode;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t delayTimer;
    uint8_t soundTimer;
    bool debugEnabled;

    void loadFontset();
};

#endif