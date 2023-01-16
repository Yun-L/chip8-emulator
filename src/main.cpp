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
bool display[2048] = {0}; // 64 x 32 bits, 8 x 32 uint8_t's

void writeDisplayStateToFile() {
    FILE *pFile;
    pFile = fopen("display.txt", "w");

    if (pFile == NULL) {
        return;
    }
    printf("file opened: display.txt\n");

    for (int i = 0; i < 2048; ++i) {
        if (i % 64 == 0) {
            fputc('\n', pFile);
        }

        if (display[i]) {
            fputc('X', pFile);
        } else {
            fputc('-', pFile);
        }
    }
}

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
    int debug_count = 0;
    loadProgramToRam();
    while (true && debug_count < 25) {
        debug_count++;
        // read keypad?

        // fetch instruction
        instruction = ((uint16_t)(ram[pc]) << 8) | ((uint16_t)(ram[pc+1]));
        pc += 2;
        count++;
        // std::printf("%d: %X\n", count, instruction);
        switch (NIBBLE_1(instruction)) {
            case 0x0: {
                switch (NIBBLE_234(instruction)) {
                    case 0x0E0: { // CLS
                        std::printf("CLS\n");
                        for (int i = 0; i < 2048; ++i) {
                            display[i] = false;
                        }
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
            case 0x1: {           // JP addr
                pc = NIBBLE_234(instruction);
                std::printf("JP addr=0x%X\n", pc);
                break;
            }
            case 0x2: {           // CALL addr
                uint16_t addr = instruction & 0x0FFF;
                break;
            }
            case 0x3: {           // SE Vx, byte
                break;
            }
            case 0x4: {           // SNE Vx, byte
                break;
            }
            case 0x5: {           // SE Vx, Vy
                break;
            }
            case 0x6: {           // LD Vx, byte
                std::printf("LD Vx, byte\n");
                uint8_t x = NIBBLE_2(instruction);
                uint8_t val = NIBBLE_34(instruction);
                registers[x] = val;
                break;
            }
            case 0x7: {           // ADD Vx, byte
                std::printf("ADD Vx, byte\n");
                // TODO: add value to Vx register
                uint8_t x = NIBBLE_2(instruction);
                uint8_t val = NIBBLE_34(instruction);
                registers[x] += val;
                break;
            }
            case 0x8: {           // LD:OR:AND:XOR:ADD:SUB:SHR:SUBN:SHL Vx, Vy
                std::printf("8\n");
                break;
            }
            case 0x9: {           // SNE Vx, Vy
                break;
            }
            case 0xA: {          // LD I, addr
                std::printf("LD I, addr\n");
                index = NIBBLE_234(instruction);
                break;
            }
            case 0xB: {          // JP V0, addr
                std::printf("JP V0\n");
                break;
            }
            case 0xC: {          // RND Vx, byte
                break;
            }
            case 0xD: {          // DRW Vx, Vy, nibble
                registers[0xF] = 0;
                uint8_t _x = NIBBLE_2(instruction);
                uint8_t _y = NIBBLE_3(instruction);
                uint8_t n = NIBBLE_4(instruction);
                uint8_t x = registers[_x];
                uint8_t y = registers[_y];
                std::printf("DRW x=%d, y=%d, n=%d, i=0x%X ", x, y, n, index);
                // set vf on collisions

                for (int i = 0; i < n; ++i) {
                    int disp_index = (y+i)*64 + x;
                    int sprite_index = index + i;
                    for (int j = 0; j < 8; ++j) {
                        int curr_display_bit = display[disp_index + j] ? 1 : 0;
                        int curr_sprite_bit = (ram[sprite_index] >> (7 - j)) & 1;
                        if (curr_sprite_bit != curr_display_bit) {
                            if (curr_display_bit == 1) {
                                registers[0xF] = 1;
                            }
                            display[disp_index + j] = !display[disp_index + j];
                        }
                    }
                }
                std::printf("VF=%d\n", registers[0xF]);
                break;
            }
                std::printf("SKP\n");
            case 0xE: {          // SKP:SKNP Vx
                break;
            }
                std::printf("LD F\n");
            case 0xF: {          // LD Vx DT: LD Vx, K: LD ST, Vx: ...
                break;
            }
            default:
                break;
        }

    }

    writeDisplayStateToFile();
    return 0;
}
