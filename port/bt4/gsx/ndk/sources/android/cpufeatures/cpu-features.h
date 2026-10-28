/* Minimal stand-in for the NDK's cpu-features (arm64 only: NEON is always there). */
#pragma once
#include <stdint.h>
typedef enum { ANDROID_CPU_FAMILY_UNKNOWN = 0, ANDROID_CPU_FAMILY_ARM, ANDROID_CPU_FAMILY_X86, ANDROID_CPU_FAMILY_MIPS,
               ANDROID_CPU_FAMILY_ARM64, ANDROID_CPU_FAMILY_X86_64, ANDROID_CPU_FAMILY_MIPS64 } AndroidCpuFamily;
enum { ANDROID_CPU_ARM_FEATURE_ARMv7 = 1, ANDROID_CPU_ARM_FEATURE_VFPv3 = 2, ANDROID_CPU_ARM_FEATURE_NEON = 4 };
enum { ANDROID_CPU_ARM64_FEATURE_FP = 1, ANDROID_CPU_ARM64_FEATURE_ASIMD = 2 };
#ifdef __cplusplus
extern "C" {
#endif
AndroidCpuFamily android_getCpuFamily(void);
uint64_t android_getCpuFeatures(void);
int android_getCpuCount(void);
#ifdef __cplusplus
}
#endif
