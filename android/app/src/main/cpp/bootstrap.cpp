#include <android/log.h>
#include <android/native_activity.h>

#include <filesystem>
#include <string>

#include "core/AppCore.hpp"

namespace
{
    class NullRenderer final : public IRenderer
    {
    public:
        void resize(int width, int height) override
        {
            (void)width;
            (void)height;
        }

        void beginFrame() override {}
        void clear(float red, float green, float blue) override
        {
            (void)red;
            (void)green;
            (void)blue;
        }

        void drawCircle(float centerX, float centerY, float radius, float red, float green, float blue) override
        {
            (void)centerX;
            (void)centerY;
            (void)radius;
            (void)red;
            (void)green;
            (void)blue;
        }

        void fillCircle(float centerX, float centerY, float radius, float red, float green, float blue) override
        {
            (void)centerX;
            (void)centerY;
            (void)radius;
            (void)red;
            (void)green;
            (void)blue;
        }

        void drawRect(float x, float y, float width, float height, float red, float green, float blue) override
        {
            (void)x;
            (void)y;
            (void)width;
            (void)height;
            (void)red;
            (void)green;
            (void)blue;
        }

        void fillRect(float x, float y, float width, float height, float red, float green, float blue) override
        {
            (void)x;
            (void)y;
            (void)width;
            (void)height;
            (void)red;
            (void)green;
            (void)blue;
        }

        void drawLine(float x1, float y1, float x2, float y2, float red, float green, float blue) override
        {
            (void)x1;
            (void)y1;
            (void)x2;
            (void)y2;
            (void)red;
            (void)green;
            (void)blue;
        }

        void drawText(float x, float y, const std::string &text, float red, float green, float blue) override
        {
            (void)x;
            (void)y;
            (void)text;
            (void)red;
            (void)green;
            (void)blue;
        }

        void endFrame() override {}
    };

    AppCore g_appCore;
    NullRenderer g_nullRenderer;
} // namespace

extern "C" void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize)
{
    (void)activity;
    (void)savedState;
    (void)savedStateSize;

    const bool initialized = g_appCore.initialize(&g_nullRenderer, std::filesystem::path{});
    if (initialized)
    {
        __android_log_print(ANDROID_LOG_INFO, "GUI_CPP", "Android NativeActivity scaffold initialized (core wired)");
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR, "GUI_CPP", "Core init failed: %s", g_appCore.lastError().c_str());
    }
}
