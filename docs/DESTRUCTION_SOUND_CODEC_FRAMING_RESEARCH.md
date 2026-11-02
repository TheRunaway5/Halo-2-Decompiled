# MCC Classic destruction sound framing and PCM ownership

Research date: 2026-11-01. Scope: static inspection of the installed MCC Halo 2
DLL and native decoder, followed by reading the parent task's retained metadata
census and bounded original-instruction observation. This research agent did
not execute a game, decoder, emulator, test, or probe and changed only this note.
The separately produced execution evidence is identified and bounded below.

## Finding

The owning MCC code establishes signed-length packet framing, a 240-sample
decode capacity, rounded **allocation** sizes, and separate logical PCM byte
lengths at playback. `769480` does only allocation arithmetic. The recovered
decode/submission path does **not** establish an additional constant Opus
pre-skip. It does establish a time/seek byte cursor applied after decoding, and
per-chunk end trimming through the length submitted to Miles. These are distinct
operations. [Installed MCC DLL, RVAs `769370`, `769480`, `6ACAC0`, `779210`,
`6E7C9C`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

The inspected **pre-fix** native decoder appended each chunk's rounded decoded
output and then retained the first `input.samples` frames globally. That differs
from individual logical chunk submissions when a non-final chunk has padding.
The parent task has now changed native decode to discard each chunk's allocation
padding before appending the next chunk. The measured destruction cohort has
**zero such non-final-padding cases**, so this general contract correction is
not an audible fix demonstrated on that cohort. The current source and retained
census are pinned below. [Current native trim](/home/eric/Developer/Halo-2-Decompiled/src/audio/mcc_audio.cpp:51);
[Cohort shape report](/home/eric/Developer/Halo-2-Decompiled/build/test-results/vehicle-destruction-waveforms-20261101/chunk-shapes.json);
[MCC DLL, RVA `77924B`–`77928E`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

## Evidence pins and interpretation rules

The primary binary is
`/home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll`,
SHA-256
`de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946`,
preferred image base `0x180000000`. Its actual bytes were checked in this
research. The existing original import provenance names the same file and hash.
All code addresses below are **RVAs**, not absolute virtual addresses or file
offsets. Function names are descriptive interpretations, not recovered symbols.
[Existing sound-bank provenance](/home/eric/Developer/Halo-2-Decompiled/analysis/asset-setup/mcc-native-20261013/continuation/sound-bank-source/source.json:1)

Inspection used the repository's read-only PE/Capstone helper, including PE
exception-directory bounds, direct-call verification, and the delay-import
table. Some exception entries describe compiler fragments rather than whole
source functions; `769480` and `7792E0` are leaf code without an exception-table
owner. The following hashes cover the exact half-open RVA intervals listed,
including only bytes in those intervals.
[Inspection helper](/home/eric/Developer/Halo-2-Decompiled/tools/pc_postprocess_inspect.py:1)

| DLL byte interval | SHA-256 | Owning evidence |
| --- | --- | --- |
| `769370..769471` | `428198f407af9239d4fa6e9189567f34615c4f2d2c963e2512494b387a8c498b` | Framed packet decode |
| `769480..769496` | `9e6827fbb73f2f4a57f2343c0fa832f6d97ee8bc8b4c7de5ea71b84c8c67170c` | Allocation round-up |
| `6ACAC0..6ACC9B` | `59a8c8a0850ae1322619b58fa33c277bbfa95757c234a9c4b3c0495964b47904` | Decode/cache wrapper |
| `779210..7792C7` | `ef040baf0d59be33b3f872dc314df5a16a48fe6182359e245aaea085b855e99c` | PCM pointer/length submission |
| `7792E0..7792EB` | `c46cc965efa3c08d71f2be6d73b8336745579a7725fb83b7672d62c8777c8fbd` | Byte cursor setter |
| `76A110..76A15B` | `2c8d3276270447272ca1b4658551c1c50172f7284bbd221284c9fd06e12d7c47` | Sum of prior chunks' logical bytes |
| `76A160..76A208` | `8bcc2161ddf6c4d7dcca89f4047c851799db1dff2dbe0d6993f0a94b5e8f713c` | Time-to-byte conversion |
| `6E7BF7..6E7D65` | `49f9de38b6ff35351cfddcdbdbf2bfc3bc63b5d85241dbd593ca06ed854b4d1d` | Resume/seek submission fragment |
| `6E3A20..6E3AD9` | `d5aae2db845690a501b151e46a75ecb70da9296722da9193b707688c08282290` | Initial stream time zeroing |

**Pre-fix inspected source:** repository HEAD
`7a9538c94f828aa164d92e3cc750c6d094cb7df0`, with the working files pinned
independently because concurrent work may differ from HEAD:

| File | SHA-256 at inspection |
| --- | --- |
| `src/audio/mcc_audio.cpp` | `53c186a6b803119ed45711638a2fb34ae1f84d100afaee40bb55abf0e0b3c161` |
| `src/assets/mcc_cache.cpp` | `fb564ecb0382365d17a3fd357c72f7610c1a187fa3cc6bee1c5bd05701011d34` |
| `tests/mcc_audio_tests.cpp` | `c5e1475abecbac33af499a3a745574b53189e8a36b59243eaacb53a5a4f55ace` |
| `tools/pc_postprocess_inspect.py` | `5b1836d6f63d2c42d84f2591ef0c751ffdfb65ac98c7f670eceeef33b6f8f68b` |

**Post-fix source inspected before finalizing this note:**

| File | SHA-256 |
| --- | --- |
| `src/audio/mcc_audio.cpp` | `0f20a049b0c6821415bda24c1d92d29d8a2f3a46db689cce85f56a308a57d2d8` |
| `tests/mcc_audio_tests.cpp` | `a2310db8fdfca8c73f4c986c094ae362be467c6f4be8f0b3bbe65e58691637ea` |

## Proven binary contract

### Allocation is not playback duration

`769480..769495` is the following complete leaf routine. For positive PCM byte
counts and channel counts, its signed integer arithmetic is
`ceil(pcm_bytes / (480 * channels)) * (480 * channels)`. With 16-bit PCM at
48 kHz, this is a 240-frame/5 ms allocation unit. It neither reads audio nor
adjusts a source pointer. [MCC DLL, RVA `769480`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

```text
769480  imul r8d, edx, 0x1e0
769487  lea  eax, [rcx-1]
76948A  add  eax, r8d
76948D  cdq
76948E  idiv r8d
769491  imul eax, r8d
769495  ret
```

Verified direct callers are `6AC3D1` and `6ACC24`. The first reads the chunk's
`+8` word, rounds only for codec 5, and passes the result to an allocation
routine at `6AC3E2`. The second rounds that same chunk word before passing it
as the decoder's PCM target. Neither call supplies a lookahead or beginning
sample count. [MCC DLL, RVAs `6AC3C6`–`6AC3E2`, `6ACC1B`–`6ACC66`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

### Packet framing and decoder state

`769370` takes a state holder, encoded pointer/extent, channel count, PCM
destination/capacity, and two output byte-count pointers. Its observed operations
are:

1. Read a little-endian 16-bit signed length and use its absolute magnitude for
   the payload length; advance the encoded cursor by that magnitude plus two.
2. Call `57EA40` with the state pointer, payload, payload length, current PCM
   destination, `frame_size=240`, and `decode_fec=0`.
3. Advance the PCM destination by `returned_frames * channels * 2` and accumulate
   that decoded byte count. Negative decode results fail.
4. Finish when the signed length is negative, the accumulated PCM count reaches
   the target, or the encoded cursor reaches the supplied extent. A negative
   length signals a framed decode stop; it is not itself the state-release flag.

The entire recovered routine has no initial-output discard, destination
rewind, pre-skip operand, or sample-shifting copy. `57EA40` calls an inner
floating-point decode routine and converts its returned samples to consecutive
16-bit output starting at the caller's destination; the inspected wrapper adds
no beginning discard either. This identifies raw Opus-style decode behavior;
the exact bundled libopus source revision has not been authenticated.
[MCC DLL, RVAs `7693C0`–`769456`, `57EA40`–`57EBE8`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

The DLL's decoder is less strict than the current native validator: a negative
packet can stop before the supplied encoded extent or rounded target, and the
routine reports consumed bytes rather than requiring exact equality to both.
The native implementation instead requires all chunk bytes consumed and exactly
the rounded decoded byte count. The binary alone does not prove that any owned
asset needs the looser behavior, or that unsafe malformed-input behavior should
be copied. [MCC DLL, RVAs `769420`–`769458`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>);
[Native checks](/home/eric/Developer/Halo-2-Decompiled/src/audio/mcc_audio.cpp:26)

`6ACAC0` obtains the chunk's encoded allocation, copies its encoded bytes to a
temporary buffer, and decodes PCM back into the allocation's beginning. After
decode it tests bit 29 of the chunk packed-size word and, when set, calls
`769280`. That routine destroys/removes the pooled decoder entry. The wrapper
returns the allocation's starting address, not an offset into it. The other
verified decoder call, `6AC6D5`, also decodes to the allocation start and makes
the corresponding conditional release at `6AC6E3`; that loading path supplies
a fixed `0x80000` PCM capacity. This establishes state-release placement, not
bit-exact equivalence between pooled destruction and native `OPUS_RESET_STATE`
for every possible sequence. [MCC DLL, RVAs `6ACBE2`–`6ACC89`,
`6AC640`–`6AC6E8`, `769280`–`769369`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

### The actual submitted range belongs to the chunk and the stream cursor

`779210` is the verified direct caller of `6ACAC0`. The delay-import entry at
RVA `E6EE20` names `mss64.dll!AIL_load_sample_buffer`. Immediately before calling
it at `77928E`, this wrapper takes the buffer pointer returned by `6ACAC0` and
loads **the unrounded chunk `+8` byte count** as its submission length. Thus
rounded decode/allocation padding is not automatically part of the submitted
audio. [MCC DLL, RVAs `779236`–`77928E` and delay-import table](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

The relevant Windows x64 ABI and recovered offsets are below. Structure labels
describe the observed use rather than an authenticated C++ type name.
[MCC DLL, RVAs `769370`–`7693F1`, `6ACAF6`–`6ACC89`, `779224`–`77928E`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

| Entry / object | Inputs or offsets |
| --- | --- |
| `769370` entry | `RCX=state-holder pointer`; `RDX=encoded start`; `R8D=encoded extent`; `R9D=channels`; entry `[RSP+28h]=PCM destination`; `[RSP+30h]=PCM byte target/capacity`; `[RSP+38h]=decoded-byte output pointer`; `[RSP+40h]=consumed-encoded-byte output pointer`. Stack offsets include the caller's return address and shadow space. |
| `6ACAC0` entry | `RCX=16-byte chunk pointer`; returns PCM allocation start in `RAX`. |
| Chunk row | `+0=resource file offset`; `+4=packed encoded size/flags` (`size & 1FFFFFFFh`, state-release bit `20000000h`); `+8=logical decoded PCM bytes`; `+Ch=mutable allocation handle`. |
| `779210` entry | `RCX=Miles wrapper object`; `RDX=chunk pointer`; `R8D=buffer-slot index`; `R9B=conditional-start control`. |
| Miles wrapper | `+18h=one-shot byte skip`; `+20h/+28h=two chunk pointers`; `+58h=Miles sample handle`. |
| `AIL_load_sample_buffer` call | `RCX=sample handle`; `EDX=FFFFFFFFh`; `R8=submitted PCM pointer`; `R9D=submitted PCM byte length`. |

For independent static review without executing the codec, the exact decisive
instructions are transcribed here. `RBX` is the wrapper object, `RDI` the chunk,
and `RAX` the decoded allocation pointer after `6ACAC0`. The byte interval is
part of the pinned `779210..7792C7` hash above. `779279..77928A` is the separate
null-chunk branch; the non-null cases jump directly to `77928E`.
[MCC DLL, RVA `779239`–`779294`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

```text
779239  e88238f3ff        call 6ACAC0
77923E  48897ceb20        mov  [rbx+rbp*8+20h], rdi
779243  baffffffff        mov  edx, FFFFFFFFh
779248  8b4b18            mov  ecx, [rbx+18h]
77924B  448b4f08          mov  r9d, [rdi+8]
77924F  85c9              test ecx, ecx
779251  741d              je   779270
779253  413bc9            cmp  ecx, r9d
779256  c7431800000000    mov  dword ptr [rbx+18h], 0
77925D  410f4fc9          cmovg ecx, r9d
779261  442bc9            sub  r9d, ecx
779264  4c63c1            movsxd r8, ecx
779267  488b4b58          mov  rcx, [rbx+58h]
77926B  4c03c0            add  r8, rax
77926E  eb1e              jmp  77928E
779270  488b4b58          mov  rcx, [rbx+58h]
779274  4c8bc0            mov  r8, rax
779277  eb15              jmp  77928E
77928E  ff158c5b6f00      call qword ptr [image_base+E6EE20h]
```

When the stream object's `+0x18` cursor is nonzero, it computes
`skip=min(cursor, chunk_pcm_bytes)`, sets that cursor to zero, advances the
submitted PCM pointer by `skip`, and reduces the submitted byte length by the
same amount. The object initializer `779000` and cleanup routine `77A160` zero
this cursor. Leaf setter `7792E0` stores `max(requested_bytes, 0)` into it.
[MCC DLL, RVAs `77924B`–`77926E`, `7790D5`, `77A263`, `7792E0`–`7792EA`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

The setter's verified direct-call chain is `6E7CF1 -> 6CC1D0 -> 7792E0`.
The caller reads a stream time value from its record `+0x0C`, converts it to a
byte position using `76A160`, subtracts `76A110`'s sum of earlier chunks'
**unrounded** `+8` PCM byte counts, and clamps the resulting within-chunk offset
to `[0,current_chunk_pcm_bytes]`. Codec 5 forces rate enum 3, whose table value
is 48000. The relevant table values at `B1BCB0`, `B1BCC0`, `B1BCD0`, `B1BCE8`,
and `B1BD00` reduce this conversion for mono/stereo Opus to frame-quantized
48 kHz PCM bytes. No fixed codec-delay term is added in this recovered chain.
[MCC DLL, RVAs `6E7C9C`–`6E7CF1`, `76A110`–`76A207`, `6CC1D0`–`6CC21A`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

The ordinary first-chunk attachment path `6E3A20` writes zero to the same
stream-time field at `6E3AB6` when it installs the first permutation/chunk.
The update path adds playback progress into that field at `6E4970`. Together
these support interpreting the optional byte offset as a playback/resume/seek
cursor. They do not establish a hidden constant Opus pre-skip, nor do these
bounded reads prove the semantics of every external callback or upstream
playback route. [MCC DLL, RVAs `6E3A98`–`6E3AB6`, `6E4956`–`6E4970`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>)

### Permutation duration metadata is a separate owner

`6CCBB0` selects the current language's eight-byte permutation info row and
falls back to row zero only when the selected row is entirely empty.
`6CEE00` uses that row's `+4` first-chunk index to address a 16-byte chunk.
`7699A0` reads the selected info row's first word for duration arithmetic.
The native loader likewise reads localized `pcm_bytes`, first chunk, and chunk
count, derives `input.samples=pcm_bytes/(channels*2)` for Opus, and separately
preserves each chunk's `+8` logical PCM count and packed bit 29. A permutation
duration word and a rounded chunk allocation are therefore independent operands;
neither is evidence for a beginning discard.
[MCC DLL, RVAs `6CCBB0`–`6CCC33`, `6CEE00`–`6CEE26`, `769A0A`–`769A95`](</home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll>);
[Native metadata join](/home/eric/Developer/Halo-2-Decompiled/src/assets/mcc_cache.cpp:301)

## What official Opus evidence does and does not supply

The official libopus API defines `opus_decode` as a raw-packet interface with
decoder state, packet bytes/length, PCM output, capacity in samples per channel,
and FEC selection. It returns the number of decoded samples. It has no
container pre-skip argument. That supports the call-signature interpretation
above; it does not authenticate MCC's bundled version or encode settings.
[Official Opus 1.5 decoder API](https://opus-codec.org/docs/opus_api-1.5/group__opus__decoder.html)

Official libopus `v1.5.2` documents `OPUS_GET_LOOKAHEAD` as an **encoder** query
for total codec delay and says the encoder contribution can vary by
implementation, version, and initial configuration. A fixed 120-, 240-, or
312-sample beginning discard cannot be selected for these MCC assets from that
generic information alone.
[Official libopus v1.5.2, `opus_defines.h`, lines 448–461](https://github.com/xiph/opus/blob/v1.5.2/include/opus_defines.h#L448-L461)

RFC 7845 is specifically the April 2016 **Ogg Opus encapsulation** standard. Its
pre-skip comes from the Ogg identification header; its end trimming comes from
granule positions; seek pre-roll is a separate operation. The recovered MCC
signed-length wrapper consumes neither an `OpusHead` pre-skip field nor Ogg
granule positions. Importing those container rules into MCC without an owning
field/caller would be an unsupported format change.
[RFC 7845, sections 1, 4.2, 4.4, 4.6, and 5.1](https://www.rfc-editor.org/rfc/rfc7845.html)

## Proven, inferred, and open

| Status | Conclusion |
| --- | --- |
| Proven | `769480` is allocation round-up only; it does not implement codec lookahead compensation. |
| Proven | Packet payload length is signed-16 magnitude, with a negative stop marker; the recovered decoder writes consecutive returned PCM samples from destination offset zero. |
| Proven | Miles receives each chunk's logical `+8` bytes, optionally reduced by a separately supplied stream byte cursor. |
| Proven, pre-fix source | Native retained rounded chunk output until a final permutation-wide tail trim; its former silence-only fixture could not distinguish leading discard or misplaced inter-chunk padding. |
| Proven, post-fix source | Native now truncates each chunk to its logical PCM frames after decoding all packet output, with eight added non-silent native/reference decoder cases. |
| Proven, measured cohort | 31 banks / 273 permutations include 10 multi-chunk permutations, zero non-final-padding cases, and no mismatch between summed logical chunk bytes and localized permutation bytes. |
| Inferred within the recovered path | A normal zero-time start submits the decoded prefix, with per-chunk tail padding excluded; the cursor is playback-time ownership rather than a fixed Opus delay. |
| Open | Exact bundled libopus revision, exporter/encoder settings, and whether source audio was shifted/cropped before packet encoding. |
| Open | Real packet negative-marker placement and original bundled decoder-state behavior across chunks/permutations; metadata shape alone does not decode the packets. |
| Open | Whether all playback entry routes have the same zero-time start; whether Miles or another layer changes timing after this submission seam. |
| Not established | A new constant beginning skip, bit-exact Xbox/MCC audio equivalence, or an audible decoder defect merely because different codecs produce different waveforms. |

The native-source and test claims in this table are owned by the separately pinned
[decoder](/home/eric/Developer/Halo-2-Decompiled/src/audio/mcc_audio.cpp:20) and
[test fixtures](/home/eric/Developer/Halo-2-Decompiled/tests/mcc_audio_tests.cpp:19).
The measured cohort is owned by the retained report cited below. Binary claims
are bounded by the exact RVA evidence above. Open items are explicit gaps, not
positive claims about the original exporter or runtime.

## Follow-up correction and bounded observations

The parent task's source change retains the full decoded packet contribution
to decoder state, then resizes the appended PCM to
`first_frame + chunk.pcm_bytes/(channels*2)` before proceeding to another chunk.
It retains the separate final permutation duration bound. The eight added
non-silent cases cover mono/stereo, continuing/reset decoder state, and whole/
shortened final duration; they encode real Opus packets and independently build
the reference from logical prefixes. This note inspected the new source and
tests; it does not claim to have run them or to have compared the bundled MCC
decoder's PCM. [Current decoder](/home/eric/Developer/Halo-2-Decompiled/src/audio/mcc_audio.cpp:51);
[New test cases](/home/eric/Developer/Halo-2-Decompiled/tests/mcc_audio_tests.cpp:19)

The retained shape census covers 31 banks and 273 permutations: 21 codec-5
Opus banks and 10 codec-1 banks. Reading its ordered metadata rows gives 10
multi-chunk permutations, no non-final logical padding, and exact equality
between each localized permutation's PCM bytes and the sum of its chunk PCM
bytes. Consequently, per-chunk truncation does not change this corpus's
submitted PCM ordering/length relative to the previous global-trim behavior.
It cannot be presented as an audible destruction-corpus fix.
[Cohort census, SHA-256 `3817ae21dd391e96e6e430e5e7007953dc7ba56afd2a20ce44c6af060c21ca23`](/home/eric/Developer/Halo-2-Decompiled/build/test-results/vehicle-destruction-waveforms-20261101/chunk-shapes.json)

The parent task also independently observed the unchanged original
**post-decode** fragment, starting at `77923E` and stopping **before** the Miles
call at `77928E`. Its 122 cases visit 17 original instruction sites and execute
1,628 instructions. Code is read/execute protected, chunk and nominal PCM
regions are read-only, and the report records no code patches or supplied
callee results. The fragment's half-open interval SHA-256 is
`27af3522437ddd46df5c041e5ef67733d9f00e212ddf2c7c9bd37f9aac75c1fa`.
Its observed registers/read/write traces confirm
`R8=pcm_start+min(cursor,logical_bytes)` and
`R9D=logical_bytes-min(cursor,logical_bytes)`, with the one-shot cursor cleared
when nonzero. The declared entry already begins after decode; this is **not**
execution of the codec, Miles, device output, or Xbox hardware and supplies no
PCM waveform or original decoder-state proof.
[Original submission observation, SHA-256 `c12e8b56c37d16bc325598fd127c240948a8db0d255957d5d1c00711eae8bdde`](/home/eric/Developer/Halo-2-Decompiled/build/test-results/vehicle-destruction-waveforms-20261101/original-chunk-submission.json);
[Owning observation tool](/home/eric/Developer/Halo-2-Decompiled/tools/oracle_mcc_sound_chunk_submission.py:1)

## Concrete next validation

The metadata shape join and bounded original submission observation above are
complete. The remaining validation is the packet/PCM side: retain small hashes
and metadata for packet lengths, decoded frame counts, reset boundaries, and
permutation duration for the same authenticated destruction inputs. Those
operands are already represented by
[the native loader](/home/eric/Developer/Halo-2-Decompiled/src/assets/mcc_cache.cpp:301).

Then use a non-silent owned permutation, including a multi-chunk case if one
exists, to compare the same packet stream at the **MCC pre-Miles PCM submission
seam** with native decode. Record actual submitted pointer offsets, byte counts,
and packet/state boundaries before comparing sound onset. Model the recovered
MCC operation as `concat(decoded_chunk[0:logical_chunk_frames])`, followed by
the evidenced stream seek selection; validate permutation duration ownership
separately. Keep current native bounds checks unless a real owned input proves
their strict equality assumption wrong. Do not add an arbitrary Opus pre-skip
to make Xbox/MCC correlation look better. A cross-codec comparison can follow
this same-payload decoder/submission check, but cannot substitute for it.

Any future execution/capture must follow
[the repository test-storage workflow](/home/eric/Developer/Halo-2-Decompiled/docs/TEST_STORAGE.md:1)
through `tools/run_test.py`/`HALO2_TEST_SCRATCH`, retaining raw PCM only when
required as explicit reference evidence. This research generated no scratch
build trees, binaries, memory banks, captures, or raw PCM.
