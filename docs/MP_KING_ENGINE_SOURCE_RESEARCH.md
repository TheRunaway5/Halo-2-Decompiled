# Original King engine ownership

Research snapshot: 2026-10-11. King is frontend family3, original source type4. This note identifies complete original owners and executable recovery work. It does **not** admit King into the native runtime or establish that any King variant is operational.

The original-only probe below executed complete initialization, round-start and spline geometry for all12 original disc multiplayer maps and all57 authored hill groups. Retail, TU1.1 and TU1.5 produce byte-identical complete `0x1DC`-byte states:81 states per image and162 pairwise comparisons against retail. There are no replaced callbacks or supplied geometry outputs. Scenario ABI headers, simulation backing and a30Hz clock are explicit isolated inputs; cache loading, original world construction and live gameplay are outside this result.

## Primary authorities

- [Engine registration inventory](../analysis/multiplayer/game-variants/engine-tables.json), produced by [original registration execution](../tools/inspect_game_engines.py): retail source4 object`0047FC88`, vtable`0045C878`; TU1.1 object`00487190`, vtable`00463CE8`. Registration alone does not execute an objective engine.
- [Retail executable](../assets/disc/default.xbe), SHA256`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
- [TU1.1 executable](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03), SHA256`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- [Imported TU1.5 executable](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>), SHA256`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`; [inspected original King instructions](../analysis/multiplayer/game-variants/king-engine-20261011/instructions/u5-002c6f50.asm). The parent recovered these addresses by executing original registration; this research independently inspected their bytes and executed the whole initialization/geometry chain below.
- Complete readable retail [initialization/update owners](../analysis/retail/functions/002bd140.c), [round start](../analysis/retail/functions/002bd200.c), [occupancy](../analysis/retail/functions/002bd2c0.c), [update](../analysis/retail/functions/002bd680.c), [geometry](../analysis/retail/functions/002be050.c) and [scoring](../analysis/retail/functions/002beee0.c). Decompiler omissions and incorrect register recovery must be resolved against unchanged executable instructions.
- TU1.1 exports contain complete helpers [selector2C3440](../analysis/map-pack-update/functions/002c3440.c), [default points2C3CA0](../analysis/map-pack-update/functions/002c3ca0.c), [hill setter2C3FB0](../analysis/map-pack-update/functions/002c3fb0.c), [geometry2C4390](../analysis/map-pack-update/functions/002c4390.c), [occupancy predicate2C4BC0](../analysis/map-pack-update/functions/002c4bc0.c), [scoring2C5220](../analysis/map-pack-update/functions/002c5220.c), [scenario marker enumeration1A25A0](../analysis/map-pack-update/functions/001a25a0.c) and [active update caller15D7C0](../analysis/map-pack-update/functions/0015d7c0.c). Most virtual callback bodies are absent from this export, even though their actual executable bytes are present and execute successfully.
- Readable reference reconstructions [unknown_2bd0b0.cpp](../tools/reference/halo2-decompiled/src/unknown_2bd0b0.cpp) and [unknown_2bd960.cpp](../tools/reference/halo2-decompiled/src/unknown_2bd960.cpp) help navigate the original. Their subclass `vNN` numbering is displaced by30 in places. Use the literal51-slot original table and original callers as authority, as [engine_peer.h](../tools/reference/halo2-decompiled/include/engine_peer.h) itself explains.

Retail/TU1.1 correspondence is backed by both literal vtables, inspected original instructions and bounded execution. Subsequently recovered TU1.5 bodies were independently inspected and its initialization/start/geometry chain also executed; the table below distinguishes this evidence from live runtime behavior.

## Whole owners and required inputs

