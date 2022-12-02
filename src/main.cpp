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
    printf("test");
    return 0;
}
