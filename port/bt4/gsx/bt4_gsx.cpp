// Budokai Tenkaichi 4 drawn by the ARMSX2 emulator's GS (port/bt4/gsx): libbt4gsx.so.
//
// The game (libbt4.so: Team BT4's mod recompiled with BT3-Recomp) sends what it draws as a live GS stream in the
// format of a PCSX2 GS dump, into a ring in memory (ps2_live_ring.h). Dragon Rage's own GS reads it by default
// (bt4_main.cpp). With the renderer "ARMSX2" chosen, this library reads it instead: ARMSX2's PCSX2 core replays the
// stream as it comes (GSDumpReplayer's live mode, patches/armsx2-bt4.patch) through PCSX2's hardware renderer, with
// ARMSX2's Vulkan / OpenGL ES fixes for Mali and Adreno. The host around it is ARMSX2's pcsx2-gsrunner (its
// Host:: functions and settings), given the game's window here. Exactly what dbz-bt4-recomp does on Windows and
// macOS with PCSX2's pcsx2-gsrunner in a process of its own; here it is the same process.
//
// bt4_main.cpp loads this library with dlopen and calls Bt4Gsx_Run on a thread of its own; the pad, the menu and
// the window's events stay with bt4_main.cpp.
#include "common/WindowInfo.h"
#include "pcsx2/GSDumpReplayer.h"
#include "pcsx2/Host.h"
#include "pcsx2/MTGS.h"
#include "pcsx2/VMManager.h"

#include <android/log.h>
#include <android/native_window.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "ps2_live_ring.h"

int bt4gsx_runner_main(int argc, char* argv[]); // pcsx2-gsrunner's main() (Main.cpp, renamed by the build)

namespace
{
	std::mutex s_window_lock;
	ANativeWindow* s_window = nullptr;   // the app's (SDL's) window, while it has a surface
	ANativeWindow* s_acquired = nullptr; // the GS's own reference to it
	int s_width = 0, s_height = 0;

	ps2live::RingHeader* s_ring = nullptr;
	uint8_t* s_data = nullptr;
	uint64_t s_capacity = 0;
	std::vector<uint8_t> s_packet;
	std::atomic<bool> s_stop{false};
	std::atomic<bool> s_hold{false}; // the app's menu is up: the stream is not read (the game waits on its ring)

	constexpr uint8_t kTypeTransfer = 0, kTypeVSync = 1, kTypeReadFifo2 = 2, kTypeRegisters = 3;
	constexpr uint64_t kPrivRegsSize = 8192u;

	uint64_t available()
	{
		const int64_t w = __atomic_load_n(&s_ring->writePos, __ATOMIC_ACQUIRE);
		return static_cast<uint64_t>(w - s_ring->readPos);
	}

	void peek(uint64_t skip, void* dst, uint64_t size)
	{
		uint8_t* out = static_cast<uint8_t*>(dst);
		uint64_t pos = static_cast<uint64_t>(s_ring->readPos) + skip;
		while (size)
		{
			const uint64_t offset = pos % s_capacity;
			const uint64_t chunk = std::min(size, s_capacity - offset);
			std::memcpy(out, s_data + offset, chunk);
			out += chunk;
			pos += chunk;
			size -= chunk;
		}
	}

	template <typename T>
	T value(uint64_t skip)
	{
		T v;
		peek(skip, &v, sizeof(T));
		return v;
	}

	void consume(uint64_t size)
	{
		__atomic_store_n(&s_ring->readPos, s_ring->readPos + static_cast<int64_t>(size), __ATOMIC_RELEASE);
		__atomic_thread_fence(__ATOMIC_SEQ_CST);
		if (s_ring->producerWaiting)
			ps2live::eventSet(&ps2live::ringEvents(s_ring)->space);
	}

	// Waits (a little) for `bytes` in the ring. false: not yet (the caller pumps the CPU thread's work and asks again).
	bool wait_for(uint64_t bytes)
	{
		if (available() >= bytes)
			return true;
		s_ring->consumerWaiting = 1u;
		__atomic_thread_fence(__ATOMIC_SEQ_CST);
		if (available() < bytes)
			ps2live::eventWait(&ps2live::ringEvents(s_ring)->data, 10);
		s_ring->consumerWaiting = 0u;
		return available() >= bytes;
	}

