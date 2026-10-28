// [android] Budokai Tenkaichi 4 in Dragon Rage (port/bt4): BT3-Recomp's headless raylib of its browser build
// (ps2xRuntime/web/bt3gl_headless.cpp, GPLv3, z3xox/BT3-Recomp + recomp-code/dbz-bt4-recomp), for Android: no window
// (Dragon Rage's GS draws the live stream, bt4_main.cpp), audio mixed as before and played through SDL3's audio.
// Changes from the browser's: the emscripten parts out, SDL3 audio for the AudioWorklet ring.
// [web] The raylib API the runtime calls, for the browser build (web/web.cmake), where it has no window of its own:
// the page draws the live GS stream with Play!'s GS (web/play_gs_host.cpp) and reads the keyboard and gamepads.
//
//   window, drawing, textures, shaders, rlgl   accepted and ignored (EndDrawing paces the present loop to the target
//                                               frame rate, as raylib's frame cap does)
//   input                                      nothing pressed (the page publishes the pad through the live ring)
//   audio streams and sounds                   mixed here to 48 kHz stereo into a ring in the wasm memory, which an
//                                               AudioWorklet on the page plays (the memory is a SharedArrayBuffer)

#include "raylib.h"
#include "rlgl.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
    using Clock = std::chrono::steady_clock;
    const Clock::time_point g_t0 = Clock::now();
    int g_winW = 640, g_winH = 480;
    bool g_winReady = false;
    int g_targetFps = 0;
    Clock::time_point g_nextFrame = Clock::now();
    std::atomic<unsigned> g_nextId{1};
    int g_defaultLocs[32] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                             -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    int bytesPerPixel(int format)
    {
        switch (format)
        {
        case PIXELFORMAT_UNCOMPRESSED_GRAYSCALE: return 1;
        case PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA:
        case PIXELFORMAT_UNCOMPRESSED_R5G6B5:
        case PIXELFORMAT_UNCOMPRESSED_R5G5B5A1:
        case PIXELFORMAT_UNCOMPRESSED_R4G4B4A4: return 2;
        case PIXELFORMAT_UNCOMPRESSED_R8G8B8: return 3;
        case PIXELFORMAT_UNCOMPRESSED_R32: return 4;
        case PIXELFORMAT_UNCOMPRESSED_R32G32B32: return 12;
        case PIXELFORMAT_UNCOMPRESSED_R32G32B32A32: return 16;
        default: return 4;
        }
    }

    // ---- audio ------------------------------------------------------------------------------------------------
    constexpr uint32_t kOutRate = 48000;
    constexpr uint32_t kRingFrames = 32768; // ~680 ms
    alignas(16) int16_t g_ring[kRingFrames * 2];
    alignas(8) std::atomic<uint32_t> g_ringIndex[2]; // [0] written (here), [1] read (by the worklet)

    struct Voice
    {
        bool sound = false;          // a one-shot Sound (mono PCM kept whole) rather than a stream
        bool playing = false;
        uint32_t rate = kOutRate;
        uint32_t channels = 2;
        uint32_t subFrames = 4096;   // raylib's sub-buffer size: a stream is "processed" when one is free
        std::deque<int16_t> fifo;    // stream: queued stereo frames (L, R)
        std::vector<int16_t> pcm;    // sound: mono samples
        double pos = 0.0;            // read position in source frames (fraction for the resampler)
        float volume = 1.0f, pitch = 1.0f;
    };
    std::mutex g_audioMx;
    std::vector<Voice *> g_voices;
    std::atomic<bool> g_audioUp{false};
    int g_defaultSub = 4096;
    uint64_t g_streamUnderruns = 0;

    Voice *voiceOf(const AudioStream &s) { return reinterpret_cast<Voice *>(s.buffer); }

    void mixFrames(float *out, uint32_t frames)
    {
        std::memset(out, 0, sizeof(float) * frames * 2u);
        std::lock_guard<std::mutex> lk(g_audioMx);
        for (Voice *v : g_voices)
        {
            if (!v->playing)
                continue;
            const double step = static_cast<double>(v->rate) * v->pitch / kOutRate;
            if (v->sound)
            {
                const size_t n = v->pcm.size();
                for (uint32_t i = 0; i < frames; ++i)
                {
                    const size_t k = static_cast<size_t>(v->pos);
                    if (k + 1 >= n) { v->playing = false; break; }
                    const double f = v->pos - k;
                    const float s = static_cast<float>((v->pcm[k] + (v->pcm[k + 1] - v->pcm[k]) * f) * v->volume);
                    out[i * 2] += s;
                    out[i * 2 + 1] += s;
                    v->pos += step;
                }
                continue;
            }
            // stream: frame k of the fifo is (fifo[2k], fifo[2k+1]); consumed frames are popped as pos passes them
            for (uint32_t i = 0; i < frames; ++i)
            {
                const size_t avail = v->fifo.size() / 2u;
                const size_t k = static_cast<size_t>(v->pos);
                if (k + 1 >= avail) { ++g_streamUnderruns; break; }
                const double f = v->pos - k;
                const float l0 = v->fifo[k * 2], r0 = v->fifo[k * 2 + 1];
                const float l1 = v->fifo[k * 2 + 2], r1 = v->fifo[k * 2 + 3];
                out[i * 2] += static_cast<float>((l0 + (l1 - l0) * f) * v->volume);
                out[i * 2 + 1] += static_cast<float>((r0 + (r1 - r0) * f) * v->volume);
                v->pos += step;
            }
            const size_t used = static_cast<size_t>(v->pos);
            if (used > 0)
            {
                const size_t drop = std::min(used, v->fifo.size() / 2u);
                v->fifo.erase(v->fifo.begin(), v->fifo.begin() + static_cast<std::ptrdiff_t>(drop * 2u));
                v->pos -= static_cast<double>(drop);
            }
        }
    }

    // [android] SDL3 pulls the mix: 48 kHz stereo 16-bit, as much as the device asks for
    SDL_AudioStream *g_out = nullptr;
    void SDLCALL pull(void *, SDL_AudioStream *stream, int additional, int)
    {
        constexpr uint32_t kBlock = 256;
        float mix[kBlock * 2];
        int16_t pcm[kBlock * 2];
        int frames = additional / 4;
        while (frames > 0)
        {
            const uint32_t n = static_cast<uint32_t>(std::min<int>(frames, static_cast<int>(kBlock)));
            mixFrames(mix, n);
            for (uint32_t i = 0; i < n * 2; ++i)
                pcm[i] = static_cast<int16_t>(std::clamp(mix[i], -32768.0f, 32767.0f));
            SDL_PutAudioStreamData(stream, pcm, static_cast<int>(n * 4));
            frames -= static_cast<int>(n);
        }
    }
}

