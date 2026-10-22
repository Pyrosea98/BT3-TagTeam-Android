// Budokai Tenkaichi 4 on Android (port/bt4): libbt4.so, started by Bt4Activity (SDL3's activity) with
//   argv[1] the game folder (bt4/), argv[2] the app's native libraries, argv[3] the disc image (/proc/self/fd/<n>).
//
// Two halves in one process, as in BT3-Recomp's browser build (ps2xRuntime/web), with Dragon Rage's GS in place of
// Play!'s:
//   - the game: Team BT4's mod recompiled (BT3-Recomp + dbz-bt4-recomp), its runtime's own main() on a thread of its
//     own, with no window (the headless raylib, bt3gl_android.cpp). It sends what it draws as a live GS stream
//     (ps2_gs_live.cpp, the format of a PCSX2 GS dump) into a ring in this process's memory;
//   - the picture: this thread (SDL's, the app's main thread for SDL) reads the ring and hands it to Dragon Rage's
//     GS, port/src/gs, exactly the renderer of Budokai Tenkaichi 3 (Vulkan or OpenGL ES, its settings, texture
//     packs, the on-screen controller and the gamepads of gs_input.c), and sends the pad back through the ring.
#include <SDL3/SDL.h>
#include <android/log.h>
#include <jni.h>

#include <atomic>
#include <cerrno>
#include <map>
#include <mutex>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include "ps2_live_ring.h"

extern "C" {
void Gs_Gif(const uint8_t *p, uint32_t qwc);                   /* gs_core.c: GIF data, whole quadwords */
void Port_GsGifChannel(uint32_t addr, uint32_t qwc, int chain); /* gs_core.c: (here only to start the GS) */
int GsGpu_Enabled(void);
void GsGpu_FrameEnd(void);
void GsGpu_PollEvents(void);
int Port_PadRead(int socket, unsigned char *data);             /* gs_input.c: controller, keyboard, on-screen pad */
void Port_TouchPad(unsigned buttons, int lx, int ly, int rx, int ry);
int Port_Setting(const char *name, int def);
extern int gGsMainFbp;
extern unsigned gPortVBlanks;
void Bt4_Quit(void);
int GsGpu_PauseFrame(void);                                    /* gs_gpu.c: the last picture again (and the events) */
int GsGpu_PresentGet(void);
void GsGpu_PresentSet(int mode);
extern int gGsFragDepth;                                       /* gs_draw.c */
void Port_SettingSave(const char *name, int value);
void Port_SettingsWrite(void);
}
#include "../../src/gs/ui.h"                                   /* PortVideo, GsGpu_GetSettings / GsGpu_SetSettings */

int ps2x_runtime_main(int argc, char *argv[]); // the runtime's main() (src/main.cpp, renamed by the build)

