#include "gs/hw/hw_gs_mirror.h"
#include "gs/cpu/gs_command_sink.h"
#include "host/sdl/sdl_viewer_window.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

extern "C" {
int Bt2Hw_Start(void);
void Bt2Hw_Gif(const std::uint8_t* p,std::uint32_t qwc);
void Bt2Hw_FrameEnd(void);
void Bt2Hw_Field(void);
void Bt2Hw_SetLateUs(int us);
extern int gGsMainFbp;
extern int gsScale4; // (gs/hw/bt3/gs_draw.c: the internal resolution in quarters)
}

namespace {
ViewerWindow* viewer_window=nullptr;
}

// What the renderer's copy asks of BT2 (gs/hw/bt3/gs_draw.c, gs_gpu.c, gs_gl.c).
extern "C" SDL_Window* Bt2Hw_ExistingWindow(void) {return viewer_window?viewer_window->ensureWindow():nullptr;}
extern "C" void Bt2Hw_Event(const SDL_Event* event) {if(viewer_window && event)viewer_window->handle(*event);}

namespace HwGs {
double gif_seconds=0;

bool wanted() {
    const char* renderer=std::getenv("BT2_RENDERER");
    return renderer && std::string_view(renderer)=="bt3";
}

bool attach(GsCommandSink& sink,ViewerWindow& viewer) {
    viewer_window=&viewer;
    setenv("BT3_GS","gpu",1); // (the copy's own switch: its GPU back end, in the window)
    if(!Bt2Hw_Start()) {
        std::fprintf(stderr,"BT3 renderer: no GPU device; drawing with paraLLEl-GS\n");
        viewer_window=nullptr;
        return false;
    }
    sink.on_accepted=[](std::span<const std::uint8_t> data) {
        const auto begin=std::chrono::steady_clock::now();
        Bt2Hw_Gif(data.data(),std::uint32_t(data.size()/16));
        gif_seconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    };
    std::fprintf(stderr,"BT3 renderer: drawing with the GPU's rasteriser\n");
    return true;
}

double scale() {return gsScale4*0.25;}
void field() {Bt2Hw_Field();}
void setLate(double seconds) {Bt2Hw_SetLateUs(int(seconds*1e6));}
void flip(const std::array<std::uint64_t,15>& privileged) {
    // The read circuit that is on (PMODE EN2, else EN1) and the frame buffer it shows (DISPFB.FBP, in the same
    // 2048-word units as FRAME.FBP).
    const std::uint64_t pmode=privileged[0],dispfb=(pmode&2)?privileged[9]:privileged[7];
    if(pmode&3)gGsMainFbp=int(dispfb&0x1FF);
    Bt2Hw_FrameEnd();
}

void readFifo(std::span<std::uint8_t> out) {std::memset(out.data(),0,out.size());}
}
