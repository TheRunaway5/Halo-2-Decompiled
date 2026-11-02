# Native pending-response eligibility: original TU1.5 contract

Research date: 2026-11-01. The initial sections are read-only primary-source research for the
pending-response continuation in `D8330`; no gameplay source, tests, build
configuration, runtime, or reference input was changed. No build, original-code
execution, emulator, or capture was run. Static inspection commands used
`tools/run_test.py` and its private `HALO2_TEST_SCRATCH` lifecycle.

## Main result

`DC4C0` is **default-allow**. It returns `AL=0` only when all of these hold:

1. The response's signed 16-bit runtime region index is nonnegative.
2. That index is less than the signed region count returned by `BC6C0`.
3. The live region-state byte at `states + region*8 + 1`, interpreted as a
   **signed 8-bit value**, is greater than or equal to the response's signed
   16-bit requested state.

A negative or out-of-range region index returns the initialized `AL=1`, as
does an in-range region whose current signed state is below the requested
state. A missing region is therefore **not a rejection result**. The object
query runs before any of those region tests, however: neither this result nor
the default local byte provides a missing/invalid-object fallback.
[Original XBE, `DC4C0..DC539`](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>)

`BC6C0` is a region-bank **field reader**, not an admission query and not a
Boolean success-returning function. It performs an unchecked low-16-bit object
slot lookup, divides the object's signed descriptor extent by ten with signed
truncation toward zero, and publishes count/active/cache/state pointers. It
contains no object-type, model-presence, salt, bitmap, null-body, region-name,
or region-index checks. The caller must already own a valid constructed object.
[Original XBE, `BC6C0..BC71F`](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>)

## Primary sources and pins

The authoritative executable is
`/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`,
SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Its full hash was checked before the principal static extraction. Addresses
below are original **virtual addresses**. Interval ends are exclusive; they are
not RVA or file-offset notation.

| Exact original byte interval | SHA-256 | Scope |
| --- | --- | --- |
| `DC4C0..DC539` | `402d9afc9903d668ecd74de89d6786333034ae8fb528bdd8f6991148c425f7ed` | Complete eligibility body, including both `RET 4` exits |
| `BC6C0..BC71F` | `b32df6832b8ff6f29346a50998986a9c88d5ae83908701f2cd4473cbdf60eb5f` | Complete region-bank reader, excluding following `CC` padding |
| `D842C..D847B` | `c9de79e225867dabe89abcc8d13a78b42d2c63f3dd506772d9a86b8153a525cd` | Pending continuation through actual call, `TEST AL`, and zero-result branch |
| `DC73C..DC778` | `abfaa6eaa6b559e653e1816db79008961050acc59dfc7be62848f8752fbad244` | Response execution's separate `-1` guards and requested region/state forwarding |
| `BC7CB..BC819` | `44a18bae851d38bf82bf7a1faca2e6a80a651f01ce1a5ea6322715ce3d2cedd2` | Region transition's call to actual model-state owner `1704C0` |

The navigation correspondence is canonical TU1.1 `DB570 -> DC4C0`,
`D73E0 -> D8330`, and `BB580 -> BC6C0`. Existing paired original listings and
the generated selected-image mapping record those identities; the claims here
come from the current authenticated TU1.5 bytes rather than the labels.
[Paired-original handoff](/home/eric/Developer/Halo-2-Decompiled/docs/DAMAGE_VITALITY_DELIVERY_HANDOFF.md:10);
[Generated `run_db570`](/home/eric/Developer/Halo-2-Decompiled/src/gameplay/units/source_damage_delivery_runtime_generated.cpp:5787)

The pinned Assembly schema is commit
`fd14ef3c46135e9422966c6a64cd926fb0bd23fb`,
`src/Assembly/Plugins/Halo2/hlmt.xml`, SHA-256
`84e26ca06e937f7984e814bfb7ff6f3248d735581a40d0fc875abe21d5dfe6be`.
That plugin file was clean in its checkout when inspected. Assembly owns the
field labels; the XBE owns the reads, signed comparisons, strides, and effects
on eligibility. [Local pinned schema](/home/eric/Developer/Halo-2-Decompiled/tools/reference/assembly/src/Assembly/Plugins/Halo2/hlmt.xml:326);
[Pinned upstream schema](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/hlmt.xml)

