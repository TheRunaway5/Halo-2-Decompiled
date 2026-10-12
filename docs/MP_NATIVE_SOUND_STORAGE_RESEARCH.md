# Real native backing for source sound constructors

`NativeSoundStorage` supplies actual owned OS memory to the complete sound cache constructor. It replaces the earlier test-only raw allocation/protection callbacks with a reusable production service. It does not construct a source pool, choose source physical-arena fields, produce original allocator state or mark a map/cache/device ready. The original CPU constructor still owns all pool/page/header/reservation writes.

The caller must supply a nonzero allocator identity and a private 32-bit alias interval. Those aliases translate retained source pointers to actual host storage; they are explicitly native address controls, not invented retail addresses. Existing startup/physical backing is mapped through explicit caller-supplied aliases and extents. No source arena index or limits are inferred. A declared allocator identity binds the native raw service; it does not prove that the original allocator lifecycle has run.

## Platform service

On POSIX hosts the service uses anonymous private `mmap` and `mprotect`. On Windows it uses `VirtualAlloc` with `MEM_RESERVE | MEM_COMMIT` and `VirtualProtect`. OS-owned mappings supply actual zeroed pages. Page size comes from the host, and alias intervals use that actual granularity. [Linux memory mapping](https://man7.org/linux/man-pages/man2/mmap.2.html), [Linux protection](https://man7.org/linux/man-pages/man2/mprotect.2.html), [Windows allocation](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc), [Windows protection](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect).

The represented original cache request has protection value4, verified in complete `21A6E0` and its authenticated final-update counterpart. This service supports that read/write request. Missing backing, other modes, non-page-aligned protection, wrapping intervals, overlapping mappings, unavailable allocator identities and exhausted private alias space stop explicitly. They cannot become source-success returns. Only an actual failed OS allocation returns allocation0; no forced OS failure is claimed by this test. Actual allocated storage remains retained if a later source protection request stops.

The three `SoundCacheStartupServices` callbacks retain the same owned backing after the facade is destroyed. Source-address reads require one complete nonempty interval inside one real allocation. No gap or adjacent allocation is silently stitched into a valid span. Updates run on the owning thread; callers must retain the service or storage owner while using a returned span. There is no source callback, field write or device shutdown in the final native mapping destructor.

## Original differential check

The existing frozen original cache fixture retains six source constructor cases. The new test supplies their explicit entry state and native alias control, maps real physical backing, then invokes the complete production `source_sound_cache_startup` with the actual native storage services. Allocation callbacks do not replay a recorded output or write expected pool/page fields. The native allocator obtains fresh OS mappings; original C++ constructors create their contents.

All six cases match 172,032 complete output bytes, all retained globals and physical-limit arrays, and 4,090 ordered original instruction writes. They include successful reservation, equality, insufficient space, zero reservation, signed-negative rejection and another actual arena slot. Missing physical backing preserves the exact original state before protection and does not publish a fake buffer. Additional controls check interval bounds, overlap, private alias allocation, rejected requests preserving existing bytes, actual read/write protection and callback-owned lifetime.

Fresh production GCC normal and ASan/UBSan tests pass. Independent isolated Clang normal and ASan/UBSan checks also pass. The Windows implementation compiles with MinGW and executes the same six cases under Wine, including actual `VirtualAlloc`/`VirtualProtect` calls. Wine required the new prefix directory to be created first; both the initial setup failure and successful retry logs are retained. Its unrelated graphics initialization warnings do not affect this non-graphical memory test. This is not native Windows hardware or game-window validation.

## Live integration boundary

This service can back the original sound startup/cache CPU owners and later source memory callbacks. Current source arena reservation, original allocator lifecycle, clocks/CRT RNG, saved output settings, complete driver/platform construction, map plugin/effects, voice admission and asynchronous resource lifetime remain separate owners. The canonical launcher has not been replaced by this checkpoint. It does not establish audible reload playback or all-map sound correctness.
