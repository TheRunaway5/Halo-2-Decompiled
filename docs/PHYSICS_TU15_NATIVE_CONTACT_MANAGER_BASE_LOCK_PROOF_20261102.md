# Native TU1.5 manager cleanup with an absent registry key

The current native base-lock helper is reused without a second implementation.
`include/halo2/native_tu15_base_lock.hpp` SHA256 is
`6e096e533898e433e4f1594b5cb57d26775b715a808f7124028014ff5aa88c4f`.
It exactly matches the GCC and Clang physical component proof at
`build/test-results/vehicle-destruction-pending-live-20261101/base-lock-component-v2/report.json`:
25 cases,13 absent-key completions,12 registered-key refusals,168 ordered reads,
13 stores and four rejected mutants. The separately named
`base-lock-registered/report.json` registers/runs that CTest; its name does not
prove successful registered-entry storage destruction.

The native helper implements flags0 of2E12A0 through current2E11E0 registry
lookup: global48C93C+8, current mask and keys, four-byte linear probing from
`u32((object>>4)*9E3779B1)&mask`, and the source signed limit reread. A missing
key completes with the byte AND7F at2E12C2. A registered key refuses before the
unqualified2E32E0/storage/string children and before a native write. Constructed
power-of-two tables have a bounded source-derived extent; physical declared
profiles cover masks0,1,3,15,31 and the prior genuine input has mask15.
`analysis/physics/native-contact-base-lock-reuse-audit-20261101/report.json`
pins the unchanged helper/driver/validator and all four authenticated source
listings. It predates the new actual input and does not itself claim that input.

## Genuine locked-manager input

The protected v1470 iteration08 typed manager BEFORE input is
`analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-merge-point-original-v1470-20261101/iteration-08-native-manager-locked-actual-before-visible-retry-vehicle-camera/required-native-manager-cleanup-before/actual-native-before.tar.zst`.
SHA256 is `c3556251eb879aa2d28621d6ffcfd244f4fa3ac63654d1ef924e312d10f742d1`.
The13,550,556-byte archive preserves4,408 banks/114,429,109 input bytes plus its
typed manifest. No GP image, expected after-DATA or callback result is supplied.

Actual manager1007BF40 has collector1007BF50 and flagsBCDC. The current registry
is10065158 with keys10065170, mask15 and hash slot5 containing zero. Registry
absence is read from the genuine BEFORE bytes; it is not inferred from the
flags. NativeB4150 first clears the source lock bit in the collector flags,
then the native2E12A0 helper independently performs the current registry lookup
and source byte write. The original and native2E12C2 write preimages are both3C.

`analysis/physics/native-contact-manager-base-lock-actual1470-tu15-20261102-first/report.json`
qualifies the full native3232B0→3231B0→31F660/325AF0/B4150 path, current body
listener/refcount cleanup, native missing-key base lock and native149E00/pool
release under GCC and Clang. Each of the actual disposing1 control and declared
disposing0/3 controls compares every captured bank, FP, return identity and the
complete ordered persistent operand/store ledger. There are178 ordered reads,
77 stores and10 native allocator entries over the three cases. Source3232B0
covers all15 sites andB4150 all14. Set-flag and registered-key branches remain
outside this full-return domain. Mutants that corrupt the base byte or force
a registered-key refusal are rejected. No original CPU child service is used.

The shared manager integration can therefore admit a set lock bit only when
its current owning registry lookup is absent:
`!(flags&8000) || !native_tu15_base_lock_registered(memory,collector)`.
The actual release rechecks the registry rather than consuming a precomputed
result. Existing manager array/current-world/body constraints remain separate.
This proof does not admit registered keys, positive+A4 cleanup, body-zero
destructors or exception-unwind branches that lack their own qualification.

All original ELF, bank and compiler copies belonged to one `run_test.py` owner;
967,356,416 measured allocated bytes were removed and its owner path is absent.
The protected reference archive remains untouched. Visible multiplayer and its
vehicle-camera gate belong to the following coherent runtime cohort.
