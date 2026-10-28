# libbt4gsx.so: Budokai Tenkaichi 4 drawn by ARMSX2's GS (see bt4_gsx.cpp). Included at the end of ARMSX2's Android
# build (platforms/android/app/src/main/cpp/CMakeLists.txt, with -DARMSX2_EXTRA_CMAKE=<this file>), so it has that
# build's targets: the PCSX2 core and its 3rdparty libraries, built as ARMSX2's own app builds them.
#   BT4GSX_DIR       this folder
#   BT4GSX_RING_DIR  the folder with ps2_live_ring.h (BT3-Recomp's ps2xRuntime/src/lib)
set(GSR "${ARMSX2_ROOT}/pcsx2-gsrunner")
add_library(bt4gsx SHARED
	"${GSR}/Main.cpp"
	"${GSR}/GSLadder.cpp"
	"${GSR}/GSReplayPayload.cpp"
	"${GSR}/RenderDocCapture.cpp"
	"${BT4GSX_DIR}/bt4_gsx.cpp")
target_compile_definitions(bt4gsx PRIVATE BT4GSX=1)
set_source_files_properties("${GSR}/Main.cpp" PROPERTIES COMPILE_DEFINITIONS "main=bt4gsx_runner_main")
target_include_directories(bt4gsx PRIVATE
	"${CMAKE_BINARY_DIR}/common/include"
	"${ARMSX2_ROOT}"
	"${ARMSX2_ROOT}/pcsx2"
	"${ARMSX2_ROOT}/3rdparty/include"
	"${BT4GSX_RING_DIR}")
target_link_libraries(bt4gsx PRIVATE
	PCSX2_FLAGS
	PCSX2
	common
	imgui
	libzip::zip
	SDL3::SDL3
	shaderc
	vixl
	adrenotools
	Zstd::Zstd
	android
	EGL
	GLESv2
	log
	unwind
	${CMAKE_DL_LIBS})
# only Bt4Gsx_* leave the library: its copies of SDL, ImGui, fmt... stay its own next to libbt4.so's
target_link_options(bt4gsx PRIVATE -Wl,--gc-sections -Wl,-z,max-page-size=16384 -Wl,--exclude-libs,ALL -static-libstdc++)

# This build is configured as Linux told it is Android (build_bt4.py): the libraries that test CMAKE_SYSTEM_NAME for
# "Android" miss their Android sources, which are added back here.
if(TARGET cpuinfo AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cpuinfo/src/arm/android/properties.c")
	target_sources(cpuinfo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cpuinfo/src/arm/android/properties.c")
endif()