	// The stream's start: the dump's header (crc, header, GS state, registers), as a file the VM boots.
	bool write_header(const std::string& path)
	{
		const auto t0 = std::chrono::steady_clock::now();
		while (!wait_for(8))
		{
			if (s_stop || std::chrono::steady_clock::now() - t0 > std::chrono::seconds(60))
				return false;
		}
		const uint32_t crc = value<uint32_t>(0), header_size = value<uint32_t>(4);
		uint64_t state_size = header_size, header_bytes = 0;
		if (crc == 0xFFFFFFFFu)
		{
			header_bytes = header_size;
			while (!wait_for(8 + header_bytes))
				if (s_stop)
					return false;
			std::vector<uint8_t> header(header_bytes);
			peek(8, header.data(), header_bytes);
			uint32_t dump_state_size = 0;
			std::memcpy(&dump_state_size, header.data() + 4, sizeof(dump_state_size));
			state_size = dump_state_size;
		}
		const uint64_t total = 8 + header_bytes + state_size + kPrivRegsSize;
		while (!wait_for(total))
			if (s_stop)
				return false;
		std::vector<uint8_t> bytes(total);
		peek(0, bytes.data(), total);
		consume(total);
		FILE* f = std::fopen(path.c_str(), "wb");
		if (!f)
		{
			std::fprintf(stderr, "bt4gsx: %s cannot be written\n", path.c_str());
			return false;
		}
		std::fwrite(bytes.data(), 1, bytes.size(), f);
		std::fclose(f);
		std::fprintf(stderr, "bt4gsx: GS stream connected (%llu bytes of state)\n", static_cast<unsigned long long>(total));
		return true;
	}

	// GSDumpReplayer's live source, on the VM's CPU thread: the next whole packet of the stream.
	bool live_next(GSDumpReplayer::LivePacket* out)
	{
		for (;;)
		{
			if (s_stop || s_ring->consumerQuit)
				return false;
			if (s_hold || available() < 1)
			{
				if (!s_hold)
					wait_for(1);
				else
					std::this_thread::sleep_for(std::chrono::milliseconds(8));
				Host::PumpMessagesOnCPUThread(); // (the window's changes and the app's requests wait for this thread)
				continue;
			}
			const uint8_t type = value<uint8_t>(0);
			switch (type)
			{
				case kTypeTransfer:
				{
					if (!wait_for(6))
						continue;
					const uint8_t path = value<uint8_t>(1);
					const uint32_t length = value<uint32_t>(2);
					if (!wait_for(6ull + length))
						continue;
					s_packet.resize(length);
					peek(6, s_packet.data(), length);
					consume(6ull + length);
					*out = {type, path, s_packet.data(), length};
					return true;
				}
				case kTypeVSync:
				{
					if (!wait_for(2))
						continue;
					s_packet.resize(1);
					peek(1, s_packet.data(), 1);
					consume(2);
					*out = {type, 0, s_packet.data(), 1};
					return true;
				}
				case kTypeReadFifo2:
				{
					if (!wait_for(5))
						continue;
					s_packet.resize(4);
					peek(1, s_packet.data(), 4);
					consume(5);
					*out = {type, 0, s_packet.data(), 4};
					return true;
				}
				case kTypeRegisters:
				{
					if (!wait_for(1 + kPrivRegsSize))
						continue;
					s_packet.resize(kPrivRegsSize);
					peek(1, s_packet.data(), kPrivRegsSize);
					consume(1 + kPrivRegsSize);
					*out = {type, 0, s_packet.data(), static_cast<uint32_t>(kPrivRegsSize)};
					return true;
				}
				default:
					std::fprintf(stderr, "bt4gsx: unknown packet 0x%02X in the GS stream, stopping\n", type);
					s_ring->consumerQuit = 1;
					return false;
			}
		}
	}

	// A ReadFIFO2's data back to the game (the ring's readback area; the game takes it if it waits for it).
	void live_readback(const uint8_t* data, uint32_t bytes)
	{
		ps2live::RingEvents* ev = ps2live::ringEvents(s_ring);
		const uint32_t n = static_cast<uint32_t>(std::min<uint64_t>(bytes, ps2live::kReadbackCapacity));
		std::memcpy(ps2live::readbackData(s_ring), data, n);
		ev->readbackBytes = n;
		__atomic_add_fetch(&ev->readbackSeq, 1u, __ATOMIC_RELEASE);
		ps2live::eventSet(&ev->readback);
	}
} // namespace

// pcsx2-gsrunner's window on Android (patched Main.cpp): the app's surface, or none while it has none.
std::optional<WindowInfo> Bt4Gsx_WindowInfo()
{
	std::lock_guard<std::mutex> lock(s_window_lock);
	WindowInfo wi;
	if (s_acquired)
	{
		ANativeWindow_release(s_acquired);
		s_acquired = nullptr;
	}
	if (!s_window)
	{
		wi.type = WindowInfo::Type::Surfaceless; // (no swapchain until the surface is back)
		return wi;
	}
	ANativeWindow_acquire(s_window);
	s_acquired = s_window;
	wi.type = WindowInfo::Type::Android;
	wi.window_handle = s_window;
	wi.surface_width = static_cast<u32>(s_width > 0 ? s_width : ANativeWindow_getWidth(s_window));
	wi.surface_height = static_cast<u32>(s_height > 0 ? s_height : ANativeWindow_getHeight(s_window));
	wi.surface_scale = static_cast<float>(std::max(wi.surface_width, wi.surface_height)) / 800.0f;
	return wi;
}

void Bt4Gsx_ReleaseWindow()
{
	std::lock_guard<std::mutex> lock(s_window_lock);
	if (s_acquired)
	{
		ANativeWindow_release(s_acquired);
		s_acquired = nullptr;
	}
}

