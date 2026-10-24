// libbt2.so's start on Android (Dragon Rage's Bt2GameActivity, android/app/.../engine/Bt2GameActivity.kt):
// SDL's activity calls SDL_main here with
//   argv[1]  the game's folder (<app files>/bt2: data/BT2.iso, saves/, startup/, user/)
//   argv[2]  the app's native library folder
//   argv[3], argv[4]  a custom GPU driver's folder and file ("" for none), loaded through libadrenotools
// and this does what the PC's game program does before it starts the runtime (runtime/game/game_main.cpp):
// the folders, the settings (the disc image and a memory card chosen when they name none), the startup
// inputs in the runtime's argument order, and the environment of a player's run. The runtime's main
// (probe/entry_probe.cpp, built as bt2_entry_main) then runs the game in this process. Its messages go to
// the log (adb logcat -s bt2) and to user/last-run.log. The startup inputs are made here from the
// player's disc image (game/startup_inputs.h: no capture, no BIOS), again whenever the image changes.
//
// The on-screen controller reaches the game through TouchBridge's native functions (host/sdl/touch_pad.h);
// leaving the game (back twice, host/sdl/sdl_player_session.h) brings the app's launcher forward.
#include "game/startup_inputs.h"
#include "gs/gpu/gpu_gs_backend.h"
#include "host/game_folders.h"
#include "host/player_session.h"
#include "host/sdl/android_performance.h"
#include "host/sdl/touch_pad.h"
#include <SDL3/SDL.h>
#include <adrenotools/driver.h>
#include <dlfcn.h>
#include <android/log.h>
#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

int bt2_entry_main(int argc,char** argv); // probe/entry_probe.cpp's main (built with -Dmain=bt2_entry_main)