## Exact eligibility ABI and instruction bytes

At entry to `DC4C0`:

| Location | Actual input / output |
| --- | --- |
| `EAX` | Section index used in 32-bit multiply by `38h` |
| `ECX` | Response index used in 32-bit multiply by `50h` |
| `EDX` | Damage-info pointer; its `+C0h` word supplies section-array pointer |
| `[entry ESP+4]` | Target object datum; subsequently reused as the query's count output cell |
| `AL` on return | Eligibility only; the whole `EAX` is not a normalized Boolean |
| Cleanup | `RET 4`; saved `EBX/EBP/ESI/EDI` restored |

The original computes the authored response before querying the object:

```text
sections = load_u32(damage_info + C0h)
section = sections + section_index * 38h
responses = load_u32(section + 10h)
response = responses + response_index * 50h
```

These are original 32-bit address calculations. `DC4C0` does not bounds-check
either authored index or validate these metadata pointers. Source ownership
must establish their validity independently of region eligibility.
[Original XBE, `DC4C3..DC4E3`](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>)

Complete original body:

```text
000DC4C0 83ec10                   sub esp, 10h
000DC4C3 8b92c0000000             mov edx, [edx+C0h]
000DC4C9 6bc038                   imul eax, eax, 38h
000DC4CC 53                       push ebx
000DC4CD 55                       push ebp
000DC4CE 56                       push esi
000DC4CF 8b740210                 mov esi, [edx+eax+10h]
000DC4D3 8d2c89                   lea ebp, [ecx+ecx*4]
000DC4D6 57                       push edi
000DC4D7 8d442414                 lea eax, [esp+14h]
000DC4DB c1e504                   shl ebp, 4
000DC4DE 50                       push eax
000DC4DF 8b442428                 mov eax, [esp+28h]
000DC4E3 03ee                     add ebp, esi
000DC4E5 8d5c241c                 lea ebx, [esp+1Ch]
000DC4E9 8d7c2420                 lea edi, [esp+20h]
000DC4ED 8d742428                 lea esi, [esp+28h]
000DC4F1 c644241701               mov byte ptr [esp+17h], 1
000DC4F6 e8c501feff               call BC6C0
000DC4FB 668b4522                 mov ax, [ebp+22h]
000DC4FF 6685c0                   test ax, ax
000DC502 7c27                     jl DC52B
000DC504 8b4c2424                 mov ecx, [esp+24h]
000DC508 0fbfc0                   movsx eax, ax
000DC50B 3bc1                     cmp eax, ecx
000DC50D 7d1c                     jge DC52B
000DC50F 8b4c2414                 mov ecx, [esp+14h]
000DC513 660fbe54c101             movsx dx, byte ptr [ecx+eax*8+1]
000DC519 663b5520                 cmp dx, [ebp+20h]
000DC51D 7c0c                     jl DC52B
000DC51F 5f                       pop edi
000DC520 5e                       pop esi
000DC521 5d                       pop ebp
000DC522 32c0                     xor al, al
000DC524 5b                       pop ebx
000DC525 83c410                   add esp, 10h
000DC528 c20400                   ret 4
000DC52B 8a442413                 mov al, [esp+13h]
000DC52F 5f                       pop edi
000DC530 5e                       pop esi
000DC531 5d                       pop ebp
000DC532 5b                       pop ebx
000DC533 83c410                   add esp, 10h
000DC536 c20400                   ret 4
```

The initialized local byte at `DC4F1` is the byte loaded by `DC52B` after
`BC6C0` has removed its stack argument. It is **one**, not zero. Let `S` be
`DC4C0`'s entry ESP. Its non-overlapping query outputs are:

| Output | Actual cell in this caller |
| --- | --- |
| Count (`ESI`) | `S+4`, overwriting the already-read target argument |
| Active pointer (`EDI`) | `S-4` |
| Cached pointer (`EBX`) | `S-8` |
| State pointer (callee stack argument) | `S-Ch` |
| Default eligibility byte | `S-Dh` |

Thus the query's pointer writes do not initialize that default Boolean, and
reading its result as a query-success flag would be incorrect.
[Original XBE, `DC4D7..DC4FB`, `DC52B..DC539`](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>)

For valid owned inputs, the semantic predicate is:

```cpp
// Query the actual constructed object's region bank first.
const int region = sign_extend_16(response.runtime_region_index);
if (region < 0 || region >= bank.signed_count) return true;
const int current = sign_extend_8(bank.states[region].byte_1);
const int requested = sign_extend_16(response.new_state);
return current < requested;
```

The comparisons are signed: 16-bit `TEST/JL` for the index sign; 32-bit signed
`CMP/JGE` for index versus count; then sign-extension of the state byte into
`DX` and 16-bit signed `CMP/JL` against response `+20h`. In particular raw
state byte `FFh` means `-1` in this predicate, and an authored negative requested
state is not silently clamped into the schema's nominal `0..4` enum.
[Original XBE, `DC4FB..DC51D`](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>)

## Complete `BC6C0` region-bank reader

The exact nonstandard ABI is:

| Location | Meaning |
| --- | --- |
| `EAX` | Object datum; only its low 16 bits are used |
| `ESI` | Mandatory pointer to writable signed 32-bit count output |
| `EDI` | Optional pointer to active-permutation-pointer output; zero skips the store |
| `EBX` | Optional pointer to cached-permutation-pointer output; zero skips the store |
| `[entry ESP+4]` | Optional pointer to eight-byte-region-state-pointer output; zero skips the store |
| Return | `RET 4`; `EBP` saved/restored; `ESI/EDI/EBX` not changed; `EAX/ECX/EDX` are working registers, not a Boolean result contract |

Complete original instructions, with no type-dependent paths:

```text
000BC6C0 8b0dd0cb4e00             mov ecx, [4ECBD0h]
000BC6C6 8b5144                   mov edx, [ecx+44h]
000BC6C9 25ffff0000               and eax, FFFFh
000BC6CE 8d0440                   lea eax, [eax+eax*2]
000BC6D1 8b448208                 mov eax, [edx+eax*4+8]
000BC6D5 0fbf9018010000           movsx edx, word ptr [eax+118h]
000BC6DC 8916                     mov [esi], edx
000BC6DE 0fbf881a010000           movsx ecx, word ptr [eax+11Ah]
000BC6E5 03c8                     add ecx, eax
000BC6E7 b867666666               mov eax, 66666667h
000BC6EC f7ea                     imul edx
000BC6EE c1fa02                   sar edx, 2
000BC6F1 8bc2                     mov eax, edx
000BC6F3 c1e81f                   shr eax, 1Fh
000BC6F6 03c2                     add eax, edx
000BC6F8 85ff                     test edi, edi
000BC6FA 55                       push ebp
000BC6FB 8b6c2408                 mov ebp, [esp+8]
000BC6FF 8906                     mov [esi], eax
000BC701 7402                     je BC705
000BC703 890f                     mov [edi], ecx
000BC705 85db                     test ebx, ebx
000BC707 7406                     je BC70F
000BC709 8b16                     mov edx, [esi]
000BC70B 03d1                     add edx, ecx
000BC70D 8913                     mov [ebx], edx
000BC70F 85ed                     test ebp, ebp
000BC711 7408                     je BC71B
000BC713 8b06                     mov eax, [esi]
000BC715 8d0c41                   lea ecx, [ecx+eax*2]
000BC718 894d00                   mov [ebp], ecx
000BC71B 5d                       pop ebp
000BC71C c20400                   ret 4
```

In order, the original operations are:

1. `pool=load_u32(4ECBD0h)`, `headers=load_u32(pool+44h)`,
   `body=load_u32(headers+12*(object&FFFFh)+8)`.
