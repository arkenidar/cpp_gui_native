#pragma once

#include <string>
#include <vector>

enum class DrawCommandKind
{
    Rect,
    Circle,
    Line,
    Text,
};

struct DrawCommand
{
    DrawCommandKind kind = DrawCommandKind::Rect;
    bool filled = false;

    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;

    float x2 = 0.0F;
    float y2 = 0.0F;
    float radius = 0.0F;

    float r = 1.0F;
    float g = 1.0F;
    float b = 1.0F;

    std::string text;
};

struct InputState
{
    float mouseX = 0.0F;
    float mouseY = 0.0F;
    bool mouseDown = false;
    bool backspacePressed = false;
    bool enterPressed = false;
    bool toggleThemePressed = false;
    std::string textInput;
};

struct FrameState
{
    float bgR = 0.08F;
    float bgG = 0.10F;
    float bgB = 0.14F;

    bool textInputActive = false;

    std::vector<DrawCommand> drawCommands;
};
