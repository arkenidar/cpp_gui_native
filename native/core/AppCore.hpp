#pragma once

#include <filesystem>
#include <string>

#include "core/IRenderer.hpp"
#include "core/ScriptEngine.hpp"
#include "core/Types.hpp"

class AppCore
{
public:
    bool initialize(IRenderer *renderer, const std::filesystem::path &scriptPath);
    void onResize(int width, int height);
    void tick(float deltaSeconds, const InputState &inputState);

    [[nodiscard]] const std::string &lastError() const;
    [[nodiscard]] int scriptVersion() const;
    [[nodiscard]] bool textInputActive() const;

private:
    IRenderer *renderer_ = nullptr;
    ScriptEngine scriptEngine_;
    FrameState frameState_;
    std::string lastError_;
};