namespace {
namespace fs=std::filesystem;
constexpr const char* tag="bt2";

// The startup inputs for `disc` in `folder`: made when missing or when the disc image (its size and time)
// or the way they are made changed.
void prepareStartupInputs(const fs::path& folder,const fs::path& disc) {
    std::error_code error;
    const auto stamp=folder/".made";
    // (The time as whole seconds: libc++'s file clock counts in a 128-bit type on Android.)
    const auto seconds=std::chrono::duration_cast<std::chrono::seconds>(fs::last_write_time(disc,error).time_since_epoch()).count();
    const std::string made="1 "+std::to_string(static_cast<unsigned long long>(fs::file_size(disc,error)))+" "+std::to_string(static_cast<long long>(seconds));
    {
        std::ifstream in(stamp);std::string line;
        if(std::getline(in,line) && line==made) {
            bool complete=true;
            for(const char* name:startup_input_names)complete=complete && fs::is_regular_file(folder/name,error);
            if(complete)return;
        }
    }
    std::fprintf(stderr,"Making the startup inputs from %s\n",disc.c_str());
    fs::remove_all(folder,error);
    writeStartupInputs(folder,readGameProgram(disc));
    std::ofstream(stamp)<<made<<"\n";
}

// stdout and stderr, line by line, to the log and to a file the player can send.
FILE* log_file=nullptr;int log_pipe[2];
void* logThread(void*) {
    char buffer[2048];std::size_t used=0;
    for(;;) {
        const ssize_t n=read(log_pipe[0],buffer+used,sizeof buffer-1-used);
        if(n<=0)break;
        used+=std::size_t(n);
        for(;;) {
            char* newline=static_cast<char*>(std::memchr(buffer,'\n',used));
            const std::size_t length=newline?std::size_t(newline-buffer):used==sizeof buffer-1?used:0;
            if(!length && !newline)break;
            buffer[length]='\0';
            __android_log_write(ANDROID_LOG_INFO,tag,buffer);
            if(log_file) {std::fprintf(log_file,"%s\n",buffer);std::fflush(log_file);}
            const std::size_t taken=length+(newline?1:0);
            std::memmove(buffer,buffer+taken,used-taken);used-=taken;
        }
    }
    return nullptr;
}
void redirectOutput(const fs::path& log) {
    log_file=std::fopen(log.c_str(),"w");
    if(pipe(log_pipe))return;
    setvbuf(stdout,nullptr,_IOLBF,0);setvbuf(stderr,nullptr,_IONBF,0);
    dup2(log_pipe[1],1);dup2(log_pipe[1],2);
    pthread_t thread;pthread_create(&thread,nullptr,logThread,nullptr);pthread_detach(thread);
}

// The activity's quitToApp (GameActivity.kt): the launcher comes forward and this process ends shortly after.
void quitToApp() {
    auto* env=static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    auto activity=static_cast<jobject>(SDL_GetAndroidActivity());
    if(env && activity) {
        if(jclass type=env->GetObjectClass(activity)) {
            if(jmethodID method=env->GetMethodID(type,"quitToApp","()V"))env->CallVoidMethod(activity,method);
            env->DeleteLocalRef(type);
        }
        if(env->ExceptionCheck())env->ExceptionClear();
        env->DeleteLocalRef(activity);
    }
    for(;;)std::this_thread::sleep_for(std::chrono::seconds(1)); // The activity ends the process.
}

// A custom Vulkan driver chosen in the app (GpuDrivers.kt; BT3's loader.c does the same): loaded through
// libadrenotools with its hooks from the app's native library folder; the renderer then uses its loader.
// When it cannot be loaded, the system's driver is used.
void customDriver(const std::string& libdir,const std::string& dir,const std::string& file) {
    void* handle=adrenotools_open_libvulkan(RTLD_NOW,ADRENOTOOLS_DRIVER_CUSTOM,nullptr,(libdir+"/").c_str(),(dir+"/").c_str(),file.c_str(),nullptr,nullptr);
    void* loader=handle?dlsym(handle,"vkGetInstanceProcAddr"):nullptr;
    if(!loader) {std::fprintf(stderr,"Custom GPU driver %s/%s could not be loaded: the system's driver is used\n",dir.c_str(),file.c_str());return;}
    GpuGsBackend::setVulkanLoader(loader);
    std::fprintf(stderr,"Custom GPU driver: %s/%s\n",dir.c_str(),file.c_str());
}

void setEnv(const char* name,const std::string& value) {if(value.empty())unsetenv(name);else setenv(name,value.c_str(),1);}

// A message for the player, then back to the launcher.
int stop(const std::string& text) {
    std::fprintf(stderr,"%s\n",text.c_str());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Budokai Tenkaichi 2",text.c_str(),nullptr);
    quitToApp();
    return 1;
}
}

