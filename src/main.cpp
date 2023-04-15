#include <string>
#include <time.h>
#include <windows.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <thread>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer.h"
#include "chip8_emulator.h"


// TODO: replace with cli arg
// TODO: fix relative paths, this is relative to working dir, not executable

// WCHAR modulePath[512];
Mix_Chunk *gTone = NULL;

// SDL
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
//Screen dimension constants
//const int SCREEN_WIDTH = 1280;   // each pixel 20x20
//const int SCREEN_HEIGHT = 640;
const int SCREEN_WIDTH = 1920;   // each pixel 20x20
const int SCREEN_HEIGHT = 1080;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return false;
    }

//    gWindow = SDL_CreateWindow("Chip 8",
//                               SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
//                               SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    gWindow = SDL_CreateWindow("Chip 8",
                               SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN_DESKTOP);
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

bool loadMedia() {
    gTone = Mix_LoadWAV("../data/tone.wav");
    if (gTone == NULL) {
        printf("Failed to load tone sound effect! SDL_mixer Error: %s\n", Mix_GetError());
        return false;
    }
    return true;
}

void close() {
    printf("cleanup before exit");
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    Mix_FreeChunk(gTone);
    gWindow = NULL;
    gRenderer = NULL;
    gTone = NULL;
    Mix_Quit();
    SDL_Quit();
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

SDL_Texture* updateWindow(cpu *ch8Cpu) {
    constexpr int DISPLAY_HEIGHT = 32;
    constexpr int DISPLAY_WIDTH = 64;
    constexpr int PH = 1;
    constexpr int PW = 1;

    //Update screen
    SDL_Texture* scene = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (scene == NULL) {
        printf("error create texture: %s\n", SDL_GetError());
    }
    SDL_SetRenderTarget(gRenderer, scene);

    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(gRenderer);

    int err = 0;
    SDL_Rect scaledPixel = {0, 0, PW, PH};
    SDL_SetRenderDrawColor(gRenderer, 0x77, 0xDD, 0x77, SDL_ALPHA_OPAQUE);
    for (int i = 0; i < 2048; ++i) {
        if (ch8Cpu->display[i]) {
            scaledPixel.x = PW * (i % 64);
            scaledPixel.y = PH * (i / 64);
            //err = SDL_RenderFillRect(gRenderer, &scaledPixel);
            err = SDL_RenderDrawPoint(gRenderer, scaledPixel.x, scaledPixel.y);
            if (err < 0) {
                printf("error fillRect i=%d x=%d y=%d: %s\n",
                       i, scaledPixel.x, scaledPixel.y, SDL_GetError());
                //return err;
            }
        }
    }

    SDL_SetRenderTarget(gRenderer, NULL);
    //SDL_RenderPresent(gRenderer);
    return scene;
    //return err;
}

void writeDisplayStateToFile(cpu *ch8Cpu) {
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

        if (ch8Cpu->display[i]) {
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
int loadProgramToRam(cpu *ch8Cpu, const char *romPath) {
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
        return -1;
    }
    printf("num bytes in file: %lld\n", fileNumBytes.QuadPart);

    if (ReadFile(hFile, (ch8Cpu->ram) + 0x200, fileNumBytes.QuadPart, NULL, NULL) < 0) {
        printf("Failed to read file\n");
        return -1;
    }
    return 0;
}

void handleQueuedEvents(systemState* state) {
    static SDL_Event e = {};

    while (SDL_PollEvent(&e) != 0) {
        ImGui_ImplSDL2_ProcessEvent(&e);
        if (e.type == SDL_QUIT)
            state->quit = true;
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE && e.window.windowID == SDL_GetWindowID(gWindow))
            state->quit = true;
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            keyState pressed = (e.type == SDL_KEYDOWN) ?
                keyState::PRESSED : keyState::RELEASED;
            printf("key down event: %d\n", e.key.keysym.scancode);
            switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_1: state->keyStates[1]  = pressed; break;
                case SDL_SCANCODE_2: state->keyStates[2]  = pressed; break;
                case SDL_SCANCODE_3: state->keyStates[3]  = pressed; break;
                case SDL_SCANCODE_4: state->keyStates[12] = pressed; break;
                case SDL_SCANCODE_Q: state->keyStates[4]  = pressed; break;
                case SDL_SCANCODE_W: state->keyStates[5]  = pressed; break;
                case SDL_SCANCODE_E: state->keyStates[6]  = pressed; break;
                case SDL_SCANCODE_R: state->keyStates[13] = pressed; break;
                case SDL_SCANCODE_A: state->keyStates[7]  = pressed; break;
                case SDL_SCANCODE_S: state->keyStates[8]  = pressed; break;
                case SDL_SCANCODE_D: state->keyStates[9]  = pressed; break;
                case SDL_SCANCODE_F: state->keyStates[14] = pressed; break;
                case SDL_SCANCODE_Z: state->keyStates[10] = pressed; break;
                case SDL_SCANCODE_X: state->keyStates[0]  = pressed; break;
                case SDL_SCANCODE_C: state->keyStates[11] = pressed; break;
                case SDL_SCANCODE_V: state->keyStates[15] = pressed; break;
                default: break;
            }
        }
    }
}