| Owner | Retail | TU1.1 | Verified ABI / inputs |
| --- | --- | --- | --- |
| Engine initialization, slot1 | `2BD140` | `2C34D0` | No explicit arguments, AL boolean; ECX engine object is not consumed by the body. Requires scenario and multiplayer-global pointers plus original static default-point data. |
| Round start, slot3 | `2BD200` | `2C3590` | No explicit arguments, AL boolean. Reads options+230 signed16 hill-move seconds and clock+2 signed16 rate; calls complete setter with state,0. |
| Hill choice | `2BD0B0` | `2C3440` | Excluded group is stack argument; `ret4`; signed32 EAX result. Requires real retained RNG state, signed16 discovered count and signed16 group bank. |
| Default points | `2BD960` | `2C3CA0` | No arguments; copies four12-byte points from original pointer`468710`/`46FBB0` to state+1AC/+1B8/+1C4/+1D0. |
| Set/build selected hill | `2BDC70` | `2C3FB0` | Two stack arguments `(state,index)`, `ret8`. Original enumerator fills marker IDs; original spline owner fills all geometry. |
| Copy marker points | `2BDD20` | `2C4060` | ESI polygon, EDX output XYZ bank; plain`ret`. |
| Spline control points | `2BDD70` | `2C40B0` | EAX output four-point bank, ECX input point bank, EDX segment index, polygon on stack; `ret4`. |
| Spline evaluation | `2BDE90` | `2C41D0` | XMM2 parameter t, ESI output point, EDX four control points; plain`ret`. Preserve SSE binary32 operation order and original literal matrix. |
| Full hill geometry | `2BE050` | `2C4390` | EDI polygon and selected index on stack; `ret4`. Whole caller supplies the remaining live registers. |
| Per-player occupancy, slot15 | `2BD2C0` | `2C3650` | Player datum on stack, `ret4`; signed16 player+1B8 timer. Calls predicate with EAX player datum and EDI polygon. |
| Geometric occupancy predicate | `2BE880` | `2C4BC0` | EAX player datum, EDI polygon; AL predicate. Requires source player/unit and actual original object-position provider. |
| Player-mask to team-mask | `2BD330` | `2C36C0` | EBX input mask, EAX result; original pool salt, high-water, stride, data pointer and team bytes. |
| Occupant-change notifications | `2BD460` | `2C37F0` | EAX previous-player mask; next-player mask on stack, `ret4`; original event/player providers. |
| Active engine update, slot18 | `2BD680` | `2C3A10` | No explicit arguments. Original active-player iterator, notifications, scoring, RNG and geometry all participate. |
| Hill scoring | `2BEEE0` | `2C5220` | No explicit arguments. Requires actual player pool/iterator, selected engine table, phase/status/options, clock, score mutation and score-change/stat providers. |
| Hill-dependent traits, slot35 | `2BD9D0` | `2C3D10` | Two stack arguments `(player,trait_kind)`, `ret8`; source base method fallback when timer<=0. |
| Hill kill/death statistics, slot30 | `2BDA60` | `2C3DA0` | Four stack arguments `(attacker,victim,betrayal,award)`, `ret10`; uses signed-positive inside timers, source stat counters1B/1C. |

The full [TU point-copy instructions](../analysis/multiplayer/game-variants/king-engine-20261011/instructions/tu11-002c4060.asm), [control-point instructions](../analysis/multiplayer/game-variants/king-engine-20261011/instructions/tu11-002c40b0.asm) and [evaluation instructions](../analysis/multiplayer/game-variants/king-engine-20261011/instructions/tu11-002c41d0.asm) directly establish their register conventions.

| Inspected corresponding owner | TU1.1 | TU1.5 |
| --- | --- | --- |
| Initialization | `2C34D0` | `2C6F50` |
| Round start | `2C3590` | `2C7010` |
| Default points | `2C3CA0` | `2C7720` |
| Selector | `2C3440` | `2C6EC0` |
| Hill setter | `2C3FB0` | `2C7A30` |
| Copy points | `2C4060` | `2C7AE0` |
| Spline control points | `2C40B0` | `2C7B30` |
| Spline evaluation | `2C41D0` | `2C7C50` |
| Full geometry | `2C4390` | `2C7E10` |
| Per-player occupancy | `2C3650` | `2C70D0` |
| Occupancy predicate | `2C4BC0` | `2C8640` |
| Team mask | `2C36C0` | `2C7140` |
| Occupancy notifications | `2C37F0` | `2C7270` |
| Active update | `2C3A10` | `2C7490` |
| Scoring | `2C5220` | `2C8CA0` |

Sources: the corresponding [TU1.1 full instruction files](../analysis/multiplayer/game-variants/king-engine-20261011/instructions/tu11-002c34d0.asm) and [TU1.5 full instruction files](../analysis/multiplayer/game-variants/king-engine-20261011/instructions/u5-002c6f50.asm), including each table address's neighboring file. Initialization, start and geometry equality are executed below; occupancy/update/scoring entries are inspected correspondences and are not claimed as executed by this probe.