extern "C"
{
// ---- window / timing ------------------------------------------------------------------------------------------
void InitWindow(int width, int height, const char *title)
{
    (void)title;
    g_winW = width > 0 ? width : 640;
    g_winH = height > 0 ? height : 480;
    g_winReady = true;
    std::printf("[android] headless raylib: window %dx%d (Dragon Rage's GS draws the game)\n", g_winW, g_winH);
}
void CloseWindow(void) { g_winReady = false; }
bool IsWindowReady(void) { return g_winReady; }
bool WindowShouldClose(void) { return false; }
void SetConfigFlags(unsigned int flags) { (void)flags; }
void SetWindowState(unsigned int flags) { (void)flags; }
void SetWindowSize(int width, int height) { if (width > 0) g_winW = width; if (height > 0) g_winH = height; }
void SetWindowMonitor(int monitor) { (void)monitor; }
void ClearWindowState(unsigned int flags) { (void)flags; }
void ToggleFullscreen(void) {}
bool IsWindowFullscreen(void) { return false; }
void SetWindowIcon(Image image) { (void)image; }
void *GetWindowHandle(void) { return nullptr; }
int GetScreenWidth(void) { return g_winW; }
int GetScreenHeight(void) { return g_winH; }
int GetRenderWidth(void) { return g_winW; }
int GetRenderHeight(void) { return g_winH; }
int GetMonitorCount(void) { return 1; }
int GetCurrentMonitor(void) { return 0; }
int GetMonitorWidth(int monitor) { (void)monitor; return 1920; }
int GetMonitorHeight(int monitor) { (void)monitor; return 1080; }
int GetMonitorRefreshRate(int monitor) { (void)monitor; return 60; }
const char *GetMonitorName(int monitor) { (void)monitor; return "android"; }
double GetTime(void) { return std::chrono::duration<double>(Clock::now() - g_t0).count(); }
void SetTargetFPS(int fps) { g_targetFps = fps > 0 ? fps : 0; }

void BeginDrawing(void) {}
void EndDrawing(void)
{
    if (g_targetFps <= 0)
        return;
    const auto period = std::chrono::nanoseconds(1000000000ll / g_targetFps);
    const auto now = Clock::now();
    if (g_nextFrame + std::chrono::milliseconds(100) < now)
        g_nextFrame = now; // fell far behind: do not race to catch up
    g_nextFrame += period;
    if (g_nextFrame > now)
        std::this_thread::sleep_until(g_nextFrame);
}
void ClearBackground(Color color) { (void)color; }
void BeginBlendMode(int mode) { (void)mode; }
void EndBlendMode(void) {}
void BeginShaderMode(Shader shader) { (void)shader; }
void EndShaderMode(void) {}
void DrawRectangle(int posX, int posY, int width, int height, Color color) { (void)posX; (void)posY; (void)width; (void)height; (void)color; }
void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint)
{
    (void)texture; (void)source; (void)dest; (void)origin; (void)rotation; (void)tint;
}

