# Original CPU upload through decoder, relocation and request publication

The later real-GP consumption and complete CPU return comparison is documented
in `tests/fixtures/sound_gp_upload_return_tu15.md`. The evidence below retains
the original first-poll boundary and supplies no acknowledgement.

The DSP module now owns17 whole bodies/690 instructions/1860 original bytes.
The four new entries are388275 (object initialization),388294 (bootstrap and
scratch construction),38850A (descriptor append wrapper) and388576 (resource
preparation/upload caller). Their114 instructions/321 bytes reproduce from the
owned TU1.5 executable; all576 previous instruction lines remain unchanged.

`native_tu15_sound_gp_upload_tests` loads the authenticated original bank and
DSPImage section, retains a real PipeWire output and composes the actual SDK
pool/page owners. Source387A32 allocates a caller-provided3C-byte root object;
the original constructors allocate its bootstrap and scratch objects. The root
extent, two GP descriptor requirements and four upstream page descriptors are
declared native caller/resource inputs. The complete upstream APU factory is
not established by this experiment.

Original388294 copies5CC (1484) bytes from390C18. Its size includes eight bytes
beyond the1476-byte control-program extent previously compared as DSP code.
Original388576 clears/prepares the header and copies the raw encrypted section;
388309 allocates the actual returned descriptor table and data pool, decodes
the seed/effect payloads, relocates descriptors and writes request3 itself.
Execution stops before388470's first poll. No acknowledgement, ready flag,
kernel result, selected gain or clock rebase is supplied.

The independent comparison starts at original388383 after the native resource
calls. The native prefix exports its actual caller frame and retained pages as
declared inputs to unchanged Xbox x86 instructions. These are native context
inputs, not captured original RAM or kernel-result fixtures. The oracle verifies
every executed opcode against the owned executable, executes the decoder and
relocation suffix, and stops before the poll without any external service.
Native execution starts afresh from the original bank/resources and compares
the input ranges, all output ranges, GP/XMM, defined flags and ordered stores.
The exported pages are not runtime inputs to the native engine.

Reproduce each stage serially with fresh directories:

```sh
python3 tools/tag_match_memory_budget.py --output-root NEW_PREFIX_GUARD --timeout 60 -- \
  ABSOLUTE_BUILD/native_tu15_sound_gp_upload_tests OWNED_XBE NEW_NATIVE_BEFORE

python3 tools/tag_match_memory_budget.py --output-root NEW_ORIGINAL_GUARD --timeout 90 -- \
  /tmp/halo2-fov-reference-env/bin/python tools/oracle_sound_gp_upload_suffix_tu15.py \
  --native-before NEW_NATIVE_BEFORE --output NEW_ORIGINAL_SUFFIX

python3 tools/tag_match_memory_budget.py --output-root NEW_NATIVE_GUARD --timeout 60 -- \
  ABSOLUTE_BUILD/native_tu15_sound_gp_upload_tests OWNED_XBE NEW_NATIVE_RESULT \
  NEW_ORIGINAL_SUFFIX
```

The native constructor prefix passes v463. The original suffix v464 executes189
instruction addresses and187654 ordered stores. Clang v465 and frozen GCC v470
match its complete retained ranges, GP/XMM, defined flags and store digest. The
composed native path executes663 original instruction addresses and retains
request3, its real allocated descriptor table and201 native SGEs. The native
reader also verifies original-before ranges on reaching the suffix, so changes
to construction cannot hide behind an eventual decoder match.

Private controls fail for5C4 instead of5CC bootstrap bytes (v471),8000 instead
ofC000 descriptor relocation (v472), and an omitted38846E request store (v473).
The actual earlier module fails on the missing initializer (v460). The new
ordinary CPU oracle also adds five cases for these callers:67 total cases,
including constructor/append branches and first real pool-call boundaries.
Fresh source regeneration v467 reproduces all eleven tracked modules:
184 complete bodies/17264 instructions/62633 original bytes.

Copy the complete fresh original suffix to the previously absent ignored
`tests/fixtures/sound_gp_upload_suffix_tu15`. Copy the fresh extended ordinary
suite to `tests/fixtures/sound_gp_upload_callers_tu15`. The registered
`original_native_tu15_sound_gp_upload_suffix` uses a fresh UUID result directory
on every run and skips when owned original data/reference is absent. Run it and
`original_sound_gp_upload_callers_tu15` through the same60/64MiB/zero-swap guard.
All compilation stays serial within1GiB/zero swap.

Final current-root Clang/CMake v474 and all seven registered tests v475 pass,
including the67-case reader's134 comparisons/196479 ordered stores and the
native upload's187654-store suffix. Frozen GCC v469/v470 independently passes
the native resource/CPU composition. The wrapper preserves a separate UUID
result directory on each run.

Initial v455 declined launch while another verified validation process ran.
v457 was deliberately stopped after the settings-library dependency pulled an
unrelated full-world build; the standalone probe now compiles the settings
implementation it actually needs. v458 was a fixture string-comparison compile
error, and v468 was an analysis build-setup syntax error. Those artifacts remain
preserved and are not positive validation.

The native GP instruction core still needs a native MCPX peripheral/transfer
owner, device/IRQ/timer and GP/EP/output clocks. The ordinary live world still
stops before its unowned38846E publication. This checkpoint closes CPU caller
composition through the first poll; acknowledgement/return, the complete parent
factory, final routing/mix/sample timing, audiovisual parity, installation and
the requested finished video remain open.
