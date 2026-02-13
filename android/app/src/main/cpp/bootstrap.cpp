#include <android/log.h>
#include <android/native_activity.h>

#include <android/asset_manager.h>
#include <android/input.h>
#include <android/looper.h>
#include <android/native_window.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <jni.h>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "core/AppCore.hpp"

namespace
{
    constexpr int kLooperInputId = 1;

    std::string keycodeToText(int keycode, int metaState)
    {
        const bool shifted = (metaState & AMETA_SHIFT_ON) != 0 ||
                             (metaState & AMETA_SHIFT_LEFT_ON) != 0 ||
                             (metaState & AMETA_SHIFT_RIGHT_ON) != 0;

        if (keycode >= AKEYCODE_A && keycode <= AKEYCODE_Z)
        {
            char character = static_cast<char>('a' + (keycode - AKEYCODE_A));
            if (shifted)
            {
                character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
            }
            return std::string(1, character);
        }

        if (keycode >= AKEYCODE_0 && keycode <= AKEYCODE_9)
        {
            return std::string(1, static_cast<char>('0' + (keycode - AKEYCODE_0)));
        }

        switch (keycode)
        {
        case AKEYCODE_SPACE:
            return " ";
        case AKEYCODE_PERIOD:
            return ".";
        case AKEYCODE_COMMA:
            return ",";
        case AKEYCODE_MINUS:
            return "-";
        case AKEYCODE_EQUALS:
            return "=";
        case AKEYCODE_SEMICOLON:
            return ";";
        case AKEYCODE_APOSTROPHE:
            return "'";
        case AKEYCODE_SLASH:
            return "/";
        default:
            return "";
        }
    }

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

    std::filesystem::path prepareFontPath(ANativeActivity *activity)
    {
        if (activity == nullptr || activity->internalDataPath == nullptr)
        {
            return {};
        }

        const std::filesystem::path internalDataPath(activity->internalDataPath);
        const std::filesystem::path fontPath = internalDataPath / "fonts" / "UiFont.ttf";
        const bool extracted = extractAssetToFile(activity, "fonts/UiFont.ttf", fontPath);
        if (!extracted)
        {
            __android_log_print(ANDROID_LOG_ERROR, "GUI_CPP", "Failed to extract asset fonts/UiFont.ttf");
            return {};
        }

        __android_log_print(ANDROID_LOG_INFO, "GUI_CPP", "UI font prepared: %s", fontPath.string().c_str());
        return fontPath;
    }

    class AndroidSoftwareRenderer final : public IRenderer
    {
    public:
        void setUiScale(float scale)
        {
            uiScale_ = std::max(1.0F, scale);
            if (fontReady_)
            {
                glyphCache_.clear();
                updateFontScale();
            }
        }

        void setFontPath(const std::filesystem::path &fontPath, float pixelSize)
        {
            fontReady_ = false;
            glyphCache_.clear();
            fontFileData_.clear();

            std::ifstream in(fontPath, std::ios::binary | std::ios::ate);
            if (!in.good())
            {
                return;
            }

            const std::streamsize size = in.tellg();
            if (size <= 0)
            {
                return;
            }

            in.seekg(0, std::ios::beg);
            fontFileData_.resize(static_cast<size_t>(size));
            if (!in.read(reinterpret_cast<char *>(fontFileData_.data()), size))
            {
                fontFileData_.clear();
                return;
            }

            if (!stbtt_InitFont(&fontInfo_, fontFileData_.data(), 0))
            {
                fontFileData_.clear();
                return;
            }

            fontPixelSize_ = pixelSize;
            updateFontScale();
            stbtt_GetFontVMetrics(&fontInfo_, &fontAscent_, &fontDescent_, &fontLineGap_);
            fontReady_ = true;
        }

        void setWindow(ANativeWindow *window)
        {
            window_ = window;
            if (window_ != nullptr)
            {
                ANativeWindow_setBuffersGeometry(window_, 0, 0, WINDOW_FORMAT_RGBA_8888);
            }
        }

        void resize(int width, int height) override
        {
            width_ = width;
            height_ = height;
        }