2. Read **signed 16-bit** `extent=body[118h]`, and immediately store that raw
   sign-extended extent to `*ESI`.
3. Read **signed 16-bit** `offset=body[11Ah]`; compute `base=body+offset` with
   wrapping 32-bit addition.
4. Compute `count=trunc_toward_zero(extent/10)` and overwrite `*ESI` with it.
5. If `EDI!=0`, publish `base` as the active pointer.
6. If `EBX!=0`, reread `*ESI` and publish `base+count` as the cached pointer.
7. If the stack output pointer is nonzero, reread `*ESI` and publish
   `base+2*count` as the state pointer.

The one-operand signed `IMUL`, arithmetic shift of its high half, and final
sign correction implement signed division by ten, not unsigned division or a
ten-byte alignment requirement. Thus positive extent `11` yields count `1`,
extent `-1..-9` yields `0`, and extent `-10` yields `-1`; original code does
not reject those descriptor values. Optional outputs suppress only their own
stores. The raw-extent store, later count store, and later count rereads matter
to an instruction-compatible adapter, particularly if outputs alias. The
actual eligibility caller supplies distinct output cells.
[Original XBE, `BC6D5..BC71C`](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>)

For a valid constructed descriptor of `10*N` bytes, storage is therefore
`N` active bytes, `N` cached bytes, then `N` eight-byte state rows. A zero-count
descriptor still computes/publishes the pointer arithmetic; eligibility then
allows every region index without dereferencing a state row. A null body,
missing pool, stale salt, `FFFFFFFFh` object, negative offset, or escaping
descriptor is not assigned a safe success/failure result by this reader: the
original continues unchecked reads/arithmetic. Such malformed-object cases
must remain separate from the valid-object/missing-region default-allow rule.
[Original XBE, complete `BC6C0` and `DC4FB..DC51D`](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>)

## Authored field meaning versus authenticated consumption

Assembly labels `hlmt` damage sections at damage-info `+BCh`, stride `38h`,
with instant responses at section `+Ch`, stride `50h`. The pointer halves of
those Xbox tag blocks explain original reads at info `+C0h` and section `+10h`.
It labels response `+1Ch` as Region StringID, `+20h` as New State, and `+22h`
as Runtime Region Index. The New State labels are Default/Minor/Medium/Major/
Destroyed (`0..4`). These are schema names, not extra executable validation.
[Pinned `hlmt` schema](/home/eric/Developer/Halo-2-Decompiled/tools/reference/assembly/src/Assembly/Plugins/Halo2/hlmt.xml:326)

The executable independently authenticates their roles: `DC4C0` indexes the
live eight-byte state array using signed `+22h` and compares its state byte
against signed `+20h`. It does not read response `+1Ch`, search by StringID,
consult an active permutation byte, or validate an authored state against the
`0..4` label range. A negative requested state can pass eligibility when the
region is absent; it can fail for a valid region already above that state.
[Original XBE, `DC4FB..DC51D`](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>)

The actual response executor `DC660` separately skips its region-transition
branch when either response word is exactly `FFFFh`. Otherwise it sign-extends
`+20h` and `+22h` and forwards them as state and region arguments to `BC720` at
`DC76C`. `BC720` reaches the actual model-state owner `1704C0` at `BC7F9`.
That owner loads the state-bank pointer, advances it by one to the damage byte,
reads the current byte with sign extension at `17051C`, and writes a selected
state into that byte at `1706AA`. This reinforces the state interpretation,
while preserving the distinction between eligibility and mutation. A response
may be eligible while its region-change sub-branch is absent; eligibility does
not promise that every later effect/constraint/death operation is admitted.
[Original XBE, `DC73C..DC771`, `BC7CB..BC819`, `170502..170526`, `1706A0..1706B0`](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>)

## Pending caller join

The original due continuation obtains the response index from the packed word's
low three bits, forwards its current section index and damage-info pointer,
and passes the actual target datum on the stack. It tests **only AL** after
`DC4C0`; a zero result skips the pending response continuation at `D84DC`.
The decisive original instructions are:

```text
000D8458 8b942484000000           mov edx, [esp+84h]
000D845F 8bf0                     mov esi, eax
000D8461 66894c2430               mov [esp+30h], cx
000D8466 83e607                   and esi, 7
000D8469 52                       push edx
000D846A 8b54241c                 mov edx, [esp+1Ch]
000D846E 8bce                     mov ecx, esi
000D8470 8bc3                     mov eax, ebx
000D8472 e849400000               call DC4C0
000D8477 84c0                     test al, al
000D8479 7461                     je D84DC
```

This supplies no new lookup result or permission Boolean. The countdown becoming
due and the current region-state eligibility are separate owners. The native
countdown change already present in the navigation source does not establish
completion of `BC6C0` or response execution.
[Original XBE, `D842C..D847B`](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>);
[Current pending continuation](/home/eric/Developer/Halo-2-Decompiled/src/gameplay/units/source_damage_delivery_runtime_generated.cpp:2697)

## Existing native ownership routes

These are existing implementations, not fabricated eligibility-service values:

| Existing owner/API | Useful contract and limit |
| --- | --- |
| `source_live_model_regions_tu15(memory,native_pool,object)` | Reads the actual constructed body's `118h/11Ah` bank after full-identity lookup and body/extent validation. Returns native `SourceModelRegionBank`, preserving the damage byte's raw bits. Rejects invalid descriptors instead of reproducing unchecked original memory reads. |
| `MultiplayerObjectRuntime::object_model_regions(object)` | Existing public wrapper around that reader with its `Continuation::PublicCall` ownership boundary. It reads the retained authoritative TU1.5 world, not a new selection or RNG result. |
| `SourceModelRegionBank::selection.states` | Native typed current-state vector. `SourceInitialModelRegionState::damage` is stored as `uint8_t`, so this eligibility comparison must explicitly reinterpret it as `int8_t`. |
| `source_model_regions_change(...)` | Existing native state transition owns current damage/properties, choices and effect handles; it already sign-reinterprets the previous damage byte. It requires genuine RNG/effect owners only on consuming branches. It is not itself the eligibility predicate. |
| `PropSandbox::region_selection(full_handle)` | Returns an existing prop generation's selection or null. Suitable only when that prop owns the authoritative current state; a missing/stale mirror must not become an invented zero-region bank. |

[Live region reader](/home/eric/Developer/Halo-2-Decompiled/src/gameplay/objects/source_live_objects_tu15.cpp:44);
[Public wrapper](/home/eric/Developer/Halo-2-Decompiled/src/gameplay/multiplayer/multiplayer_object_runtime.cpp:1484);
[Native state types](/home/eric/Developer/Halo-2-Decompiled/include/halo2/model_selection.hpp:35);
[Native transition](/home/eric/Developer/Halo-2-Decompiled/src/rendering/models/source_model_regions.cpp:13);
[Prop generation lookup](/home/eric/Developer/Halo-2-Decompiled/src/gameplay/objects/prop_sandbox.cpp:210)

The reader's full-salt lookup is an existing native C++ implementation:
`source_raw_datum_lookup` rejects NONE, out-of-range high-water indices, and
salt mismatches. Its checks belong to native object admission, and are not
checks found in `BC6C0`. The region reader additionally requires a constructed
body, nonnegative extent divisible by ten, at most 16 regions, and an in-body
bank when nonempty. It permits an empty bank without consuming the unused
offset. These deliberately checked preconditions should be documented as such,
not mislabeled as the original query's failure return.
[Native full-salt lookup](/home/eric/Developer/Halo-2-Decompiled/src/gameplay/objects/raw_datum_pool_identity.cpp:78);
[Region descriptor checks](/home/eric/Developer/Halo-2-Decompiled/src/gameplay/objects/source_live_objects_tu15.cpp:47);
[Existing descriptor/identity fixtures, not rerun here](/home/eric/Developer/Halo-2-Decompiled/tests/source_live_objects_tu15_tests.cpp:49)

