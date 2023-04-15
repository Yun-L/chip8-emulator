#include "chip8_emulator.h"

#define NIBBLE_1(ins) ((uint8_t)(ins >> 12))
#define NIBBLE_2(ins) ((uint8_t)(ins >> 8) & 0x0F)
#define NIBBLE_3(ins) ((uint8_t)(ins >> 4) & 0x0F)
#define NIBBLE_4(ins) ((uint8_t)(ins & 0x000F))
#define NIBBLE_234(ins) (ins & 0x0FFF)
#define NIBBLE_34(ins) ((uint8_t)(ins & 0x00FF))


bool runInstruction(uint16_t instruction, systemState *state, cpu *ch8Cpu) {
	switch (NIBBLE_1(instruction)) {
		case 0x0: {
			switch (NIBBLE_234(instruction)) {
				case 0x0E0: { // CLS
					for (int i = 0; i < 2048; ++i) {
						ch8Cpu->display[i] = false;
					}
					break;
				}
				case 0x0EE: { // RET
					--(ch8Cpu->stack_p);
					ch8Cpu->pc = ch8Cpu->stack[ch8Cpu->stack_p];
					break;
				}
				default: {
					// TODO: report error, obsolete command
				}
			}
			break;
		}
		case 0x1: {           // JP addr
			ch8Cpu->pc = NIBBLE_234(instruction);
			return false;
		}
		case 0x2: {           // CALL addr
			uint16_t addr = instruction & 0x0FFF;
			++(ch8Cpu->stack_p);
			ch8Cpu->stack[(ch8Cpu->stack_p)-1] = ch8Cpu->pc;
			ch8Cpu->pc = addr;
			return false;
		}
		case 0x3: {           // SE Vx, byte
			uint8_t x = NIBBLE_2(instruction);
			uint8_t val = NIBBLE_34(instruction);
			if (ch8Cpu->registers[x] == val) {
				ch8Cpu->pc += 2;
			}
			break;
		}
		case 0x4: {           // SNE Vx, byte
			uint8_t x = NIBBLE_2(instruction);
			uint8_t val = NIBBLE_34(instruction);
			if (ch8Cpu->registers[x] != val) {
				ch8Cpu->pc += 2;
			}
			break;
		}
		case 0x5: {           // SE Vx, Vy
			uint8_t x = NIBBLE_2(instruction);
			uint8_t y = NIBBLE_3(instruction);
			if (ch8Cpu->registers[x] == ch8Cpu->registers[y]) {
				ch8Cpu->pc += 2;
			}
			break;
		}
		case 0x6: {           // LD Vx, byte
			uint8_t x = NIBBLE_2(instruction);
			uint8_t val = NIBBLE_34(instruction);
			ch8Cpu->registers[x] = val;
			break;
		}
		case 0x7: {           // ADD Vx, byte
			uint8_t x = NIBBLE_2(instruction);
			uint8_t val = NIBBLE_34(instruction);
			ch8Cpu->registers[x] += val;
			break;
		}
		case 0x8: {           // LD:OR:AND:XOR:ADD:SUB:SHR:SUBN:SHL Vx, Vy
			uint8_t x = NIBBLE_2(instruction);
			uint8_t y = NIBBLE_3(instruction);
			uint8_t op = NIBBLE_4(instruction);
			switch(op) {
				case 0: {
					ch8Cpu->registers[x] = ch8Cpu->registers[y];
					break;
				}
				case 1: {
					ch8Cpu->registers[x] = ch8Cpu->registers[x] | ch8Cpu->registers[y];
					break;
				}
				case 2: {
					ch8Cpu->registers[x] = ch8Cpu->registers[x] & ch8Cpu->registers[y];
					break;
				}
				case 3: {
					ch8Cpu->registers[x] = ch8Cpu->registers[x] ^ ch8Cpu->registers[y];
					break;
				}
				case 4: {
					uint16_t sum = ch8Cpu->registers[x] + ch8Cpu->registers[y];
					if (sum > 255) {
						ch8Cpu->registers[0xF] = 1;
					} else {
						ch8Cpu->registers[0xF] = 0;
					}
					ch8Cpu->registers[x] = (uint8_t)sum;
					break;
				}
				case 5: {
					if (ch8Cpu->registers[x] > ch8Cpu->registers[y]) {
						ch8Cpu->registers[x] = ch8Cpu->registers[x] - ch8Cpu->registers[y];
						ch8Cpu->registers[0xF] = 1;
					} else {
						ch8Cpu->registers[x] = 256 - (ch8Cpu->registers[y] - ch8Cpu->registers[x]);
						ch8Cpu->registers[0xF] = 0;
					}
					break;
				}
				case 6: {
					ch8Cpu->registers[0xF] = ch8Cpu->registers[x] & 1;
					ch8Cpu->registers[x] = ch8Cpu->registers[x] >> 1;
					break;
				}
				case 7: {
					if (ch8Cpu->registers[y] > ch8Cpu->registers[x]) {
						ch8Cpu->registers[x] = ch8Cpu->registers[y] - ch8Cpu->registers[x];
						ch8Cpu->registers[0xF] = 1;
					} else {
						ch8Cpu->registers[x] = 256 - (ch8Cpu->registers[x] - ch8Cpu->registers[y]);
						ch8Cpu->registers[0xF] = 0;
					}
					break;
				}
				case 0xE: {
					ch8Cpu->registers[0xF] = (ch8Cpu->registers[x] & 0b10000000) >> 7;
					ch8Cpu->registers[x] = ch8Cpu->registers[x] << 1;
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
			if (ch8Cpu->registers[x] != ch8Cpu->registers[y]) {
				ch8Cpu->pc += 2;
			}
			break;
		}
		case 0xA: {          // LD I, addr
			ch8Cpu->index = NIBBLE_234(instruction);
			break;
		}
		case 0xB: {          // JP V0, addr
			ch8Cpu->pc = ch8Cpu->registers[0] + NIBBLE_234(instruction);
			return false;
		}
		case 0xC: {          // RND Vx, byte
			uint8_t mask = NIBBLE_34(instruction);
			uint8_t x = NIBBLE_2(instruction);
			ch8Cpu->registers[x] = (uint8_t)rand() & mask;
			break;
		}
		case 0xD: {          // DRW Vx, Vy, nibble
			ch8Cpu->registers[0xF] = 0;
			uint8_t _x = NIBBLE_2(instruction);
			uint8_t _y = NIBBLE_3(instruction);
			uint8_t n = NIBBLE_4(instruction);
			uint8_t x = ch8Cpu->registers[_x];
			uint8_t y = ch8Cpu->registers[_y];

			for (int i = 0; i < n; ++i) {
				int disp_index = (y+i)*64 + x;
				int sprite_index = (ch8Cpu->index) + i;
				for (int j = 0; j < 8; ++j) {
					// (disp_index + j >= 2048)
					if (y+i < 32) {
						int wrapped;
						if (disp_index + j >= (y+i+1)*64) {
							wrapped = disp_index + j - 64;
						} else {
							wrapped = disp_index + j;
						}
						int curr_display_bit = ch8Cpu->display[wrapped] ? 1 : 0;
						int curr_sprite_bit = ((ch8Cpu->ram[sprite_index]) >> (7 - j)) & 1;
						if (curr_sprite_bit != curr_display_bit) {
							ch8Cpu->display[wrapped] = true;
						} else {
							ch8Cpu->display[wrapped] = false;
							if (curr_display_bit == 1) {
								// set vf on collisions
								ch8Cpu->registers[0xF] = 1;
							}
						}
					}
				}
			}
			break;
		}
		case 0xE: {          // SKP:SKNP Vx
			uint8_t op = NIBBLE_34(instruction);
			uint8_t x = ch8Cpu->registers[NIBBLE_2(instruction)];
			switch (op) {
				case 0x9E: {
					if (state->keyStates[x] == keyState::PRESSED) {
						ch8Cpu->pc += 2;
					}
					break;
				}
				case 0xA1: {
					if (state->keyStates[x] != keyState::PRESSED) {
						ch8Cpu->pc += 2;
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
					ch8Cpu->registers[x] = ch8Cpu->delay_timer;
					break;
				}
				case 0x0A: {
					for (uint8_t i = 0; i < 16; ++i) {
						if (state->keyStates[i] == keyState::RELEASED) {
							ch8Cpu->registers[x] = i;
							return true;
						}
					}
					return false;
				}
				case 0x15: {
					ch8Cpu->delay_timer = ch8Cpu->registers[x];
					break;
				}
				case 0x18: {
					ch8Cpu->sound_timer = ch8Cpu->registers[x];
					break;
				}
				case 0x1E: {
					ch8Cpu->index += ch8Cpu->registers[x];
					break;
				}
				case 0x29: {
					ch8Cpu->index = (ch8Cpu->registers[x] & 0x0F) * 5;
					break;
				}
				case 0x33: {
					// TODO: check this, BCD notation
					ch8Cpu->ram[ch8Cpu->index] = ch8Cpu->registers[x] / 100;
					ch8Cpu->ram[ch8Cpu->index+1] = (ch8Cpu->registers[x] / 10) % 10;
					ch8Cpu->ram[ch8Cpu->index+2] = ch8Cpu->registers[x] % 10;
					break;
				}
				// these two have different behavior depending on version of rom
				case 0x55: { // LD [I], Vx
					for (uint8_t i = 0; i <= x; ++i) {
						ch8Cpu->ram[ch8Cpu->index + i] = ch8Cpu->registers[i];
					}
					break;
				}
				case 0x65: { // LD Vx, [i]
					for (uint8_t i = 0; i <= x; ++i) {
						ch8Cpu->registers[i] = ch8Cpu->ram[ch8Cpu->index + i];
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
