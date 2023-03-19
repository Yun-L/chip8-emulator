#include <string>
#include <windows.h>
#include <iostream>

#define NIBBLE_1(ins) ((uint8_t)(ins >> 12))
#define NIBBLE_2(ins) ((uint8_t)(ins >> 8) & 0x0F)
#define NIBBLE_3(ins) ((uint8_t)(ins >> 4) & 0x0F)
#define NIBBLE_4(ins) ((uint8_t)(ins & 0x000F))
#define NIBBLE_234(ins) (ins & 0x0FFF)
#define NIBBLE_34(ins) ((uint8_t)(ins & 0x00FF))

// std::string INPUT_ROM = "../data/Chip8 Picture.ch8";
// std::string INPUT_ROM = "../data/Delay Timer Test [Matthew Mikolay, 2010].ch8";
std::string INPUT_ROM = "../data/Clock Program [Bill Fisher, 1981].ch8";
uint8_t rom[4096] = {0}; // roms should probably not be bigger than this
std::string OUTPUT_FILE = "./translated.txt";

int64_t loadProgram() {
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

    if (ReadFile(hFile, rom, fileNumBytes.QuadPart, NULL, NULL) < 0) {
        printf("Failed to read file\n");
        CloseHandle(hFile);
        return -1;
    }
    CloseHandle(hFile);
    return fileNumBytes.QuadPart;
}

char hexToChar(uint8_t nibble) {
    switch (nibble) {
        case 0x0: return '0';
        case 0x1: return '1';
        case 0x2: return '2';
        case 0x3: return '3';
        case 0x4: return '4';
        case 0x5: return '5';
        case 0x6: return '6';
        case 0x7: return '7';
        case 0x8: return '8';
        case 0x9: return '9';
        case 0xA: return 'A';
        case 0xB: return 'B';
        case 0xC: return 'C';
        case 0xD: return 'D';
        case 0xE: return 'E';
        case 0xF: return 'F';
        default: return '?';
    }
}

typedef struct {
    uint32_t textLen;
    char text[256];
} instructionText;

int appendText(const char text[], int len, instructionText *iText) {
    for (size_t i = 0; i < len; ++i) {
        iText->text[i + iText->textLen] = text[i];
    }
    iText->textLen += len;
    return 0;
}

