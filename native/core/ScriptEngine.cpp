#include "core/ScriptEngine.hpp"

#include <iostream>
#include <memory>
#include <utility>

#include <sol/sol.hpp>

namespace
{
    float numberOr(float fallback, const sol::table &table, const char *key)
    {
        sol::optional<float> value = table[key];
        return value.value_or(fallback);
    }

    bool boolOr(bool fallback, const sol::table &table, const char *key)
    {
        sol::optional<bool> value = table[key];
        return value.value_or(fallback);
    }

    std::string stringOr(const std::string &fallback, const sol::table &table, const char *key)
    {
        sol::optional<std::string> value = table[key];
        return value.value_or(fallback);
    }

    DrawCommandKind drawCommandKindOr(DrawCommandKind fallback, const sol::table &table, const char *key)
    {
        const std::string kind = stringOr("", table, key);
        if (kind == "rect")
        {
            return DrawCommandKind::Rect;
        }
        if (kind == "circle")
        {
            return DrawCommandKind::Circle;
        }
        if (kind == "line")
        {
            return DrawCommandKind::Line;
        }
        if (kind == "text")
        {
            return DrawCommandKind::Text;
        }
        return fallback;
    }
} // namespace

class ScriptEngine::Impl
{
public:
    sol::state lua;
};

ScriptEngine::~ScriptEngine()
{
    delete impl_;
    impl_ = nullptr;
}

bool ScriptEngine::initialize(const std::filesystem::path &scriptPath)
{
    scriptPath_ = scriptPath;

    delete impl_;
    impl_ = nullptr;

    impl_ = new Impl();
    impl_->lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::package, sol::lib::string);

    if (!std::filesystem::exists(scriptPath_))
    {
        lastError_ = "Lua script not found: " + scriptPath_.string();
        return false;
    }

    lastWriteTime_ = std::filesystem::last_write_time(scriptPath_);
    return evaluateCurrentScript();
}

bool ScriptEngine::evaluateCurrentScript()
{
    sol::protected_function_result result = impl_->lua.safe_script_file(scriptPath_.string(), &sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error err = result;
        lastError_ = err.what();
        return false;
    }

    ++scriptVersion_;
    lastError_.clear();
    return true;
}

bool ScriptEngine::reloadIfChanged()
{
    if (!std::filesystem::exists(scriptPath_))
    {
        return true;
    }

    const auto newWriteTime = std::filesystem::last_write_time(scriptPath_);
    if (newWriteTime == lastWriteTime_)
    {
        return true;
    }

    lastWriteTime_ = newWriteTime;
    const bool reloaded = evaluateCurrentScript();
    if (reloaded)
    {
        std::cout << "Lua reloaded (v" << scriptVersion_ << "): " << scriptPath_.string() << '\n';
    }
    else
    {
        std::cerr << "Lua reload failed: " << scriptPath_.string() << " | " << lastError_ << '\n';
    }
    return reloaded;
}

void ScriptEngine::update(float deltaSeconds, const InputState &inputState, FrameState &frameState)
{
    if (impl_ == nullptr)
    {
        return;
    }

    sol::protected_function updateFn = impl_->lua["update"];
    if (!updateFn.valid())
    {
        return;
    }

    sol::table input = impl_->lua.create_table();
    input["mouse_x"] = inputState.mouseX;
    input["mouse_y"] = inputState.mouseY;
    input["mouse_down"] = inputState.mouseDown;
    input["text_input"] = inputState.textInput;
    input["backspace_pressed"] = inputState.backspacePressed;
    input["enter_pressed"] = inputState.enterPressed;
    input["toggle_theme_pressed"] = inputState.toggleThemePressed;

    sol::protected_function_result updateResult = updateFn(deltaSeconds, input);
    if (!updateResult.valid())
    {
        sol::error err = updateResult;
        lastError_ = err.what();
        return;
    }

    if (updateResult.return_count() < 1)
    {
        return;
    }

    sol::optional<sol::table> maybeState = updateResult;
    if (!maybeState.has_value())
    {
        return;
    }

    const sol::table state = maybeState.value();
    frameState.bgR = numberOr(frameState.bgR, state, "bg_r");
    frameState.bgG = numberOr(frameState.bgG, state, "bg_g");
    frameState.bgB = numberOr(frameState.bgB, state, "bg_b");
    frameState.textInputActive = boolOr(frameState.textInputActive, state, "text_input_active");

    frameState.drawCommands.clear();
    sol::optional<sol::table> maybeCommands = state["draw_commands"];
    if (maybeCommands.has_value())
    {
        const sol::table commands = maybeCommands.value();
        for (const auto &entry : commands)
        {
            const sol::object value = entry.second;
            if (!value.is<sol::table>())
            {
                continue;
            }

            const sol::table commandTable = value.as<sol::table>();
            DrawCommand command;
            command.kind = drawCommandKindOr(command.kind, commandTable, "kind");
            command.filled = boolOr(command.filled, commandTable, "filled");

            command.x = numberOr(command.x, commandTable, "x");
            command.y = numberOr(command.y, commandTable, "y");
            command.width = numberOr(command.width, commandTable, "w");
            command.height = numberOr(command.height, commandTable, "h");
            command.x2 = numberOr(command.x2, commandTable, "x2");
            command.y2 = numberOr(command.y2, commandTable, "y2");
            command.radius = numberOr(command.radius, commandTable, "radius");

            command.r = numberOr(command.r, commandTable, "r");
            command.g = numberOr(command.g, commandTable, "g");
            command.b = numberOr(command.b, commandTable, "b");
            command.text = stringOr(command.text, commandTable, "text");

            frameState.drawCommands.push_back(std::move(command));
        }
    }
}

const std::string &ScriptEngine::lastError() const
{
    return lastError_;
}

int ScriptEngine::scriptVersion() const
{
    return scriptVersion_;
}
