#include <cstdio>
#include <stdint.h>
#include <string>
#include <time.h>
#include <stdlib.h>
#include <windows.h>
#include <SDL.h>
#include <chrono>
#include <thread>


#define NIBBLE_1(ins) ((uint8_t)(ins >> 12))
#define NIBBLE_2(ins) ((uint8_t)(ins >> 8) & 0x0F)
#define NIBBLE_3(ins) ((uint8_t)(ins >> 4) & 0x0F)
#define NIBBLE_4(ins) ((uint8_t)(ins & 0x000F))
#define NIBBLE_234(ins) (ins & 0x0FFF)
#define NIBBLE_34(ins) ((uint8_t)(ins & 0x00FF))

typedef struct systemState {
    bool quit;
    bool waitingKeyPress;
    bool currentlyPressed[16];
    // here we represent each key by using its value as the index in this array.
    // this means that the order in the array doesn't match the order of the
    // actual physical key layout.
    // 1 2 3 C
    // 4 5 6 D
    // 7 8 9 E
    // A 0 B F
} systemState;

// TODO: replace with cli arg
// TODO: fix relative paths, this is relative to working dir, not executable
// std::string INPUT_ROM = "../data/IBM Logo.ch8";
// std::string INPUT_ROM = "../data/ibm_logo_loop.ch8";
// std::string INPUT_ROM = "../data/test_opcode.ch8";
// std::string INPUT_ROM = "../data/bc_test.ch8";
// std::string INPUT_ROM = "../data/keyboard_test.ch8";
// std::string INPUT_ROM = "../data/Chip8 Picture.ch8";
// std::string INPUT_ROM = "../data/Airplane.ch8";
// std::string INPUT_ROM = "../data/Addition Problems [Paul C. Moews].ch8";
std::string INPUT_ROM = "../data/Jumping X and O [Harry Kleinberg, 1977].ch8";
// std::string INPUT_ROM = "../data/Delay Timer Test [Matthew Mikolay, 2010].ch8";
// std::string INPUT_ROM = "../data/Clock Program [Bill Fisher, 1981].ch8";
// WCHAR modulePath[512];
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
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
    // the rest should be 0 initialized
};
uint16_t stack[64] = {0};
bool display[2048] = {0}; // 64 x 32 bits, 8 x 32 uint8_t's

// SDL
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
//Screen dimension constants
const int SCREEN_WIDTH = 1280;   // each pixel 20x20
const int SCREEN_HEIGHT = 640;


bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    gWindow = SDL_CreateWindow("Chip 8",
                               SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    //Create renderer for window
    gRenderer = SDL_CreateRenderer(
        gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (gRenderer == NULL) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    //Initialize renderer color
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0xFF);

    return true;
}

void close() {
    printf("cleanup before exit");
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
    gRenderer = NULL;
    SDL_Quit();
}

int updateWindow() {
    //Update screen
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(gRenderer);

    int err = 0;
    SDL_Rect scaledPixel = {0, 0, 20, 20};
    SDL_SetRenderDrawColor(gRenderer, 0x77, 0xDD, 0x77, SDL_ALPHA_OPAQUE);
    for (int i = 0; i < 2048; ++i) {
        if (display[i]) {
            scaledPixel.x = 20 * (i % 64);
            scaledPixel.y = 20 * (i / 64);
            err = SDL_RenderFillRect(gRenderer, &scaledPixel);
            if (err < 0) {
                printf("error fillRect i=%d x=%d y=%d: %s\n",
                       i, scaledPixel.x, scaledPixel.y, SDL_GetError());
                return err;
            }
        }
    }

    SDL_RenderPresent(gRenderer);
    return err;
}

void writeDisplayStateToFile() {
    FILE *pFile;
    pFile = fopen("display.txt", "w");

    if (pFile == NULL) {
        printf("file did not open for some reason: display.txt");
        return;
    }
    printf("file opened: display.txt\n");

    for (int i = 0; i < 2048; ++i) {
        if (i % 64 == 0) {
            printf("\n");
            fputc('\n', pFile);
        }

        if (display[i]) {
            printf("X");
            fputc('X', pFile);
        } else {
            printf("-");
            fputc('-', pFile);
        };
    }
    fclose(pFile);
}