        void beginFrame() override
        {
            if (window_ == nullptr)
            {
                pixels_ = nullptr;
                return;
            }

            if (ANativeWindow_lock(window_, &buffer_, nullptr) != 0)
            {
                pixels_ = nullptr;
                return;
            }

            pixels_ = static_cast<std::uint32_t *>(buffer_.bits);
        }

        void clear(float red, float green, float blue) override
        {
            if (pixels_ == nullptr)
            {
                return;
            }

            const std::uint32_t color = packColor(red, green, blue);
            for (int y = 0; y < buffer_.height; ++y)
            {
                std::uint32_t *row = pixels_ + y * buffer_.stride;
                std::fill(row, row + buffer_.width, color);
            }
        }

        void drawCircle(float centerX, float centerY, float radius, float red, float green, float blue) override
        {
            if (pixels_ == nullptr)
            {
                return;
            }

            const float pxCenterX = toPx(centerX);
            const float pxCenterY = toPx(centerY);
            const float pxRadius = toPx(radius);

            constexpr float pi = 3.1415926535F;
            const int segments = 64;
            const float step = (2.0F * pi) / static_cast<float>(segments);
            const std::uint32_t color = packColor(red, green, blue);

            for (int i = 0; i < segments; ++i)
            {
                const float a0 = static_cast<float>(i) * step;
                const float a1 = static_cast<float>(i + 1) * step;
                drawLineInternal(pxCenterX + std::cos(a0) * pxRadius,
                                 pxCenterY + std::sin(a0) * pxRadius,
                                 pxCenterX + std::cos(a1) * pxRadius,
                                 pxCenterY + std::sin(a1) * pxRadius,
                                 color);
            }
        }

        void fillCircle(float centerX, float centerY, float radius, float red, float green, float blue) override
        {
            if (pixels_ == nullptr)
            {
                return;
            }

            const float pxCenterX = toPx(centerX);
            const float pxCenterY = toPx(centerY);
            const float pxRadius = toPx(radius);

            const std::uint32_t color = packColor(red, green, blue);
            const int minY = static_cast<int>(std::floor(pxCenterY - pxRadius));
            const int maxY = static_cast<int>(std::ceil(pxCenterY + pxRadius));
            for (int y = minY; y <= maxY; ++y)
            {
                const float dy = static_cast<float>(y) - pxCenterY;
                const float inside = pxRadius * pxRadius - dy * dy;
                if (inside < 0.0F)
                {
                    continue;
                }
                const float dx = std::sqrt(inside);
                drawLineInternal(pxCenterX - dx, static_cast<float>(y), pxCenterX + dx, static_cast<float>(y), color);
            }
        }

        void drawRect(float x, float y, float width, float height, float red, float green, float blue) override
        {
            if (pixels_ == nullptr)
            {
                return;
            }

            const float pxX = toPx(x);
            const float pxY = toPx(y);
            const float pxWidth = toPx(width);
            const float pxHeight = toPx(height);
            const std::uint32_t color = packColor(red, green, blue);
            const float x2 = pxX + pxWidth;
            const float y2 = pxY + pxHeight;
            drawLineInternal(pxX, pxY, x2, pxY, color);
            drawLineInternal(x2, pxY, x2, y2, color);
            drawLineInternal(x2, y2, pxX, y2, color);
            drawLineInternal(pxX, y2, pxX, pxY, color);
        }

        void fillRect(float x, float y, float width, float height, float red, float green, float blue) override
        {
            if (pixels_ == nullptr)
            {
                return;
            }

            const float pxX = toPx(x);
            const float pxY = toPx(y);
            const float pxWidth = toPx(width);
            const float pxHeight = toPx(height);
            const int x0 = clampX(static_cast<int>(std::floor(pxX)));
            const int y0 = clampY(static_cast<int>(std::floor(pxY)));
            const int x1 = clampX(static_cast<int>(std::ceil(pxX + pxWidth)));
            const int y1 = clampY(static_cast<int>(std::ceil(pxY + pxHeight)));
            const std::uint32_t color = packColor(red, green, blue);

            for (int yy = y0; yy <= y1; ++yy)
            {
                for (int xx = x0; xx <= x1; ++xx)
                {
                    putPixel(xx, yy, color);
                }
            }
        }

        void drawLine(float x1, float y1, float x2, float y2, float red, float green, float blue) override
        {
            if (pixels_ == nullptr)
            {
                return;
            }

            drawLineInternal(toPx(x1), toPx(y1), toPx(x2), toPx(y2), packColor(red, green, blue));
        }

