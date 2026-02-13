#pragma once

#include <string>

#include "core/Types.hpp"

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void resize(int width, int height) = 0;
    virtual void beginFrame() = 0;
    virtual void clear(float red, float green, float blue) = 0;
    virtual void drawCircle(float centerX, float centerY, float radius, float red, float green, float blue) = 0;
    virtual void fillCircle(float centerX, float centerY, float radius, float red, float green, float blue) = 0;
    virtual void drawRect(float x, float y, float width, float height, float red, float green, float blue) = 0;
    virtual void fillRect(float x, float y, float width, float height, float red, float green, float blue) = 0;
    virtual void drawLine(float x1, float y1, float x2, float y2, float red, float green, float blue) = 0;
    virtual void drawText(float x, float y, const std::string &text, float red, float green, float blue) = 0;
    virtual void endFrame() = 0;
};
