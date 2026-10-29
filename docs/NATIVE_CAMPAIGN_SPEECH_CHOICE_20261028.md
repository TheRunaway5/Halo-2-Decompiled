# Native weighted speech choice, TU1.5

`native_campaign_ai_speech_choice_tu15` implements the actual call-free
`217370` weighted chooser with typed candidate/RNG inputs and real SSE/x87
arithmetic. Candidate records are twenty bytes; only the final weight DWORD
is read/modified, and the sixteen payload bytes stay opaque and unchanged.
There is no virtual register file, software x87 operand stack, instruction
dispatcher, runtime binary, guessed score or supplied random result.

Count is the original signed WORD. Count1 returns index0 without reading any
candidate, RNG or literal, changing FP state, or requiring a temporary; its
null-data/full-x87 case is proved. Other counts find the maximum, retain
weights at least `maximum * 0.8`, zero discarded weights, and sum accepted
weights in source operand/order. Even nonpositive counts advance the real
RNG state. The owner is read lazily from `4F3CD8` after weight publications;
its state advances by `seed * 1664525 + 1013904223` modulo32bits.

The draw uses the new unsigned high16, actual normalization literal
`46A160=37800080` (float approximation of1/65535), and the actual single-
precision total with one real extended temporary and the original final
single store. It is not replaced by1/65536 or a host random generator.
Cumulative selection retains the inclusive boundary and real COMISS NaN
behavior. Raw-address binding preserves a seed aliasing a weight. Missing
RNG backing is required input; the kernel never creates or seeds it.

The public API returns selected index and actual cumulative/target/last-term
numeric outcomes for thin caller-live SIMD result transport. FP controls and
flags use the existing proved native hardware environment, preserving caller
hardware state and live operands. Except count1, the caller must own one
available extended temporary and masked FP exceptions. Those bounds are
explicit rather than substituting overflow/trap results.

The typed span overload shares the same algorithm. Its219 disjoint native-
local profiles compare score publications at every engine-memory read, final
candidate bytes, actual RNG publications and FP/SIMD outcomes against the
physical original. `native-candidate-caller-profile.json` authenticates
`213D30`'s signed SI count, twenty-byte stride,100-record buffer, and actual
`213DE8` call with EDI=`chooserSP+1C`. The corresponding private-layout
physical case is included. The complete collector/`213D30` entry is not
claimed executed; no source stack is reconstructed by the native caller API.

Independent offline observation executes unchanged physical-i386 TU1.5 text,
image SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
GCC and Clang ASan/UBSan pass221 calls,21,806 instructions,2,446 ordered reads
and323 stores. All76 reachable original sites are authenticated and reached.
The remaining FADD2^32 site is mathematically unreachable: logical high16
extraction always yields0..65535 before signed conversion. It is a domain
proof, not an unimplemented reachable policy branch.

Checks cover retained bytes and payload preservation, read/store interleaving,
individual stores/sites, index, all eight original SIMD outcomes, FP controls
and sticky flags, occupied80-bit x87 operands, host FP preservation, precision
and rounding modes, DAZ/FTZ, NaNs/infinities/subnormals, signed zero, overflow,
RNG extremes and aliases. No original instruction/state edit or dependency
return is supplied. Six semantic mutants are rejected: excluding equal
threshold weights, using1/65536, excluding cumulative equality, changing the
LCG increment, advancing RNG for count1, and moving owner capture before
weight publication. Unmapped faults, unavailable extended scratch and
unmasked FP traps are outside the stated caller profile.

Reports are in `analysis/campaign-native-speech-choice-20261028/gcc-v1`,
`clang-sanitizers-v1`, and `module-integration-v1`. The real module
`cmake/CampaignNativeAiSpeechChoice.cmake` compiles production and test sources,
checks the provided native executable, returns missing-reference skip77 with
false/not-run evidence, and rejects a wrong pin. Official wrappers remove
reference ELF, banks, private mutants and builds; cleanup preserves hashes
and small reports only. Prior cancellation/audio modules remain unchanged.

Actual candidate collection `214EA0`, request publication `215390`, the
constant34 `203AA0 -> 213D30` composition and full `1BD8F0` cancellation remain
open. Existing RNG startup backing is a real required data owner; its ABI
clock transport is not claimed here as proof of the entire seed initializer.
No broader campaign or audio readiness gate is enabled by this chooser.
