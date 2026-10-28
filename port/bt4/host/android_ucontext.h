// [android] Bionic has ucontext_t but not getcontext / makecontext / swapcontext: these are a small aarch64
// implementation (android_ucontext.S) on bionic's own ucontext_t, enough for the runtime's fibers.
#pragma once
#if defined(__ANDROID__) && defined(__aarch64__)
#include <ucontext.h>
#include <cstddef>
extern "C" {
int getcontext(ucontext_t *ucp);
int setcontext(const ucontext_t *ucp);
int swapcontext(ucontext_t *oucp, const ucontext_t *ucp);
void makecontext(ucontext_t *ucp, void (*fn)(), int argc, ...);
}
// the offsets android_ucontext.S assumes
static_assert(offsetof(ucontext_t, uc_mcontext) == 176, "android_ucontext: uc_mcontext offset");
static_assert(offsetof(mcontext_t, regs) == 8 && offsetof(mcontext_t, sp) == 256 && offsetof(mcontext_t, pc) == 264,
              "android_ucontext: sigcontext layout");
static_assert(offsetof(ucontext_t, uc_stack) == 16, "android_ucontext: uc_stack offset");
#endif