## Data contracts and behavior

Initialization binds the state to multiplayer globals+FC, clears exactly1DC bytes, discovers unique netgame marker types11..18 in scenario row order, and records group=`type-11`. The source does not sort groups. TU scenario pointer`4E7920`, multiplayer globals pointer`4F10B8`, state pointer`526558`, signed16 group count`510AD8` and signed16 eight-entry bank`510AE0` correspond to retail`4E0350`, `4E9AE8`, `51ECC8`, `5092E8`, `5092F0`. [Retail init](../analysis/retail/functions/002bd140.c), [TU selector](../analysis/map-pack-update/functions/002c3440.c).

The corresponding inspected TU1.5 pointers are scenario`4ECC20`, multiplayer globals`4F63B8`, state`52D8A0`, count`515DC8`, group bank`515DD0`, options`4F3218` and clock`51DF54`. Its selector's retained RNG pointer is`4F3CD8`, versus TU1.1`4EE9D8` and retail`4E7408`. Every selector invocation advances the actual retained32-bit LCG seed, even if group count<=0; it then scans from the original random first index, excluding the supplied current group. [TU1.5 init](../analysis/multiplayer/game-variants/king-engine-20261011/instructions/u5-002c6f50.asm), [start](../analysis/multiplayer/game-variants/king-engine-20261011/instructions/u5-002c7010.asm), [selector](../analysis/multiplayer/game-variants/king-engine-20261011/instructions/u5-002c6ec0.asm).

The original scenario root uses count+118/pointer+11C for netgame rows with stride20. Each row has XYZ at+00/+04/+08, facing+0C, signed16 marker type+10, team+12, identifier+14, flags+16 and two string identifiers+18/+1C. Type11..18 identifies hill groups0..7. `1A25A0`/retail`19EC40` select matching markers using the actual row fields, not a separate guessed hill tag or a bounding box. The [Xbox scenario schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2Xbox/scnr.xml) provides names; [original enumeration](../analysis/map-pack-update/functions/001a25a0.c) and actual supplied map bytes own the behavior.

The hill setter selects at most16 markers of `type=index+11`, identifier0, any team, and only sets a new even marker count after finding at least4. It truncates an odd count by one and distributes32 samples over the count/2 spline segments using integer differences `floor((segment+1)*32/segments)-floor(segment*32/segments)`. `index=NONE` explicitly writes polygon count0. Enumerating only1..3 markers retains the previous count, so the actual setter can rebuild with stale count; an implementation must preserve this source edge instead of inventing a clearing rule. The full original geometry constructs the closed spline,32 XY samples, center/radius/perimeter and vertical bounds; it also incorporates up to8 identifier1 markers for the vertical range. It preserves original arithmetic, boundary comparisons and source-order markers. The spline uses SSE binary32 operations, while perimeter accumulation follows x87 double intermediates under source FPCW`027F`. It is not equivalent to a rectangle, convex hull or distance-only hill. [Original setter](../analysis/map-pack-update/functions/002c3fb0.c), [original complete geometry](../analysis/map-pack-update/functions/002c4390.c), [actual TU geometry instructions](../analysis/multiplayer/game-variants/king-engine-20261011/instructions/tu11-002c4390.asm).

The polygon occupies state+00..17F:16 marker indices+00,8 segment sample counts+40, marker count+60,32 XY vertices+64, XYZ center+164, radius+170, z minimum/maximum+174/+178 and perimeter+17C. Team elapsed timers are eight32-bit words+180..19C; selected hill+1A0, move countdown+1A4, occupant mask16+1A8 and four default points+1AC..1D8. These offsets are directly consumed by the original [setter](../analysis/map-pack-update/functions/002c3fb0.c), [geometry](../analysis/map-pack-update/functions/002c4390.c), [scoring](../analysis/map-pack-update/functions/002c5220.c) and [default initializer](../analysis/map-pack-update/functions/002c3ca0.c).

