#include "core/ScriptEngine.hpp"

ScriptEngine::~ScriptEngine() = default;

bool ScriptEngine::initialize(const std::filesystem::path &scriptPath)
{
    scriptPath_ = scriptPath;
    lastError_.clear();
    scriptVersion_ = 1;
    return true;
}

bool ScriptEngine::reloadIfChanged()
{
    return true;
}

void ScriptEngine::update(float deltaSeconds, const InputState &inputState, FrameState &frameState)
{
    (void)deltaSeconds;
    (void)inputState;
    frameState.drawCommands.clear();
}

const std::string &ScriptEngine::lastError() const
{
    return lastError_;
}

int ScriptEngine::scriptVersion() const
{
    return scriptVersion_;
}