        void drawText(float x, float y, const std::string &text, float red, float green, float blue) override
        {
            if (pixels_ == nullptr || text.empty())
            {
                return;
            }

            const float pxX = toPx(x);
            const float pxY = toPx(y);

            if (!fontReady_)
            {
                int cursorX = static_cast<int>(pxX);
                const int baseline = static_cast<int>(pxY);
                for (char ch : text)
                {
                    if (ch != ' ')
                    {
                        const float barW = std::max(1.0F, std::floor(4.0F * uiScale_));
                        const float barH = std::max(1.0F, std::floor(8.0F * uiScale_));
                        const float barY = static_cast<float>(baseline) - barH;
                        const int x0 = clampX(static_cast<int>(std::floor(static_cast<float>(cursorX))));
                        const int y0 = clampY(static_cast<int>(std::floor(barY)));
                        const int x1 = clampX(static_cast<int>(std::ceil(static_cast<float>(cursorX) + barW)));
                        const int y1 = clampY(static_cast<int>(std::ceil(barY + barH)));
                        const std::uint32_t color = packColor(red, green, blue);
                        for (int yy = y0; yy <= y1; ++yy)
                        {
                            for (int xx = x0; xx <= x1; ++xx)
                            {
                                putPixel(xx, yy, color);
                            }
                        }
                    }
                    cursorX += static_cast<int>(std::max(1.0F, std::floor(6.0F * uiScale_)));
                }
                return;
            }

            const std::uint8_t srcR = toByte(red);
            const std::uint8_t srcG = toByte(green);
            const std::uint8_t srcB = toByte(blue);

            float penX = pxX;
            const float baseline = pxY + fontPixelSizePx_;
            std::size_t offset = 0;

            while (offset < text.size())
            {
                int codepoint = 0;
                const std::size_t consumed = decodeUtf8Codepoint(text, offset, codepoint);
                if (consumed == 0)
                {
                    break;
                }
                offset += consumed;

                const GlyphBitmap &glyph = getGlyph(codepoint);
                if (!glyph.bitmap.empty() && glyph.width > 0 && glyph.height > 0)
                {
                    const int drawX = static_cast<int>(std::lround(penX + static_cast<float>(glyph.xOffset)));
                    const int drawY = static_cast<int>(std::lround(baseline + static_cast<float>(glyph.yOffset)));

                    for (int gy = 0; gy < glyph.height; ++gy)
                    {
                        for (int gx = 0; gx < glyph.width; ++gx)
                        {
                            const std::uint8_t alpha = glyph.bitmap[static_cast<std::size_t>(gy * glyph.width + gx)];
                            if (alpha == 0)
                            {
                                continue;
                            }
                            blendPixel(drawX + gx, drawY + gy, srcR, srcG, srcB, alpha);
                        }
                    }
                }

                penX += static_cast<float>(glyph.advance);
                if (offset < text.size())
                {
                    int nextCodepoint = 0;
                    const std::size_t peek = decodeUtf8Codepoint(text, offset, nextCodepoint);
                    if (peek > 0)
                    {
                        penX += fontScale_ * static_cast<float>(stbtt_GetCodepointKernAdvance(&fontInfo_, codepoint, nextCodepoint));
                    }
                }
            }
        }

        void endFrame() override
        {
            if (pixels_ == nullptr || window_ == nullptr)
            {
                return;
            }

            ANativeWindow_unlockAndPost(window_);
            pixels_ = nullptr;
        }

    private:
        static std::uint8_t toByte(float value)
        {
            const float clamped = std::clamp(value, 0.0F, 1.0F);
            return static_cast<std::uint8_t>(clamped * 255.0F);
        }

        float toPx(float value) const
        {
            return value * uiScale_;
        }

        static std::uint32_t packColor(float red, float green, float blue)
        {
            const std::uint32_t r = static_cast<std::uint32_t>(toByte(red));
            const std::uint32_t g = static_cast<std::uint32_t>(toByte(green));
            const std::uint32_t b = static_cast<std::uint32_t>(toByte(blue));
            return 0xFF000000U | (b << 16U) | (g << 8U) | r;
        }