namespace
{
const char *TAG = "bt4";
std::string g_root, g_image;
std::atomic<ps2live::RingHeader *> g_ring{nullptr};
std::atomic<bool> g_quit{false};
FILE *g_log = nullptr;
int g_pipe[2] = {-1, -1};

// ---- the log: stdout and stderr to logcat ("bt4") and to bt3_log.txt in the game folder (the launcher's log
// export and its "the game closed" check read it, as for BT3), the two runs before kept as bt3_log.1/2.txt
void *logThread(void *)
{
    char buf[2048];
    size_t used = 0;
    for (;;)
    {
        ssize_t n = read(g_pipe[0], buf + used, sizeof(buf) - 1 - used);
        if (n <= 0)
        {
            if (n < 0 && errno == EINTR)
                continue;
            break;
        }
        used += static_cast<size_t>(n);
        for (;;)
        {
            char *nl = static_cast<char *>(std::memchr(buf, '\n', used));
            size_t len = nl ? static_cast<size_t>(nl - buf) : used == sizeof(buf) - 1 ? used : 0;
            if (len == 0 && !nl)
                break;
            buf[len] = '\0';
            __android_log_write(ANDROID_LOG_INFO, TAG, buf);
            if (g_log)
            {
                std::fprintf(g_log, "%s\n", buf);
                std::fflush(g_log);
            }
            len += nl != nullptr;
            std::memmove(buf, buf + len, used - len);
            used -= len;
        }
    }
    return nullptr;
}

void redirectOutput(const std::string &dir)
{
    const std::string path = dir + "/bt3_log.txt", old = dir + "/bt3_log.1.txt", older = dir + "/bt3_log.2.txt";
    std::rename(old.c_str(), older.c_str());
    std::rename(path.c_str(), old.c_str());
    g_log = std::fopen(path.c_str(), "w");
    setvbuf(stdout, nullptr, _IOLBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    if (pipe(g_pipe) == 0)
    {
        dup2(g_pipe[1], 1);
        dup2(g_pipe[1], 2);
        pthread_t th;
        pthread_create(&th, nullptr, logThread, nullptr);
        pthread_detach(th);
    }
}

// ---- the ring (the consumer's side; ps2_live_ring.h, as web/play_gs_host.cpp reads it)
struct Reader
{
    ps2live::RingHeader *ring = nullptr;
    uint8_t *data = nullptr;
    uint64_t capacity = 0;
    bool headerDone = false;
    std::vector<uint8_t> packet;
    std::vector<uint8_t> priv = std::vector<uint8_t>(8192u, 0u);
    uint64_t frames = 0, transfers = 0, bytes = 0;

    uint64_t available() const
    {
        const int64_t w = __atomic_load_n(&ring->writePos, __ATOMIC_ACQUIRE);
        return static_cast<uint64_t>(w - ring->readPos);
    }
    void peek(uint64_t skip, void *dst, uint64_t size) const
    {
        uint8_t *out = static_cast<uint8_t *>(dst);
        uint64_t pos = static_cast<uint64_t>(ring->readPos) + skip;
        while (size)
        {
            const uint64_t offset = pos % capacity;
            const uint64_t chunk = std::min(size, capacity - offset);
            std::memcpy(out, data + offset, chunk);
            out += chunk;
            pos += chunk;
            size -= chunk;
        }
    }
    template <typename T> T value(uint64_t skip) const
    {
        T v;
        peek(skip, &v, sizeof(T));
        return v;
    }
    void consume(uint64_t size)
    {
        __atomic_store_n(&ring->readPos, ring->readPos + static_cast<int64_t>(size), __ATOMIC_RELEASE);
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        if (ring->producerWaiting)
            ps2live::eventSet(&ps2live::ringEvents(ring)->space);
    }
};

constexpr uint8_t kTypeTransfer = 0, kTypeVSync = 1, kTypeReadFifo2 = 2, kTypeRegisters = 3;
constexpr uint8_t kPath2 = 1, kPath3 = 2, kPath1New = 3;
constexpr uint64_t kPrivRegsSize = 8192u;

// The shown frame buffer, from the privileged registers (PMODE: which read circuit is on; DISPFB1/2: its FBP).
void applyPriv(const std::vector<uint8_t> &priv)
{
    uint64_t pmode, dispfb1, dispfb2;
    std::memcpy(&pmode, priv.data() + 0x00, 8);
    std::memcpy(&dispfb1, priv.data() + 0x70, 8);
    std::memcpy(&dispfb2, priv.data() + 0x90, 8);
    const uint64_t dispfb = (pmode & 1u) || !(pmode & 2u) ? dispfb1 : dispfb2;
    gGsMainFbp = static_cast<int>(dispfb & 0x1FFu);
}

void sendPad(ps2live::RingHeader *ring)
{
    unsigned char d[18];
    if (!GsGpu_Enabled() || Port_PadRead(0, d) <= 0)
        return;
    const unsigned buttons = static_cast<unsigned>(~(d[0] | (d[1] << 8))) & 0xFFFFu; // active high, libpad order
    ring->padButtons = buttons;
    ring->padAxes = static_cast<uint32_t>(d[4]) | (static_cast<uint32_t>(d[5]) << 8) | (static_cast<uint32_t>(d[2]) << 16) |
                    (static_cast<uint32_t>(d[3]) << 24); // lx, ly, rx, ry
    ring->padSequence = ring->padSequence + 1;
}

// Whole packets until a VSync (a picture ends: true) or until the data runs out (false).
bool processPackets(Reader &r)
{
    if (!r.headerDone)
    {
        if (r.available() < 8)
            return false;
        const uint32_t crc = r.value<uint32_t>(0), headerSize = r.value<uint32_t>(4);
        uint64_t stateSize = headerSize, headerBytes = 0;
        if (crc == 0xFFFFFFFFu)
        {
            headerBytes = headerSize;
            if (r.available() < 8 + headerBytes)
                return false;
            std::vector<uint8_t> header(headerBytes);
            r.peek(8, header.data(), headerBytes);
            uint32_t dumpStateSize = 0;
            std::memcpy(&dumpStateSize, header.data() + 4, sizeof(dumpStateSize));
            stateSize = dumpStateSize;
        }
        const uint64_t total = 8 + headerBytes + stateSize + kPrivRegsSize;
        if (r.available() < total)
            return false;
        r.peek(8 + headerBytes + stateSize, r.priv.data(), kPrivRegsSize);
        r.consume(total);
        applyPriv(r.priv);
        r.headerDone = true;
        std::fprintf(stderr, "bt4: GS stream connected\n");
    }
    for (;;)
    {
        const uint64_t avail = r.available();
        if (avail < 1)
            return false;
        switch (r.value<uint8_t>(0))
        {
        case kTypeTransfer:
        {
            if (avail < 6)
                return false;
            const uint8_t path = r.value<uint8_t>(1);
            const uint32_t length = r.value<uint32_t>(2);
            if (avail < 6ull + length)
                return false;
            r.packet.resize(length);
            r.peek(6, r.packet.data(), length);
            r.consume(6ull + length);
            if ((path == kPath1New || path == kPath2 || path == kPath3) && length >= 16)
            {
                Gs_Gif(r.packet.data(), length / 16u);
                r.transfers++;
                r.bytes += length;
            }
            break;
        }
        case kTypeVSync:
            if (avail < 2)
                return false;
            r.consume(2);
            return true;
        case kTypeReadFifo2:
            if (avail < 5)
                return false;
            r.consume(5); // (GS -> EE downloads: not answered yet; the game keeps its own data, as in the browser build)
            break;
        case kTypeRegisters:
            if (avail < 1 + kPrivRegsSize)
                return false;
            r.peek(1, r.priv.data(), kPrivRegsSize);
            r.consume(1 + kPrivRegsSize);
            applyPriv(r.priv);
            break;
        default:
            std::fprintf(stderr, "bt4: unknown packet 0x%02X in the GS stream, stopping\n", r.value<uint8_t>(0));
            r.ring->consumerQuit = 1;
            g_quit = true;
            return false;
        }
    }
}

// ---- the touch menu (GameMenu.kt through MenuBridge): the same keys as BT3's (bt3_settings.txt), those this
// engine has. While it is up the stream is not read: the game waits on its full ring, the last picture stays.
std::mutex g_menuLock;
std::atomic<int> g_menuWant{-1}; // asked by the app: 1 open, 0 closed
std::atomic<bool> g_menuOpen{false};
std::vector<std::pair<std::string, int>> g_menuQueue;
std::map<std::string, int> g_menuSnap;
int g_menuGen = 0;
std::string g_gpuName;
int g_gl = 0;

void menuSnapshot()
{
    PortVideo v;
    GsGpu_GetSettings(&v);
    std::lock_guard<std::mutex> lock(g_menuLock);
    g_menuSnap.clear();
    g_menuSnap["scale4"] = v.scale4;
    g_menuSnap["filter"] = v.filter;
    g_menuSnap["smooth_2d"] = v.smooth2d;
    g_menuSnap["texture_pack"] = v.texPack;
    g_menuSnap["texture_count"] = v.texPackCount;
    g_menuSnap["gpu_api"] = Port_Setting("gpu_api", 0) == 1 ? 1 : 0;
    g_menuSnap["running_gl"] = g_gl;
    g_menuSnap["frag_depth"] = gGsFragDepth;
    g_menuSnap["present"] = g_gl ? 0 : GsGpu_PresentGet();
    g_menuSnap["fx_off"] = v.fxOff;
    g_menuSnap["glow"] = v.glow;
    g_menuGen++;
}

void menuApply(const std::string &key, int value)
{
    PortVideo v, was;
    GsGpu_GetSettings(&v);
    was = v;
    if (key == "scale4") v.scale4 = value;
    else if (key == "filter") v.filter = value;
    else if (key == "smooth_2d") v.smooth2d = value;
    else if (key == "texture_pack") v.texPack = value;
    else if (key == "fx_off") v.fxOff = value;
    else if (key == "glow") v.glow = value;
    else if (key == "gpu_api")
    {
        Port_SettingSave("gpu_api", value ? 1 : 0);
        Port_SettingSave("dr_renderer", (value ? 1 : 0) + 1);
        Port_SettingsWrite();
    }
    else if (key == "frag_depth")
    {
        gGsFragDepth = value != 0;
        Port_SettingSave("frag_depth", gGsFragDepth);
        Port_SettingsWrite();
    }
    else if (key == "present" && !g_gl)
    {
        GsGpu_PresentSet(value);
        Port_SettingsWrite();
    }
    if (std::memcmp(&v, &was, sizeof(v)) != 0)
        GsGpu_SetSettings(&v);
}

// On this thread, once a loop: the app's requests. true while the menu is up.
bool menuFrame()
{
    const int want = g_menuWant.exchange(-1);
    std::vector<std::pair<std::string, int>> q;
    {
        std::lock_guard<std::mutex> lock(g_menuLock);
        q.swap(g_menuQueue);
    }
    for (auto &e : q)
        menuApply(e.first, e.second);
    if (want == 1)
    {
        g_menuOpen = true;
        std::fprintf(stderr, "bt4: menu open, the game waits\n");
    }
    else if (want == 0 && g_menuOpen)
    {
        g_menuOpen = false;
        std::fprintf(stderr, "bt4: menu closed\n");
    }
    if (want == 1 || !q.empty())
        menuSnapshot();
    return g_menuOpen;
}

void quitToApp()
{
    JNIEnv *env = static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv());
    jobject act = env ? static_cast<jobject>(SDL_GetAndroidActivity()) : nullptr;
    jclass cls = act ? env->GetObjectClass(act) : nullptr;
    jmethodID m = cls ? env->GetMethodID(cls, "quitToApp", "()V") : nullptr;
    if (!m)
    {
        if (env)
            env->ExceptionClear();
        std::fflush(nullptr);
        _exit(0);
    }
    env->CallVoidMethod(act, m);
    if (env->ExceptionCheck())
        env->ExceptionClear();
}
} // namespace

