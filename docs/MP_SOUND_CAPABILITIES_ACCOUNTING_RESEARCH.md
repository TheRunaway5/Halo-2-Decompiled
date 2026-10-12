# Original sound capability accounting

The original16-byte `GetCaps` result is SDK resource accounting. It is not a query of PCM format, a host device's channel count, saved audio settings, or hardware capability registers. The native CPU module now reproduces the complete reader and its bounded counter/storage constructors. Normal DirectSound startup remains stopped at its earlier real saved-setting request.

The authority is the owned disc1.1 XBE and authenticated supplied1.5 XBE, pinned in [the complete correspondence report](../analysis/multiplayer/integration/sound-capabilities-native/paired-v2/report.json). All27 complete bodies, internal branches and direct-call identities correspond. Public381BEE maps to38568E; internal380810 maps to3842B0. The public wrapper converts a nonzero interface pointer to `pointer-8`, but the internal reader never dereferences that argument. A zero return therefore proves only the four copies completed; it does not prove a valid initialized device. [Exact reader](../analysis/multiplayer/integration/sound-capabilities-native/paired-v2/instructions/00380810-old.asm).

| Output |1.1 owning storage|1.5 owning storage|Verified producer/consumer|
|---|---|---|---|
|+0 free2D|38D130|390BD0|384C7C/38871C writes192;384D77/388817 claims managed voice slots;384E08/3888A8 returns slots|
|+4 free3D|38D134|390BD4|Same constructors write64; claims/frees select this counter for source voice-definition flags `0x200010`|
|+8 freeSGE|38D138|390BD8|38689A/38A33A installs the counter pointer;3868DC/38A37C writes low16 caller count and builds its descriptor lists;386967/38A407 subtracts actual claimed extent|
|+C allocatedbytes|38D120+38D124|390BC0+390BC4|383ECC/38796C adds actual raw pool block capacity through the source-owned statistics pointer; reader sums both DWORDs modulo32|

The voice constructor384C7C contains the immediate192 and64 stores. It initializes the actual list heads, source counter pointers, base/interface identities, retained parameter reference and SGE child. Raw source allocation clears the object's256 voice-slot words before construction. The claim owner selects3D indices starting0 or2D indices starting64, checks the corresponding free count against its channel-count byte, subtracts that count, then records actual empty slot identities. These dynamic claim/free bodies are completely paired static evidence; this checkpoint does **not** port or execute a live voice-allocation lifecycle.

The SGE count is an authored SDK initialization argument:3858D8/389378 pushes `0x7FF` at3858F3/389393 into3868DC/38A37C. The leaf allocates `(low16(count)+1)*20` bytes, initializes the first descriptor with start0/count, and places `count` remaining descriptors on its free list. Thus the stock argument yields2,047 free entries and2,048 descriptor records. The preceding whole38B1EB/38EC8B hardware initialization is mandatory in the normal caller and remains unexecuted here. The value is not obtained from an SDL format or an observed MMIO result. [Complete caller](../analysis/multiplayer/integration/sound-capabilities-native/paired-v2/instructions/003858d8-old.asm), [complete descriptor initializer](../analysis/multiplayer/integration/sound-capabilities-native/paired-v2/instructions/003868dc-old.asm).

## Executed composition and exact limits

The new oracle freshly executes the unchanged frozen heap/CRT/statistics/controller/audio/device prefix on the complete recorded compatibility thread context and recorded measured clock inputs. Real anonymous OS mappings back the pool provider; original383ECC owns accounting/clearing and383F92 owns the block marker/client pointer. Actual DirectSound creation again stops at `ExQueryNonVolatileSetting(XC_AUDIO)`, ordinal24. No saved setting, platform success, source device, hardware register, readiness or admission result is supplied. Its original partial objects, stack and held critical section are retained.

The subsequent experiment is explicitly **independent raw-ABI composition**, using a separate declared stack. It does not resume or return the stopped DirectSoundCreate. The parameter object is the actual source-created partial3832AA object;384C7C executes its real3817AF→virtual38034C reference increment. The APU object is real raw storage returned by the original SDK allocator. The separate SGE leaf uses the literal caller argument, without claiming that its earlier hardware prerequisite returned.

|Executed point|Actual16-byte result interpreted as DWORDs|Audio readiness|
|---|---|---|
|Real partial startup, NULL or actual partial device argument|0,0,0,8192|0|
|Independent complete APU CPU constructor|192,64,0,12288|0|
|Independent stock SGE CPU descriptor initializer|192,64,2047,57344|0|