int translateInstruction(uint16_t instruction,
                         instructionText *iText) {
    const char hexStr[] = {
        hexToChar(NIBBLE_1(instruction)),
        hexToChar(NIBBLE_2(instruction)),
        hexToChar(NIBBLE_3(instruction)),
        hexToChar(NIBBLE_4(instruction)),
        ':',
        ' '
    };
    appendText(hexStr, 6, iText);
    std::string t = "";
    std::string n2(1, hexToChar(NIBBLE_2(instruction)));
    std::string n3(1, hexToChar(NIBBLE_3(instruction)));
    std::string n4(1, hexToChar(NIBBLE_4(instruction)));
    switch (NIBBLE_1(instruction)) {
        case 0x0: {
            switch (NIBBLE_234(instruction)) {
                case 0x0E0: {
                    t += "CLS";
                    break;
                }
                case 0x0EE: {
                    t += "RET";
                    break;
                }
                default: {
                    // TODO: report error, obsolete command
                }
            }
            break;
        }
        case 0x1: {
            t += "JP " + n2 + n3 + n4;
            break;
        }
        case 0x2: {
            t += "CALL " + n2 + n3 + n4;;
            break;
        }
        case 0x3: {
            t += "SE ";
            t += "V" + n2;
            t += ", " + n3 + n4;
            break;
        }
        case 0x4: {
            t += "SNE ";
            t += "V" + n2;
            t += ", " + n3 + n4;
            break;
        }
        case 0x5: {
            t += "SN ";
            t += "V" + n2;
            t += ", V" + n3;
            break;
        }
        case 0x6: {
            t += "LD ";
            t += "V" + n2;
            t += ", " + n3 + n4;
            break;
        }
        case 0x7: {
            t += "ADD ";
            t += "V" + n2;
            t += ", " + n3 + n4;
            break;
        }
        case 0x8: {
            uint8_t op = NIBBLE_4(instruction);
            switch(op) {
                case 0:
                    t += "LD ";
                    break;
                case 1:
                    t += "OR ";
                    break;
                case 2:
                    t += "AND ";
                    break;
                case 3:
                    t += "XOR ";
                    break;
                case 4:
                    t += "ADD ";
                    break;
                case 5:
                    t += "SUB ";
                    break;
                case 6:
                    t += "SHR ";
                    break;
                case 7:
                    t += "SUBN ";
                    break;
                case 0xE:
                    t += "SHL";
                    break;
                default:
                    break;
            }
            t += "V" + n2 + ", V" + n3;
            break;
        }
        case 0x9: {
            t += "SNE ";
            t += "V" + n2 + ", V" + n3;
            break;
        }
        case 0xA: {
            t += "LD ";
            t += "I, " + n2 + n3 + n4;
            break;
        }
        case 0xB: {
            t += "JP ";
            t += "V0 ," + n2 + n3 + n4;
            break;
        }
        case 0xC: {
            t += "RND ";
            t += "V" + n2;
            t += ", " + n3 + n4;
            break;
        }
        case 0xD: {
            t += "DRW ";
            t += "V" + n2 + ", V" + n3 + ", " + n4;
            break;
        }
        case 0xE: {
            uint8_t op = NIBBLE_34(instruction);
            switch (op) {
                case 0x9E: {
                    t += "SKP ";
                    t += "V" + n2;
                    break;
                }
                case 0xA1: {
                    t += "SKNP ";
                    t += "V" + n2;
                    break;
                }
                default: {
                    break;
                }
            }
            break;
        }
        case 0xF: {
            uint8_t op = NIBBLE_34(instruction);
            switch (op) {
                case 0x07: {
                    t += "LD ";
                    t += "V" + n2;
                    t += ", DT";
                    break;
                }
                case 0x0A: {
                    t += "LD ";
                    t += "V" + n2;
                    t += ", K";
                    break;
                }
                case 0x15: {
                    t += "LD ";
                    t += "DT";
                    t += ", V" + n2;
                    break;
                }
                case 0x18: {
                    t += "LD ";
                    t += "ST";
                    t += ", V" + n2;
                    break;
                }
                case 0x1E: {
                    t += "ADD ";
                    t += "I, V" + n2;
                    break;
                }
                case 0x29: {
                    t += "LD ";
                    t += "F, V" + n2;
                    break;
                }
                case 0x33: {
                    t += "LD ";
                    t += "B, V" + n2;
                    break;
                }
                case 0x55: {
                    t += "LD ";
                    t += "[I], V" + n2;
                    break;
                }
                case 0x65: {
                    t += "LD ";
                    t += "V" + n2 + " [I]";
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
    appendText(t.c_str(), t.length(), iText);
    return 0;
}

int64_t translateAndWrite(int romLength) {
    HANDLE hFile = CreateFileA(
        OUTPUT_FILE.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    printf("file opened: %s\n", OUTPUT_FILE.c_str());
    LARGE_INTEGER fileNumBytes = {0};

    instructionText iText = {};
    for (int i = 0; i < romLength; i += 2) {
        uint16_t instruction = ((uint16_t)rom[i] << 8) | rom[i+1];
        iText.textLen = 0;
        uint16_t addr = i + 0x200;
        const char hexStr[] = {
            hexToChar(NIBBLE_1(addr)),
            hexToChar(NIBBLE_2(addr)),
            hexToChar(NIBBLE_3(addr)),
            hexToChar(NIBBLE_4(addr)),
            ' ', '<', '-', ' '
        };
        appendText(hexStr, 8, &iText);
        translateInstruction(instruction, &iText);
        appendText("\n", 1, &iText);
        if (WriteFile(hFile,
                      iText.text,
                      iText.textLen,
                      NULL,
                      NULL) == 0) {
            printf("Failed to write file");
            CloseHandle(hFile);
            return -1;
        }    
    }
    
    CloseHandle(hFile);
    return 0;
}

int main(int argc, char *argv[]) {
    int64_t romLen = loadProgram();
    translateAndWrite(romLen);
}
