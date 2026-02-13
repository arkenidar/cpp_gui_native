#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include <SDL3/SDL.h>

#if GUI_HAS_SDL_TTF
#include <SDL3_ttf/SDL_ttf.h>
#endif

#include "core/IRenderer.hpp"

class RendererSdl : public IRenderer
{
public:
    explicit RendererSdl(SDL_Renderer *sdlRenderer);
    ~RendererSdl() override;

    void resize(int width, int height) override;
    void beginFrame() override;
    void clear(float red, float green, float blue) override;
    void drawCircle(float centerX, float centerY, float radius, float red, float green, float blue) override;
    void fillCircle(float centerX, float centerY, float radius, float red, float green, float blue) override;
    void drawRect(float x, float y, float width, float height, float red, float green, float blue) override;
    void fillRect(float x, float y, float width, float height, float red, float green, float blue) override;
    void drawLine(float x1, float y1, float x2, float y2, float red, float green, float blue) override;
    void drawText(float x, float y, const std::string &text, float red, float green, float blue) override;
    void endFrame() override;

private:
    static std::uint8_t toByte(float value);

    SDL_Renderer *sdlRenderer_ = nullptr;
    int width_ = 360;
    int height_ = 640;

#if GUI_HAS_SDL_TTF
    TTF_Font *font_ = nullptr;
    bool ttfInitialized_ = false;
    std::string loadedFontPath_;

    bool tryLoadFont(const std::filesystem::path &fontPath, float pointSize);
#endif
};