// ---- images / textures / shaders --------------------------------------------------------------------------------
int GetPixelDataSize(int width, int height, int format) { return width * height * bytesPerPixel(format); }
Image GenImageColor(int width, int height, Color color)
{
    Image img{};
    if (width <= 0 || height <= 0)
        return img;
    auto *px = static_cast<Color *>(std::malloc(static_cast<size_t>(width) * height * sizeof(Color)));
    if (!px)
        return img;
    for (int i = 0; i < width * height; ++i)
        px[i] = color;
    img.data = px;
    img.width = width;
    img.height = height;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    return img;
}
Image ImageCopy(Image image)
{
    Image out = image;
    out.data = nullptr;
    if (image.data && image.width > 0 && image.height > 0)
    {
        const size_t n = static_cast<size_t>(GetPixelDataSize(image.width, image.height, image.format));
        out.data = std::malloc(n);
        if (out.data)
            std::memcpy(out.data, image.data, n);
    }
    return out;
}
void UnloadImage(Image image) { std::free(image.data); }
Image LoadImage(const char *fileName) { (void)fileName; return Image{}; }
Image LoadImageFromScreen(void) { return Image{}; }
bool ExportImage(Image image, const char *fileName) { (void)image; (void)fileName; return false; }
void ImageFlipVertical(Image *image)
{
    if (!image || !image->data || image->height <= 1)
        return;
    const size_t row = static_cast<size_t>(image->width) * bytesPerPixel(image->format);
    std::vector<uint8_t> tmp(row);
    auto *p = static_cast<uint8_t *>(image->data);
    for (int y = 0; y < image->height / 2; ++y)
    {
        uint8_t *a = p + row * y, *b = p + row * (image->height - 1 - y);
        std::memcpy(tmp.data(), a, row);
        std::memcpy(a, b, row);
        std::memcpy(b, tmp.data(), row);
    }
}
void ImageFormat(Image *image, int newFormat) { (void)image; (void)newFormat; }
void ImageResize(Image *image, int newWidth, int newHeight) { (void)image; (void)newWidth; (void)newHeight; }
Texture2D LoadTextureFromImage(Image image)
{
    Texture2D t{};
    t.id = g_nextId++;
    t.width = image.width;
    t.height = image.height;
    t.mipmaps = 1;
    t.format = image.format;
    return t;
}
void UnloadTexture(Texture2D texture) { (void)texture; }
void UpdateTexture(Texture2D texture, const void *pixels) { (void)texture; (void)pixels; }
void SetTextureFilter(Texture2D texture, int filter) { (void)texture; (void)filter; }
void SetTextureWrap(Texture2D texture, int wrap) { (void)texture; (void)wrap; }
Shader LoadShaderFromMemory(const char *vsCode, const char *fsCode)
{
    (void)vsCode; (void)fsCode;
    Shader s{};
    s.id = g_nextId++;
    s.locs = g_defaultLocs;
    return s;
}
int GetShaderLocation(Shader shader, const char *uniformName) { (void)shader; (void)uniformName; return -1; }
void SetShaderValue(Shader shader, int locIndex, const void *value, int uniformType) { (void)shader; (void)locIndex; (void)value; (void)uniformType; }

