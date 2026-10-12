# Whole sound creation with concurrent cache completion

The complete retained `source_sound_create` owner now composes with the complete cache request, queue constructor, publication, worker and file callback under the frozen `NativeExecutionGate`. Three unchanged original cases pass normal, ASan/UBSan and ThreadSanitizer comparisons. This is a named native serialization policy for these cooperating CPU owners, not recovered Xbox atomic behavior or original thread scheduling.

The new implementation is the focused composition test in `tests/sound_create_cache_gate_tests.cpp`, plus its localized CMake registration. It does not change any frozen Source CPU module, runtime, renderer, Unit state or release. Evidence and reproducible commands are under `analysis/multiplayer/integration/spawn-source/sound-create-cache-gate`.

## Original inputs and owning order

The authority is the independently repeated TU1.1 whole-create/cache freeze, `analysis/multiplayer/integration/sound-create-cache-production-20261009/frozen/manifest.json` (`d173b52f6e78c5ef2b7f83ace948f6e8c2134b94305af1373dcae4367a99de31`). Its literal fixture (`ccd0d64396100d4a88c521c306a2afc70e238bf81ca0fb05320ac94dd7e1f77f`) contains the actual retained 127750 caller inputs, full 160-byte request, source-created storage, queue-constructor before bank, and six full before/after region sets for ready, reload and empty-reload. The new test restores those captured banks; it does not reexecute whole title startup or manufacture source readiness. Complete source instructions and the prior authenticated image/body correspondence remain in that immutable dependency. No new u5 execution or wider parity is asserted here.

The queue constructor executes its complete CPU body with genuine native mutex/semaphore creation and a retained real `std::jthread` worker. The actual native worker alias is stored by the source constructor. Unsupported original priority/stack/TLS translation stops explicitly after that store; the independent worker remains host-owned. Source resource handles, successful platform status and a source shutdown path are not supplied.

One full-entry gate lease then covers complete SourceCreate, its direct complete cache callback, queue publication, and the stores after publication through the original return. Nested entries are rejected. Inside the same lease the test verifies the actual returned source datum `EF730000`, all ordered stores and every captured region, including the pending source `+4` bit2 and cache pin. It also verifies that no worker store or file read has happened before this return. The effect-identifier and controller-enable services remain absent; a reached unavailable branch fails rather than returning a successful default. The literal floating-point caller context `027F/1F80` is the explicit same ABI input as the frozen native whole-create comparison, not a newly recovered startup producer.

The caller next invokes complete cache flags0 and observes pending1. Complete flags1 performs its own done-byte loop and genuinely yields the native gate. The actual worker acquires the semaphore and work mutex, captures the queued node, releases the mutex, invokes the complete original file callback with an actual retained `shared.map` stream, and performs the source done/unlink/free stores. It retains the gate through each CPU segment; only actual wait/yield adapters suspend it. All raw-memory callbacks, observers and file-byte writes require the same active lease. Private spans are consumed immediately and never exported or retained across a suspended provider.

Flags1 observes completion2 while the worker is still alive, before stop or join. This is an explicit native caller control of the already-verified complete source cache body. Its completion stores and full regions match the frozen original completed flags0 stage; no original flags1 caller schedule is claimed. Subsequent flags4 and flags5 match return6 and the exact pin stores. Native stop cancels the next unavailable-permit wait before any source post-wait write; join occurs outside every entry. Full region and store-count comparisons remain unchanged after join. Gate failures revoke the active entry until unwind, as tested in the separate frozen gate suite.

## Exact comparisons and file outcomes

The original serial reference has 8,678 ordered stores. This derivative matches them all in their respective complete phases and observes six additional explained stores: one genuine constructor worker-alias store and one native pending flags0 touch per case. The native counts are 2,896 / 2,894 / 2,894, total 8,684. The only expected region substitution is the four-byte real worker alias at the original worker slot. Nothing else is rewritten to satisfy a comparison.

Each case compares 3,654,420 bytes in six whole-region comparisons, totaling 65,779,560 compared bytes. These include the actual read destination and source/cache/pool/queue state. Each case performs one real file read; actual authored ranges are:

| Case | Actual shared.map offset | Bytes |
| --- | ---: | ---: |
| ready | 79,925,760 | 53,760 |
| reload | 80,033,280 | 32,768 |
| empty_reload | 80,033,280 | 32,768 |

The actual `shared.map` SHA256 is `084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af`. Full destination bytes agree with the original serial capture. The file callback still owns terminal-result versus full-transfer semantics: this positive composition does not alter the previously frozen failed-seek/short-read result1 behavior. File tokens/path bindings are explicit native owned-file inputs; original file-open/token lifetime remains unported.

## Validation and usable seam

The isolated build compiles the exact copied new test and frozen gate, SourceCreate/cache/async/file/storage/synchronization/thread sources. It links no mutable Cache/Systems archive. Normal, GCC ASan/UBSan, GCC ThreadSanitizer and an independent normal repeat compare identical conclusions and full source buffers. Sanitizers are separate builds. A real host observation deadline only throws on a failed experiment; it supplies no source clock, wait outcome, read result or completion. There were no GPU jobs.

The usable integration rule is one retained gate shared by every cooperating Source CPU entry and all backing-memory/file-byte access, with actual native waits/yields suspending and reacquiring that lease. Cache callbacks inside SourceCreate stay within the caller's existing entry. An adapter must not expose raw spans, enter recursively, join while entered, continue a revoked entry, or mix unguarded consumers with this owner. The test proves concurrent done polling under that policy, not just quiescent post-join reading.

Original audio admission, local first-person routing, source OS file/thread/TLS/priority ownership, voice/playback, resource-active/current-transform consumers and the live Unit/Q constructor bridge remain separate boundaries. No source-created sound here is claimed audible or admitted by the whole engine.
