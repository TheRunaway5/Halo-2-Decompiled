# Native dialogue byte-envelope sampling

Whole Xbox TU1.5 `21F860` now uses a native semantic algorithm in
`src/audio/native_campaign_speech_envelope_tu15.cpp`, with its thin ABI adapter
and an actual entry in the unit-speech sound-manager registry. The kernel
resolves the actual tag, pitch range, permutation resource, signed curve index
and sample bank. It rejects unmatched permutations, clamps the sample ordinal
as the original does and normalizes the selected byte under the original
SIMD policy. Its sole direct original caller is `18F5C6`; `18F510` sends the
result to `18F7A0` before the facial-curve path.

The core has no register array, virtual CPU, virtual x87 stack, guest-private
frame, supplied resource readiness or playback result. The ABI layer reads
the real sample argument first, then transports the scalar and late RET4.
Valid samples retain incoming upper96 SIMD bits; refusal clears all lanes.
A valid zero sample therefore differs from an absent envelope. A rejected
normalization-operand read retains its real converted byte sample.

[Independent proof](../analysis/campaign-native-audio-20261030/speech-envelope-proof.json)
covers454 unchanged physical original cases, all70 instruction sites and7,729
ordered reads. There are388 completed calls,63 rejected read prefixes, three
genuine unmapped faults, two pending byte samples and four late return cuts.
The corpus includes every byte value, signed metadata/permutation/sample
bounds, mutable input timelines,64 active-MMX profiles and86 profiles with all
eight older x87 slots occupied. Live values, complete scalar lanes, admitted
FP controls/status, unchanged storage and actual stack cleanup match.

GCC and Clang ASan/UBSan pass. Clang and all11 semantic mutations pass after
correcting a type error in one private mutation source; that compile failure
is retained as diagnostic evidence and is not counted as a semantic rejection.
Production and test pins are unchanged across the compiler runs. An independent
physical original repeat produces identical fixture bytes. Tests enter through
the actual sound manager and unit-speech registry. Unrelated group inventories
are omitted from that focused test, and every requested algorithm dependency
would reject rather than supply a result.

The original is SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The required original input is
`tests/fixtures/native_campaign_speech_envelope_tu15.json.gz` (568,778 bytes).
All owned original execution, source, compiler and mutation trees were verified
removed. No executable or raw bank is retained.

This closes one dialogue dependency. The old unit-speech generated body has
not yet been retired. Resource admission, smoothing, full facial curves,
dialogue queues/playback, complete cutscene/SFX audio, DSP/output, native-only
mission/co-op and launcher acceptance remain open. The forty-entry campaign
audio group still has only four semantically qualified required owners.