extern "C" __attribute__((visibility("default"))) int SDL_main(int argc,char* argv[]) {
    const fs::path root=argc>1?fs::path(argv[1]):fs::current_path();
    const std::string libdir=argc>2?argv[2]:"";
    const fs::path user=root/"user",inputs=root/"startup",settings_path=user/"settings";
    std::error_code error;
    fs::create_directories(user,error);
    redirectOutput(user/"last-run.log");
    if(chdir(root.c_str()))std::fprintf(stderr,"Cannot enter %s\n",root.c_str());
    if(argc>4 && *argv[3] && *argv[4])customDriver(libdir,argv[3],argv[4]);
    try {
        const GameFolders folders(root);
        folders.create();
        Settings settings;
        if(auto stored=Settings::load(settings_path.string()))settings=*stored;
        // The disc image the app copied (data/BT2.iso), and a memory card, when the settings name none.
        if(!folders.configured(settings)) {
            const auto discs=folders.discImages();
            if(discs.empty())return stop("No Budokai Tenkaichi 2 disc image in "+utf8(folders.data)+". Choose your ISO in the app.");
            settings.disc=discs.front();
        }
        if(folders.card(settings).empty()) {
            const auto cards=folders.cards();
            settings.memory_card=cards.empty()?folders.newCard():cards.front();
        }
        settings.save(settings_path.string());

        // MEMORY_IMAGE STOP_REPORT inputs... with the disc before the last input, as game_main.cpp builds it.
        prepareStartupInputs(inputs,folders.disc(settings));
        const std::size_t count=startup_input_names.size();
        std::vector<std::string> args{"bt2",(inputs/startup_input_names[0]).string(),(user/"last-run.json").string()};
        for(std::size_t n=1;n<count;++n) {
            if(n==count-1)args.push_back(folders.disc(settings).string());
            args.push_back((inputs/startup_input_names[n]).string());
        }

        // A player's run (game_main.cpp: runGame).
        for(const char* name:{"BT2_VIEWER","BT2_AUDIO_DEVICE","BT2_HOST_INPUT"})setenv(name,"1",1);
        setenv("BT2_GPU_GS","on",1);
        setenv("BT2_SELF_STARTUP","1",1); // The inputs above are made, not captured (game/startup_inputs.h).
        setEnv("BT2_PLAYER_SETTINGS",settings_path.string());
        setEnv("BT2_HD_TEXTURES",settings.hd_textures?folders.texturePack().string():"");
        setEnv("BT2_SUPER_SAMPLING",std::to_string(settings.super_sampling));
        setEnv("BT2_GAME_PATCHES",settings.widescreen?"widescreen":"");
        setEnv("BT2_SIXTY_FPS",settings.sixty_fps?"1":"");
        setEnv("BT2_CARD_FOLDER",folders.card(settings).string());
        for(const char* name:{"BT2_FRAME_LIMIT","BT2_PAD_SCRIPT","BT2_PAD_RECORD","BT2_VIEWER_SCALE","BT2_GPU_GS_SNAPSHOT"})unsetenv(name);
        // Android-only options the app writes beside the engine's settings (Dragon Rage's Bt2Settings.kt), which
        // the settings file cannot hold: "gs_hacks = <list>", paraLLEl-GS's speed-for-accuracy options.
        {
            std::ifstream in(user/"android");std::string line,hacks;
            while(std::getline(in,line))if(line.rfind("gs_hacks = ",0)==0)hacks=line.substr(11);
            setEnv("BT2_GS_HACKS",hacks);
        }

        PlayerSession::quit_to_launcher=quitToApp;
        std::vector<char*> pointers;
        for(auto& arg:args)pointers.push_back(arg.data());
        pointers.push_back(nullptr);
        std::fprintf(stderr,"Starting Budokai Tenkaichi 2 from %s\n",root.c_str());
        pinToFastCores(); // The game runs on this thread (host/sdl/android_performance.h).
        const int code=bt2_entry_main(int(args.size()),pointers.data());
        std::fprintf(stderr,"The game ended (%d).\n",code);
        if(code)return stop("The game stopped. The messages are in "+utf8(user/"last-run.log")+".");
        quitToApp();
        return 0;
    } catch(const std::exception& e) {
        return stop(e.what());
    }
}

// ---- The on-screen controller (com.dfdx047.dragonrage.engine.TouchBridge).
extern "C" JNIEXPORT void JNICALL Java_com_dfdx047_dragonrage_engine_TouchBridge_nativeSetPad(JNIEnv*,jclass,jint buttons,jint lx,jint ly,jint rx,jint ry) {
    TouchPad::set(unsigned(buttons),lx,ly,rx,ry);
}
// The game's own menu (BT3's) is never open here: the controller always stays.
extern "C" JNIEXPORT jboolean JNICALL Java_com_dfdx047_dragonrage_engine_TouchBridge_nativeMenuOpen(JNIEnv*,jclass) {return JNI_FALSE;}
// No battle roster is read from BT2 yet: unknown.
extern "C" JNIEXPORT jint JNICALL Java_com_dfdx047_dragonrage_engine_TouchBridge_nativePlayerTeamSize(JNIEnv*,jclass) {return -1;}
