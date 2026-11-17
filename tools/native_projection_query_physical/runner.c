// Freestanding i386 runner: unchanged TU1.5 .text/.rdata/.data at their VAs.
// Built by tools/validate_native_projection_query_tu15.py (gcc -m32, ld -melf_i386).
typedef unsigned u32; typedef unsigned short u16; typedef unsigned char u8;
static long sys3(long n, long a, long b, long c) { long r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory"); return r; }
static int readall(void *p, u32 n) { u8 *q = p; while (n) { long r = sys3(3, 0, (long)q, n); if (r <= 0) return 0; q += r; n -= r; } return 1; }
static void writeall(const void *p, u32 n) { const u8 *q = p; while (n) { long r = sys3(4, 1, (long)q, n); if (r <= 0) sys3(1, 3, 0, 0); q += r; n -= r; } }
static void zero(u32 a, u32 n) { u32 *p = (u32 *)a; for (u32 i = 0; i < n / 4; ++i) p[i] = 0; }
extern void call_original(void);
extern u32 io_entry, io_esp, io_regs[4], io_env[7], io_mxcsr, io_out[8];
extern u32 allocator_object[2];
extern u32 io_crashed;
extern void crash_handler(void);
struct KAction { u32 handler, flags, restorer, mask[2]; };
static void install(void) {
    static u32 alt[3] = {0x6fd00000u, 0, 0x100000u};   // ss_sp, ss_flags, ss_size
    sys3(186, (long)alt, 0, 0);
    struct KAction a = {(u32)crash_handler, 0x40000000u | 0x08000000u, 0, {0, 0}}; // NODEFER|ONSTACK
    for (int sig = 4; sig <= 11; ++sig) {  // ILL TRAP ABRT BUS FPE KILL(skip) USR1 SEGV
        if (sig == 9 || sig == 10) continue;
        long r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(174), "b"(sig), "c"(&a), "d"(0), "S"(8) : "memory");
    }
}
struct Header { u32 entry, esp, cw, sw, mxcsr, ebx, ebp, esi, edi, pages; };
void runner_main(void) {
    *(u32 *)0x48c938u = (u32)allocator_object;
    install();
    for (;;) {
        struct Header h;
        if (!readall(&h, sizeof h)) break;
        zero(0x20000000u, 0x100000u); zero(0x30000000u, 0x40000u);
        for (u32 i = 0; i < h.pages; ++i) { u32 a; if (!readall(&a, 4) || !readall((void *)a, 4096)) sys3(1, 2, 0, 0); }
        io_entry = h.entry; io_esp = h.esp; io_mxcsr = h.mxcsr;
        io_regs[0] = h.ebx; io_regs[1] = h.ebp; io_regs[2] = h.esi; io_regs[3] = h.edi;
        for (int i = 0; i < 7; ++i) io_env[i] = 0;
        io_env[0] = 0xffff0000u | h.cw; io_env[1] = 0xffff0000u | h.sw; io_env[2] = 0xffffffffu;
        call_original();
        // regs: ebx, esp, ebp, esi, edi, fsw, fcw, mxcsr; then heap low/high regions.
        writeall(&io_crashed, 4);
        writeall(io_out, 32);
        const u32 top = *(u32 *)0x20000000u;
        u32 size = top > 0x20080000u && top < 0x20100000u ? top - 0x20080000u : 0;
        writeall(&size, 4);
        writeall((void *)0x20000000u, 0x10000u);
        writeall((void *)0x20080000u, size);
    }
    sys3(1, 0, 0, 0);
}