extern "C" {

struct Bt4GsxArgs
{
	void* ring;             // ps2live::RingHeader*
	void* window;           // ANativeWindow* (SDL's)
	int width, height;      // its size in pixels
	const char* root;       // the folder: resources/ (shaders, fonts), live.gs, the PCSX2 data folder
	const char* ini;        // the GS settings (dbz.ini)
	const char* renderer;   // "vulkan" or "gl" (gsrunner's names)
	float upscale;          // internal resolution, 1 = the PlayStation 2's
	const char* driverDir;  // a custom Vulkan driver (adrenotools pack folder), or "" / null
	const char* driverLib;
	const char* hookDir;    // the app's native libraries (adrenotools' hooks)
};

__attribute__((visibility("default"))) void Bt4Gsx_SurfaceChanged(void* window, int width, int height)
{
	{
		std::lock_guard<std::mutex> lock(s_window_lock);
		ANativeWindow* w = static_cast<ANativeWindow*>(window);
		if (w)
			ANativeWindow_acquire(w);
		if (s_window)
			ANativeWindow_release(s_window);
		s_window = w;
		s_width = width;
		s_height = height;
	}
	Host::RunOnCPUThread([]() {
		if (MTGS::IsOpen())
			MTGS::UpdateDisplayWindow();
	});
}

__attribute__((visibility("default"))) void Bt4Gsx_SurfaceDestroyed(void)
{
	{
		std::lock_guard<std::mutex> lock(s_window_lock);
		if (s_window)
			ANativeWindow_release(s_window);
		s_window = nullptr;
	}
	Host::RunOnCPUThread([]() {
		if (MTGS::IsOpen())
			MTGS::UpdateDisplayWindow();
	});
}

// The memory page size this core was built for: it refuses to start on another (VMManager's hardware checks).
__attribute__((visibility("default"))) int Bt4Gsx_PageSize(void)
{
	return static_cast<int>(__pagesize);
}

__attribute__((visibility("default"))) void Bt4Gsx_Hold(int hold)
{
	s_hold = hold != 0;
}

__attribute__((visibility("default"))) void Bt4Gsx_Stop(void)
{
	s_stop = true;
}

// Runs until the stream ends or Bt4Gsx_Stop: call it from a thread of its own. 0 = it ran and ended.
__attribute__((visibility("default"))) int Bt4Gsx_Run(const Bt4GsxArgs* a)
{
	s_ring = static_cast<ps2live::RingHeader*>(a->ring);
	s_data = ps2live::ringData(s_ring);
	s_capacity = s_ring->capacity;
	s_stop = false;
	{
		std::lock_guard<std::mutex> lock(s_window_lock);
		s_window = static_cast<ANativeWindow*>(a->window);
		if (s_window)
			ANativeWindow_acquire(s_window);
		s_width = a->width;
		s_height = a->height;
	}
	const std::string root = a->root;
	const std::string dump = root + "/live.gs";
	// PCSX2's folders: its program folder (resources/) and its data folder ($XDG_CONFIG_HOME/ARMSX2) both here
	setenv("ARMSX2_APPROOT", root.c_str(), 1);
	setenv("XDG_CONFIG_HOME", root.c_str(), 1);
	std::fprintf(stderr, "bt4gsx: ARMSX2's GS, %s, %.2gx, waiting for the game's stream\n", a->renderer, static_cast<double>(a->upscale));
	if (!write_header(dump))
	{
		std::fprintf(stderr, "bt4gsx: no GS stream from the game\n");
		return 1;
	}
	GSDumpReplayer::SetLive(live_next, live_readback);

	char upscale[32];
	std::snprintf(upscale, sizeof(upscale), "%g", static_cast<double>(a->upscale));
	std::vector<std::string> args = {"bt4gsx", "-renderer", a->renderer, "-upscale", upscale, "-window", "-loop", "0"};
	if (a->ini && a->ini[0])
	{
		args.push_back("-ini");
		args.push_back(a->ini);
	}
	if (a->driverDir && a->driverDir[0] && a->driverLib && a->driverLib[0] && a->hookDir && a->hookDir[0])
	{
		// (the launcher's custom driver: ARMSX2 loads it through libadrenotools itself, as for its own games)
		args.push_back("-custom-driver");
		args.push_back(std::string(a->driverDir) + "/");
		args.push_back(a->driverLib);
		args.push_back(a->hookDir);
	}
	args.push_back(dump);
	std::vector<char*> argv;
	for (std::string& s : args)
		argv.push_back(s.data());
	argv.push_back(nullptr);
	const int code = bt4gsx_runner_main(static_cast<int>(args.size()), argv.data());
	std::fprintf(stderr, "bt4gsx: ended (%d)\n", code);
	GSDumpReplayer::SetLive(nullptr, nullptr);
	Bt4Gsx_ReleaseWindow();
	{
		std::lock_guard<std::mutex> lock(s_window_lock);
		if (s_window)
			ANativeWindow_release(s_window);
		s_window = nullptr;
	}
	return code;
}

} // extern "C"
