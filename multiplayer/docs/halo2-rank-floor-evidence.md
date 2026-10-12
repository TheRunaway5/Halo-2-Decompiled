# Original Halo 2 advertised rank floor and search envelope

Research date: 2026-10-12. Scope: the original raw-rank summary, hopper mapping,
advertisement floor and outgoing search bounds. No product, build, deployment or
earlier evidence file was changed.

**The advertised minimum is `max(raw_minimum, F(raw_maximum))`.** It is computed
when building the description, independently from the stored adjusted-player
properties. `F` is original `0x1931A0`, using the selected hopper's128-byte table.
The complete launch-default table, all128 inverse results, summary cases and
search envelopes have been executed from unchanged original bytes.

## Authority and controls

Executable: `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

```sh
.tools/python/bin/python .local/rank-floor-research-20261012/oracle_rank_floor.py
```

There are **179 passing controls**, covering **3,060 original instruction
addresses**, each checked against the pinned image. Artifacts are under
[`.local/rank-floor-research-20261012`](../../.local/rank-floor-research-20261012):

- `oracle_rank_floor.py`, `oracle-rank-floor.json`: every input and expected/
  actual output, including `[1,8] → floor3` and `[80,100] → floor90`.
- `retail-rank-map-and-search-ranges.json`: full128-byte table, full inverse
  vector and eight search envelopes with initial/intermediate/final bounds.
- `original-function-spans.json`, `original-*.asm`: original function sizes,
  source status, hashes and disassembly.
- `executed-original-instructions.txt`: verified instruction trace.

Runner SHA-256:
`c52dd3d451da1c1a98ee35b9a1d008a68118494fc7ab331614f221ec9fc51971`.
Trace SHA-256:
`589a451df307b09deb63549af02b09259f63001eca0c22882bf1a9aa934e709b`.

No original instruction or callee is replaced. Full original constructors,
inverse getter and aggregation run with synthetic records. The producer floor
control executes `0x736A8..0x736FA` with its actual stack/register inputs,
including the unchanged inverse callee; it does not execute or stub CRT time.
Search initialization controls begin at the original successful-allocation
arithmetic block `0x9098B`, after allocation, and stop before its query call at
`0x90A5F`. Query controls execute the complete `0x90B30` prefix and stop before
the real Live wrapper `0x8FA80`. These are bounded client controls, not platform
allocation, a service response or live matchmaking validation.

## Mapping table and exact inverse

Original `0x1936E0` initializes table `T` at hopper `+0x51C`:

| Raw table index `i` | Default `T[i]` |
| --- | --- |
| 0…5 | `min(i+5,127)` |
| 6…15 | `min(i+6,127)` |
| 16…25 | `min(i+7,127)` |
| 26…30 | `min(i+8,127)` |
| 31…35 | `min(i+9,127)` |
| 36…127 | `min(i+10,127)` |

All seven built-in hopper constructors produce exactly this table in independent
controls. This establishes executable launch defaults. Later downloaded tables
are not recovered and must not be inferred from these defaults.
[Constructor](../../tools/reference/halo2-decompiled/src/unknown_1936e0.cpp#L152),
`original-001936e0.asm`, `oracle-rank-floor.json`.

Matched original `0x1931A0` is:

```text
F(rank, T):
    i = rank
    while i > 0 and signed_byte(T[i-1]) >= rank:
        i -= 1
    return i
```

For validated original tables, entries lie in0…127. This is an inverse lookup,
not a fixed subtraction. Default examples: F(8)=3, F(20)=14, F(50)=40,
F(80)=70, F(100)=90 and F(127)=117. F(0)=0 and F(NONE)=NONE are literal getter
results. Do not index the forward table with NONE.
[Inverse](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp#L236),
[table validity](../../tools/reference/halo2-decompiled/src/unknown_193560.cpp#L36),
`original-001931a0.asm`, `retail-rank-map-and-search-ranges.json`.

## Raw aggregation and the advertisement floor

`0x72540` collects the selected-hopper **raw** rank and XP. Its integer average,
minimum and maximum use the raw cached ranks. It neither generates nor reads
the adjacent adjusted-player rank to compute those values. The existing
[party-average evidence](halo2-party-average-evidence.md) independently verifies
that boundary.
[Collector](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1090).

`0x7E210` takes the gathering summary's raw `first[]` at summary `+0x248`, keeps
that array unchanged in its output `+0xD4`, and derives nonguest raw minimum,
maximum and signed-integer average. Player identity flags1/2 exclude the player
from those aggregates. Missing nonguest ranks have a separate upstream
readiness gate; the generic aggregation alone is not permission to accept an
incomplete party.
[Aggregation](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp#L1102),
`original-0007e210.asm` at0x7E4C5–0x7E54E.

`0x73530` then writes:

```text
description.average       (+0x78) = gathered raw average
description.maximum       (+0x7C) = gathered raw maximum
description.raw_minimum   (+0x80) = gathered raw minimum
description.mapped_minimum(+0x84) = NONE if raw_minimum == NONE
                                    else max(raw_minimum, F(raw_maximum,T))
