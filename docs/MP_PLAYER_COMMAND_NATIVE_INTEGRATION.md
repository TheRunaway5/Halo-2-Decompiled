# Native neutral command and cached delivery

The native neutral initializer and cached-command delivery match both owning
original cases, comparing complete `5C` command/delivery buffers and `88`-byte
simulation records. The omitted configuration producer rejects the age-zero
command and leaves delivery untouched; the actual source configuration handler
admits one literal codec command. Normal and address/undefined/leak sanitizer
comparisons pass. Authority is the independently repeated
[original command composition](MP_COMMAND_INPUT_RESEARCH.md).

`14E780` writes the original zero and NONE fields. `14E7E0` preserves the
original exponent checks, ordered scalar comparisons, signed selection gates,
reference branch and subrecord type check. Only its successful neutral-codec
path executes in the comparison; alternative validator branches retain static
source coverage. No native-generated cases or expected buffers are substituted.

The cached `85460` branch consumes actual record kind0 and literal source clock
and expiration bytes. It preserves modulo32 tick subtraction, signed
`CVTSI2SS`, separate `MULSS`, strict expiration comparison and defined AL.
Source rejection leaves every output byte unchanged. Both original records
remain byte-identical. Watcher-refresh kinds4/5 fail explicitly because their
owner/network producers are not implemented here.

This module does not implement `852A0`'s submission codec, `69350`'s packet
assembler, `151C90`'s control producer, local device/profile/view ownership,
map readiness/state4 scheduling or full player/object motion integration. The
original trace proves their bounded source composition; these three native
kernels cover only the stated initializer, validator and cached delivery.
The rebuilt runtime in captures57/58 contains this module and passes title,
four-view local and three-participant loopback LAN smoke checks, with stable
binaries/shaders and zero Vulkan validation errors. The complete source device
and frame route remains open; captures55/56 are historical.

## Reproduction

```sh
cmake -S . -B build-multiplayer
cmake --build build-multiplayer --target player_command_tests -j4
python3 tools/differential_player_command.py
ctest --test-dir build-multiplayer --output-on-failure -R '^retail_player_command$'

ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/differential_player_command.py \
  --binary build-multiplayer-asan/player_command_tests \
  --output analysis/multiplayer/integration/player-command-asan-differential.json
```

The exporter pins the source oracle, document, report and independent
verification, checks all42 prior dependencies and all298 instruction files,
and requires literal original report/listing repeats. Clock/configuration and
record/delivery inputs come from each actual original call. The fixture remains
in ignored local analysis storage; its CTest check skips77 when absent.

Broader-suite, rendered sanitizer, ordinary live device/frame and full
multiplayer parity are not established by these two-case checks.
