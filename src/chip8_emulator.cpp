#include "chip8_emulator.h"
#include <windows.h>
#include <string>

#define NIBBLE_1(ins) ((uint8_t)(ins >> 12))
#define NIBBLE_2(ins) ((uint8_t)(ins >> 8) & 0x0F)
#define NIBBLE_3(ins) ((uint8_t)(ins >> 4) & 0x0F)
#define NIBBLE_4(ins) ((uint8_t)(ins & 0x000F))
#define NIBBLE_234(ins) (ins & 0x0FFF)
#define NIBBLE_34(ins) ((uint8_t)(ins & 0x00FF))


bool runInstruction(chip8 *p_chip8State, uint16_t instruction) {
	switch (NIBBLE_1(instruction)) {
		case 0x0: {
			switch (NIBBLE_234(instruction)) {
				case 0x0E0: { // CLS
					for (int i = 0; i < 2048; ++i) {
						p_chip8State->display[i] = false;
					}
					break;
				}
				case 0x0EE: { // RET
					--(p_chip8State->stack_p);
					p_chip8State->pc = p_chip8State->stack[p_chip8State->stack_p];
					break;
				}
				default: {
					// TODO: report error, obsolete command
				}
			}
			break;
		}
		case 0x1: {           // JP addr
			p_chip8State->pc = NIBBLE_234(instruction);
			return false;
		}
		case 0x2: {           // CALL addr
			uint16_t addr = instruction & 0x0FFF;
			++(p_chip8State->stack_p);
			p_chip8State->stack[(p_chip8State->stack_p)-1] = p_chip8State->pc;
			p_chip8State->pc = addr;
			return false;
		}
		case 0x3: {           // SE Vx, byte
			uint8_t x = NIBBLE_2(instruction);
			uint8_t val = NIBBLE_34(instruction);
			if (p_chip8State->registers[x] == val) {
				p_chip8State->pc += 2;
			}
			break;
		}
		case 0x4: {           // SNE Vx, byte
			uint8_t x = NIBBLE_2(instruction);
			uint8_t val = NIBBLE_34(instruction);
			if (p_chip8State->registers[x] != val) {
				p_chip8State->pc += 2;
			}
			break;
		}
		case 0x5: {           // SE Vx, Vy
			uint8_t x = NIBBLE_2(instruction);
			uint8_t y = NIBBLE_3(instruction);
			if (p_chip8State->registers[x] == p_chip8State->registers[y]) {
				p_chip8State->pc += 2;
			}
			break;
		}
		case 0x6: {           // LD Vx, byte
			uint8_t x = NIBBLE_2(instruction);
			uint8_t val = NIBBLE_34(instruction);
			p_chip8State->registers[x] = val;
			break;
		}
		case 0x7: {           // ADD Vx, byte
			uint8_t x = NIBBLE_2(instruction);
			uint8_t val = NIBBLE_34(instruction);
			p_chip8State->registers[x] += val;
			break;
		}
		case 0x8: {           // LD:OR:AND:XOR:ADD:SUB:SHR:SUBN:SHL Vx, Vy
			uint8_t x = NIBBLE_2(instruction);
			uint8_t y = NIBBLE_3(instruction);
			uint8_t op = NIBBLE_4(instruction);
			switch(op) {
				case 0: {
					p_chip8State->registers[x] = p_chip8State->registers[y];
					break;
				}
				case 1: {
					p_chip8State->registers[x] = p_chip8State->registers[x] | p_chip8State->registers[y];
					break;
				}
				case 2: {
					p_chip8State->registers[x] = p_chip8State->registers[x] & p_chip8State->registers[y];
					break;
				}
				case 3: {
					p_chip8State->registers[x] = p_chip8State->registers[x] ^ p_chip8State->registers[y];
					break;
				}
				case 4: {
					uint16_t sum = p_chip8State->registers[x] + p_chip8State->registers[y];
					if (sum > 255) {
						p_chip8State->registers[0xF] = 1;
					} else {
						p_chip8State->registers[0xF] = 0;
					}
					p_chip8State->registers[x] = (uint8_t)sum;
					break;
				}
				case 5: {
					if (p_chip8State->registers[x] > p_chip8State->registers[y]) {
						p_chip8State->registers[x] = p_chip8State->registers[x] - p_chip8State->registers[y];
						p_chip8State->registers[0xF] = 1;
					} else {
						p_chip8State->registers[x] = 256 - (p_chip8State->registers[y] - p_chip8State->registers[x]);
						p_chip8State->registers[0xF] = 0;
					}
					break;
				}
				case 6: {
					p_chip8State->registers[0xF] = p_chip8State->registers[x] & 1;
					p_chip8State->registers[x] = p_chip8State->registers[x] >> 1;
					break;
				}
				case 7: {
					if (p_chip8State->registers[y] > p_chip8State->registers[x]) {
						p_chip8State->registers[x] = p_chip8State->registers[y] - p_chip8State->registers[x];
						p_chip8State->registers[0xF] = 1;
					} else {
						p_chip8State->registers[x] = 256 - (p_chip8State->registers[x] - p_chip8State->registers[y]);
						p_chip8State->registers[0xF] = 0;
					}
					break;
				}
				case 0xE: {
					p_chip8State->registers[0xF] = (p_chip8State->registers[x] & 0b10000000) >> 7;
					p_chip8State->registers[x] = p_chip8State->registers[x] << 1;
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
			if (p_chip8State->registers[x] != p_chip8State->registers[y]) {
				p_chip8State->pc += 2;
			}
			break;
		}
		case 0xA: {          // LD I, addr
			p_chip8State->index = NIBBLE_234(instruction);
			break;
		}
		case 0xB: {          // JP V0, addr
			p_chip8State->pc = p_chip8State->registers[0] + NIBBLE_234(instruction);
			return false;
		}
		case 0xC: {          // RND Vx, byte
			uint8_t mask = NIBBLE_34(instruction);
			uint8_t x = NIBBLE_2(instruction);
			p_chip8State->registers[x] = (uint8_t)rand() & mask;
			break;
		}
		case 0xD: {          // DRW Vx, Vy, nibble
			p_chip8State->registers[0xF] = 0;
			uint8_t _x = NIBBLE_2(instruction);
			uint8_t _y = NIBBLE_3(instruction);
			uint8_t n = NIBBLE_4(instruction);
			uint8_t x = p_chip8State->registers[_x];
			uint8_t y = p_chip8State->registers[_y];

			for (int i = 0; i < n; ++i) {
				int disp_index = (y+i)*64 + x;
				int sprite_index = (p_chip8State->index) + i;
				for (int j = 0; j < 8; ++j) {
					// (disp_index + j >= 2048)
					if (y+i < 32) {
						int wrapped;
						if (disp_index + j >= (y+i+1)*64) {
							wrapped = disp_index + j - 64;
						} else {
							wrapped = disp_index + j;
						}
						int curr_display_bit = p_chip8State->display[wrapped] ? 1 : 0;
						int curr_sprite_bit = ((p_chip8State->ram[sprite_index]) >> (7 - j)) & 1;
						if (curr_sprite_bit != curr_display_bit) {
							p_chip8State->display[wrapped] = true;
						} else {
							p_chip8State->display[wrapped] = false;
							if (curr_display_bit == 1) {
								// set vf on collisions
								p_chip8State->registers[0xF] = 1;
							}
						}
					}
				}
			}
			break;
		}
		case 0xE: {          // SKP:SKNP Vx
			uint8_t op = NIBBLE_34(instruction);
			uint8_t x = p_chip8State->registers[NIBBLE_2(instruction)];
			switch (op) {
				case 0x9E: {
					if (p_chip8State->keyStates[x] == keyState::PRESSED) {
						p_chip8State->pc += 2;
					}
					break;
				}
				case 0xA1: {
					if (p_chip8State->keyStates[x] != keyState::PRESSED) {
						p_chip8State->pc += 2;
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
		case 0xF: {  //        LD Vx, K: LD ST, Vx: ...
			uint8_t op = NIBBLE_34(instruction);
			uint8_t x = NIBBLE_2(instruction);
			switch (op) {
				case 0x07: { // LD Vx DT
					p_chip8State->registers[x] = p_chip8State->delay_timer;
					break;
				}
				case 0x0A: {
					for (uint8_t i = 0; i < 16; ++i) {
						if (p_chip8State->keyStates[i] == keyState::RELEASED) {
							p_chip8State->registers[x] = i;
							return true;
						}
					}
					return false;
				}
				case 0x15: {
					p_chip8State->delay_timer = p_chip8State->registers[x];
					break;
				}
				case 0x18: {
					p_chip8State->sound_timer = p_chip8State->registers[x];
					break;
				}
				case 0x1E: {
					p_chip8State->index += p_chip8State->registers[x];
					break;
				}
				case 0x29: {
					p_chip8State->index = (p_chip8State->registers[x] & 0x0F) * 5;
					break;
				}
				case 0x33: {
					// TODO: check this, BCD notation
					p_chip8State->ram[p_chip8State->index] = p_chip8State->registers[x] / 100;
					p_chip8State->ram[p_chip8State->index+1] = (p_chip8State->registers[x] / 10) % 10;
					p_chip8State->ram[p_chip8State->index+2] = p_chip8State->registers[x] % 10;
					break;
				}
				// these two have different behavior depending on version of rom
				case 0x55: { // LD [I], Vx
					for (uint8_t i = 0; i <= x; ++i) {
						p_chip8State->ram[p_chip8State->index + i] = p_chip8State->registers[i];
					}
					break;
				}
				case 0x65: { // LD Vx, [i]
					for (uint8_t i = 0; i <= x; ++i) {
						p_chip8State->registers[i] = p_chip8State->ram[p_chip8State->index + i];
					}
					break;
				}
			}
			break;
		}
		default:
			break;
	}


	return true;
}

// runs 10 instructions, decrements timers, assumed to be called every 1/60th second
void ch8Step(chip8 *p_chip8State) {
	for (int i = 0; i < 10; i++) {
		uint16_t instruction = ((uint16_t)(p_chip8State->ram[p_chip8State->pc]) << 8) | ((uint16_t)(p_chip8State->ram[p_chip8State->pc + 1]));
		// printf("ins: 0x%04X\n", instruction);

		if (runInstruction(p_chip8State, instruction)) {
			p_chip8State->pc += 2;
		}

		// return released key states back to idle
		for (uint8_t i = 0; i < 16; ++i) {
			if (p_chip8State->keyStates[i] == keyState::RELEASED) {
				p_chip8State->keyStates[i] = keyState::IDLE;
			}
		}
	}

	// decrement timers
	if (p_chip8State->delay_timer > 0) {
		p_chip8State->delay_timer--;
	}
	if (p_chip8State->sound_timer > 0) {
		//Mix_PlayChannel(1, gTone, 0);
		p_chip8State->sound_timer--;
	}
}

// TODO: validate, rom is not too big, handle error codes, harden
bool initEmulatorWithRom(chip8 *p_chip8State, const char *romPath) {
	HANDLE hFile = CreateFileA(
		romPath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	printf("file opened: %s\n", romPath);

	LARGE_INTEGER fileNumBytes = {0};
	if (GetFileSizeEx(hFile, &fileNumBytes) < 0) {
		printf("Failed to get file size\n");
		return false;
	}
	printf("num bytes in file: %lld\n", fileNumBytes.QuadPart);

	if (ReadFile(hFile, (p_chip8State->ram) + 0x200, fileNumBytes.QuadPart, NULL, NULL) < 0) {
		printf("Failed to read file\n");
		return false;
	}
	return true;
}