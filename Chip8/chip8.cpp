#include "chip8.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <cstdlib>

static const uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8() {
    initialize();
}

void Chip8::initialize() {
    pc = 0x200;
    opcode = 0;
    I = 0;
    sp = 0;
    delayTimer = 0;
    soundTimer = 0;
    drawFlag = false;
    debugEnabled = false;

    std::memset(memory, 0, sizeof(memory));
    std::memset(V, 0, sizeof(V));
    std::memset(stack, 0, sizeof(stack));
    std::memset(keypad, 0, sizeof(keypad));
    std::memset(video, 0, sizeof(video));

    loadFontset();
}

void Chip8::setDebug(bool enabled) {
    debugEnabled = enabled;
}

void Chip8::loadFontset() {
    for (int i = 0; i < 80; i++) {
        memory[0x50 + i] = fontset[i];
    }
}

bool Chip8::loadROM(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Impossible d'ouvrir la ROM : " << filename << '\n';
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > (4096 - 0x200)) {
        std::cerr << "ROM trop grande.\n";
        return false;
    }

    if (!file.read(reinterpret_cast<char*>(&memory[0x200]), size)) {
        std::cerr << "Erreur de lecture ROM.\n";
        return false;
    }

    return true;
}

void Chip8::cycle() {
    opcode = (memory[pc] << 8) | memory[pc + 1];

    if (debugEnabled) {
        std::cout << "PC: " << std::hex << pc
                  << " OPCODE: " << opcode << '\n';
    }

    pc += 2;

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0: // CLS
                    std::memset(video, 0, sizeof(video));
                    drawFlag = true;
                    break;

                case 0x00EE: // RET
                    if (sp > 0) {
                        sp--;
                        pc = stack[sp];
                    } else {
                        std::cerr << "Stack underflow\n";
                    }
                    break;

                default:
                    std::cerr << "Opcode inconnu: " << std::hex << opcode << '\n';
                    break;
            }
            break;

        case 0x1000: // JP addr
            pc = opcode & 0x0FFF;
            break;

        case 0x2000: // CALL addr
            if (sp < 16) {
                stack[sp] = pc;
                sp++;
                pc = opcode & 0x0FFF;
            } else {
                std::cerr << "Stack overflow\n";
            }
            break;

        case 0x3000: { // SE Vx, byte
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            if (V[x] == nn) {
                pc += 2;
            }
            break;
        }

        case 0x4000: { // SNE Vx, byte
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            if (V[x] != nn) {
                pc += 2;
            }
            break;
        }

        case 0x5000: { // SE Vx, Vy
            if ((opcode & 0x000F) == 0x0) {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t y = (opcode & 0x00F0) >> 4;
                if (V[x] == V[y]) {
                    pc += 2;
                }
            } else {
                std::cerr << "Opcode inconnu: " << std::hex << opcode << '\n';
            }
            break;
        }

        case 0x6000: { // LD Vx, byte
            uint8_t x = (opcode & 0x0F00) >> 8;
            V[x] = opcode & 0x00FF;
            break;
        }

        case 0x7000: { // ADD Vx, byte
            uint8_t x = (opcode & 0x0F00) >> 8;
            V[x] += opcode & 0x00FF;
            break;
        }

        case 0x8000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            switch (opcode & 0x000F) {
                case 0x0: // LD Vx, Vy
                    V[x] = V[y];
                    break;

                case 0x1: // OR Vx, Vy
                    V[x] |= V[y];
                    break;

                case 0x2: // AND Vx, Vy
                    V[x] &= V[y];
                    break;

                case 0x3: // XOR Vx, Vy
                    V[x] ^= V[y];
                    break;

                case 0x4: { // ADD Vx, Vy
                    uint16_t sum = V[x] + V[y];
                    V[0xF] = (sum > 0xFF) ? 1 : 0;
                    V[x] = static_cast<uint8_t>(sum & 0xFF);
                    break;
                }

                case 0x5: // SUB Vx, Vy
                    V[0xF] = (V[x] > V[y]) ? 1 : 0;
                    V[x] = V[x] - V[y];
                    break;

                case 0x6: // SHR Vx
                    V[0xF] = V[x] & 0x01;
                    V[x] >>= 1;
                    break;

                case 0x7: // SUBN Vx, Vy
                    V[0xF] = (V[y] > V[x]) ? 1 : 0;
                    V[x] = V[y] - V[x];
                    break;

                case 0xE: // SHL Vx
                    V[0xF] = (V[x] & 0x80) >> 7;
                    V[x] <<= 1;
                    break;

                default:
                    std::cerr << "Opcode inconnu: " << std::hex << opcode << '\n';
                    break;
            }
            break;
        }

        case 0x9000: { // SNE Vx, Vy
            if ((opcode & 0x000F) == 0x0) {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t y = (opcode & 0x00F0) >> 4;
                if (V[x] != V[y]) {
                    pc += 2;
                }
            } else {
                std::cerr << "Opcode inconnu: " << std::hex << opcode << '\n';
            }
            break;
        }

        case 0xA000: // LD I, addr
            I = opcode & 0x0FFF;
            break;

        case 0xC000: { // RND Vx, byte
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            V[x] = static_cast<uint8_t>((std::rand() % 256) & nn);
            break;
        }

        case 0xD000: { // DRW Vx, Vy, nibble
            uint8_t x = V[(opcode & 0x0F00) >> 8];
            uint8_t y = V[(opcode & 0x00F0) >> 4];
            uint8_t height = opcode & 0x000F;

            V[0xF] = 0;

            for (unsigned int row = 0; row < height; row++) {
                uint8_t spriteByte = memory[I + row];

                for (unsigned int col = 0; col < 8; col++) {
                    if ((spriteByte & (0x80 >> col)) != 0) {
                        int xPos = (x + col) % 64;
                        int yPos = (y + row) % 32;
                        int index = yPos * 64 + xPos;

                        if (video[index] == 0xFFFFFFFF) {
                            V[0xF] = 1;
                        }

                        video[index] ^= 0xFFFFFFFF;
                    }
                }
            }

            drawFlag = true;
            break;
        }

        case 0xE000: {
            uint8_t x = (opcode & 0x0F00) >> 8;

            switch (opcode & 0x00FF) {
                case 0x9E: // SKP Vx
                    if (V[x] < 16 && keypad[V[x]]) {
                        pc += 2;
                    }
                    break;

                case 0xA1: // SKNP Vx
                    if (V[x] < 16 && !keypad[V[x]]) {
                        pc += 2;
                    }
                    break;

                default:
                    std::cerr << "Opcode inconnu: " << std::hex << opcode << '\n';
                    break;
            }
            break;
        }

        case 0xF000: {
            uint8_t x = (opcode & 0x0F00) >> 8;

            switch (opcode & 0x00FF) {
                case 0x07: // LD Vx, DT
                    V[x] = delayTimer;
                    break;

                case 0x0A: { // LD Vx, K
                    bool keyPressed = false;
                    for (uint8_t i = 0; i < 16; i++) {
                        if (keypad[i]) {
                            V[x] = i;
                            keyPressed = true;
                            break;
                        }
                    }

                    if (!keyPressed) {
                        pc -= 2;
                    }
                    break;
                }

                case 0x15: // LD DT, Vx
                    delayTimer = V[x];
                    break;

                case 0x18: // LD ST, Vx
                    soundTimer = V[x];
                    break;

                case 0x1E: // ADD I, Vx
                    I += V[x];
                    break;

                case 0x29: // LD F, Vx
                    I = 0x50 + (V[x] * 5);
                    break;

                case 0x33: // LD B, Vx
                    memory[I]     = V[x] / 100;
                    memory[I + 1] = (V[x] / 10) % 10;
                    memory[I + 2] = V[x] % 10;
                    break;

                case 0x55: // LD [I], V0..Vx
                    for (uint8_t i = 0; i <= x; i++) {
                        memory[I + i] = V[i];
                    }
                    break;

                case 0x65: // LD V0..Vx, [I]
                    for (uint8_t i = 0; i <= x; i++) {
                        V[i] = memory[I + i];
                    }
                    break;

                default:
                    std::cerr << "Opcode inconnu: " << std::hex << opcode << '\n';
                    break;
            }
            break;
        }

        default:
            std::cerr << "Opcode inconnu: " << std::hex << opcode << '\n';
            break;
    }
}

void Chip8::updateTimers() {
    if (delayTimer > 0) {
        delayTimer--;
    }

    if (soundTimer > 0) {
        soundTimer--;
    }
}

void Chip8::printDisplay() const {
    std::cout << "\x1B[2J\x1B[H";

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            if (video[y * 64 + x]) {
                std::cout << "##";
            } else {
                std::cout << "  ";
            }
        }
        std::cout << '\n';
    }

    std::cout << std::flush;
}