Player occupancy uses player pool stride21C, current unit datum+2C, team byte+C0 and signed16 inside timer+1B8. TU slot15 increments the timer with a16-bit store only when the unit exists, options+1128 is0, options mode+C is not4 and full predicate2C4BC0 succeeds. Otherwise it resets the timer0. A source implementation must retain16-bit wrap behavior; occupancy masks/scoring test timer!=0 while trait and kill-stat consumers test timer>0. The predicate consumes the real original unit position via TU`BACC0`/retail`B9DD0`, rejects invalid/eccentric polygon counts, applies inclusive vertical bounds, a strict radial test and the full32-edge crossing test. [Retail occupancy](../analysis/retail/functions/002bd2c0.c), [TU predicate](../analysis/map-pack-update/functions/002c4bc0.c), original TU`2C3650..2C36B3` instructions.

Common variant bytes use their existing original descriptor contracts. King-specific parameters38/39/41/42/101 are variant+F0 bits0/1/2/3/4; parameter40 is signed16 variant+F4 (hill-move seconds). Parameter103 aliases bit3. The engine sees them as options+22C and+230 because its variant begins options+13C. Parameters mean uncontested scoring, team time multiplier, damage, resistance and camouflage on the hill respectively; the original descriptor offsets and literal masks are authoritative. The authored `sily` parameter enum names101 as `UNS. King Invis In Hill` and103 as `UNS. King Dmg. Res. In Hill`; parameter97 is `UNS. Sticky Fuse`, not a King camouflage setting. [Descriptors](../src/source_game_variant_descriptors.inc), [authored parameter enum schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/sily.xml), [original trait callback](../analysis/retail/functions/002bd9d0.c), [original scoring](../analysis/map-pack-update/functions/002c5220.c), [readable flag names](../tools/reference/cartographer/xlive/Blam/Engine/saved_games/game_variant.h).

Scoring first collects occupied-player and occupied-team masks. Bit0 prevents scoring unless exactly one player occupies the hill in FFA or exactly one team occupies it in team play. With bit0 set, empty or contested rejection occurs before team-timer writes, preserving existing timers on that path. The original scoring-admission helper TU`15C930`/retail`15B2F0` additionally requires an engine, lifecycle1 and authoritative status1 except options mode4. Thus a positive inside timer alone does not authorize an award. In FFA, nonzero signed16 inside time divisible by signed clock rate calls the original recording-event owner with `(4,NONE)`, then score+1 and score-change publication if the value changed. The recording call is not a gameplay player-state request. [Scoring owner](../analysis/map-pack-update/functions/002c5220.c), [admission](../analysis/map-pack-update/functions/0015c930.c), [recording owner](../analysis/map-pack-update/functions/00198da0.c).

In team play, the engine increments each occupied team's32-bit timer and zeroes unoccupied teams. Every clock-rate ticks it examines the active occupants of that team. With multiplier bit1 it awards every occupant. Without bit1, greater signed16 inside time wins; equal time chooses the greater low8 datum slot. This tie-break is confirmed by original TU`2C54ED..2C5505`, including the signed comparisons. Each selected member uses source score getter`1A35D0`, complete award`15CE00` with EAX delta1 and player on stack, then source score publication`1A2DD0` if needed. The last awarded player's inside timer is reset to1 at`2C557B`. Do not replace this owner with an unconditional team points-per-second loop. [TU scoring](../analysis/map-pack-update/functions/002c5220.c), [actual score mutation](../analysis/map-pack-update/functions/0015ce00.c), [readable retail reconstruction](../tools/reference/halo2-decompiled/src/unknown_2bd960.cpp).

The active iterator`1A2C60` takes ESI pointing to `{current_player,pool,salted_datum,absolute_slot}`, returns AL and skips occupied records whose current unit datum isNONE. It owns real pool bitmap/high-water/salt behavior. The score getter`1A35D0` takes EDX player datum and returns the source team's score in team play, or the player's score in FFA; score-change comparisons in the scorer therefore use the team score in that branch. Score publication`1A2DD0` takes EAX player plus updated score on the stack and emits the original score-remaining events. These dependencies are complete source owners, not arbitrary score-provider returns. [Iterator](../analysis/map-pack-update/functions/001a2c60.c), [getter](../analysis/map-pack-update/functions/001a35d0.c), [publication](../analysis/map-pack-update/functions/001a2dd0.c).

### Recording and score-notification dependencies