// ---- input (the page publishes keyboard and gamepad through the live ring) ------------------------------------------
bool IsKeyDown(int key) { (void)key; return false; }
bool IsKeyPressed(int key) { (void)key; return false; }
int GetKeyPressed(void) { return 0; }
int GetCharPressed(void) { return 0; }
bool IsMouseButtonDown(int button) { (void)button; return false; }
Vector2 GetMousePosition(void) { return Vector2{0.0f, 0.0f}; }
Vector2 GetMouseWheelMoveV(void) { return Vector2{0.0f, 0.0f}; }
bool IsGamepadAvailable(int gamepad) { (void)gamepad; return false; }
bool IsGamepadButtonDown(int gamepad, int button) { (void)gamepad; (void)button; return false; }
int GetGamepadAxisCount(int gamepad) { (void)gamepad; return 0; }
float GetGamepadAxisMovement(int gamepad, int axis) { (void)gamepad; (void)axis; return 0.0f; }
const char *GetGamepadName(int gamepad) { (void)gamepad; return ""; }
int SetGamepadMappings(const char *mappings) { (void)mappings; return 0; }

// ---- audio ---------------------------------------------------------------------------------------------------------
void InitAudioDevice(void)
{
    if (g_audioUp.exchange(true))
        return;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        std::printf("[android] audio: SDL_InitSubSystem: %s\n", SDL_GetError());
        return;
    }
    const SDL_AudioSpec spec = {SDL_AUDIO_S16, 2, static_cast<int>(kOutRate)};
    g_out = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, pull, nullptr);
    if (!g_out)
    {
        std::printf("[android] audio: SDL_OpenAudioDeviceStream: %s\n", SDL_GetError());
        return;
    }
    SDL_ResumeAudioStreamDevice(g_out);
    std::printf("[android] audio: streams mixed to 48 kHz stereo, played by SDL3\n");
}
void CloseAudioDevice(void) {}
bool IsAudioDeviceReady(void) { return g_audioUp.load(); }
void SetAudioStreamBufferSizeDefault(int size) { if (size > 0) g_defaultSub = size; }
AudioStream LoadAudioStream(unsigned int sampleRate, unsigned int sampleSize, unsigned int channels)
{
    AudioStream s{};
    auto *v = new Voice();
    v->rate = sampleRate ? sampleRate : kOutRate;
    v->channels = channels == 1 ? 1 : 2;
    v->subFrames = static_cast<uint32_t>(g_defaultSub);
    {
        std::lock_guard<std::mutex> lk(g_audioMx);
        g_voices.push_back(v);
    }
    s.buffer = reinterpret_cast<rAudioBuffer *>(v);
    s.sampleRate = v->rate;
    s.sampleSize = sampleSize;
    s.channels = v->channels;
    return s;
}
void UnloadAudioStream(AudioStream stream)
{
    Voice *v = voiceOf(stream);
    if (!v)
        return;
    std::lock_guard<std::mutex> lk(g_audioMx);
    g_voices.erase(std::remove(g_voices.begin(), g_voices.end(), v), g_voices.end());
    delete v;
}
void PlayAudioStream(AudioStream stream) { if (Voice *v = voiceOf(stream)) { std::lock_guard<std::mutex> lk(g_audioMx); v->playing = true; } }
void StopAudioStream(AudioStream stream)
{
    if (Voice *v = voiceOf(stream))
    {
        std::lock_guard<std::mutex> lk(g_audioMx);
        v->playing = false;
        v->fifo.clear();
        v->pos = 0.0;
    }
}
bool IsAudioStreamProcessed(AudioStream stream)
{
    Voice *v = voiceOf(stream);
    if (!v)
        return false;
    std::lock_guard<std::mutex> lk(g_audioMx);
    return v->fifo.size() / 2u <= v->subFrames; // at most one of raylib's two sub-buffers still queued
}
void UpdateAudioStream(AudioStream stream, const void *data, int frameCount)
{
    Voice *v = voiceOf(stream);
    if (!v || !data || frameCount <= 0)
        return;
    const auto *in = static_cast<const int16_t *>(data);
    std::lock_guard<std::mutex> lk(g_audioMx);
    for (int i = 0; i < frameCount; ++i)
    {
        const int16_t l = in[v->channels == 1 ? i : i * 2];
        const int16_t r = in[v->channels == 1 ? i : i * 2 + 1];
        v->fifo.push_back(l);
        v->fifo.push_back(r);
    }
}
Sound LoadSoundFromWave(Wave wave)
{
    Sound snd{};
    if (!wave.data || wave.frameCount == 0 || wave.sampleSize != 16)
        return snd;
    auto *v = new Voice();
    v->sound = true;
    v->rate = wave.sampleRate ? wave.sampleRate : kOutRate;
    const auto *in = static_cast<const int16_t *>(wave.data);
    v->pcm.resize(wave.frameCount);
    for (unsigned i = 0; i < wave.frameCount; ++i)
        v->pcm[i] = wave.channels == 2 ? static_cast<int16_t>((in[i * 2] + in[i * 2 + 1]) / 2) : in[i];
    {
        std::lock_guard<std::mutex> lk(g_audioMx);
        g_voices.push_back(v);
    }
    snd.stream.buffer = reinterpret_cast<rAudioBuffer *>(v);
    snd.stream.sampleRate = v->rate;
    snd.stream.sampleSize = 16;
    snd.stream.channels = 1;
    snd.frameCount = wave.frameCount;
    return snd;
}
void UnloadSound(Sound sound) { UnloadAudioStream(sound.stream); }
void PlaySound(Sound sound) { if (Voice *v = voiceOf(sound.stream)) { std::lock_guard<std::mutex> lk(g_audioMx); v->pos = 0.0; v->playing = true; } }
void StopSound(Sound sound) { if (Voice *v = voiceOf(sound.stream)) { std::lock_guard<std::mutex> lk(g_audioMx); v->playing = false; } }
bool IsSoundPlaying(Sound sound) { Voice *v = voiceOf(sound.stream); if (!v) return false; std::lock_guard<std::mutex> lk(g_audioMx); return v->playing; }
void SetSoundPitch(Sound sound, float pitch) { if (Voice *v = voiceOf(sound.stream)) { std::lock_guard<std::mutex> lk(g_audioMx); v->pitch = pitch > 0.0f ? pitch : 1.0f; } }
void SetSoundVolume(Sound sound, float volume) { if (Voice *v = voiceOf(sound.stream)) { std::lock_guard<std::mutex> lk(g_audioMx); v->volume = volume; } }

