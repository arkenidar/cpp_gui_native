#pragma once

#include <filesystem>
#include <string>

#include "core/Types.hpp"

class ScriptEngine
{
public:
    ~ScriptEngine();

    bool initialize(const std::filesystem::path &scriptPath);
    bool reloadIfChanged();
    void update(float deltaSeconds, const InputState &inputState, FrameState &frameState);

    [[nodiscard]] const std::string &lastError() const;
    [[nodiscard]] int scriptVersion() const;

private:
    bool evaluateCurrentScript();

    std::filesystem::path scriptPath_;
    std::filesystem::file_time_type lastWriteTime_{};
    std::string lastError_;
    int scriptVersion_ = 0;

    class Impl;
    Impl *impl_ = nullptr;
};