The byte totals reflect the **actual host provider's**4,096-byte allocation granularity, not verified retail Xbox pool rounding. Three additional explicitly declared negative raw fault-word controls prove the reader's E_FAIL branch preserves the output. They are not source-produced fault transitions or positive admission inputs. There is no capability-counter assignment in these positive cases.

GetCaps3804DE reads the actual recorded compatibility KPCR IRQL byte and executes the previously frozen complete compatibility critical-section owner. The source snapshot is distinct from ordinary Xbox thread initialization. The native comparison consumes that recorded byte explicitly and uses real `NativeSynchronization` recursive locks. It compares all final raw source records and all20,537 ordered SDK stores;14 compatibility-platform recursion stores are separately retained in original evidence. The host mutex is not presented as a bit-identical guest kernel critical-section implementation.

## Native contract

[sound_capabilities.hpp](../include/halo2/sound_capabilities.hpp) and [sound_capabilities.cpp](../src/sound_capabilities.cpp) provide:

- Complete381BEE→380810/3804DE reader, including the fault/lock branches and ordered output stores.
- Complete384C7C,38689A,38AD61,38B257,3817AF and the actually selected38034C reference method. A different virtual method stops rather than receiving a fabricated return.
- Complete383F92→383ECC allocation/accounting order. Real raw allocation/capacity are mandatory services; a selected non-NULL private pool requires its separate complete386BE7 owner.
- Complete3868DC/38683D descriptor construction, preserving low16 count, allocation failure result, intrusive-list write order and full storage.

The ten mutable addresses in `SoundCapabilitiesBindings` must resolve actual retained source storage. The six `SoundCapabilitiesConstructorConstants` fields are immutable **identities**, in order: reference vtable, APU vtable, APU interface vtable, SGE vtable, base vtable, reference-add method. [Exact old/u5 packet](../analysis/multiplayer/integration/sound-capabilities-native/paired-v2/providers.json) retains instruction-derived addresses and the reference vtable method bytes. These are not host device handles, successful callbacks, counter values or runtime readiness.

Required native services are bounded memory, an actual source thread-context input, real enter/leave synchronization, real raw pool allocation and actual returned capacity. Missing backing or services throw before their unsupported operation; they do not become source success. No default hardware/SavedAudio/SDL capability provider exists. A native platform may retain these managed CPU resource pools only if actual allocation/free ownership remains connected; it may not report immutable totals forever while consuming untracked voices or SGE ranges.

## Validation and remaining live seam

The canonical [fixtures](../analysis/multiplayer/integration/sound-capabilities-native/fixtures-v2/fixtures.tsv) contain10 actual original controls,20,537 SDK stores and450,648 complete output bytes. Fresh original reexecution produces byte-identical exported fixtures; complete reports differ only in explicitly excluded real native mapping-address observations. All27 old/u5 body listings repeat exactly. Independent copied-source normal and actual GCC ASan/UBSan runs pass twice with real OS allocation and real recursive synchronization. Exact commands, compiler output and observed dependency snapshots are under [validation-final](../analysis/multiplayer/integration/sound-capabilities-native/validation-final/commands.json) and [validation-final-repeat](../analysis/multiplayer/integration/sound-capabilities-native/validation-final-repeat/commands.json). Earlier failed development checks are retained, including a missing storage binding and an observer-PC correction; final validation uses the current complete inputs and corrected exact instruction identity.

No shared CMake, OriginalData, runtime, launcher, shader or prior frozen source is edited. This is an isolated source/native CPU checkpoint, not a fresh complete-tree build, a working audio startup, voice admission or live reload-sound claim. The earliest original device-startup gap is still genuine saved-audio policy before successful3832AA and hardware38B1EB/interrupt/MMIO initialization. The native SDL backend supplies real queue/device operations but does not own those Xbox policy/hardware semantics. The prior composed FP/world/raw-submit checkpoint also retains its explicit local/world/BSP/listener producers and naturally cold submit return. This capability module must not be used to turn either boundary into success.

Reproduce with [reproduction.json](../analysis/multiplayer/integration/sound-capabilities-native/reproduction.json); [final-manifest.json](../analysis/multiplayer/integration/sound-capabilities-native/final-manifest.json) pins the exact source, original inputs, repeats, native binaries and prior protected evidence.
