// [android] What bionic (API 29) does not have, for the BT3-Recomp runtime: force-included on Android only.
#pragma once
#if defined(__ANDROID__)
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <spawn.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
// thread affinity: the _np calls by way of the thread's kernel id
static inline int pthread_setaffinity_np(pthread_t t, size_t n, const cpu_set_t *set) {
    return sched_setaffinity(pthread_gettid_np(t), n, set) == 0 ? 0 : errno;
}
static inline int pthread_getaffinity_np(pthread_t t, size_t n, cpu_set_t *set) {
    return sched_getaffinity(pthread_gettid_np(t), n, set) == 0 ? 0 : errno;
}
// backtraces (crash reports): <execinfo.h> only from API 33
#if __ANDROID_API__ < 33
static inline int backtrace(void **buf, int n) { (void)buf; (void)n; return 0; }
static inline char **backtrace_symbols(void *const *buf, int n) { (void)buf; (void)n; return NULL; }
static inline void backtrace_symbols_fd(void *const *buf, int n, int fd) { (void)buf; (void)n; (void)fd; }
#endif
// the PCSX2 GS host process (ps2_gs_live.cpp) does not exist on Android: Dragon Rage's GS takes its place
static inline int shm_open(const char *name, int flags, int mode) { (void)name; (void)flags; (void)mode; errno = ENOSYS; return -1; }
static inline int shm_unlink(const char *name) { (void)name; errno = ENOSYS; return -1; }
static inline int posix_spawn_file_actions_addchdir_np(posix_spawn_file_actions_t *a, const char *path) { (void)a; (void)path; return ENOSYS; }
#ifdef __cplusplus
}
#endif
#endif
