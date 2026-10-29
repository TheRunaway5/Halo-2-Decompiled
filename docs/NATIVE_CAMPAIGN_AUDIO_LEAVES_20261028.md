# Native campaign audio: class fades and voice routing

The complete TU1.5 `228310` sound-class fade update and `225D70` voice routing
mask builder now have semantic C++ kernels in
[`native_tu15_sound_kernels_campaign_channels.cpp`](../src/audio/native_tu15_sound_kernels_campaign_channels.cpp).
Their typed interfaces are
[`native_campaign_audio_leaves.hpp`](../include/halo2/native_campaign_audio_leaves.hpp).
Neither algorithm uses a virtual register file, an instruction dispatcher,
an original executable, or a lifted fallback. Compatibility adapters transport
the actual retained bank and scalar arguments.

`228310` updates all 54 original 16-byte sound-class records. It preserves the
nonpositive/unordered elapsed-time gate, remaining-time completion test,
single-precision interpolation order, NaN operand order and ordered gain/time
publication. Target and fourth-word flags remain unchanged. Reads stay lazy
instead of snapshotting the entire bank before a publication.

`225D70` clears the 24 output words, initializes voice availability, builds the
signed three-bit category masks, clears claimed signed source-voice bits and
copies all six signed routing selectors in the original paired order. It
reloads the voice count and applicable flags at their actual phases.

The ABI adapters preserve callee-saved GPRs, the elapsed-time SIMD input and
other preserved SIMD values, return-stack ownership and the complete admitted
FP environment. Arbitrary scratch EAX/ECX/EDX, XMM0/2/3/4, EFLAGS and dead-stack
bytes are not reconstructed. The original compiled-caller direction flag is
clear; unmasked FP traps remain outside this backend's admitted contract.

## Independent reference proof

The reference image is the owned English Xbox TU1.5 executable with SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
[`validate_native_campaign_audio_leaves_tu15.py`](../tools/validate_native_campaign_audio_leaves_tu15.py)
executes its unchanged i386 instructions on the host CPU under the offline
[`observer`](../tools/observe_native_campaign_audio_reference_tu15.cpp).
There are no instruction patches, dependency return values or resumed unknown
owners. The original bodies have zero external calls.

The corpus contains 262 complete original calls: 192 fade calls across every
SSE rounding/DAZ/FTZ combination and 70 routing calls. Inputs include signed
zeros, denormals, infinities, signaling/quiet and distinct-payload NaNs,
nonpositive/unordered elapsed times, completion/interpolation, zero/negative
voice counts, capacity word boundaries, signed categories/source indices and
all six selectors. The 34 fade instruction sites and 78 routing sites are all
executed. Native output matches 14,632 ordered persistent writes, complete
retained banks and original FP control/status/MXCSR after 115,693 original
instructions. Host FP state is restored after each native call.

Two private source mutations are independently rejected by those original
results: zeroing the interpolated contribution and dropping the category bit.
The mutations never modify the production source or reference instructions.

The final GCC proof, rebuilt after semantic eligibility metadata was added, is
[`gcc-after-semantic-gate-v5/report.json`](../analysis/campaign-native-audio-leaves-20261028/gcc-after-semantic-gate-v5/report.json).
The independent Clang ASan/UBSan build passes the same native and mutation
checks in
[`clang-sanitizers-v1/report.json`](../analysis/campaign-native-audio-leaves-20261028/clang-sanitizers-v1/report.json).
The official `run_test.py`/`run_tests.py` owners remove the ELF references,
native/mutation executables, raw 44 MB corpus, source mutations and private
build trees. Only compact reports and bounded logs are retained.

## Integration boundary

The channels candidate table contains these two proved leaves. The campaign
audio group's existing all-entries gate remains closed: 18 of its 40 candidate
routines now have implementations; 22 remain absent. The prior 16 candidates
have their own contracts and evidence and are not newly qualified here.
The subsequent [clock/ducking increment](NATIVE_CAMPAIGN_DUCKING_20261028.md)
adds the separate `129500` candidate, bringing that inventory to 19 of 40 and
21 absent; it preserves this increment's algorithms and retained proofs.
Its later semantic eligibility change adds only two table metadata fields to
this module. The
[metadata receipt](../analysis/campaign-native-ducking-20261028/gate-metadata-proof.json)
proves the complete algorithm/adapter prefix is unchanged and reproduces this
increment's exact source hash when those fields are removed. Only the three
new typed leaves are semantically qualified; the older 16 candidate bodies
remain ineligible for the complete native group.
No launcher/runtime selection changes or live audible campaign acceptance
follow from this component proof. An eventual individual sound-registry
binding must use the actual caller-preservation contract, not compare arbitrary
scratch register contents.

`cmake/NativeCampaignAudio.cmake` registers the physical-reference regression.
The standalone validation command is:

```sh
python3 tools/run_test.py -- python3 tools/validate_native_campaign_audio_leaves_tu15.py \
  --output analysis/campaign-native-audio-leaves-20261028/recheck
```