A genuine native64 semantic eligibility kernel can consume an owned typed
response and the same target generation's current state bank, implementing the
signed predicate directly in C++. The outer world/object owner must actually
obtain that bank through its real identity/storage ownership; it must not
supply a canned lookup, count, state, or eligibility result. For a retained
TU1.5 world, the existing live-region reader is a concrete route. It still
bridges source-layout memory; using it does not establish that the complete
object constructor, storage, update parent, or response executor has become a
native64 semantic implementation. For an independently native prop owner,
using the authoritative `SourceModelRegionBank` directly avoids duplicating the
query or selecting a new variant. These are implementation routes inferred
from the inspected APIs, not completed changes or parity claims.

## Explicit open ownership and next validation

Proven here: exact ABI, default-allow result, signed comparisons, query output
roles/order, signed descriptor division, missing-region behavior on valid
objects, and a bounded existing native ownership route. Not established here:
completion of the native object lookup/storage/constructor chain for every
vehicle, exact authored `+22h` fixup production across every model, all pending
response transitions, or effect/constraint/death delivery. No physical Xbox
acceptance follows from static disassembly.

The next bounded implementation/validation should keep two contracts separate:

1. Qualify the pure signed eligibility predicate against unchanged `DC4C0`
   with real query-derived state banks. Cover absent/zero-count regions,
   negative and boundary indices, state bytes `00/04/7F/80/FF`, requested signed
   words including `FFFFh`, equality, and immediately lower/higher values.
   Do not substitute the query's result or preload AL to force admission.
2. Qualify the native query/storage adapter separately against unchanged
   `BC6C0`, including its two count stores, optional output branches, signed
   `118h/11Ah` arithmetic, actual object-pool reads and ordered pointer writes.
   Valid constructed inputs and intentionally malformed descriptor arithmetic
   are different scopes. Invalid full identities rejected by a native owner
   must not be presented as original `BC6C0` rejection behavior.
3. Join the same full object generation and authenticated response operands in
   the due continuation only after those contracts are reviewable. Preserve
   the existing caller's pending-word and instruction-observer conventions in
   any compatibility adapter; do not replace later response/effect results.

This is a proposed validation route, not execution performed in this research.
Any future probe/build must follow the repository's
[test-storage workflow](/home/eric/Developer/Halo-2-Decompiled/docs/TEST_STORAGE.md:1).
Static inspections here retained only this bounded report and tool-returned
listings/hashes; no raw memory banks, source binary copies, captures, or build
products were generated. The private scratch directories printed by these
inspections were removed on completion.

## Inspected native-source pins

These are working-file hashes at research time, not an assertion that the
parent's separately active gameplay build includes new eligibility code:

| File | SHA-256 |
| --- | --- |
| `source_damage_delivery_runtime_generated.cpp` | `f9a5d939ee31ebee82540acb2f9bda03042d64f3f0d4c461ec48e4069f1d8901` |
| `source_live_objects_tu15.cpp` | `8da559d14641719f453d744917e87d277a17e437850f6362caa000811dba6c5c` |
| `source_live_objects_tu15.hpp` | `ec32201261dc6bb02a2f06cead0ace5a60d4093225af77f654afdd86589a95d3` |
| `multiplayer_object_runtime.cpp` | `e1cf57548de3d6ca691e300af12590fdff6da5c9c3b6d0f977e3a97cb58c4475` |
| `model_selection.hpp` | `731043bde00289b4aa750c491a371fc1bd848069ae6e836897748a0d46848ca7` |
| `source_model_regions.hpp` | `74e4b04285c105e784c92f1cda5bd752deac0119a73e1634173a41fd9645bbac` |
| `source_model_regions.cpp` | `af5d16aad240dc2e07669f307cced072404c17a5e2d741f73d74e226ddb2b179` |
| XBE parser `tools/reference/halo2-decompiled/tools/xbe.py` | `3bc3717d8a78450690ad9efa9eabf9ffa45a68e1207dde554a820c4cd80413d4` |

## Native implementation and qualification, 2026-11-01

