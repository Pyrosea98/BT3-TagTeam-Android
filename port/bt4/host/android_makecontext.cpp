// [android] makecontext for android_ucontext.S: the new context starts in fn with up to 8 integer arguments on a
// stack of its own; returning from fn ends the process (the runtime's fibers never return, uc_link is unused).
#include "android_ucontext.h"
#if defined(__ANDROID__) && defined(__aarch64__)
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
static void ctx_returned() { std::fprintf(stderr, "[fibers] a context returned\n"); std::abort(); }
extern "C" void makecontext(ucontext_t *ucp, void (*fn)(), int argc, ...) {
    va_list ap;
    va_start(ap, argc);
    for (int i = 0; i < argc && i < 8; i++) ucp->uc_mcontext.regs[i] = (uint64_t)va_arg(ap, unsigned long long);
    va_end(ap);
    uintptr_t top = (uintptr_t)ucp->uc_stack.ss_sp + ucp->uc_stack.ss_size;
    ucp->uc_mcontext.sp = top & ~(uintptr_t)15;
    ucp->uc_mcontext.pc = (uint64_t)(uintptr_t)fn;
    ucp->uc_mcontext.regs[29] = 0;
    ucp->uc_mcontext.regs[30] = (uint64_t)(uintptr_t)&ctx_returned;
}
#endif
