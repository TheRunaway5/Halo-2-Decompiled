# Original local player identity and configuration contracts

The shared profile module now recovers whole original offline identity,
full-profile/default configuration construction, local-profile conversion and
player-configuration sanitization. Retained results and this producer use the
same12-byte identity,6-byte machine and144-byte configuration types. The
existing native lobby team conversion uses the shared signed-field contract.
This original-contract recovery did not bind the complete producers to native
player creation. Subsequent
[native selected-profile delivery](MP_NATIVE_PLAYER_PROFILE_RESEARCH.md) now
uses the configuration producer and sanitizer; the genuine native machine
identity provider and complete participant callback chain remain required.
No variant gained admission.

## Original owners

| Owner | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Offline identity | `14B410` | `14CB60` | `14DF40` |
| Full profile constructor | `1A0750` | `1A40B0` | `1AA360` |
| Configuration constructor | `1537F0` | `154F40` | `156320` |
| Local profile converter | `18FD94` | `1917C6` | `192BAA` |
| Configuration sanitizer | `14BC80` | `14D3D0` | `14E7B0` |

The identity, full-profile and configuration constructors have equal complete
instruction shapes in all three images. TU1.1 and TU1.5 sanitizers match
completely. Retail's sanitizer has158 instructions, versus the later173:
retail omits the multiplayer representation-remapping branch. It is executed
independently and is not declared equivalent. Converter ABIs also differ;
each version executes its own complete owner and reached original callees.

Static TU1.5 callers`70CE0`, `192931` and`14E9E0` retain the platform address,
local-profile binding and player-copy evidence. `70CE0` obtains six bytes from
the actual`XNetGetTitleXnAddr` output. These callers are static listings here;
no successful platform call, frontend selection or player creation is supplied.

## Preserved data and field types

The identity owner forms a48-bit little-endian value from all six platform
machine bytes, shifts it16 bits and ORs it with the sign-extended32-bit local
user argument. Its last DWORD is the literal`BAD00000`. The native API retains
the original arithmetic for boundary inputs; an operational local player still
needs its genuine machine and local-user producers. A display name, salted
player datum or random transport nonce is not substituted.

The full-profile constructor clears480 bytes, sets bit0 of DWORD+4, writes
byte+102 to3 and appearance bytes+118/+11B to10. The144-byte configuration
constructor clears its entire record, then sets bytes+7E/+7F and all bytes
84..8F to`FF`. Remaining fields stay zero because the original constructors
clear them, not because unknown native fields are assumed harmless.

The local converter first constructs the configuration. If the signed
input-member selector is0..3 and the original member-presence byte is nonzero,
the actual member getter copies its complete144-byte record. An empty or
invalid member leaves the constructor result. This local input table does
not require an Xbox online-account success. Both pointer outputs are optional.

It then applies the retained local fields:

| Configuration field | Source and operation |
| --- | --- |
| `00..3F` | Local display name: at most31 UTF16 units, zero padding after NUL and final NUL |
| `40..4F` | Exact16 appearance bytes from local+130 |
| `50..6F` | Exact32 bytes from local+4EC when either account DWORD at+4E0/+4E4 is nonzero; otherwise cleared |
| `70..7B` | Exact12 bytes from local+4E0 under that same gate; otherwise cleared |
| `7C` | Signed32 local+1FC: preserve NONE=-1; other negatives become0; positive maximum16; store signed byte |
| `7D` | Raw local+200 byte |
| `80` | Raw local+46C byte |
| `81` | Low byte of local+204; TU1.5 substitutes4 when its actual51D2C8 gate is nonzero |

Other configuration bytes survive a present input-member copy. Without that
copy, their constructor values remain. The generic producer does not rebuild
an input record from a name or manufacture account metadata.

The sanitizer consumes the real game type, matg representation count, scenario
player-profile selectors, selected engine and team mask. Game type1 chooses
the first signed16 scenario selector other than NONE, stores its low byte and
sets team1. TU1.1/TU1.5 game type2 map representation1/3 to3 and all other
representations to2; retail does not perform that branch in this owner. The
selected source policy is not an editable variant option.

The subsequent representation clamp uses signed bytes and wrapping32-bit
`count-1`. NONE is retained. Bytes+7E/+7F retain NONE and clamp other negative
values to0. Invalid signed32 selector+84 resets all12 bytes84..8F to`FF`;
otherwise signed32+8C clamps0..3FFFFFFF and signed16+88/+8A clamp0..127.
Signed byte+7D clamps0..3; signed byte+81 outside0..7 resets to0. Team admission
uses the original16-bit mask and x86 shift masking, preserving team NONE.
Unclassified bytes remain unchanged.

## Verification and runtime boundary

The fixture contains9,606 whole original calls: each version executes its two
constructors,640 identity cases,512 local converters and2,048 sanitizers.
Inputs cover all signed-byte patterns, signed32/16 boundaries, present and
empty local members, invalid member selectors, all nullable-output pairs,
nonzero raw record fields, account gates, name termination/padding, scenario
selectors, real team-mask gates and version-specific representation policy.
Native tests compare every byte of the complete12/144/480-byte outputs.
No instruction patches or supplied game/platform/service returns are used.

Fixture SHA256:
`ff773fe68086ceb9808170aad27739815e3afc249b33a121f3ffa50573e651a7`.
An independent second execution reproduces the fixture, complete report and
all28 instruction listings byte for byte. Twenty-five executed-owner listings
contain1,292 instruction rows; the three separate static callers contain159.
Seven complete source pairs preserve369 rows of immutable owner correspondence.
The complete native profile implementation and its comparison harness are
checked with ASan, UBSan and leak detection. Current reproduction, regression,
production/audit logs and exact source/fixture pins are recorded in
`analysis/multiplayer/game-variants/player-profile-verification.json`.
All21 rebuilt focused regressions pass, including the shared lobby-team field,
retained remapping, game end, native match/UDP and frontend flows. The freshly
built executable advances300 ticks with four local Team Slayer players on
Lockout and observes `finished=1`; this is not300 ticks of active combat.
The fresh catalogue audit runs78 normal Split Screen/System Link start actions,
admits2/39 variants and blocks37 (expected strict exit1). Neither startup nor
the source-record comparisons prove full matches, a rendered Carnage Report,
physical LAN or original Xbox platform/wire parity.

This recovery closed the shared producer algorithms and data widths; its
verification snapshot did not close native upstream providers or the complete
live player-copy binding. The later selected-profile delivery is recorded
separately. Retained capture,
ranking, publication, peer transport, report construction/navigation and
source session/frontend handoff still require their actual owners. Full
matches, general weapons, the other engines and edited-rule flows remain
incomplete. The all-variant goal stays active with its original scope.

```sh
.tools/python/bin/python tools/oracle_player_profile.py
cmake --build build --target player_profile_tests -j2
ctest --test-dir build -R '^original_player_profile$' --output-on-failure
```