        int clampX(int x) const
        {
            return std::clamp(x, 0, std::max(0, buffer_.width - 1));
        }

        int clampY(int y) const
        {
            return std::clamp(y, 0, std::max(0, buffer_.height - 1));
        }

        void putPixel(int x, int y, std::uint32_t color)
        {
            if (pixels_ == nullptr)
            {
                return;
            }
            if (x < 0 || y < 0 || x >= buffer_.width || y >= buffer_.height)
            {
                return;
            }
            pixels_[y * buffer_.stride + x] = color;
        }

        void blendPixel(int x, int y, std::uint8_t srcR, std::uint8_t srcG, std::uint8_t srcB, std::uint8_t alpha)
        {
            if (pixels_ == nullptr)
            {
                return;
            }
            if (x < 0 || y < 0 || x >= buffer_.width || y >= buffer_.height)
            {
                return;
            }

            std::uint32_t &dst = pixels_[y * buffer_.stride + x];
            const std::uint8_t dstR = static_cast<std::uint8_t>(dst & 0xFFU);
            const std::uint8_t dstG = static_cast<std::uint8_t>((dst >> 8U) & 0xFFU);
            const std::uint8_t dstB = static_cast<std::uint8_t>((dst >> 16U) & 0xFFU);

            const int a = static_cast<int>(alpha);
            const std::uint8_t outR = static_cast<std::uint8_t>((srcR * a + dstR * (255 - a)) / 255);
            const std::uint8_t outG = static_cast<std::uint8_t>((srcG * a + dstG * (255 - a)) / 255);
            const std::uint8_t outB = static_cast<std::uint8_t>((srcB * a + dstB * (255 - a)) / 255);

            dst = 0xFF000000U | (static_cast<std::uint32_t>(outB) << 16U) |
                  (static_cast<std::uint32_t>(outG) << 8U) |
                  static_cast<std::uint32_t>(outR);
        }

        struct GlyphBitmap
        {
            int width = 0;
            int height = 0;
            int xOffset = 0;
            int yOffset = 0;
            int advance = 0;
            std::vector<std::uint8_t> bitmap;
        };

        const GlyphBitmap &getGlyph(int codepoint)
        {
            auto it = glyphCache_.find(codepoint);
            if (it != glyphCache_.end())
            {
                return it->second;
            }

            GlyphBitmap glyph;
            int advance = 0;
            int leftSideBearing = 0;
            stbtt_GetCodepointHMetrics(&fontInfo_, codepoint, &advance, &leftSideBearing);
            glyph.advance = static_cast<int>(std::lround(fontScale_ * static_cast<float>(advance)));
            if (glyph.advance <= 0)
            {
                glyph.advance = static_cast<int>(std::lround(fontPixelSizePx_ * 0.5F));
            }

            unsigned char *bitmap = stbtt_GetCodepointBitmap(&fontInfo_, 0.0F, fontScale_, codepoint,
                                                             &glyph.width, &glyph.height,
                                                             &glyph.xOffset, &glyph.yOffset);
            if (bitmap != nullptr && glyph.width > 0 && glyph.height > 0)
            {
                glyph.bitmap.assign(bitmap, bitmap + static_cast<std::size_t>(glyph.width * glyph.height));
                stbtt_FreeBitmap(bitmap, nullptr);
            }

            auto [insertedIt, inserted] = glyphCache_.emplace(codepoint, std::move(glyph));
            (void)inserted;
            return insertedIt->second;
        }

        void updateFontScale()
        {
            fontPixelSizePx_ = std::max(1.0F, fontPixelSize_ * uiScale_);
            fontScale_ = stbtt_ScaleForPixelHeight(&fontInfo_, fontPixelSizePx_);
        }

