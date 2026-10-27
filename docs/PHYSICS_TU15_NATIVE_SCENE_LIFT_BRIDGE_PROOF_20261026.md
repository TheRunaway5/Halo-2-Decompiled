# TU1.5 scene-to-native-lift integration proof

The scene-device module contained a second static `1DA750` force body. Its
internal `117A8C` caller bypassed the dedicated force module and its four native
scalar call sites. The scene module now delegates that force body to
`source_run_physics_phantom_force_tu15_runtime`. It retains one internal phantom
observation lifetime and transfers the actual ABI, floating-point state and
fault prefix through `save`/`load`. The generator emits the same delegation.

The dedicated force parent still contains legacy outer control flow. This
change establishes that the scene owner reaches the qualified native scalar,
power and CRT kernels; it does not establish native closure for the entire
physics engine.

## Independent original-code check

The disposable proof compiled the actual changed scene translation unit with
the dedicated force and native scalar sources. The existing parent test was
adapted privately to call the scene entry `1DA750`, recover its returned FP state
from the ABI, and check exactly one internal observation on entry and return.
The arithmetic expectations and comparison rules were unchanged.

- Original authority: English TU1.5 XBE SHA-256
  `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
- 576 unchanged original parent executions, 1,258,640 physically stepped i386
  instructions; 288 native calls at each of the four scalar sites.
- 249,480 semantic stores and 540,000 retained bytes compared, including caller
  arguments, ratio and errno. Caller stack protocol, callee-saved registers,
  defined flags, full FP status/control, FS and live x87 values passed.
- The internal observation fired once on entry and once on return in every case.
- A private negative control replacing the real native scalar result with zero
  was rejected at parent 0 with a semantic store-count difference.
- All seven borrowed dependency archives remained hash-identical after both
  runs. No frozen source or archive was edited by this proof.

This keeps the existing parent-proof boundary: 7,392 bytes of dead private
numerical-helper final stack and 16,416 dead helper last-writer bytes at later
stack reuse are outside its semantic comparison. No complete volatile-register
or private machine-postimage claim is made. Original Xbox/Pentium III
transcendental approximation bits and live multiplayer acceptance remain
separate gates. The unchanged whole-frame raw-bank gate is not relaxed here.

## Evidence and storage

Bounded results, source hashes, exact validator/test adaptation diffs, commands
and the expected negative-control failure are in
`analysis/physics/native-scene-lift-parent-tu15-20261026/`.
The scene source SHA-256 is
`0da01aae9f41f9898448deaebd965f0fdaa9f52d0e10b83c02466afc2da91ac6`.

`tools/run_test.py` owned the source, original fixture and raw oracle outputs;
`tools/run_tests.py` owned both compiler builds. Both builds and the enclosing
owner were verified removed, and the one-off launcher was retired. During the
run, 147,435,520 allocated bytes of private temporary files were observed and
subsequently removed; this is an observed lower bound, not a final net filesystem
space measurement. No large artifact is retained by this proof. The cleanup
receipt records the retained small-report size.
