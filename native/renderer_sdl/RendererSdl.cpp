#include "renderer_sdl/RendererSdl.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

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
} // namespace

RendererSdl::RendererSdl(SDL_Renderer *sdlRenderer) : sdlRenderer_(sdlRenderer)
{
#if GUI_HAS_SDL_TTF
    ttfInitialized_ = TTF_Init();
    if (!ttfInitialized_)
    {
        std::cout << "Font mode: SDL debug text (TTF init failed)\n";
        return;
    }

    std::vector<std::filesystem::path> candidates;
    candidates.push_back(std::filesystem::current_path() / "assets" / "fonts" / "UiFont.ttf");

    const char *basePath = SDL_GetBasePath();
    if (basePath != nullptr)
    {
        const std::filesystem::path exeBase = std::filesystem::path(basePath).lexically_normal();
        const std::filesystem::path projectRoot = findProjectRoot(exeBase);
        candidates.push_back(projectRoot / "assets" / "fonts" / "UiFont.ttf");
        candidates.push_back(exeBase / "assets" / "fonts" / "UiFont.ttf");
        candidates.push_back(exeBase.parent_path() / "assets" / "fonts" / "UiFont.ttf");
    }

    candidates.push_back(std::filesystem::path("C:/Windows/Fonts/arial.ttf"));
    candidates.push_back(std::filesystem::path("C:/Windows/Fonts/segoeui.ttf"));

    for (const std::filesystem::path &candidate : candidates)
    {
        if (tryLoadFont(candidate, 16.0F))
        {
            break;
        }
    }

    if (font_ != nullptr)
    {
        std::cout << "Font mode: TTF " << loadedFontPath_ << "\n";
    }
    else
    {
        std::cout << "Font mode: SDL debug text (no font loaded)\n";
    }
#endif
}

RendererSdl::~RendererSdl()
{
#if GUI_HAS_SDL_TTF
    if (font_ != nullptr)
    {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }

    if (ttfInitialized_)
    {
        TTF_Quit();
        ttfInitialized_ = false;
    }
#endif
}

std::uint8_t RendererSdl::toByte(float value)
{
    const float clamped = std::clamp(value, 0.0F, 1.0F);
    return static_cast<std::uint8_t>(clamped * 255.0F);
}

void RendererSdl::resize(int width, int height)
{
    width_ = width;
    height_ = height;
}

void RendererSdl::beginFrame()
{
    if (sdlRenderer_ == nullptr)
    {
        return;
    }
}

void RendererSdl::clear(float red, float green, float blue)
{
    if (sdlRenderer_ == nullptr)
    {
        return;
    }

    SDL_SetRenderDrawColor(sdlRenderer_, toByte(red), toByte(green), toByte(blue), 255);
    SDL_RenderClear(sdlRenderer_);
}

void RendererSdl::drawCircle(float centerX, float centerY, float radius, float red, float green, float blue)
{
    if (sdlRenderer_ == nullptr)
    {
        return;
    }

    SDL_SetRenderDrawColor(sdlRenderer_, toByte(red), toByte(green), toByte(blue), 255);

    constexpr float pi = 3.1415926535F;
    const int segments = 64;
    const float step = (2.0F * pi) / static_cast<float>(segments);

    for (int index = 0; index < segments; ++index)
    {
        const float a0 = static_cast<float>(index) * step;
        const float a1 = static_cast<float>(index + 1) * step;

        const float x0 = centerX + std::cos(a0) * radius;
        const float y0 = centerY + std::sin(a0) * radius;
        const float x1 = centerX + std::cos(a1) * radius;
        const float y1 = centerY + std::sin(a1) * radius;

        SDL_RenderLine(sdlRenderer_, x0, y0, x1, y1);
    }
}

void RendererSdl::fillCircle(float centerX, float centerY, float radius, float red, float green, float blue)
{
    if (sdlRenderer_ == nullptr)
    {
        return;
    }

    SDL_SetRenderDrawColor(sdlRenderer_, toByte(red), toByte(green), toByte(blue), 255);

    const int minY = static_cast<int>(centerY - radius);
    const int maxY = static_cast<int>(centerY + radius);
    for (int y = minY; y <= maxY; ++y)
    {
        const float dy = static_cast<float>(y) - centerY;
        const float inside = radius * radius - dy * dy;
        if (inside < 0.0F)
        {
            continue;
        }

        const float dx = std::sqrt(inside);
        SDL_RenderLine(sdlRenderer_, centerX - dx, static_cast<float>(y), centerX + dx, static_cast<float>(y));
    }
}

void RendererSdl::drawRect(float x, float y, float width, float height, float red, float green, float blue)
{
    if (sdlRenderer_ == nullptr)
    {
        return;
    }

    SDL_SetRenderDrawColor(sdlRenderer_, toByte(red), toByte(green), toByte(blue), 255);
    SDL_FRect rect{x, y, width, height};
    SDL_RenderRect(sdlRenderer_, &rect);
}

void RendererSdl::fillRect(float x, float y, float width, float height, float red, float green, float blue)
{
    if (sdlRenderer_ == nullptr)
    {
        return;
    }

    SDL_SetRenderDrawColor(sdlRenderer_, toByte(red), toByte(green), toByte(blue), 255);
    SDL_FRect rect{x, y, width, height};
    SDL_RenderFillRect(sdlRenderer_, &rect);
}

void RendererSdl::drawLine(float x1, float y1, float x2, float y2, float red, float green, float blue)
{
    if (sdlRenderer_ == nullptr)
    {
        return;
    }

    SDL_SetRenderDrawColor(sdlRenderer_, toByte(red), toByte(green), toByte(blue), 255);
    SDL_RenderLine(sdlRenderer_, x1, y1, x2, y2);
}

void RendererSdl::drawText(float x, float y, const std::string &text, float red, float green, float blue)
{
    if (sdlRenderer_ == nullptr || text.empty())
    {
        return;
    }

#if GUI_HAS_SDL_TTF
    if (font_ != nullptr)
    {
        constexpr float ttfBaselineNudgeY = -4.0F;
        const SDL_Color color{toByte(red), toByte(green), toByte(blue), 255};
        SDL_Surface *surface = TTF_RenderText_Blended(font_, text.c_str(), text.size(), color);
        if (surface != nullptr)
        {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(sdlRenderer_, surface);
            if (texture != nullptr)
            {
                const SDL_FRect dest{x, y + ttfBaselineNudgeY, static_cast<float>(surface->w), static_cast<float>(surface->h)};
                SDL_RenderTexture(sdlRenderer_, texture, nullptr, &dest);
                SDL_DestroyTexture(texture);
            }
            SDL_DestroySurface(surface);
            return;
        }
    }
#endif

    SDL_SetRenderDrawColor(sdlRenderer_, toByte(red), toByte(green), toByte(blue), 255);
    SDL_RenderDebugText(sdlRenderer_, x, y, text.c_str());
}

void RendererSdl::endFrame()
{
    if (sdlRenderer_ == nullptr)
    {
        return;
    }

    SDL_RenderPresent(sdlRenderer_);
}

#if GUI_HAS_SDL_TTF
bool RendererSdl::tryLoadFont(const std::filesystem::path &fontPath, float pointSize)
{
    if (!std::filesystem::exists(fontPath))
    {
        return false;
    }

    font_ = TTF_OpenFont(fontPath.string().c_str(), pointSize);
    if (font_ != nullptr)
    {
        loadedFontPath_ = fontPath.string();
    }
    return font_ != nullptr;
}
#endif