The FFA recorder is retail`1972A0`/TU`198DA0`: EBX player datum, two stack arguments `(4,NONE)`, `ret8`. TU byte flags`518490` and`518491` correspond to retail`510CA0` and`510CA1`; recording disabled, or the second flag set, returns before reading wall time or writing the recorder timestamp. This was executed for flag pairs `(0,0)`, `(0,1)` and `(1,1)` in both pinned images: all six complete calls returned with all12 bytes of flags/index/last-timestamp unchanged, with no replaced services. Actual instructions are decisive here because the exported decompilation presents a final timestamp assignment even on those paths. [TU owner](../analysis/map-pack-update/functions/00198da0.c), [retail owner](../analysis/retail/functions/001972a0.c).

Enabled recording calls TU CRT time`32803E`, retains the literal unsigned comparison `(last_time-now)>3`, samples the player's current or fallback unit position through`19AFD0`→actual object-position owner`BACC0`, and appends a36-byte record through`198C20` to a1000-entry ring at`51D424`, advancing index`518494` modulo1000. The record includes source player slot, `(4,NONE)`, position and relative time from recording start`518814`; timestamp`518498` uses the first wall-time sample. Enabled recording needs these actual dependencies; the disabled-path execution does not establish wall-time, ring or pose behavior. [Position sampler](../analysis/map-pack-update/functions/0019afd0.c), [ring append owner](../analysis/map-pack-update/functions/00198c20.c).

Score publication retail`19F470`/TU`1A2DD0` uses EAX player datum, updated score on stack, `ret4`. It builds category0 events only when `options+18C-updated_score` is exactly10,30 or60. Subtypes are respectively47,9,7 in FFA and48,10,8 when a selected engine exists and options+184 bit0 is set. It records the actual cause player and signed team, with audience/effect fields NONE. Options mode4 suppresses dispatch; ordinary mode1 naturally returns from network publisher`A8A30`, whose network service path is restricted to mode5. Local event delivery still executes`1A2490`. [TU publication](../analysis/map-pack-update/functions/001a2dd0.c), [retail publication](../analysis/retail/functions/0019f470.c), [network publisher](../analysis/map-pack-update/functions/000a8a30.c), [local dispatcher](../analysis/map-pack-update/functions/001a2490.c).

Broadcast delivery uses whole iterator`1A2BA0`, which skips disconnected player records (flags+2 bit1) but does not require a live unit. For each recipient,`1A16F0` tries cause-player, effect-player, cause-team, effect-team and general recipient classes in source order. The complete lookup`1A1780` resolves `matg+16C` through the actual tag table to `mulg` runtime data, then reads category0 block count/pointer at+98/+9C and scans A8-byte event definitions. It matches subtype at+4, recipient class at+6, participant requirements at+10 and exclusions at+12. Real `matg`/`mulg` binding is required even for a player without a local user; supplying a successful lookup return would skip this tag contract. [Recipient iterator](../analysis/map-pack-update/functions/001a2ba0.c), [recipient selector](../analysis/map-pack-update/functions/001a16f0.c), [tag lookup](../analysis/map-pack-update/functions/001a1780.c).

After a matching real definition is found, delivery owner`1A2100` naturally returns if the recipient's signed16 local-user index at player+28 is NONE. This provides a complete source execution path for a fixture using the actual event tags, pool and nonlocal recipients without substituted HUD/audio returns. A bound local user continues into game-over filtering, split-screen filtering, actual text formatting/HUD submission and announcer queue`1A1FE0`; those services remain required to demonstrate visible/audible score notifications. The source tags select notification content and recipients; the scoring thresholds and rule branches above remain executable owners. [Delivery owner](../analysis/map-pack-update/functions/001a2100.c), [announcer owner](../analysis/map-pack-update/functions/001a1fe0.c).

## Original runtime callers