// ps2_gs_live.cpp asks for the host once the ring is ready (in the browser build: the page): here it is this thread.
bool ps2xWebStartGsHost(ps2live::RingHeader *header)
{
    header->consumerFlags = 1u;
    header->consumerAlive = 1u;
    g_ring.store(header);
    std::fprintf(stderr, "bt4: GS host: Dragon Rage's renderer\n");
    return true;
}

// The runtime's start (main.cpp calls it first): its folders, the disc, the live stream, its arguments.
void ps2xAndroidPlatformInit(int &argc, char **&argv)
{
    static std::string exeDir = g_root + "/runtime";
    static std::string elf = g_root + "/runtime/data/SLUS_216.78";
    setenv("PS2X_EXEDIR", exeDir.c_str(), 1);   // its settings.toml and memory cards (runtime/savedata)
    setenv("PS2X_GS_LIVE", "1", 1);             // the GS stream to this process's host (above)
    setenv("PS2X_CD_IMAGE", g_image.c_str(), 1);
    setenv("PS2X_LIVE_SERIAL", "SLUS-21978", 0);
    setenv("PS2X_BARPACE", "0", 0);             // no GL in the runtime: its loop is paced by the headless raylib
    static char arg0[] = "ps2EntryRunner";
    static char *args[] = {arg0, const_cast<char *>(elf.c_str()), nullptr};
    argc = 2;
    argv = args;
    std::fprintf(stderr, "bt4: game folder %s, disc %s\n", g_root.c_str(), g_image.c_str());
}

