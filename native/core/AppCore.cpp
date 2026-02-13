#include "core/AppCore.hpp"

bool AppCore::initialize(IRenderer *renderer, const std::filesystem::path &scriptPath)
{
    renderer_ = renderer;
    if (renderer_ == nullptr)
    {
        lastError_ = "Renderer is null";
        return false;
    }

    if (!scriptEngine_.initialize(scriptPath))
    {
        lastError_ = scriptEngine_.lastError();
        return false;
    }

    lastError_.clear();
    return true;
}

void AppCore::onResize(int width, int height)
{
    if (renderer_ == nullptr)
    {
        return;
    }

    renderer_->resize(width, height);
}

void AppCore::tick(float deltaSeconds, const InputState &inputState)
{
    if (renderer_ == nullptr)
    {
        return;
    }

    if (!scriptEngine_.reloadIfChanged())
    {
        lastError_ = scriptEngine_.lastError();
    }

    scriptEngine_.update(deltaSeconds, inputState, frameState_);
    if (!scriptEngine_.lastError().empty())
    {
        lastError_ = scriptEngine_.lastError();
    }

    renderer_->beginFrame();
    renderer_->clear(frameState_.bgR, frameState_.bgG, frameState_.bgB);

    for (const DrawCommand &command : frameState_.drawCommands)
    {
        switch (command.kind)
        {
        case DrawCommandKind::Rect:
            if (command.filled)
            {
                renderer_->fillRect(command.x, command.y, command.width, command.height, command.r, command.g, command.b);
            }
            else
            {
                renderer_->drawRect(command.x, command.y, command.width, command.height, command.r, command.g, command.b);
            }
            break;
        case DrawCommandKind::Circle:
            if (command.filled)
            {
                renderer_->fillCircle(command.x, command.y, command.radius, command.r, command.g, command.b);
            }
            else
            {
                renderer_->drawCircle(command.x, command.y, command.radius, command.r, command.g, command.b);
            }
            break;
        case DrawCommandKind::Line:
            renderer_->drawLine(command.x, command.y, command.x2, command.y2, command.r, command.g, command.b);
            break;
        case DrawCommandKind::Text:
            renderer_->drawText(command.x, command.y, command.text, command.r, command.g, command.b);
            break;
        }
    }

    renderer_->endFrame();
}

const std::string &AppCore::lastError() const
{
    return lastError_;
}

int AppCore::scriptVersion() const
{
    return scriptEngine_.scriptVersion();
}

bool AppCore::textInputActive() const
{
    return frameState_.textInputActive;
}