```

The detailed candidate copies the description at candidate `+0xA8`, so its
mapped minimum is **candidate `+0x12C`**. `0x733F0` sends the same floor as
property `v3` (`helper-output+0x1C`); the Live result parser puts that property's
value at brief candidate **`+0x64`**. It does not substitute literal raw minimum
or minimum adjusted-player rank. For `[1,8]`, raw minimum1 becomes3; for
`[80,100]`, raw minimum80 becomes90. A literal-minimum replacement can reject
searches the original floor admits, so it is a stricter replacement input.
[Description](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1219),
[properties](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1246),
[brief layout/parser](../../tools/reference/halo2-decompiled/src/online_matchmaking.cpp#L57),
[candidate admission](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L536),
`original-00073530.asm`, `oracle-rank-floor.json`.

## The separate adjusted array: original bytes correct a decompile error

During each `0x7E210` aggregation, the original computes a highest raw rank `H`
for each summary group/party and derives `F(H,T)`. For valid nonzero catalog
indices, a non-NONE player's separate adjusted output is the greater of its
raw rank and this group floor. Catalog index0 skips this adjustment. This work
is recomputed from raw summary ranks and group IDs; it is not fetched from the
stored adjusted property and is not performed by `0x72540`.

The reconstructed C++ currently says `adjusted = F(lower,T)` after `lower`
already holds F(H). **The original bytes do not apply the inverse twice.**
At0x7E481 the highest remains in ECX; the inline loop changes EAX. The call at
0x7E4A9 still receives ECX=H and returns F(H). Therefore source text suggesting
F(F(H)) is not authoritative here. Complete original controls distinguish it:

| Raw ranks/grouping/table | Original adjusted array |
| --- | --- |
| `[1,2,8]`, same group, launch table | `[3,3,8]` |
| `[1,2,8]`, same group, synthetic `T[i]=min(i+1,127)` | `[7,7,8]`, not `[6,6,8]` |
| `[1,2]` in one group, `[8]` in another, launch table | `[1,2,8]`; global advertisement floor still3 |
| `[1,2,8]`, catalog index0, launch table | `[1,2,8]`; advertisement floor still3 |

The split-group clan fixture deliberately does not claim later clan admission;
its production arithmetic runs before that check. The synthetic table is a
diagnostic input, not a historical coefficient.
[Reading aid with discrepancy](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp#L1140),
`original-0007e210.asm` at0x7E444–0x7E4BF, `oracle-rank-floor.json`.

At launch `0x71FF0` writes this separate adjusted output to player properties
`+0x88`, raw rank to `+0x8A`, and XP to `+0x8C`. The local-property producer
`0x65530` initially sets adjusted `+0x88` to NONE. These properties are distinct
from the summary inputs used to derive the advertisement floor.
[Launch publication](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp#L1741),
[local publication](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L1412),
`original-00071ff0.asm`.

## Original outgoing search envelope

The successful-allocation block in `0x90880` derives the following from
searching party raw minimum `m` and raw maximum `M`:

```text
f = F(M,T)                         search+0x60
l = max(f,m)                       search+0x68
u = signed_byte(T[l])              search+0x64
R = max(l-f, u-M)                  search+0x20
current_radius = 0                search+0x1C
```

For bounded radius `r`, `0x90B30` sets:

```text
minimum = max(l-r, f)              search+0x30
maximum = min(M+r, u)              search+0x28
both rank filters enabled
```

Radius NONE disables both filters. Minimum composite score uses its separate
200→100 interpolation when R>0; when R=0 or radius NONE it is100. The existing
[candidate evidence](halo2-candidate-scoring-evidence.md) contains the complete
original arithmetic and score controls. This envelope uses raw extremes and
the hopper table, not average±radius.
[Initializer](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L821),
[query prefix](../../tools/reference/halo2-decompiled/src/online_session_search.cpp#L196),
`original-00090880.asm`, `original-00090b30.asm`.

| Raw `m…M` | `f` | `l` | `u` | `R` | Radius0 bounds | RadiusR bounds |
| --- | --- | --- | --- | --- | --- | --- |
| 1…8 | 3 | 3 | 8 | 0 | 3…8 | 3…8 |
| 80…100 | 90 | 90 | 100 | 0 | 90…100 | 90…100 |
| 20…20 | 14 | 20 | 27 | 7 | 20…20 | 14…27 |
| 10…15 | 9 | 10 | 16 | 1 | 10…15 | 9…16 |
| 49…50 | 40 | 49 | 59 | 9 | 49…50 | 40…59 |
| 126…127 | 117 | 126 | 127 | 9 | 126…127 | 117…127 |

These values are independently executed through both original blocks; the
artifact includes additional0…0 and0…127 cases. This recovers bound values,
not the retired Live stored procedure or cadence of service responses. Radius
advancement remains completion-driven as established by the
[gathering-session evidence](halo2-gathering-session-evidence.md). Timed custom
stages, service-side reciprocal admission and historical downloaded tables must
retain their separately declared provenance.