// (bt4_shim.c) the renderer's start: its GPU and back end, for the menu's lines
extern "C" void Bt4_UiInit(const char *gpu, int gl)
{
    g_gpuName = gpu ? gpu : "";
    g_gl = gl;
}

// (bt4_shim.c) START + SELECT, or the back key reaching the engine: the app's menu
extern "C" void Bt4_OpenMenu(void)
{
    JNIEnv *env = static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv());
    jobject act = env ? static_cast<jobject>(SDL_GetAndroidActivity()) : nullptr;
    jclass cls = act ? env->GetObjectClass(act) : nullptr;
    jmethodID m = cls ? env->GetMethodID(cls, "openGameMenu", "()V") : nullptr;
    if (!m)
    {
        if (env)
            env->ExceptionClear();
        return;
    }
    env->CallVoidMethod(act, m);
    if (env->ExceptionCheck())
        env->ExceptionClear();
}

extern "C" void Bt4_Quit(void)
{
    if (ps2live::RingHeader *r = g_ring.load())
        r->consumerQuit = 1;
    g_quit = true;
    quitToApp();
}

// The on-screen controller (TouchBridge, as libmain.so answers it for BT3).
extern "C" JNIEXPORT void JNICALL Java_com_dfdx047_dragonrage_engine_TouchBridge_nativeSetPad(JNIEnv *, jclass, jint buttons, jint lx, jint ly,
                                                                                              jint rx, jint ry)
{
    Port_TouchPad(static_cast<unsigned>(buttons), lx, ly, rx, ry);
}
extern "C" JNIEXPORT jboolean JNICALL Java_com_dfdx047_dragonrage_engine_TouchBridge_nativeMenuOpen(JNIEnv *, jclass) { return JNI_FALSE; }
extern "C" JNIEXPORT jint JNICALL Java_com_dfdx047_dragonrage_engine_TouchBridge_nativePlayerTeamSize(JNIEnv *, jclass) { return -1; }