- Retail`1574C0`/TU`158C10` initialize the shared multiplayer globals and call engine slot1. The TU actual indirect call is`158D0C`, followed by an AL check that disables the engine on failure. The TU function is absent from its Ghidra export; its bytes were inspected directly. [Readable retail caller](../tools/reference/halo2-decompiled/src/unknown_157450.cpp), unchanged TU executable above.
- TU`159180` invokes round-start slot3 after shared preparation; TU`15E200` also dispatches slot3 on phase1 for source type4. Both are actual original callers, not inferred slot names. [First caller](../analysis/map-pack-update/functions/00159180.c), [phase caller](../analysis/map-pack-update/functions/0015e200.c).
- TU`15D7C0` executes ordinary per-player updates, then slot15 for each actual enumerated player at`15D8B7`; slot18 follows the player/team updates. This preserves occupancy-before-scoring order. The Ghidra export incorrectly displays player`NONE`; actual instructions push ESI, the iterator's actual datum. [Caller](../analysis/map-pack-update/functions/0015d7c0.c), original TU`15D840..15D8CC` instructions.
- Slot18 at TU`2C3A10` compares occupied masks, invokes change notifications`2C37F0`, publishes dirty mask40, calls scoring`2C5220`, decrements/reloads the moving-hill countdown and selects/builds another group through`2C3440`/`2C3FB0`. A selected valid new hill emits engine event `(6,4)`, writes selected hill+1A0 and publishes dirty mask20. The selection loop has source behavior for invalid candidate shapes; no arbitrary retry cap is present. [Retail complete counterpart](../analysis/retail/functions/002bd680.c), original TU`2C3A10..2C3BAD` instructions.
- TU shared death owner`15E4D0` calls slot30, while trait consumers`15C8B0`/`1615F0` call slot35. These calls need to remain inside their original shared lifecycle/stat/trait paths when bound natively. [Death owner](../analysis/map-pack-update/functions/0015e4d0.c), [first trait consumer](../analysis/map-pack-update/functions/0015c8b0.c), [second trait consumer](../analysis/map-pack-update/functions/001615f0.c).

## Reproducible original-only probe

Run the following block with `.tools/python/bin/python` from the repository root. It writes no fixture/report, mutates no executable, imports no native implementation and installs no service hook. All expected cross-image equality comes from separate complete original executions. Its explicitly isolated ABI inputs do not claim a loaded original world.

```python
import sys, struct, hashlib, json
from pathlib import Path
sys.path.insert(0, 'tools')
from oracle_game_variants import Source, VERSIONS
from inspect_multiplayer import Cache, MAPS
from research_mp_lobby_later_update import NEW, NEW_PIN
VERSIONS = {**VERSIONS, 'u5': dict(path=str(NEW), pin=NEW_PIN, globals=0x4ecc1c)}

SPECS = {
    'retail': dict(init=0x2bd140, start=0x2bd200, build=0x2bdc70,
        scenario=0x4e0350, mp=0x4e9ae8, options=0x4e6948,
        clock=0x510c54, state=0x51ecc8, count=0x5092e8, groups=0x5092f0),
    'tu11': dict(init=0x2c34d0, start=0x2c3590, build=0x2c3fb0,
        scenario=0x4e7920, mp=0x4f10b8, options=0x4edf18,
        clock=0x518444, state=0x526558, count=0x510ad8, groups=0x510ae0),
    'u5': dict(init=0x2c6f50, start=0x2c7010, build=0x2c7a30,
        scenario=0x4ecc20, mp=0x4f63b8, options=0x4f3218,
        clock=0x51df54, state=0x52d8a0, count=0x515dc8, groups=0x515dd0),
}
all_states, all_inputs, hills = [], [], 0
for name in MAPS:
    c = Cache(Path('assets/disc/maps') / (name + '.map'))
    rows = c.block(c.scenario + 0x118, 0x20)
    raw = b''.join(c.data[r:r + 0x20] for r in rows)
    all_inputs.append(raw)
    versions = {}
    for version, a in SPECS.items():
        s = Source(VERSIONS[version])
        g = s.g
        g.mem_map(0x4000000, 0x10000)
        def w(at, value):
            g.mem_write(at, struct.pack('<I', value))
        scenario, mp, options, clock = 0x1000000, 0x2000000, 0x2002000, 0x2004000
        for key, value in [('scenario', scenario), ('mp', mp),
                           ('options', options), ('clock', clock)]:
            w(a[key], value)
        w(scenario + 0x118, len(rows))
        w(scenario + 0x11c, 0x4000000)
        g.mem_write(0x4000000, raw)
        g.mem_write(mp, b'\xa5' * 0x1000)
        g.mem_write(options, b'\0' * 0x1200)
        g.mem_write(clock + 2, struct.pack('<h', 30))
        assert s.call(a['init']) & 255 == 1
        state = s.u(a['state'])
        count = struct.unpack('<h', g.mem_read(a['count'], 2))[0]
        groups = list(struct.unpack('<' + 'h' * count,
                                  g.mem_read(a['groups'], count * 2)))
        expected = list(dict.fromkeys(c.h(r + 0x10) - 11 for r in rows
                                      if 11 <= c.h(r + 0x10) <= 18))
        assert groups == expected
        states = [bytes(g.mem_read(state, 0x1dc))]
        assert s.call(a['start']) & 255 == 1
        states.append(bytes(g.mem_read(state, 0x1dc)))
        counts = []
        for group in groups:
            s.call(a['build'], (state, group))
            b = bytes(g.mem_read(state, 0x1dc))
            states.append(b)
            counts.append(struct.unpack_from('<i', b, 0x60)[0])
        versions[version] = groups, counts, states
    assert versions['retail'] == versions['tu11'] == versions['u5'], name
    groups, counts, states = versions['retail']
    all_states.extend(states)
    hills += len(groups)
    print(name, 'rows', len(rows), 'groups', groups,
          'control_counts', counts, 'cross_image_states', len(states))
print(json.dumps(dict(maps=len(MAPS), hills=hills,
    whole_init_round_start_returns=6 * len(MAPS),
    whole_states_per_image=len(all_states),
    whole_state_comparisons=2 * len(all_states),
    state_bytes_compared=2 * sum(map(len, all_states)),
    input_netgame_rows_sha256=hashlib.sha256(b''.join(all_inputs)).hexdigest(),
    original_state_corpus_sha256=hashlib.sha256(b''.join(all_states)).hexdigest())))
```

