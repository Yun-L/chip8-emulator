#include <cstdio>
#include <stdint.h>
#include <string>

#define NIBBLE_1(ins) ((uint8_t)(ins >> 12))
#define NIBBLE_2(ins) ((uint8_t)(ins >> 8) & 0x0F)
#define NIBBLE_3(ins) ((uint8_t)(ins >> 4) & 0x0F)
#define NIBBLE_4(ins) ((uint8_t)(ins & 0x000F))
#define NIBBLE_234(ins) (ins & 0x0FFF)
#define NIBBLE_34(ins) ((uint8_t)(ins & 0x00FF))

// TODO: replace with cli arg
std::string INPUT_ROM = "../data/IBM Logo.ch8";

uint8_t registers[16] = {0};
uint8_t stack_p = 0;
uint8_t delay_timer = 0;
uint8_t sound_timer = 0;
uint16_t index = 0;
uint16_t pc = 0x200;

uint8_t ram[4096] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
    // the rest should be 0 initialized
};
uint8_t stack[64] = {0};

// TODO: validate, rom is not too big, handle error codes, harden
void loadProgramToRam() {
    FILE *pFile;
    pFile = fopen(INPUT_ROM.c_str(), "r");

    if (pFile == NULL) {
        return;
    }
    printf("file opened: %s\n", INPUT_ROM.c_str());

    std::fseek(pFile, 0, SEEK_END);
    long int fileNumBytes = std::ftell(pFile);
    printf("num bytes in file: %ld\n", fileNumBytes);
    std::rewind(pFile);
    std::fread(ram + 0x200, sizeof(uint8_t), fileNumBytes, pFile);
}

int main(int argc, char *argv[]) {
    printf("test");

    uint16_t instruction;
    int count = 0;
    loadProgramToRam();
    while (true) {
        // read keypad?

        // fetch instruction
        instruction = ((uint16_t)(ram[pc]) << 8) | ((uint16_t)(ram[pc+1]));
        pc += 2;
        count++;
        // std::printf("%d: %X\n", count, instruction);
        switch (NIBBLE_1(instruction)) {
            case 0: {
                switch (NIBBLE_234(instruction)) {
                    case 0x0E0: { // CLS
                        // TODO: clear screen
                        std::printf("CLS\n");
                        break;
                    }
                    case 0x0EE: { // RET
                        // TODO: return from subroutine
                        break;
                    }
                    default: {
                        // TODO: report error, obsolete command
                    }
                }
                break;
            }
            case 1: {           // JP addr
                std::printf("JP addr\n");
                // TODO: jump to address
                uint16_t addr = NIBBLE_234(instruction);
                break;
            }
            case 2: {           // CALL addr
                uint16_t addr = instruction & 0x0FFF;
                break;
            }
            case 3: {           // SE Vx, byte
                break;
            }
            case 4: {           // SNE Vx, byte
                break;
            }
            case 5: {           // SE Vx, Vy
                break;
            }
            case 6: {           // LD Vx, byte
                std::printf("LD Vx, byte\n");
                // TODO: set Vx register to byte
                uint8_t x = NIBBLE_2(instruction);
                uint8_t val = NIBBLE_34(instruction);
                break;
            }
            case 7: {           // ADD Vx, byte
                std::printf("ADD Vx, byte\n");
                // TODO: add value to Vx register
                uint8_t x = NIBBLE_2(instruction);
                uint8_t val = NIBBLE_34(instruction);
                break;
            }
            case 8: {           // LD:OR:AND:XOR:ADD:SUB:SHR:SUBN:SHL Vx, Vy
                std::printf("8\n");
                break;
            }
            case 9: {           // SNE Vx, Vy
                break;
            }
            case 10: {          // LD I, addr
                std::printf("LD I, addr\n");
                // TODO: set I register to addr
                uint16_t val = NIBBLE_234(instruction);
                break;
            }
            case 11: {          // JP V0, addr
                std::printf("JP V0\n");
                break;
            }
            case 12: {          // RND Vx, byte
                break;
            }
            case 13: {          // DRW Vx, Vy, nibble
                std::printf("DRW Vx, Vy\n");
                // TODO: display/draw
                uint8_t x = NIBBLE_2(instruction);
                uint8_t y = NIBBLE_3(instruction);
                uint8_t val = NIBBLE_4(instruction);
                break;
            }
            case 14: {          // SKP:SKNP Vx
                std::printf("SKP\n");
                break;
            }
            case 15: {          // LD Vx DT: LD Vx, K: LD ST, Vx: ...
                std::printf("LD F\n");
                break;
            }
            default:
                break;
        }

    }
    return 0;
}
