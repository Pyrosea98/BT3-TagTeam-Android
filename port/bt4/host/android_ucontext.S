// [android] getcontext / setcontext / swapcontext for aarch64 bionic (see android_ucontext.h).
// Layout: ucontext_t.uc_mcontext at 176: regs[31] at +8, sp at +256, pc at +264, then __reserved at +288 (used
// here for d8-d15). Only what a cooperative switch needs: callee-saved x19-x30, sp, d8-d15, plus x0-x7 so that
// makecontext can pass arguments.
#define MC   176
#define REG(n) (MC + 8 + (n) * 8)
#define SP   (MC + 256)
#define PC   (MC + 264)
#define FPR  (MC + 288)

    .text
    .global getcontext
    .type getcontext, %function
getcontext:
    str     xzr, [x0, #REG(0)]          // returns 0 when resumed
    stp     x19, x20, [x0, #REG(19)]
    stp     x21, x22, [x0, #REG(21)]
    stp     x23, x24, [x0, #REG(23)]
    stp     x25, x26, [x0, #REG(25)]
    stp     x27, x28, [x0, #REG(27)]
    stp     x29, x30, [x0, #REG(29)]
    str     x30, [x0, #PC]
    mov     x2, sp
    str     x2, [x0, #SP]
    add     x2, x0, #FPR
    stp     d8, d9, [x2, #0]
    stp     d10, d11, [x2, #16]
    stp     d12, d13, [x2, #32]
    stp     d14, d15, [x2, #48]
    mov     x0, #0
    ret
    .size getcontext, .-getcontext

    .global setcontext
    .type setcontext, %function
setcontext:
    add     x2, x0, #FPR
    ldp     d8, d9, [x2, #0]
    ldp     d10, d11, [x2, #16]
    ldp     d12, d13, [x2, #32]
    ldp     d14, d15, [x2, #48]
    ldp     x19, x20, [x0, #REG(19)]
    ldp     x21, x22, [x0, #REG(21)]
    ldp     x23, x24, [x0, #REG(23)]
    ldp     x25, x26, [x0, #REG(25)]
    ldp     x27, x28, [x0, #REG(27)]
    ldp     x29, x30, [x0, #REG(29)]
    ldr     x2, [x0, #SP]
    mov     sp, x2
    ldr     x16, [x0, #PC]
    ldp     x2, x3, [x0, #REG(2)]
    ldp     x4, x5, [x0, #REG(4)]
    ldp     x6, x7, [x0, #REG(6)]
    ldp     x0, x1, [x0, #REG(0)]
    br      x16
    .size setcontext, .-setcontext

    .global swapcontext
    .type swapcontext, %function
swapcontext:
    str     xzr, [x0, #REG(0)]
    stp     x19, x20, [x0, #REG(19)]
    stp     x21, x22, [x0, #REG(21)]
    stp     x23, x24, [x0, #REG(23)]
    stp     x25, x26, [x0, #REG(25)]
    stp     x27, x28, [x0, #REG(27)]
    stp     x29, x30, [x0, #REG(29)]
    str     x30, [x0, #PC]
    mov     x2, sp
    str     x2, [x0, #SP]
    add     x2, x0, #FPR
    stp     d8, d9, [x2, #0]
    stp     d10, d11, [x2, #16]
    stp     d12, d13, [x2, #32]
    stp     d14, d15, [x2, #48]
    mov     x0, x1
    b       setcontext
    .size swapcontext, .-swapcontext

    .section .note.GNU-stack,"",%progbits