The later implementation now separates semantic kernels from their legacy-call
boundaries. `native_tu15_object_regions.hpp` owns signed extent division, both
count stores, optional-output decisions, current count rereads and wrapping
pointer arithmetic. `native_tu15_damage_region_eligibility.hpp` owns the query
ordering, signed index/count/state comparisons and default-allow decision.
Their storage adapters retain actual mutable reads/stores, source stack cells,
defined flags, partial `AL/AX/DX` updates, both observer conventions and fault
prefixes. They supply no query, eligibility, damage or effect result.

The [physical and native comparison](../build/test-results/vehicle-destruction-pending-eligibility-20261101/v4/report.json)
passes399 declared ABI cases: every one of the77 original instruction sites
executes in unchanged physical i386 code, totaling22,060 instructions, including
11 actual memory-fault prefixes. Original Unicorn observations and physical
execution agree on control/access/store order, all GP registers and defined
flags. Cases cover missing/out-of-range regions, signed byte/word boundaries,
zero/negative/unaligned descriptor extents, negative offsets, wrapped authored
indices, ignored datum salt, optional outputs, aliases to count/descriptor/stack
cells and invalid-object/address faults. Invalid objects are never turned into
default-allow or query success. The declared ABI is not an original captured
Xbox call frame.

The original six-instruction division slice is separately exhausted for all
65,536 signed descriptor words. GCC and Clang each match those numeric counts
and1,596 adapter cases (both observer conventions with empty/two-value live x87
inputs),28,424 reads and16,868 stores. Committed banks, preserved SIMD/live x87,
controls/status, defined flags, registers and fault prefixes are checked. The
new observer is synchronous, read-only and bank-bound; foreign-bank and partial
calls do not receive a completion event.

Seven wrong implementations are rejected under each compiler: divisor8,
omitted raw-extent publication, a cached count instead of the required reread,
rejecting absent regions, unsigned current state, omitted datum mask and the
wrong response stride. Required numeric references are36,985 and11,572 bytes;
they contain declared inputs and original numeric observations, not full raw
banks or native-consumed expected outcomes.

The [registered regression](../build/test-results/vehicle-destruction-pending-eligibility-20261101/registered/report.json)
passes in the active frozen gameplay build. Guarded native routes preserve the
retained literal bodies and older-image behavior. The
[delivery regeneration check](../build/test-results/vehicle-destruction-pending-eligibility-20261101/regeneration.json)
reproduces the current source exactly, authenticating90 retained routines and
8,976 static instruction sites. The production admission separately verifies
21 transitive component headers against the frozen world; unrelated root source
changes are not imported. The original cohort's read-only `tests/data` mount
is authenticated without writing through it.

The [current eight-case runtime rerun](../build/test-results/vehicle-destruction-pending-eligibility-20261101/live-eligibility-summary.json)
passes with118 actual target-native decisions:8 missing-region allows,
100 below-requested allows and10 already-at-or-above rejections. Each decision
retains its full target generation/body/definition and authored section/response
field readback. Counts match the exhaustive original numeric reference; signed
state comparisons and response addresses match the authenticated rules.
Restore/replay diagnostic executions are excluded from this live journal.

The module's actual pending due, matching native eligibility and original target
blast have observation orders8 ->9 ->10. Its six committed countdown transitions
still match the original word reference. The cohort retains144 complete-world
restore/replay comparisons. Neither a query result nor a later response/blast
result is provided. A fixture byte-reader compilation error and its correction
remain recorded in separate build/refresh reports.

This proof does not close every pending response, complete destruction
definition, later response/model-state owner, sound/device output or framebuffer
gate. No physical Xbox wet-feedback format/time or audible parity is established.

The [qualification/cleanup receipt](../build/test-results/vehicle-destruction-pending-eligibility-20261101/qualification.json)
records the final frozen source match, admitted header closure and live reports.
All component CMake trees and60,801,024 allocated bytes of standalone original
CPU helpers/private projects have been removed. No new large artifact is
retained; the two required numeric references total48,557 bytes. The finite
owned gameplay build is reused for the next response/model-state conversion
in this same ongoing investigation. Launcher-selected runtimes are preserved.