Measured:12 maps,57 hills,72 complete init/start returns,81 states per image,162 whole-state pair comparisons and77,112 compared state bytes. Input row concatenation SHA256`a5878a38967e33ad2f85f1b78e58cd867d1f56e9a1f11d9d18ee363a9212dfb4`; state corpus SHA256`e470ef6bb9acdeeb139847e8d2ad1819ca9f2e72e4aa5df3248870b56dddc1f8`. The corpus hash describes one image's81 states, which all three images produce literally. Map order is the literal `inspect_multiplayer.MAPS` tuple. The retail/TU1.1 probe was re-extracted from this document and repeated in a separate process with identical hashes before adding the independently executed TU1.5 image. These are stdout observations, not a frozen independently verified native differential fixture.

## Implementation order and remaining gates

1. Implement and independently freeze the complete original initialization, source-order group discovery, RNG selector and default-point owners. Their real inputs and complete original execution are available now.
2. Implement/freeze the complete setter, marker enumerator and spline geometry, retaining binary32 operation order and original metadata/marker contracts. Use all57 authored hills and boundary/alias cases; source outputs must be independently captured, never generated by the native implementation.
3. Bind the complete geometric occupancy predicate to the actual source unit-position owner, then the per-player timer producer. A semantic player-position callback supplied by a fixture does not prove native world pose ownership.
4. Implement/freeze scoring and active update with real shared admission/score/stat/notification owners. Cover contested FFA, contested teams, multiplier both ways, ties, timer wrap, score cap/round end, moving hills and source replication masks. These owners must execute in their actual update order.
5. Bind all five built-ins and editable settings through normal setup/spawn/combat/objective/score/round/end/HUD/results, including actual local and native UDP flows. King trait callbacks, hill visuals/markers/spawn influence, source replication, final results and multi-round reset remain necessary. No named-variant admission exception closes them.

The probe does not execute a complete shared world update, player allocation, source pose, occupancy, scoring, notifications, HUD/render submission, source replication or a native session. TU1.5 live behavior, map-pack hill coverage, original physical LAN/protocol and Xbox behavior remain unverified by this note. The all39-variant goal remains active and incomplete.

## Literal King callback inventory

Every address below is read from the unchanged original registered vtable. This research reran both original registrations and checked all51 pointers against the report. Separate getter calls returned0 from slot0 and3 from slot39 in both images; neither return is the registered source type4. Repeated shared callbacks do not establish their semantics; unlabelled slots still require their actual original caller contract. These are registration observations, not51 executed gameplay callbacks. Source: [literal registration report](../analysis/multiplayer/game-variants/engine-tables.json).