int main(int argc, char *argv[]) {

    if (!init()) {
        return 1;
    }

    if (!loadMedia()) {
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(gWindow, gRenderer);
    ImGui_ImplSDLRenderer_Init(gRenderer);

    cpu ch8Cpu = {};
    ch8Cpu.pc = 0x200;
    systemState state = {};
    uint16_t instruction;
    int debug_count = 0;
    srand(time(NULL)); // TODO: check what passing in NULL does

    int err = 0;
    uint64_t performanceFreq = SDL_GetPerformanceFrequency();
    uint64_t timeLoopPrev = SDL_GetPerformanceCounter();
    uint64_t timeLoopCurr;
    uint64_t debugTimer = timeLoopPrev;
    double deltaSeconds;


    // GUI state vars
    bool emulatorRunning = false;
    bool emulatorPause = false;
    bool emulatorStep = false;
    const char* romPaths[] = {
        "../data/IBM Logo.ch8",
        "../data/ibm_logo_loop.ch8",
        "../data/test_opcode.ch8",
        "../data/bc_test.ch8",
        "../data/keyboard_test.ch8",
        "../data/Chip8 Picture.ch8",
        "../data/Airplane.ch8",
        "../data/Tank.ch8",
        "../data/Addition Problems [Paul C. Moews].ch8",
        "../data/Jumping X and O [Harry Kleinberg, 1977].ch8",
        "../data/Delay Timer Test [Matthew Mikolay, 2010].ch8",
        "../data/Clock Program [Bill Fisher, 1981].ch8",
        "../data/Life [GV Samways, 1980].ch8",
        "../data/Astro Dodge [Revival Studios, 2008].ch8",
        "../data/Blitz [David Winter].ch8",
        "../data/Cave.ch8",
        "../data/Animal Race [Brian Astle].ch8",
        "../data/Keypad Test [Hap, 2006].ch8",
        "../data/Minimal game [Revival Studios, 2007].ch8",
        "../data/Random Number Test [Matthew Mikolay, 2010].ch8",
        "../data/Division Test [Sergey Naydenov, 2010].ch8",
        "../data/Tron.ch8",
        "../data/chip8-test-rom-with-audio.ch8"
    };
    static int selected_rom_idx = 0;


    if (loadProgramToRam(&ch8Cpu, romPaths[selected_rom_idx]) < 0) {
        std::printf("Failed to load rom. Exiting.\n");
        return 1;
    };


    while (!state.quit && err == 0) {

        timeLoopCurr = SDL_GetPerformanceCounter();
        deltaSeconds = (double)(timeLoopCurr - timeLoopPrev)/performanceFreq;
        timeLoopPrev = timeLoopCurr;
        std::printf("elapsed seconds: %f\n", deltaSeconds);

        for (int i = 0; i < 10; i++) {
            handleQueuedEvents(&state);

            instruction = ((uint16_t)(ch8Cpu.ram[ch8Cpu.pc]) << 8) | ((uint16_t)(ch8Cpu.ram[ch8Cpu.pc+1]));
            // printf("ins: 0x%04X\n", instruction);
            debug_count++;

            if (runInstruction(instruction, &state, &ch8Cpu)) {
                ch8Cpu.pc += 2;
            }

            // return released key states back to idle
            for (uint8_t i = 0; i < 16; ++i) {
                if (state.keyStates[i] == keyState::RELEASED) {
                    state.keyStates[i] = keyState::IDLE;
                }
            }
        }

        // decrement timers
        if (ch8Cpu.delay_timer > 0) {
            ch8Cpu.delay_timer--;
        }
        if (ch8Cpu.sound_timer > 0) {
            Mix_PlayChannel(1, gTone, 0);
            ch8Cpu.sound_timer--;
        }

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // WINDOW: Hello World
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);                          // Create a window called "Hello, world!" and append into it.
            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // WINDOW: ROM Selection
        {
            // TODO replace with actual filelist fetching, replace 'romPaths'
            ImGui::Begin("Rom Browser", NULL);
            if (ImGui::BeginListBox("##roms", ImVec2(-FLT_MIN, 0))) {
                for (int n = 0; n < IM_ARRAYSIZE(romPaths); ++n) {
                    const bool is_selected = (selected_rom_idx == 0);
                    if (ImGui::Selectable(romPaths[n], is_selected)) {
                        selected_rom_idx = n;
                    }

                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndListBox();
            }
            ImGui::End();
        }

        // EMULATOR WINDOW
        SDL_Texture* scene;
        {
            ImGui::Begin(romPaths[selected_rom_idx], NULL,
                         //ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoCollapse);
            // control bar
            static bool started = false;
            {
                ImVec2 controllerSize(0, 25);
                ImGui::BeginChild(
                    "Emulator Control Bar",
                    controllerSize,
                    false,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
                );
                ImGui::BeginDisabled(started);
                if (ImGui::Button("Start")) {
                    started = true;
                }
                ImGui::SameLine();
                ImGui::EndDisabled();
                if (started) {
                    if (ImGui::Button("Stop")) {
                        started = false;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Step")) {
                    started = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Pause")) {
                    started = false;
                }
                ImGui::EndChild();
            }

            // emulator display
            scene = updateWindow(&ch8Cpu);
            ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
            ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
            ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);
            ImGui::Image(scene, ImVec2(SCREEN_WIDTH/2, SCREEN_HEIGHT/2), uv_min, uv_max, tint_col, border_col);
            ImGui::End();
        }

        // uncomment to get example functionality
        // ImGui::ShowDemoWindow();;


        ImGui::Render();
        SDL_RenderSetScale(gRenderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(gRenderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(gRenderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(gRenderer);
        SDL_DestroyTexture(scene);
    }
    double elapsed = (double)(SDL_GetPerformanceCounter() - debugTimer)/performanceFreq;
    printf("%d instructions in %lf nanoseconds\n", debug_count, elapsed);
    writeDisplayStateToFile(&ch8Cpu);
    close();
    return 0;
}
