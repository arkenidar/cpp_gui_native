#include <android/log.h>
#include <android/native_activity.h>

#include <android/asset_manager.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "core/AppCore.hpp"

namespace
{
    bool extractAssetToFile(ANativeActivity *activity, const char *assetPath, const std::filesystem::path &destPath)
    {
        if (activity == nullptr || activity->assetManager == nullptr)
        {
            return false;
        }

        AAsset *asset = AAssetManager_open(activity->assetManager, assetPath, AASSET_MODE_STREAMING);
        if (asset == nullptr)
        {
            return false;
        }

        const off_t assetLength = AAsset_getLength(asset);
        if (assetLength <= 0)
        {
            AAsset_close(asset);
            return false;
        }

        std::vector<char> bytes(static_cast<size_t>(assetLength));
        const int bytesRead = AAsset_read(asset, bytes.data(), static_cast<size_t>(assetLength));
        AAsset_close(asset);
        if (bytesRead != assetLength)
        {
            return false;
        }

        std::error_code errorCode;
        std::filesystem::create_directories(destPath.parent_path(), errorCode);
        if (errorCode)
        {
            return false;
        }

        std::ofstream out(destPath, std::ios::binary | std::ios::trunc);
        if (!out.good())
        {
            return false;
        }

        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        return out.good();
    }

    std::filesystem::path prepareScriptPath(ANativeActivity *activity)
    {
        if (activity == nullptr || activity->internalDataPath == nullptr)
        {
            return {};
        }

        const std::filesystem::path internalDataPath(activity->internalDataPath);
        const std::filesystem::path scriptPath = internalDataPath / "scripts" / "main.lua";
        const bool extracted = extractAssetToFile(activity, "scripts/main.lua", scriptPath);
        if (!extracted)
        {
            __android_log_print(ANDROID_LOG_ERROR, "GUI_CPP", "Failed to extract asset scripts/main.lua");
            return {};
        }

        __android_log_print(ANDROID_LOG_INFO, "GUI_CPP", "Lua script prepared: %s", scriptPath.string().c_str());
        return scriptPath;
    }

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
    (void)savedState;
    (void)savedStateSize;

    const std::filesystem::path scriptPath = prepareScriptPath(activity);
    const bool initialized = g_appCore.initialize(&g_nullRenderer, scriptPath);
    if (initialized)
    {
        __android_log_print(ANDROID_LOG_INFO, "GUI_CPP", "Android NativeActivity scaffold initialized (core wired)");
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR, "GUI_CPP", "Core init failed: %s", g_appCore.lastError().c_str());
    }
}