// The touch menu (MenuBridge, as libmain.so answers it for BT3).
extern "C" JNIEXPORT jboolean JNICALL Java_com_dfdx047_dragonrage_engine_MenuBridge_nativeOpen(JNIEnv *, jclass, jboolean open)
{
    g_menuWant = open ? 1 : 0;
    return GsGpu_Enabled() ? JNI_TRUE : JNI_FALSE;
}
extern "C" JNIEXPORT jint JNICALL Java_com_dfdx047_dragonrage_engine_MenuBridge_nativeGen(JNIEnv *, jclass)
{
    std::lock_guard<std::mutex> lock(g_menuLock);
    return g_menuGen;
}
extern "C" JNIEXPORT jint JNICALL Java_com_dfdx047_dragonrage_engine_MenuBridge_nativeGet(JNIEnv *env, jclass, jstring key, jint def)
{
    const char *k = key ? env->GetStringUTFChars(key, nullptr) : nullptr;
    jint v = def;
    if (k)
    {
        std::lock_guard<std::mutex> lock(g_menuLock);
        auto it = g_menuSnap.find(k);
        if (it != g_menuSnap.end())
            v = it->second;
    }
    if (k)
        env->ReleaseStringUTFChars(key, k);
    return v;
}
extern "C" JNIEXPORT void JNICALL Java_com_dfdx047_dragonrage_engine_MenuBridge_nativeSet(JNIEnv *env, jclass, jstring key, jint value)
{
    const char *k = key ? env->GetStringUTFChars(key, nullptr) : nullptr;
    if (!k)
        return;
    {
        std::lock_guard<std::mutex> lock(g_menuLock);
        g_menuQueue.emplace_back(k, value);
        auto it = g_menuSnap.find(k);
        if (it != g_menuSnap.end())
            it->second = value;
    }
    env->ReleaseStringUTFChars(key, k);
}
extern "C" JNIEXPORT jstring JNICALL Java_com_dfdx047_dragonrage_engine_MenuBridge_nativeText(JNIEnv *env, jclass, jstring key)
{
    const char *k = key ? env->GetStringUTFChars(key, nullptr) : nullptr;
    std::string out;
    if (k && std::strcmp(k, "gpu") == 0)
        out = g_gpuName;
    if (k)
        env->ReleaseStringUTFChars(key, k);
    return env->NewStringUTF(out.c_str());
}
extern "C" JNIEXPORT void JNICALL Java_com_dfdx047_dragonrage_engine_MenuBridge_nativeAction(JNIEnv *, jclass, jint action)
{
    if (action == 1)
        g_menuWant = 0;
}