// ---- rlgl -----------------------------------------------------------------------------------------------------------
void rlActiveDrawBuffers(int count) { (void)count; }
void rlActiveTextureSlot(int slot) { (void)slot; }
void rlBegin(int mode) { (void)mode; }
bool rlCheckRenderBatchLimit(int vCount) { (void)vCount; return false; }
void rlClearColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { (void)r; (void)g; (void)b; (void)a; }
void rlClearScreenBuffers(void) {}
void rlColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { (void)r; (void)g; (void)b; (void)a; }
void rlColorMask(bool r, bool g, bool b, bool a) { (void)r; (void)g; (void)b; (void)a; }
void rlDisableBackfaceCulling(void) {}
void rlDisableColorBlend(void) {}
void rlDisableDepthMask(void) {}
void rlDisableDepthTest(void) {}
void rlDisableFramebuffer(void) {}
void rlDisableScissorTest(void) {}
void rlDisableShader(void) {}
void rlDisableTexture(void) {}
void rlDrawRenderBatchActive(void) {}
void rlEnableBackfaceCulling(void) {}
void rlEnableColorBlend(void) {}
void rlEnableDepthMask(void) {}
void rlEnableDepthTest(void) {}
void rlEnableFramebuffer(unsigned int id) { (void)id; }
void rlEnableScissorTest(void) {}
void rlEnd(void) {}
void rlFramebufferAttach(unsigned int fboId, unsigned int texId, int attachType, int texType, int mipLevel)
{
    (void)fboId; (void)texId; (void)attachType; (void)texType; (void)mipLevel;
}
bool rlFramebufferComplete(unsigned int id) { (void)id; return true; }
unsigned int rlGetShaderIdDefault(void) { return 1; }
int *rlGetShaderLocsDefault(void) { return g_defaultLocs; }
unsigned int rlLoadFramebuffer(void) { return g_nextId++; }
void rlLoadIdentity(void) {}
unsigned int rlLoadTexture(const void *data, int width, int height, int format, int mipmapCount)
{
    (void)data; (void)width; (void)height; (void)format; (void)mipmapCount;
    return g_nextId++;
}
void rlMatrixMode(int mode) { (void)mode; }
void rlNormal3f(float x, float y, float z) { (void)x; (void)y; (void)z; }
void rlOrtho(double left, double right, double bottom, double top, double znear, double zfar)
{
    (void)left; (void)right; (void)bottom; (void)top; (void)znear; (void)zfar;
}
void *rlReadTexturePixels(unsigned int id, int width, int height, int format)
{
    (void)id;
    if (width <= 0 || height <= 0)
        return nullptr;
    return std::calloc(static_cast<size_t>(GetPixelDataSize(width, height, format)), 1);
}
void rlScalef(float x, float y, float z) { (void)x; (void)y; (void)z; }
void rlScissor(int x, int y, int width, int height) { (void)x; (void)y; (void)width; (void)height; }
void rlSetBlendFactors(int glSrcFactor, int glDstFactor, int glEquation) { (void)glSrcFactor; (void)glDstFactor; (void)glEquation; }
void rlSetBlendFactorsSeparate(int glSrcRGB, int glDstRGB, int glSrcAlpha, int glDstAlpha, int glEqRGB, int glEqAlpha)
{
    (void)glSrcRGB; (void)glDstRGB; (void)glSrcAlpha; (void)glDstAlpha; (void)glEqRGB; (void)glEqAlpha;
}
void rlSetBlendMode(int mode) { (void)mode; }
void rlSetFramebufferHeight(int height) { (void)height; }
void rlSetFramebufferWidth(int width) { (void)width; }
void rlSetShader(unsigned int id, int *locs) { (void)id; (void)locs; }
void rlSetTexture(unsigned int id) { (void)id; }
void rlTexCoord2f(float x, float y) { (void)x; (void)y; }
void rlTextureParameters(unsigned int id, int param, int value) { (void)id; (void)param; (void)value; }
void rlTranslatef(float x, float y, float z) { (void)x; (void)y; (void)z; }
void rlVertex2f(float x, float y) { (void)x; (void)y; }
void rlVertex3f(float x, float y, float z) { (void)x; (void)y; (void)z; }
void rlViewport(int x, int y, int width, int height) { (void)x; (void)y; (void)width; (void)height; }

// The vendored rlgl's statistics, read by the runtime's [fps] line.
unsigned long long g_rlglDrawCalls = 0, g_rlglBatchFlushes = 0;
double g_rlglFlushNs = 0.0;
unsigned long long g_rlglVbRingWaits = 0, g_rlglVbRingWraps = 0, g_rlglVbRingBytes = 0, g_rlglVbRingMvpSkips = 0;
int g_rlglVbRingOn = 0;
}