| Slot | Offset | Retail callback | TU1.1 callback | Established role in this note |
| --- | --- | --- | --- | --- |
|0|`00`|`00099810`|`0031A760`|Literal0 return; caller semantics not established here|
|1|`04`|`002BD140`|`002C34D0`|King initialization|
|2|`08`|`00175F40`|`00187550`|Shared callback; caller role not established here|
|3|`0C`|`002BD200`|`002C3590`|King round start|
|4|`10`|`000A45D0`|`000A1100`|Shared callback; caller role not established here|
|5|`14`|`00072C70`|`0027E760`|Shared callback; caller role not established here|
|6|`18`|`002BD260`|`002C35F0`|Engine event submission|
|7|`1C`|`00072C70`|`0027E760`|Shared callback; caller role not established here|
|8|`20`|`00072C70`|`0027E760`|Shared callback; caller role not established here|
|9|`24`|`0024DC50`|`002B4590`|Shared callback; caller role not established here|
|10|`28`|`00072C70`|`0027E760`|Shared callback; caller role not established here|
|11|`2C`|`00175F40`|`00187550`|Shared callback; caller role not established here|
|12|`30`|`00175F40`|`00187550`|Shared callback; caller role not established here|
|13|`34`|`002BD240`|`002C35D0`|Hill visual submission|
|14|`38`|`002BD820`|`002C3BB0`|Hill and opposing-player markers|
|15|`3C`|`002BD2C0`|`002C3650`|Per-player occupancy timer|
|16|`40`|`000A5750`|`000A1110`|Shared callback; caller role not established here|
|17|`44`|`0024DC50`|`002B4590`|Shared callback; caller role not established here|
|18|`48`|`002BD680`|`002C3A10`|Occupant update, scoring, moving hill|
|19|`4C`|`0024DD50`|`00253EF0`|Shared callback; caller role not established here|
|20|`50`|`0024DD60`|`00253F00`|Shared callback; caller role not established here|
|21|`54`|`00072C70`|`0027E760`|Shared callback; caller role not established here|
|22|`58`|`00072C70`|`0027E760`|Shared callback; caller role not established here|
|23|`5C`|`0024DC50`|`002B4590`|Shared callback; caller role not established here|
|24|`60`|`0024DC50`|`002B4590`|Shared callback; caller role not established here|
|25|`64`|`0024DD40`|`00253ED0`|Shared callback; caller role not established here|
|26|`68`|`00224240`|`00253EE0`|Shared callback; caller role not established here|
|27|`6C`|`0024DD70`|`00253F10`|Shared team friendliness|
|28|`70`|`002BD910`|`002C30C0`|Shared string/tag consumer; full semantics not established here|
|29|`74`|`002DAE30`|`002E13A0`|Shared callback; caller role not established here|
|30|`78`|`002BDA60`|`002C3DA0`|Hill kill/death statistics|
|31|`7C`|`0024DD90`|`00253F30`|Shared callback; caller role not established here|
|32|`80`|`002BDB00`|`002C3E40`|Hill spawn influence|
|33|`84`|`000A5750`|`000A1110`|Shared callback; caller role not established here|
|34|`88`|`002BCC80`|`000A1450`|Shared callback; caller role not established here|
|35|`8C`|`002BD9D0`|`002C3D10`|Hill-dependent traits|
|36|`90`|`00072C70`|`0027E760`|Shared callback; caller role not established here|
|37|`94`|`0024DE40`|`00253FE0`|Shared callback; caller role not established here|
|38|`98`|`002BCC80`|`000A1450`|Shared callback; caller role not established here|
|39|`9C`|`0009A280`|`00320B00`|Literal3 return; shared current-ID consumer|
|40|`A0`|`00175F40`|`00187550`|Shared callback; caller role not established here|
|41|`A4`|`0024E320`|`002544C0`|Shared base state initialization|
|42|`A8`|`0024E360`|`00254500`|Shared base state differences|
|43|`AC`|`0024E490`|`00254630`|Shared base state application|
|44|`B0`|`002BDB40`|`002C3E80`|King replication bank initialization|
|45|`B4`|`002BDB80`|`002C3EC0`|King hill/mask replication differences|
|46|`B8`|`002BDC00`|`002C3F40`|King hill/mask replication application|
|47|`BC`|`0024DE50`|`00253FF0`|Shared callback; caller role not established here|
|48|`C0`|`0024DE80`|`00254020`|Shared callback; caller role not established here|
|49|`C4`|`0024E0E0`|`00254280`|Shared callback; caller role not established here|
|50|`C8`|`00241510`|`00243620`|Shared event token formatting|