extern "C" __attribute__((visibility("default"))) int SDL_main(int argc, char *argv[])
{
    g_root = argc > 1 ? argv[1] : ".";
    g_image = argc > 3 ? argv[3] : "";
    if (chdir(g_root.c_str()) != 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "cannot use the game folder %s: %s", g_root.c_str(), std::strerror(errno));
        return 1;
    }
    redirectOutput(g_root);
    std::fprintf(stderr, "bt4: Dragon Rage, Budokai Tenkaichi 4 engine\n");
    if (g_image.empty() || access(g_image.c_str(), R_OK) != 0)
    {
        std::fprintf(stderr, "bt4: the disc image cannot be read (%s): set it up again in the launcher\n", g_image.c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Budokai Tenkaichi 4",
                                 "The disc image cannot be read. Set it up again in the launcher.", nullptr);
        quitToApp();
        return 1;
    }

    // the settings by their full path: the game's own code may change the current folder later (its fioChdir), and
    // the renderer reads and writes bt3_settings.txt when it is asked (a setting changed in the menu)
    static std::string settingsPath = g_root + "/bt3_settings.txt";
    setenv("BT3_SETTINGS", settingsPath.c_str(), 1);

    // Dragon Rage's GS: its window and renderer (Vulkan or OpenGL ES, the launcher's choice), on this thread
    setenv("BT3_GS", "gpu", 1);
    Port_GsGifChannel(0, 0, 0);
    if (!GsGpu_Enabled())
    {
        std::fprintf(stderr, "bt4: the renderer could not start\n");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Budokai Tenkaichi 4", "The renderer could not start (see the log).", nullptr);
        quitToApp();
        return 1;
    }

    // the game, on a thread of its own (large stack: the runtime's main thread runs the game's fibers' scheduler)
    std::thread([] {
        static char arg0[] = "ps2EntryRunner";
        static char *args[] = {arg0, nullptr};
        const int code = ps2x_runtime_main(1, args);
        std::fprintf(stderr, "bt4: the game ended (%d)\n", code);
        g_quit = true;
    }).detach();

    Reader r;
    auto lastPump = std::chrono::steady_clock::now();
    auto lastStat = lastPump;
    while (!g_quit)
    {
        ps2live::RingHeader *ring = g_ring.load();
        if (!ring || ring->magic != ps2live::kMagic)
        {
            menuFrame(); // (the menu's values, before the game's stream is there)
            GsGpu_PollEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (!r.ring)
        {
            r.ring = ring;
            r.data = ps2live::ringData(ring);
            r.capacity = ring->capacity;
        }
        if (menuFrame())
        {
            // the menu is up: the last picture again, no pad; the stream waits (the game with it)
            ring->padButtons = 0;
            ring->padAxes = 0x80808080u;
            ring->padSequence = ring->padSequence + 1;
            if (!GsGpu_PauseFrame())
                GsGpu_PollEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            lastPump = std::chrono::steady_clock::now();
            continue;
        }
        if (processPackets(r))
        {
            gPortVBlanks++;
            GsGpu_FrameEnd(); // the picture: drawn and shown (events polled there too)
            sendPad(ring);
            r.frames++;
            lastPump = std::chrono::steady_clock::now();
            if (lastPump - lastStat > std::chrono::seconds(10))
            {
                std::fprintf(stderr, "bt4: %llu pictures, %llu transfers, %.1f MB of GS data in the last 10 s\n",
                             static_cast<unsigned long long>(r.frames), static_cast<unsigned long long>(r.transfers), r.bytes / 1048576.0);
                r.frames = r.transfers = r.bytes = 0;
                lastStat = lastPump;
            }
            continue;
        }
        // nothing to draw yet: keep the window alive and the pad flowing, then wait for data
        const auto now = std::chrono::steady_clock::now();
        if (now - lastPump > std::chrono::milliseconds(16))
        {
            GsGpu_PollEvents();
            sendPad(ring);
            lastPump = now;
        }
        ring->consumerWaiting = 1u;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        if (r.available() == 0)
            ps2live::eventWait(&ps2live::ringEvents(ring)->data, 5);
        ring->consumerWaiting = 0u;
    }
    std::fprintf(stderr, "bt4: leaving\n");
    std::fflush(nullptr);
    quitToApp();
    return 0;
}
