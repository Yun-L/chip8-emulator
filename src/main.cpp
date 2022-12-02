#include <cstdio>
#include <stdint.h>
#include <string>

int main(int argc, char *argv[])
{
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

uint8_t ram[4096] = {0};
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

    printf("test");
    return 0;
}
