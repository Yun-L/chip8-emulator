#include <cstdint> // fixed int size types
#include <cstdlib> // rand

enum class keyState {
	IDLE,
	PRESSED,
	RELEASED
	// for the instruction Fx0A, we only count a key as pressed if it was
	// pressed, then released.
};

// Holds onto state of entire chip8 system
typedef struct chip8 {
    bool quit = false; // exit out of program
	bool running = false; // if a rom is active
	bool display[2048] = {0}; // 64 x 32 bits, 8 x 32 uint8_t's
	keyState keyStates[16] = {};
	// ^ here we represent each key by using its value as the index in this array.
	// this means that the order in the array doesn't match the order of the
	// actual physical key layout. e.g keyState[0] corresponds to the '0' key
	// in the physical layout, not the '1' key.
	// physical layout:
	// 1 2 3 C
	// 4 5 6 D
	// 7 8 9 E
	// A 0 B F
	uint8_t registers[16] = {0};
	uint16_t pc = 0x200;
	uint16_t index = 0;
	uint8_t delay_timer = 0;
	uint8_t sound_timer = 0;
	uint8_t stack_p = 0;
	uint16_t stack[64] = {0};
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
} chip8;

bool runInstruction(chip8 *p_chip8State, uint16_t instruction);
void ch8Step(chip8 *p_chip8State);
bool initEmulatorWithRom(chip8 *p_chip8State, const char *romPath);