        static std::size_t decodeUtf8Codepoint(const std::string &text, std::size_t offset, int &codepoint)
        {
            if (offset >= text.size())
            {
                return 0;
            }

            const unsigned char c0 = static_cast<unsigned char>(text[offset]);
            if ((c0 & 0x80U) == 0)
            {
                codepoint = c0;
                return 1;
            }
            if ((c0 & 0xE0U) == 0xC0U && offset + 1 < text.size())
            {
                const unsigned char c1 = static_cast<unsigned char>(text[offset + 1]);
                codepoint = static_cast<int>(((c0 & 0x1FU) << 6U) | (c1 & 0x3FU));
                return 2;
            }
            if ((c0 & 0xF0U) == 0xE0U && offset + 2 < text.size())
            {
                const unsigned char c1 = static_cast<unsigned char>(text[offset + 1]);
                const unsigned char c2 = static_cast<unsigned char>(text[offset + 2]);
                codepoint = static_cast<int>(((c0 & 0x0FU) << 12U) | ((c1 & 0x3FU) << 6U) | (c2 & 0x3FU));
                return 3;
            }
            if ((c0 & 0xF8U) == 0xF0U && offset + 3 < text.size())
            {
                const unsigned char c1 = static_cast<unsigned char>(text[offset + 1]);
                const unsigned char c2 = static_cast<unsigned char>(text[offset + 2]);
                const unsigned char c3 = static_cast<unsigned char>(text[offset + 3]);
                codepoint = static_cast<int>(((c0 & 0x07U) << 18U) | ((c1 & 0x3FU) << 12U) |
                                             ((c2 & 0x3FU) << 6U) | (c3 & 0x3FU));
                return 4;
            }

            codepoint = '?';
            return 1;
        }

        void drawLineInternal(float fx1, float fy1, float fx2, float fy2, std::uint32_t color)
        {
            int x1 = static_cast<int>(std::lround(fx1));
            int y1 = static_cast<int>(std::lround(fy1));
            const int x2 = static_cast<int>(std::lround(fx2));
            const int y2 = static_cast<int>(std::lround(fy2));

            const int dx = std::abs(x2 - x1);
            const int sx = x1 < x2 ? 1 : -1;
            const int dy = -std::abs(y2 - y1);
            const int sy = y1 < y2 ? 1 : -1;
            int err = dx + dy;

            while (true)
            {
                putPixel(x1, y1, color);
                if (x1 == x2 && y1 == y2)
                {
                    break;
                }

                const int e2 = 2 * err;
                if (e2 >= dy)
                {
                    err += dy;
                    x1 += sx;
                }
                if (e2 <= dx)
                {
                    err += dx;
                    y1 += sy;
                }
            }
        }

        ANativeWindow *window_ = nullptr;
        ANativeWindow_Buffer buffer_{};
        std::uint32_t *pixels_ = nullptr;
        int width_ = 0;
        int height_ = 0;
        float uiScale_ = 1.0F;
        stbtt_fontinfo fontInfo_{};
        std::vector<unsigned char> fontFileData_;
        std::unordered_map<int, GlyphBitmap> glyphCache_;
        bool fontReady_ = false;
        float fontScale_ = 1.0F;
        float fontPixelSize_ = 16.0F;
        float fontPixelSizePx_ = 16.0F;
        int fontAscent_ = 0;
        int fontDescent_ = 0;
        int fontLineGap_ = 0;
    };

    struct AndroidRuntime
    {
        AndroidRuntime(ANativeActivity *activityIn,
                       std::filesystem::path scriptPathIn,
                       std::filesystem::path fontPathIn)
            : activity(activityIn),
              scriptPath(std::move(scriptPathIn)),
              fontPath(std::move(fontPathIn))
        {
            running = true;
        }

        ~AndroidRuntime()
        {
            setSoftInputVisible(false);

            running = false;
            if (renderThread.joinable())
            {
                renderThread.join();
            }

            if (window != nullptr)
            {
                ANativeWindow_release(window);
                window = nullptr;
            }
        }

        void onWindowCreated(ANativeWindow *newWindow)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (newWindow != nullptr)
            {
                ANativeWindow_acquire(newWindow);
            }
            if (window != nullptr)
            {
                ANativeWindow_release(window);
            }
            window = newWindow;
            windowDirty = true;
        }

        void onWindowDestroyed(ANativeWindow *destroyedWindow)
        {
            (void)destroyedWindow;
            std::lock_guard<std::mutex> lock(mutex);
            if (window != nullptr)
            {
                ANativeWindow_release(window);
                window = nullptr;
            }
            windowDirty = true;
        }

        void onInputQueueCreated(AInputQueue *queue)
        {
            std::lock_guard<std::mutex> lock(mutex);
            inputQueue = queue;
            inputQueueDirty = true;
        }