// TODO: validate, rom is not too big, handle error codes, harden
int loadProgramToRam() {
    // TODO: get relative paths from exe, not where shell calls exe
    // int returnedLength = GetModuleFileNameW(NULL, modulePath, 256);

    // if (returnedLength == 256 || returnedLength == 0) {
    //     printf("couldn't get module file name\n");
    //     return -1;
    // }
    // printf("got module file name: %ws\n", modulePath);

    HANDLE hFile = CreateFileA(
        INPUT_ROM.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    printf("file opened: %s\n", INPUT_ROM.c_str());

    LARGE_INTEGER fileNumBytes = {0};
    if (GetFileSizeEx(hFile, &fileNumBytes) < 0) {
        printf("Failed to get file size\n");
        return -1;
    }
    printf("num bytes in file: %lld\n", fileNumBytes.QuadPart);

    if (ReadFile(hFile, ram + 0x200, fileNumBytes.QuadPart, NULL, NULL) < 0) {
        printf("Failed to read file\n");
        return -1;
    }
    return 0;
}

void handleQueuedEvents(systemState* state) {
    static SDL_Event e = {};
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            state->quit = true;
        }
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            bool pressed = (e.type == SDL_KEYDOWN);
            printf("key down event: %d\n", e.key.keysym.scancode);
            switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_1: state->currentlyPressed[1] = pressed; break;
                case SDL_SCANCODE_2: state->currentlyPressed[2] = pressed; break;
                case SDL_SCANCODE_3: state->currentlyPressed[3] = pressed; break;
                case SDL_SCANCODE_4: state->currentlyPressed[12] = pressed; break;
                case SDL_SCANCODE_Q: state->currentlyPressed[4] = pressed; break;
                case SDL_SCANCODE_W: state->currentlyPressed[5] = pressed; break;
                case SDL_SCANCODE_E: state->currentlyPressed[6] = pressed; break;
                case SDL_SCANCODE_R: state->currentlyPressed[13] = pressed; break;
                case SDL_SCANCODE_A: state->currentlyPressed[7] = pressed; break;
                case SDL_SCANCODE_S: state->currentlyPressed[8] = pressed; break;
                case SDL_SCANCODE_D: state->currentlyPressed[9] = pressed; break;
                case SDL_SCANCODE_F: state->currentlyPressed[14] = pressed; break;
                case SDL_SCANCODE_Z: state->currentlyPressed[10] = pressed; break;
                case SDL_SCANCODE_X: state->currentlyPressed[0] = pressed; break;
                case SDL_SCANCODE_C: state->currentlyPressed[11] = pressed; break;
                case SDL_SCANCODE_V: state->currentlyPressed[15] = pressed; break;
                default: break;
            }
        }
    }
}

