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

SDL_Texture* updateWindow(chip8 *p_chip8State) {
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
        if (p_chip8State->display[i]) {
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

void writeDisplayStateToFile(chip8 *p_chip8State) {
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

        if (p_chip8State->display[i]) {
            printf("X");
            fputc('X', pFile);
        } else {
            printf("-");
            fputc('-', pFile);
        };
    }
    fclose(pFile);
}

void handleQueuedEvents(chip8* p_chip8State) {
    static SDL_Event e = {};

    while (SDL_PollEvent(&e) != 0) {
        ImGui_ImplSDL2_ProcessEvent(&e);
        if (e.type == SDL_QUIT)
            p_chip8State->quit = true;
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE && e.window.windowID == SDL_GetWindowID(gWindow))
            p_chip8State->quit = true;
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            keyState pressed = (e.type == SDL_KEYDOWN) ?
                keyState::PRESSED : keyState::RELEASED;
            printf("key down event: %d\n", e.key.keysym.scancode);
            switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_1: p_chip8State->keyStates[1]  = pressed; break;
                case SDL_SCANCODE_2: p_chip8State->keyStates[2]  = pressed; break;
                case SDL_SCANCODE_3: p_chip8State->keyStates[3]  = pressed; break;
                case SDL_SCANCODE_4: p_chip8State->keyStates[12] = pressed; break;
                case SDL_SCANCODE_Q: p_chip8State->keyStates[4]  = pressed; break;
                case SDL_SCANCODE_W: p_chip8State->keyStates[5]  = pressed; break;
                case SDL_SCANCODE_E: p_chip8State->keyStates[6]  = pressed; break;
                case SDL_SCANCODE_R: p_chip8State->keyStates[13] = pressed; break;
                case SDL_SCANCODE_A: p_chip8State->keyStates[7]  = pressed; break;
                case SDL_SCANCODE_S: p_chip8State->keyStates[8]  = pressed; break;
                case SDL_SCANCODE_D: p_chip8State->keyStates[9]  = pressed; break;
                case SDL_SCANCODE_F: p_chip8State->keyStates[14] = pressed; break;
                case SDL_SCANCODE_Z: p_chip8State->keyStates[10] = pressed; break;
                case SDL_SCANCODE_X: p_chip8State->keyStates[0]  = pressed; break;
                case SDL_SCANCODE_C: p_chip8State->keyStates[11] = pressed; break;
                case SDL_SCANCODE_V: p_chip8State->keyStates[15] = pressed; break;
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

    chip8 chip8State = {};
    uint16_t instruction;
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
    static int selected_rom_idx = 9;


    if (!initEmulatorWithRom(&chip8State, romPaths[selected_rom_idx])) {
        std::printf("Failed to load rom. Exiting.\n");
        return 1;
    };

    const ImGuiWindowFlags COMMON_WINDOW_FLAGS = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    
    while (!chip8State.quit && err == 0) {
        timeLoopCurr = SDL_GetPerformanceCounter();
        deltaSeconds = (double)(timeLoopCurr - timeLoopPrev)/performanceFreq;
        timeLoopPrev = timeLoopCurr;
        // std::printf("elapsed seconds: %f\n", deltaSeconds);

        // Emulate
        // TODO make sure key events consider focused window
        handleQueuedEvents(&chip8State);
//        if (chip8State.running) {
//            ch8Step(&chip8State);
//        }
        ch8Step(&chip8State);


        // Render GUI
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        

        // WINDOW: EMULATOR
        ImVec2 emulatorWindowPos;
        float emulatorWindowHeight;
        float emulatorWindowWidth;
        SDL_Texture* scene;
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            // 2/3rds of 1080:1920
            ImGui::Begin("Chip8 Emulator", NULL,
                         COMMON_WINDOW_FLAGS |
                         ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse);
            

            // CHILD: Controller Bar
            {
                ImVec2 controllerSize(0, 25);
                ImGui::BeginChild(
                    "Emulator Control Bar",
                    controllerSize,
                    false,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
                );

                if (emulatorRunning) {
                    if (ImGui::Button("Stop")) {
                        emulatorRunning = true;
                    }
                } else {
                    if (ImGui::Button("Start")) {
                        emulatorRunning = false;
                    }
                }
                ImGui::SameLine();

                if (emulatorPause) {
                    if (ImGui::Button("Step")) {
                        emulatorPause = false;
                    }
                    if (ImGui::Button("Pause")) {
                        emulatorStep= false;
                    }
                    ImGui::SameLine();
                }
                ImGui::EndChild();
            }

            // CHILD: Emulator Display
            scene = updateWindow(&chip8State);
            ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
            ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
            ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);
            ImGui::Image(scene, ImVec2((SCREEN_WIDTH/3)*2, (SCREEN_HEIGHT/3)*2), uv_min, uv_max, tint_col, border_col);

            emulatorWindowPos = ImGui::GetWindowPos();
            emulatorWindowWidth = ImGui::GetWindowWidth();
            emulatorWindowHeight = ImGui::GetWindowHeight();
            ImGui::End();
        }

        // WINDOW: LOGGING
        {
            static float f = 0.0f;
            static int counter = 0;
            ImGui::SetNextWindowPos(ImVec2(0, emulatorWindowPos.y + emulatorWindowHeight));
            ImGui::SetNextWindowSize(ImVec2(emulatorWindowWidth, SCREEN_HEIGHT - emulatorWindowHeight));
            ImGui::Begin("System Log", NULL, COMMON_WINDOW_FLAGS); 
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
        static bool newRomLoaded = false;
        {
            // TODO replace with actual filelist fetching, replace 'romPaths'
            ImGui::SetNextWindowPos(ImVec2(emulatorWindowWidth, 0));
            ImGui::SetNextWindowSize(ImVec2(SCREEN_WIDTH - emulatorWindowWidth, SCREEN_HEIGHT));
            ImGui::Begin("CPU State", NULL, COMMON_WINDOW_FLAGS);
            ImGui::Text("PC: Ox%04X", chip8State.pc);
            ImGui::Text("Index: 0x%04X", chip8State.index);
            ImGui::End();
        }

        // uncomment to get example functionality
        // ImGui::ShowDemoWindow();;

        ImGui::Render();
        SDL_RenderSetScale(
            gRenderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y
        );
        SDL_SetRenderDrawColor(
            gRenderer,
            (Uint8)(clear_color.x * 255),
            (Uint8)(clear_color.y * 255),
            (Uint8)(clear_color.z * 255),
            (Uint8)(clear_color.w * 255)
        );
        SDL_RenderClear(gRenderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(gRenderer);
        SDL_DestroyTexture(scene);
    }
    double elapsed = (double)(SDL_GetPerformanceCounter() - debugTimer)/performanceFreq;
    //printf("%d instructions in %lf nanoseconds\n", debug_count, elapsed);
    writeDisplayStateToFile(&chip8State);
    close();
    return 0;
}