        void onInputQueueDestroyed(AInputQueue *queue)
        {
            (void)queue;
            std::lock_guard<std::mutex> lock(mutex);
            inputQueue = nullptr;
            inputQueueDirty = true;
        }

        void setSoftInputVisible(bool visible)
        {
            if (activity == nullptr || activity->vm == nullptr || activity->clazz == nullptr)
            {
                return;
            }

            JNIEnv *env = nullptr;
            bool detachThread = false;
            if (activity->vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK)
            {
                if (activity->vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
                {
                    return;
                }
                detachThread = true;
            }

            jobject nativeActivity = activity->clazz;
            jclass nativeActivityClass = env->GetObjectClass(nativeActivity);
            jmethodID getWindowMethod = env->GetMethodID(nativeActivityClass, "getWindow", "()Landroid/view/Window;");
            jobject windowObject = env->CallObjectMethod(nativeActivity, getWindowMethod);
            if (windowObject == nullptr)
            {
                env->DeleteLocalRef(nativeActivityClass);
                if (detachThread)
                {
                    activity->vm->DetachCurrentThread();
                }
                return;
            }

            jclass windowClass = env->GetObjectClass(windowObject);
            jmethodID getDecorViewMethod = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");
            jobject decorView = env->CallObjectMethod(windowObject, getDecorViewMethod);

            jstring inputMethodService = env->NewStringUTF("input_method");
            jmethodID getSystemServiceMethod = env->GetMethodID(nativeActivityClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
            jobject inputMethodManager = env->CallObjectMethod(nativeActivity, getSystemServiceMethod, inputMethodService);

            if (inputMethodManager != nullptr && decorView != nullptr)
            {
                jclass immClass = env->GetObjectClass(inputMethodManager);
                if (visible)
                {
                    jmethodID showSoftInputMethod = env->GetMethodID(immClass, "showSoftInput", "(Landroid/view/View;I)Z");
                    env->CallBooleanMethod(inputMethodManager, showSoftInputMethod, decorView, 0);
                }
                else
                {
                    jclass viewClass = env->GetObjectClass(decorView);
                    jmethodID getWindowTokenMethod = env->GetMethodID(viewClass, "getWindowToken", "()Landroid/os/IBinder;");
                    jobject windowToken = env->CallObjectMethod(decorView, getWindowTokenMethod);
                    if (windowToken != nullptr)
                    {
                        jmethodID hideSoftInputMethod = env->GetMethodID(immClass, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z");
                        env->CallBooleanMethod(inputMethodManager, hideSoftInputMethod, windowToken, 0);
                        env->DeleteLocalRef(windowToken);
                    }
                    env->DeleteLocalRef(viewClass);
                }
                env->DeleteLocalRef(immClass);
            }

            if (inputMethodManager != nullptr)
            {
                env->DeleteLocalRef(inputMethodManager);
            }
            env->DeleteLocalRef(inputMethodService);
            if (decorView != nullptr)
            {
                env->DeleteLocalRef(decorView);
            }
            env->DeleteLocalRef(windowClass);
            env->DeleteLocalRef(windowObject);
            env->DeleteLocalRef(nativeActivityClass);

            if (detachThread)
            {
                activity->vm->DetachCurrentThread();
            }

            softInputVisible = visible;
        }

        float queryUiScale() const
        {
            if (activity == nullptr || activity->vm == nullptr || activity->clazz == nullptr)
            {
                return 1.0F;
            }

            JNIEnv *env = nullptr;
            bool detachThread = false;
            if (activity->vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK)
            {
                if (activity->vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
                {
                    return 1.0F;
                }
                detachThread = true;
            }

            float density = 1.0F;
            jobject nativeActivity = activity->clazz;
            jclass activityClass = env->GetObjectClass(nativeActivity);
            jmethodID getResourcesMethod = env->GetMethodID(activityClass, "getResources", "()Landroid/content/res/Resources;");
            jobject resources = env->CallObjectMethod(nativeActivity, getResourcesMethod);
            if (resources != nullptr)
            {
                jclass resourcesClass = env->GetObjectClass(resources);
                jmethodID getDisplayMetricsMethod = env->GetMethodID(resourcesClass, "getDisplayMetrics", "()Landroid/util/DisplayMetrics;");
                jobject displayMetrics = env->CallObjectMethod(resources, getDisplayMetricsMethod);
                if (displayMetrics != nullptr)
                {
                    jclass metricsClass = env->GetObjectClass(displayMetrics);
                    jfieldID densityField = env->GetFieldID(metricsClass, "density", "F");
                    if (densityField != nullptr)
                    {
                        density = static_cast<float>(env->GetFloatField(displayMetrics, densityField));
                    }
                    env->DeleteLocalRef(metricsClass);
                    env->DeleteLocalRef(displayMetrics);
                }
                env->DeleteLocalRef(resourcesClass);
                env->DeleteLocalRef(resources);
            }
            env->DeleteLocalRef(activityClass);

            if (detachThread)
            {
                activity->vm->DetachCurrentThread();
            }

            return std::max(1.0F, density);
        }

        void run()
        {
            ALooper *looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
            if (looper == nullptr)
            {
                __android_log_print(ANDROID_LOG_ERROR, "GUI_CPP", "Failed to prepare looper");
                return;
            }

            uiScale = queryUiScale();
            renderer.setUiScale(uiScale);
            __android_log_print(ANDROID_LOG_INFO, "GUI_CPP", "Android UI scale: %.2f", uiScale);

            if (!fontPath.empty())
            {
                renderer.setFontPath(fontPath, 16.0F);
            }

            const bool initialized = appCore.initialize(&renderer, scriptPath);
            if (!initialized)
            {
                __android_log_print(ANDROID_LOG_ERROR, "GUI_CPP", "Core init failed: %s", appCore.lastError().c_str());
            }
            else
            {
                __android_log_print(ANDROID_LOG_INFO, "GUI_CPP", "Android runtime loop started");
            }

            auto previous = std::chrono::steady_clock::now();
            AInputQueue *attachedQueue = nullptr;
            ANativeWindow *activeWindow = nullptr;

            while (running)
            {
                inputState.textInput.clear();
                inputState.backspacePressed = false;
                inputState.enterPressed = false;
                inputState.toggleThemePressed = false;

                {
                    std::lock_guard<std::mutex> lock(mutex);
                    if (inputQueueDirty)
                    {
                        if (attachedQueue != nullptr)
                        {
                            AInputQueue_detachLooper(attachedQueue);
                        }
                        attachedQueue = inputQueue;
                        if (attachedQueue != nullptr)
                        {
                            AInputQueue_attachLooper(attachedQueue, looper, kLooperInputId, nullptr, nullptr);
                        }
                        inputQueueDirty = false;
                    }

                    if (windowDirty)
                    {
                        activeWindow = window;
                        renderer.setWindow(activeWindow);
                        if (activeWindow != nullptr)
                        {
                            const int windowWidth = ANativeWindow_getWidth(activeWindow);
                            const int windowHeight = ANativeWindow_getHeight(activeWindow);
                            const int logicalWidth = std::max(1, static_cast<int>(std::lround(static_cast<float>(windowWidth) / uiScale)));
                            const int logicalHeight = std::max(1, static_cast<int>(std::lround(static_cast<float>(windowHeight) / uiScale)));
                            appCore.onResize(logicalWidth, logicalHeight);
                        }
                        windowDirty = false;
                    }
                }

                int ident = 0;
                int events = 0;
                while ((ident = ALooper_pollOnce(0, nullptr, &events, nullptr)) >= 0)
                {
                    if (ident == kLooperInputId && attachedQueue != nullptr)
                    {
                        AInputEvent *event = nullptr;
                        while (AInputQueue_getEvent(attachedQueue, &event) >= 0)
                        {
                            if (AInputQueue_preDispatchEvent(attachedQueue, event))
                            {
                                continue;
                            }

                            int handled = 0;
                            if (event != nullptr && AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
                            {
                                const int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
                                if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_MOVE)
                                {
                                    touchDown = true;
                                    touchX = AMotionEvent_getX(event, 0);
                                    touchY = AMotionEvent_getY(event, 0);
                                    handled = 1;
                                }
                                else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL)
                                {
                                    touchDown = false;
                                    touchX = AMotionEvent_getX(event, 0);
                                    touchY = AMotionEvent_getY(event, 0);
                                    handled = 1;
                                }
                            }
                            else if (event != nullptr && AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
                            {
                                const int action = AKeyEvent_getAction(event);
                                if (action == AKEY_EVENT_ACTION_DOWN)
                                {
                                    const int keycode = AKeyEvent_getKeyCode(event);
                                    const int metaState = AKeyEvent_getMetaState(event);

                                    if (keycode == AKEYCODE_DEL)
                                    {
                                        inputState.backspacePressed = true;
                                        handled = 1;
                                    }
                                    else if (keycode == AKEYCODE_ENTER)
                                    {
                                        inputState.enterPressed = true;
                                        handled = 1;
                                    }
                                    else
                                    {
                                        const std::string text = keycodeToText(keycode, metaState);
                                        if (!text.empty())
                                        {
                                            inputState.textInput += text;
                                            handled = 1;
                                        }
                                    }
                                }
                            }

                            AInputQueue_finishEvent(attachedQueue, event, handled);
                        }
                    }
                }

                inputState.mouseX = touchX / uiScale;
                inputState.mouseY = touchY / uiScale;
                inputState.mouseDown = touchDown;

                const auto now = std::chrono::steady_clock::now();
                const float deltaSeconds = std::chrono::duration<float>(now - previous).count();
                previous = now;

                if (activeWindow != nullptr && initialized)
                {
                    appCore.tick(deltaSeconds, inputState);

                    const bool wantSoftInput = appCore.textInputActive();
                    if (wantSoftInput != softInputVisible)
                    {
                        setSoftInputVisible(wantSoftInput);
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }

            if (attachedQueue != nullptr)
            {
                AInputQueue_detachLooper(attachedQueue);
            }
        }

        ANativeActivity *activity = nullptr;
        std::filesystem::path scriptPath;
        std::filesystem::path fontPath;
        std::mutex mutex;
        ANativeWindow *window = nullptr;
        bool windowDirty = false;
        AInputQueue *inputQueue = nullptr;
        bool inputQueueDirty = false;
        bool running = false;
        std::thread renderThread;
        AndroidSoftwareRenderer renderer;
        AppCore appCore;
        InputState inputState;
        bool touchDown = false;
        float touchX = 0.0F;
        float touchY = 0.0F;
        float uiScale = 1.0F;
        bool softInputVisible = false;
    };

    AndroidRuntime *g_runtime = nullptr;

    void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window)
    {
        (void)activity;
        if (g_runtime != nullptr)
        {
            g_runtime->onWindowCreated(window);
        }
    }

    void onNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window)
    {
        (void)activity;
        if (g_runtime != nullptr)
        {
            g_runtime->onWindowDestroyed(window);
        }
    }

    void onInputQueueCreated(ANativeActivity *activity, AInputQueue *queue)
    {
        (void)activity;
        if (g_runtime != nullptr)
        {
            g_runtime->onInputQueueCreated(queue);
        }
    }

    void onInputQueueDestroyed(ANativeActivity *activity, AInputQueue *queue)
    {
        (void)activity;
        if (g_runtime != nullptr)
        {
            g_runtime->onInputQueueDestroyed(queue);
        }
    }

    void onDestroy(ANativeActivity *activity)
    {
        (void)activity;
        if (g_runtime != nullptr)
        {
            delete g_runtime;
            g_runtime = nullptr;
        }
    }
} // namespace

extern "C" void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize)
{
    (void)savedState;
    (void)savedStateSize;

    const std::filesystem::path scriptPath = prepareScriptPath(activity);
    if (scriptPath.empty())
    {
        __android_log_print(ANDROID_LOG_ERROR, "GUI_CPP", "Script preparation failed; runtime not started");
        return;
    }

    const std::filesystem::path fontPath = prepareFontPath(activity);
    if (fontPath.empty())
    {
        __android_log_print(ANDROID_LOG_ERROR, "GUI_CPP", "Font preparation failed; runtime will use fallback glyph blocks");
    }

    g_runtime = new AndroidRuntime(activity, scriptPath, fontPath);
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    activity->callbacks->onInputQueueCreated = onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
    activity->callbacks->onDestroy = onDestroy;

    g_runtime->renderThread = std::thread([runtime = g_runtime]()
                                          { runtime->run(); });
}