void runInstruction(uint16_t instruction, systemState* state) {
    switch (NIBBLE_1(instruction)) {
        case 0x0: {
            switch (NIBBLE_234(instruction)) {
                case 0x0E0: { // CLS
                    // std::printf("CLS\n");
                    for (int i = 0; i < 2048; ++i) {
                        display[i] = false;
                    }
                    // err = updateWindow();
                    break;
                }
                case 0x0EE: { // RET
                    --stack_p;
                    pc = stack[stack_p];
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

            // std::printf("JP addr=0x%X\n", pc);
            break;
        }
        case 0x2: {           // CALL addr
            uint16_t addr = instruction & 0x0FFF;
            ++stack_p;
            stack[stack_p-1] = pc;
            pc = addr;
            break;
        }
        case 0x3: {           // SE Vx, byte
            uint8_t x = NIBBLE_2(instruction);
            uint8_t val = NIBBLE_34(instruction);
            if (registers[x] == val) {
                pc += 2;
            }
            break;
        }
        case 0x4: {           // SNE Vx, byte
            uint8_t x = NIBBLE_2(instruction);
            uint8_t val = NIBBLE_34(instruction);
            if (registers[x] != val) {
                pc += 2;
            }
            break;
        }
        case 0x5: {           // SE Vx, Vy
            uint8_t x = NIBBLE_2(instruction);
            uint8_t y = NIBBLE_3(instruction);
            if (registers[x] == registers[y]) {
                pc += 2;
            }
            break;
        }
        case 0x6: {           // LD Vx, byte
            // std::printf("LD Vx, byte\n");
            uint8_t x = NIBBLE_2(instruction);
            uint8_t val = NIBBLE_34(instruction);
            registers[x] = val;
            break;
        }
        case 0x7: {           // ADD Vx, byte
            // std::printf("ADD Vx, byte\n");
            uint8_t x = NIBBLE_2(instruction);
            uint8_t val = NIBBLE_34(instruction);
            registers[x] += val;
            break;
        }
        case 0x8: {           // LD:OR:AND:XOR:ADD:SUB:SHR:SUBN:SHL Vx, Vy
            // std::printf("8\n");
            uint8_t x = NIBBLE_2(instruction);
            uint8_t y = NIBBLE_3(instruction);
            uint8_t op = NIBBLE_4(instruction);
            switch(op) {
                case 0: {
                    registers[x] = registers[y];
                    break;
                }
                case 1: {
                    registers[x] = registers[x] | registers[y];
                    break;
                }
                case 2: {
                    registers[x] = registers[x] & registers[y];
                    break;
                }
                case 3: {
                    registers[x] = registers[x] ^ registers[y];
                    break;
                }
                case 4: {
                    uint16_t sum = registers[x] + registers[y];
                    if (sum > 255) {
                        registers[0xF] = 1;
                    } else {
                        registers[0xF] = 0;
                    }
                    registers[x] = (uint8_t)sum;
                    break;
                }
                case 5: {
                    if (registers[x] > registers[y]) {
                        registers[x] = registers[x] - registers[y];
                        registers[0xF] = 1;
                    } else {
                        registers[x] = 256 - (registers[y] - registers[x]);
                        registers[0xF] = 0;
                    }
                    break;
                }
                case 6: {
                    registers[0xF] = registers[x] & 1;
                    registers[x] = registers[x] >> 1;
                    break;
                }
                case 7: {
                    if (registers[y] > registers[x]) {
                        registers[x] = registers[y] - registers[x];
                        registers[0xF] = 1;
                    } else {
                        registers[x] = 256 - (registers[x] - registers[y]);
                        registers[0xF] = 0;
                    }
                    break;
                }
                case 0xE: {
                    registers[0xF] = (registers[x] & 0b10000000) >> 7;
                    registers[x] = registers[x] << 1;
                    break;
                }
                default: {
                    // std::printf("invalid instruction 0x%X\n", instruction);
                }
            }
            break;
        }
        case 0x9: {           // SNE Vx, Vy
            uint8_t x = NIBBLE_2(instruction);
            uint8_t y = NIBBLE_3(instruction);
            if (registers[x] != registers[y]) {
                pc += 2;
            }
            break;
        }
        case 0xA: {          // LD I, addr
            index = NIBBLE_234(instruction);
            break;
        }
        case 0xB: {          // JP V0, addr
            pc = registers[0] + NIBBLE_234(instruction);
            break;
        }
        case 0xC: {          // RND Vx, byte
            uint8_t mask = NIBBLE_34(instruction);
            uint8_t x = NIBBLE_2(instruction);
            registers[x] = (uint8_t)rand() & mask;
            break;
        }
        case 0xD: {          // DRW Vx, Vy, nibble
            registers[0xF] = 0;
            uint8_t _x = NIBBLE_2(instruction);
            uint8_t _y = NIBBLE_3(instruction);
            uint8_t n = NIBBLE_4(instruction);
            uint8_t x = registers[_x];
            uint8_t y = registers[_y];

            for (int i = 0; i < n; ++i) {
                int disp_index = (y+i)*64 + x;
                int sprite_index = index + i;
                for (int j = 0; j < 8; ++j) {
                    // (disp_index + j >= 2048)
                    if (y+i < 32) {
                        int wrapped;
                        if (disp_index + j >= (y+i+1)*64) {
                            wrapped = disp_index + j - 64;
                        } else {
                            wrapped = disp_index + j;
                        }
                        int curr_display_bit = display[wrapped] ? 1 : 0;
                        int curr_sprite_bit = (ram[sprite_index] >> (7 - j)) & 1;
                        if (curr_sprite_bit != curr_display_bit) {
                            display[wrapped] = true;
                        } else {
                            display[wrapped] = false;
                            if (curr_display_bit == 1) {
                                // set vf on collisions
                                registers[0xF] = 1;
                            }
                        }
                    }
                }
            }
            // err = updateWindow();
            break;
        }
            // std::printf("SKP\n");
        case 0xE: {          // SKP:SKNP Vx
            uint8_t op = NIBBLE_34(instruction);
            uint8_t x = ram[NIBBLE_2(instruction)];
            switch (op) {
                case 0x9E: {
                    if (state->currentlyPressed[x]) {
                        pc += 2;
                    }
                    break;
                }
                case 0xA1: {
                    if (!(state->currentlyPressed[x])) {
                        pc += 2;
                    }
                    break;
                }
                default: {
                    // TODO: break error
                    break;
                }
            }
            break;
        }
            // std::printf("LD F\n");
        case 0xF: {  //        LD Vx, K: LD ST, Vx: ...
            uint8_t op = NIBBLE_34(instruction);
            uint8_t x = NIBBLE_2(instruction);
            switch (op) {
                case 0x07: { // LD Vx DT
                    registers[x] = delay_timer;
                    break;
                }
                case 0x0A: {
                    for (uint8_t i = 0; i < 16; ++i) {
                        if (state->currentlyPressed[i]) {
                            registers[x] = i;
                            return true;
                        }
                    }
                    return false;
                }
                case 0x15: {
                    delay_timer = registers[x];
                    break;
                }
                case 0x18: {
                    sound_timer = registers[x];
                    break;
                }
                case 0x1E: {
                    index += registers[x];
                    break;
                }
                case 0x29: {
                    index = (registers[x] & 0x0F) * 5;
                    break;
                }
                case 0x33: {
                    // TODO: check this, BCD notation
                    ram[index] = registers[x] / 100;
                    ram[index+1] = (registers[x] / 10) % 10;
                    ram[index+2] = registers[x] % 10;
                    break;
                }
                    // these two have different behavior depending on version of rom
                case 0x55: { // LD [I], Vx
                    for (uint8_t i = 0; i <= x; ++i) {
                        ram[index + i] = registers[i];
                    }
                    break;
                }
                case 0x65: { // LD Vx, [i]
                    for (uint8_t i = 0; i <= x; ++i) {
                        registers[i] = ram[index + i];
                    }
                    break;
                }
            }
            break;
        }
        default:
            // std::printf("invalid instruction 0x%X\n", instruction);
            break;
    }
}

int main(int argc, char *argv[]) {
    
    if (!init()) {
        return 1;
    }

    systemState state = {};
    uint16_t instruction;
    int debug_count = 0;
    srand(time(NULL)); // TODO: check what passing in NULL does

    if (loadProgramToRam() < 0) {
        std::printf("Failed to load rom. Exiting.\n");
        return 1;
    };

    int err = 0;
    std::chrono::steady_clock::time_point debugTime = std::chrono::steady_clock::now();

    uint64_t performanceFreq = SDL_GetPerformanceFrequency();
    uint64_t timeLoopPrev = SDL_GetPerformanceCounter();
    uint64_t timeLoopCurr;
    double deltaSeconds;
    
    while (!state.quit && err == 0) {

        timeLoopCurr = SDL_GetPerformanceCounter();
        deltaSeconds = (double)(timeLoopCurr - timeLoopPrev)/performanceFreq;
        timeLoopPrev = timeLoopCurr;
        std::printf("elapsed seconds: %f\n", deltaSeconds);
        
        // handle SDL events
        for (int i = 0; i < 10; i++) {
            handleQueuedEvents(&state);

            // fetch instruction
            instruction = ((uint16_t)(ram[pc]) << 8) | ((uint16_t)(ram[pc+1]));
            // printf("ins: 0x%04X\n", instruction);
            pc += 2;
            debug_count++;
            // execute instruction
            runInstruction(instruction, &state);
        }
        // printf("delay timer: %d\n", delay_timer);
        if (delay_timer > 0) {
            delay_timer--;
        }
        updateWindow();

        // if (std::chrono::steady_clock::now() < nextStep) {
        //     // std::this_thread::sleep_until(nextStep);
        //     // TODO: sleep doesnt have enough granularity for 1 second,
        //     // also sleep only does a minimum of the requested amount, it could
        //     // take much longer, maybe try running several commands and sleeping
        //     // for the accumulated time rather than each step?
        //     std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // } else {
        //     printf("loop slow by nanoseconds: %lld\n",
        //            std::chrono::duration_cast<
        //              std::chrono::nanoseconds
        //            >(std::chrono::steady_clock::now() - nextStep).count());
        // }
    }
    std::chrono::nanoseconds elapsed = std::chrono::steady_clock::now() - debugTime;
    printf("%d instructions in %lld nanoseconds\n", debug_count, elapsed.count());
    writeDisplayStateToFile();
    close();
    return 0;
}
