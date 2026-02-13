#include <SDL3/SDL.h>

#include <filesystem>
#include <iostream>

#include "core/AppCore.hpp"
#include "renderer_sdl/RendererSdl.hpp"

namespace
{
    std::filesystem::path findProjectRoot(const std::filesystem::path &startDir)
    {
        std::filesystem::path dir = startDir;
        while (!dir.empty())
        {
            if (std::filesystem::exists(dir / "CMakeLists.txt"))
            {
                return dir;
            }

            const std::filesystem::path parent = dir.parent_path();
            if (parent == dir)
            {
                break;
            }
            dir = parent;
        }

        return {};
    }

    std::filesystem::path resolveScriptPath()
    {
        std::filesystem::path fromWorkspace = std::filesystem::current_path() / "assets" / "scripts" / "main.lua";
        if (std::filesystem::exists(fromWorkspace))
        {
            return fromWorkspace;
        }

        const char *basePath = SDL_GetBasePath();
        if (basePath != nullptr)
        {
            const std::filesystem::path exeBase = std::filesystem::path(basePath).lexically_normal();

            const std::filesystem::path projectRoot = findProjectRoot(exeBase);
            std::filesystem::path fromProjectRoot = projectRoot / "assets" / "scripts" / "main.lua";
            if (std::filesystem::exists(fromProjectRoot))
            {
                return fromProjectRoot;
            }

            std::filesystem::path fromExeAssets = exeBase / "assets" / "scripts" / "main.lua";
            if (std::filesystem::exists(fromExeAssets))
            {
                return fromExeAssets;
            }

            std::filesystem::path fromExeParentAssets = exeBase.parent_path() / "assets" / "scripts" / "main.lua";
            if (std::filesystem::exists(fromExeParentAssets))
            {
                return fromExeParentAssets;
            }
        }

        return fromWorkspace;
    }
} // namespace

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("gui_pc", 360, 640, SDL_WINDOW_RESIZABLE);
    if (window == nullptr)
    {
        std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *sdlRenderer = SDL_CreateRenderer(window, nullptr);
    if (sdlRenderer == nullptr)
    {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    RendererSdl renderer(sdlRenderer);
    AppCore appCore;
    const std::filesystem::path scriptPath = resolveScriptPath();
    const std::filesystem::path assetRoot = scriptPath.parent_path().parent_path();
    std::cout << "Asset root: " << assetRoot.string() << '\n';
    std::cout << "Lua script: " << scriptPath.string() << '\n';
    if (!appCore.initialize(&renderer, scriptPath))
    {
        std::cerr << "App init failed: " << appCore.lastError() << '\n';
    }

    int shownScriptVersion = appCore.scriptVersion();
    SDL_SetWindowTitle(window, ("gui_pc hr" + std::to_string(shownScriptVersion)).c_str());

    int initialWidth = 0;
    int initialHeight = 0;
    SDL_GetWindowSize(window, &initialWidth, &initialHeight);
    appCore.onResize(initialWidth, initialHeight);

    bool running = true;
    InputState inputState;
    std::string lastReportedError;
    bool textInputActive = false;
    bool hasActiveTouch = false;
    SDL_FingerID activeFinger = 0;
    float touchX = 0.0F;
    float touchY = 0.0F;

    std::uint64_t previousCounter = SDL_GetTicks();

    while (running)
    {
        inputState.textInput.clear();
        inputState.backspacePressed = false;
        inputState.enterPressed = false;
        inputState.toggleThemePressed = false;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                appCore.onResize(event.window.data1, event.window.data2);
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE)
                {
                    running = false;
                }
                else if (event.key.key == SDLK_BACKSPACE)
                {
                    inputState.backspacePressed = true;
                }
                else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER)
                {
                    inputState.enterPressed = true;
                }
                else if (event.key.key == SDLK_T)
                {
                    inputState.toggleThemePressed = true;
                }
                break;
            case SDL_EVENT_TEXT_INPUT:
                if (event.text.text[0] != '\0')
                {
                    inputState.textInput += event.text.text;
                }
                break;
            case SDL_EVENT_FINGER_DOWN:
            {
                if (!hasActiveTouch)
                {
                    hasActiveTouch = true;
                    activeFinger = event.tfinger.fingerID;
                }

                if (hasActiveTouch && event.tfinger.fingerID == activeFinger)
                {
                    int windowWidth = 0;
                    int windowHeight = 0;
                    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
                    touchX = event.tfinger.x * static_cast<float>(windowWidth);
                    touchY = event.tfinger.y * static_cast<float>(windowHeight);
                }
                break;
            }
            case SDL_EVENT_FINGER_MOTION:
            {
                if (hasActiveTouch && event.tfinger.fingerID == activeFinger)
                {
                    int windowWidth = 0;
                    int windowHeight = 0;
                    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
                    touchX = event.tfinger.x * static_cast<float>(windowWidth);
                    touchY = event.tfinger.y * static_cast<float>(windowHeight);
                }
                break;
            }
            case SDL_EVENT_FINGER_UP:
                if (hasActiveTouch && event.tfinger.fingerID == activeFinger)
                {
                    hasActiveTouch = false;
                }
                break;
            default:
                break;
            }
        }

        float mouseX = 0.0F;
        float mouseY = 0.0F;
        const SDL_MouseButtonFlags mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
        const SDL_WindowFlags windowFlags = SDL_GetWindowFlags(window);
        const bool hasMouseFocus = (windowFlags & SDL_WINDOW_MOUSE_FOCUS) != 0;
        if (hasActiveTouch)
        {
            inputState.mouseX = touchX;
            inputState.mouseY = touchY;
            inputState.mouseDown = true;
        }
        else
        {
            inputState.mouseX = mouseX;
            inputState.mouseY = mouseY;
            inputState.mouseDown = hasMouseFocus && ((mouseButtons & SDL_BUTTON_LMASK) != 0);
        }

        const std::uint64_t currentCounter = SDL_GetTicks();
        const float deltaSeconds = static_cast<float>(currentCounter - previousCounter) / 1000.0F;
        previousCounter = currentCounter;

        appCore.tick(deltaSeconds, inputState);

        const bool wantTextInput = appCore.textInputActive();
        if (wantTextInput != textInputActive)
        {
            if (wantTextInput)
            {
                if (!SDL_StartTextInput(window))
                {
                    std::cerr << "Text input start failed: " << SDL_GetError() << '\n';
                }
            }
            else
            {
                SDL_StopTextInput(window);
            }
            textInputActive = wantTextInput;
        }

        const int currentScriptVersion = appCore.scriptVersion();
        if (currentScriptVersion != shownScriptVersion)
        {
            shownScriptVersion = currentScriptVersion;
            SDL_SetWindowTitle(window, ("gui_pc hr" + std::to_string(shownScriptVersion)).c_str());
        }

        if (!appCore.lastError().empty() && appCore.lastError() != lastReportedError)
        {
            lastReportedError = appCore.lastError();
            std::cerr << "App error: " << lastReportedError << '\n';
        }

        SDL_Delay(1);
    }

    if (textInputActive)
    {
        SDL_StopTextInput(window);
    }
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
