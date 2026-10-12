# Campaign audio and tutorial host research

Research date: 2026-10-07. This note specifies original behavior for the next Cairo/tutorial script blockers. It does not establish full campaign, audible output, or Xbox hardware parity. No native production source was modified by this research.

## Authority and reproducible evidence

The executable authority is `assets/disc/default.xbe`, retail build `02.09.27.09809`, SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`. Source cross-checks use [halo2-decompiled](../tools/reference/halo2-decompiled) at `5f08630655e3ddc9ac45909090f11ae799fa0c27`, and compact tag layouts use [Assembly](../tools/reference/assembly) at `fd14ef3c46135e9422966c6a64cd926fb0bd23fb`.

- [audio-research.py](../tools/inspect_campaign_audio.py) reads original builtin definitions at `0x4744E0`, the class name pointers at `0x470090`, and exact function ranges from the pinned manifest. [audio-research-retail.json](../analysis/campaign/audio-research-retail.json) records the definitions and all 54 names. `audio-research-<address>.txt` records original instruction bytes; manifest `todo`/`near` labels do not substitute for inspecting those bytes.
- [audio-research-oracle.py](../tools/oracle_campaign_audio.py) executes bounded original numeric/class routines in Unicorn, with x87 control word `0x027F` and MXCSR `0x1F80`. [audio-research-golden.json](../analysis/campaign/audio-research-golden.json) records the original outputs. This does not emulate a sound driver or game session.
- [audio-research-assets.py](../tools/inspect_campaign_audio_assets.py) reads original tutorial/Cairo metadata directly, including compact `snd!` records and `lsnd` phases. [audio-research-assets.json](../analysis/campaign/audio-research-assets.json) includes source cache hashes, class counts, selected full sound records, loop phase references, and objective tag references.

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/inspect_campaign_audio.py
.tools/python/bin/python tools/oracle_campaign_audio.py
python tools/inspect_campaign_audio_assets.py
```

## Original builtin signatures

Type IDs: void=4, boolean=5, real=6, short=7, long=8, string=9. All these functions have flags zero and a null parse callback.

| Opcode | Host | Return and arguments | Evaluator |
|---|---|---|---|
| `239` | `game_is_cooperative` | boolean () | `2A9A30` |
| `23B` | `game_can_use_flashlights` | void (boolean) | `2A9AA0` |
| `267` | `sound_class_set_gain` | void (string, real, short) | `2AA2C0` |
| `268` | `sound_class_set_gain_db` | void (string, real, short) | `2AA330` |
| `2EE` | `objectives_clear` | void () | `2AB7A0` |
| `2EF` | `objectives_show_up_to` | void (long) | `2AB7D0` |
| `2F0` | `objectives_finish_up_to` | void (long) | `2AB810` |
| `372` | `cinematic_enable_ambience_details` | void (boolean) | `2AC400` |
| `386` | `sound_suppress_ambience_update_on_revert` | void () | `2AC9F0` |

## Sound tag classes and routing

The compact Xbox `snd!` definition is **20 bytes**. Its **class is byte `+0x02`**, indexing the 54 class names below. `+0x06` is the signed playback parameter index, and `+0x0D` is the custom playback index. The rest of the layout is: flags u16 `+0`, sample rate u8 `+3`, encoding u8 `+4`, compression u8 `+5`, pitch range base i16 `+8`, range count u8 `+A`, scale index u8 `+B`, promotion index u8 `+C`, extra info i16 `+E`, maximum play time i32 `+10`. [Assembly snd! layout](../tools/reference/assembly/src/Assembly/Plugins/Halo2/snd!.xml).

**Recovered source names are misleading here.** `s_sound_definition.promotion_index` at `+2` is the 54-way class index; its field named `class_index` at `+6` indexes playback parameters. The separately named `sound_get_class` at `18D170` reads **`+0x0D`**, then returns custom playback table `ugh!+0x34` with record stride `0x34`. It is not the lookup for the 54 class gains. [18D170 original bytes](../analysis/campaign/audio-research-18d170.txt), [recovered compact definition](../tools/reference/halo2-decompiled/include/unknown_218ac0.h).

Original final voice gain routine `1251E0` explicitly reads `movsx cx, byte ptr [esi+2]`, calls `127010`, and adds the returned class dB gain to the definition and caller dB gains. `127010` starts with the class fade's current gain and adds current/previous ambience ducking unless the class fade flags suppress it. This is an audio processing input, not a script-only bookkeeping flag. [1251E0 bytes](../analysis/campaign/audio-research-1251e0.txt), [127010 bytes](../analysis/campaign/audio-research-127010.txt), [recovered gain path](../tools/reference/halo2-decompiled/src/unknown_124f90.cpp).

Direct cache examples:

| Sound | Class byte | Class name |
|---|---:|---|
| Cairo `x02_01_mus` | 38 | `cinematic_music` |
| Cairo `x02_01_fol` | 49 | `scripted_cinematic_foley` |
| Cairo `x02_0010_mas`, Johnson cinematic lines | 48 | `cinematic_dialog` |
| Elevator in/loop/out phases | 28 | `device_machinery` |
| Space station big/small-room ambience loop phases | 33 | `ambient_nature` |

These come from the record bytes saved in [audio-research-assets.json](../analysis/campaign/audio-research-assets.json). A tag's directory name is insufficient to derive its class. Every start, loop, end, alternate, and detail phase must preserve its own underlying `snd!` class.

## All 54 original class name slots

There are 38 nonempty names and 16 empty slots. Empty slots contain the empty string in the executable; `unused` is an explanatory plugin label, not a matchable retail name.

| Index | Exact name | Index | Exact name |
|---:|---|---:|---|
| 0 | `projectile_impact` | 27 | empty |
| 1 | `projectile_detonation` | 28 | `device_machinery` |
| 2 | `projectile_flyby` | 29 | `device_stationary` |
| 3 | empty | 30 | empty |
| 4 | `weapon_fire` | 31 | empty |
| 5 | `weapon_ready` | 32 | `music` |
| 6 | `weapon_reload` | 33 | `ambient_nature` |
| 7 | `weapon_empty` | 34 | `ambient_machinery` |
| 8 | `weapon_charge` | 35 | empty |
| 9 | `weapon_overheat` | 36 | `huge_ass` |
| 10 | `weapon_idle` | 37 | `object_looping` |
| 11 | `weapon_melee` | 38 | `cinematic_music` |
| 12 | `weapon_animation` | 39 | empty |
| 13 | `object_impacts` | 40 | empty |
| 14 | `particle_impacts` | 41 | empty |
| 15 | empty | 42 | empty |
| 16 | empty | 43 | empty |
| 17 | empty | 44 | empty |
| 18 | `unit_footsteps` | 45 | `cortana_mission` |
| 19 | `unit_dialog` | 46 | `cortana_cinematic` |
| 20 | `unit_animation` | 47 | `mission_dialog` |
| 21 | empty | 48 | `cinematic_dialog` |
| 22 | `vehicle_collision` | 49 | `scripted_cinematic_foley` |
| 23 | `vehicle_engine` | 50 | `game_event` |
| 24 | `vehicle_animation` | 51 | `ui` |
| 25 | empty | 52 | `test` |
| 26 | `device_door` | 53 | `multilingual_test` |

The setter performs case-sensitive **substring** matching using `strstr(class_name, request)`, skipping empty class name slots. `machinery` selects 28 and 34; `music` selects 32 and 38; `cinematic` selects 38, 46, 48, 49; an empty request selects all 38 nonempty slots. An unmatched request selects nothing. [221980 bytes](../analysis/campaign/audio-research-221980.txt), [original selection fixture](../analysis/campaign/audio-research-golden.json).

## Gain conversion and fades

`sound_class_set_gain` converts the real linear gain with original `2197F0`; the dB variant passes its real argument directly. Both convert the **signed short duration from script ticks to seconds** using a float `1/30`. Cairo's duration 15 means 0.5 seconds. [267 evaluator](../analysis/campaign/audio-research-2aa2c0.txt), [268 evaluator](../analysis/campaign/audio-research-2aa330.txt).

For finite inputs, `2197F0` pins linear gain to `[0,1]`, calculates `20*log10(gain)` for positive input, then pins to `[-64,0]` dB. Zero/negative input uses -64 dB. Original x87 `2195F0` pins dB to `[-64,0]`, then evaluates `exp((db*0.05f)*2.3025851f)`. Exact float goldens:

| Input | Original output |
|---|---|
| linear 0 or negative | -64 dB, float bits `C2800000` |
| linear 0.001 | -60 dB |
| linear 0.01 | -40 dB |
| linear 0.1 | -20 dB |
| linear 0.5 | -6.020599842071533 dB, bits `C0C0A8C1` |
| linear 1 or greater | 0 dB |
| -64 dB | linear 0.0006309571908786893, bits `3A2566D2` |
| -32 dB | linear 0.025118861347436905, bits `3CCDC612` |
| -6.0206 dB | linear 0.5 |
| 0 dB or greater | linear 1 |

[2197F0 bytes](../analysis/campaign/audio-research-2197f0.txt), [2195F0 bytes](../analysis/campaign/audio-research-2195f0.txt), [numeric fixture](../analysis/campaign/audio-research-golden.json). Linear zero is a finite attenuation floor, not literal digital silence.

Each of the 54 saved fade records is 16 bytes: target float `+0`, current float `+4`, remaining seconds float `+8`, flags byte `+C`. Setter `221980` pins target to `[-64,0]`, stores `max(requested_seconds,0)`, and updates current immediately only when the **raw requested seconds equal zero**. Negative duration stores zero time but preserves current until a positive update.

Original update `221900` does nothing when `dt <= 0`. For each class:

```cpp
if (remaining > dt) {
    current = (target - current) * (dt / remaining) + current;
    remaining -= dt;
} else {
    current = target;
    remaining = 0;
}
```

These are ordered single-precision SSE operations, without FMA. This interpolates **dB**, not linear amplitude. An interrupted fade begins from its existing current value. In the original fixture, fading 0 to -64 dB over 0.5 sec yields -12.80000019 at elapsed 0.1, -32 at 0.25, -57.59999847 at 0.45, and -64 at 0.5. Interrupting at -32 with a new 0 dB target over 0.5 sec preserves -32 immediately and reaches -16 after 0.25 sec. [221900 bytes](../analysis/campaign/audio-research-221900.txt), [fixtures](../analysis/campaign/audio-research-golden.json).

The fade clock is the **audio system's elapsed wall time**, not the campaign simulation tick count. `125B40` passes `sound_system+0x1E0` to `221900`; `1269F0` derives that field from `GetTickCount()` milliseconds times 0.001. Pausing/resetting the original audio clock has its own lifecycle. [125B40 bytes](../analysis/campaign/audio-research-125b40.txt), [1269F0 bytes](../analysis/campaign/audio-research-1269f0.txt).

Native integration needs the class index retained in encoded and decoded sounds, with dynamic class gain applied to **every current voice at mix time**. A start-time gain multiplier alone misses later fades. Concatenating phases into one PCM object must retain phase/class boundaries. The original final channel driver pins combined voice dB at -64 before output, quantizes DirectSound volume, and uses a 0.1 dB change threshold; isolated class multiplication is not complete driver parity. [21FA80 bytes](../analysis/campaign/audio-research-21fa80.txt), [volume conversion](../tools/reference/halo2-decompiled/include/unknown_21e230.h).

## Cinematic ambience details and revert

`cinematic_enable_ambience_details` writes the saved loop globals byte `g_4ED288+0x240`. Original cinematic start `13BFF0` sets that flag true. [372 evaluator](../analysis/campaign/audio-research-2ac400.txt), [13BFF0 bytes](../analysis/campaign/audio-research-13bff0.txt).

The flag gates **random loop detail sounds**, leaving base ambience tracks playing:

1. Portal ambience selection `18AE80` permits details only for the primary choice (`i==0`) and when no cinematic is active or flag `+0x240` is true. False sets portal loop flags bit 7 (`0x80`).
2. Object loop update `18A7B0` translates bit 7 into looping-engine flags bit 2 (`4`).
3. Looping engine update `21B0E0` schedules `lsnd` detail sounds only when state is not stopping and `!(sound->flags & 4)`. Detail sounds have authored period bounds and alternate-state conditions. Base start/loop/end tracks are processed independently.

[18AE80 bytes](../analysis/campaign/audio-research-18ae80.txt), [18A7B0 bytes](../analysis/campaign/audio-research-18a7b0.txt), [21B0E0 bytes](../analysis/campaign/audio-research-21b0e0.txt), [portal path source](../tools/reference/halo2-decompiled/src/sound_looping_paths.cpp), [detail scheduling source](../tools/reference/halo2-decompiled/src/unknown_12a1b0.cpp). Muting all ambience/music when this flag is false would change original behavior. Merely storing the flag without a detail scheduler leaves this dependency unfinished.

`sound_suppress_ambience_update_on_revert` sets saved byte `g_4ED288+0x241` true. It is **one-shot**. Restore callback `188AC0` performs broader loop/promotion/output state repair, then if restore flags contain `0x20`, calls `18BB80(FLT_MAX)` unless this byte is true. It clears the byte in both branches, including restores without `0x20`. `18BB80` rebuilds camera/listener portal ambience and sound environments. This host suppresses that immediate ambience rebuild; it is not a command to stop every scripted impulse and loop. Full restore loop preservation depends on saved loop flags and restore bit `0x40`, and is not reproduced by a generic blanket audio reset. [386 evaluator](../analysis/campaign/audio-research-2ac9f0.txt), [188AC0 bytes](../analysis/campaign/audio-research-188ac0.txt), [18BB80 bytes](../analysis/campaign/audio-research-18bb80.txt), [restore source](../tools/reference/halo2-decompiled/src/unknown_03d380.cpp).

## Tutorial cooperative mode and flashlight permission

`game_is_cooperative` returns `game_options->state == 1 && game_options->byte134 != 0`. A native single-player campaign returns false because its actual mode is single player; this must change when a real cooperative session exists. [239 evaluator](../analysis/campaign/audio-research-2a9a30.txt).

`game_can_use_flashlights(boolean)` changes saved light globals byte `g_5107E8+0` only when the value changes. It records current game time at `+4` if active-game flag `game_options+0x1120` is set, otherwise zero. Repeated requests for the same value preserve the previous timestamp. Original light fade query `C18E0` uses `(current_tick-saved_tick)*tick_duration`, caps at one second, evaluates curve 5 through `17CA10`, and returns that curve when allowed or `1-curve` when disallowed. Global light-system enabled flag `+8` is separate. Permission plus timestamp is required saved state, but does not by itself implement a flashlight light, input toggle, battery behavior, or render parity. [23B evaluator](../analysis/campaign/audio-research-2a9aa0.txt), [C18E0 bytes](../analysis/campaign/audio-research-c18e0.txt), [curve evaluator](../analysis/campaign/audio-research-17ca10.txt), [light source](../tools/reference/halo2-decompiled/src/unknown_0bffa0.cpp).

## Original objective state and visible output

The original allocates five saved 32-bit objective states: 0 hidden, 1 active, 2 completed. [saved objective allocation and helpers](../tools/reference/halo2-decompiled/src/unknown_22c033.cpp).

- `objectives_clear` writes all five states zero.
- `objectives_show_up_to(index)` accepts 0 through 4 and visits every slot 0 through index inclusive. Only hidden 0 changes to active 1; completed 2 remains completed.
- `objectives_finish_up_to(index)` accepts 0 through 4 and writes every slot 0 through index inclusive to 2, even previously hidden objectives.
- Invalid negative or greater-than-4 indices do nothing. This is validation, not clamping the request to a valid endpoint. Query `22C075` returns zero for invalid indices.

[clear bytes](../analysis/campaign/audio-research-2ab7a0.txt), [show bytes](../analysis/campaign/audio-research-22c041.txt), [finish bytes](../analysis/campaign/audio-research-2ab810.txt), [query bytes](../analysis/campaign/audio-research-22c075.txt).

Original pause UI `23223D` reads the scenario objective `unic` tag datum from `scnr+0x3BC`. It renders only nonzero objective states, with U+2610 and a tab for active objectives, U+2611 and a tab for completed objectives, then localized objective text and CRLF. Slot-to-string-ID helper `232371` maps slots 0..4 to `0100030B`..`0100030F`. The combined UTF-16 buffer is bounded to 256 code units. Actual tags: tutorial `E8F4077F`, `ui\\screens\\singleplayer\\01a_tutorial`; Cairo `81D51F9B`, `ui\\screens\\singleplayer\\01b_spacestation`. Their compact language-list records are saved in the cache evidence. [pause renderer bytes](../analysis/campaign/audio-research-23223d.txt), [string-ID bytes](../analysis/campaign/audio-research-232371.txt), [pause source](../tools/reference/halo2-decompiled/src/unknown_23068b.cpp), [cache records](../analysis/campaign/audio-research-assets.json).

Native objective host state must be checkpointed and consumed by the actual pause UI with those original localized strings. A five-state array without visible objective text resolves script evaluation but leaves player-facing campaign behavior incomplete.

## Checkpoint ownership of class fades

**The complete 54-element fade array is saved game state.** Original initializer `221850` reads arena base at `4E6080` and CPU allocation offset at `4E6084`, returns their sum, increments the offset by **`0x360`** (54 times 16), checksums that allocation size, zeroes the region, and writes its address to `g_502118`. This is the inlined `123D40` game-state allocator. The global pointer itself is outside the saved arena, but all pointed target/current/remaining/flags records are inside it. [221850 original bytes](../analysis/campaign/audio-research-221850.txt), [123D40 bytes](../analysis/campaign/audio-research-123d40.txt), [arena allocator source](../tools/reference/halo2-decompiled/src/unknown_123b30.cpp).

The original checkpoint cache writes the game-state arena and later reads it back into the same base allocation. Restore `123F60` invokes `199520` before the arena read and `199540` after it. The former dispatches callback table `g_46E320[5..9]`, the latter `[10..29]`. Saved evidence includes the table's actual executable pointer values; the sound callbacks are pre-read index 7 `1889D0`, post-read index 12 `1264C0`, and post-read index 13 `188AC0`. **None resets the class fade array.** The post callbacks repair playback pools, hardware timing/output, and ambience; class target/current/remaining/flags retain the checkpoint values. [checkpoint restore bytes](../analysis/campaign/audio-research-123f60.txt), [cache read bytes](../analysis/campaign/audio-research-2152e0.txt), [pre dispatcher](../analysis/campaign/audio-research-199520.txt), [post dispatcher](../analysis/campaign/audio-research-199540.txt), [raw callback table](../analysis/campaign/audio-research-retail.json), [sound post callback](../analysis/campaign/audio-research-1264c0.txt).

Class lifecycle is separate from checkpoint restoration. Module table `g_440DF0` row **42**, address `4413D8`, has initializer `221850`, disposal `2218F0`, new-map initialization `2218A0`, old-map disposal `2218E0`, and null BSP callbacks. `2218A0` sets external enable flag `g_50211C` and zeros each target/current/remaining field; it **does not write the flags byte**. `2218E0` clears that external enable flag, and `2218F0` clears the pointer. The null final lifecycle slots are BSP slots, not restore callbacks. [class lifecycle table](../analysis/campaign/audio-research-retail.json), [new-map bytes](../analysis/campaign/audio-research-2218a0.txt), [dispose-map bytes](../analysis/campaign/audio-research-2218e0.txt), [dispose bytes](../analysis/campaign/audio-research-2218f0.txt).

New-map object-loop initialization separately invokes `188D60`, conditionally setting every named class to -64 dB immediately and scheduling a fade to 0 dB over two seconds. This is not part of ordinary checkpoint restore. Its requested `scripted_dialog_player` exception matches none of the 54 original names. [188D60 bytes](../analysis/campaign/audio-research-188d60.txt), [object loop lifecycle source](../tools/reference/halo2-decompiled/src/unknown_1887d0.cpp).

For the native mixer, a checkpoint must capture the actual mixer-owned **current gain and remaining time**, not only the latest script target/duration. Restore those records atomically with the checkpoint, then rebase the audio elapsed-time clock so disk/read delay does not consume the remaining fade. Reinitializing classes on restore, replaying the original full-duration requests, or retaining the pre-restore current mixer gains each changes saved behavior. The original post callback `1264C0` rebases the wall-clock timestamp and clears ambience ducking selection; it does not conflate those operations with replacing the saved class fade records.

## Compact looping detail layout and scheduling dependencies

The Xbox `lsnd` tag is `0x2C` bytes. Detail block count is u32 at **`+0x24`**, pointer at **`+0x28`**, and each compact detail record is **`0x34` bytes**. The underlying `snd!` retains its own class byte. [Assembly lsnd layout](../tools/reference/assembly/src/Assembly/Plugins/Halo2/lsnd.xml), [original decoded detail records](../analysis/campaign/audio-research-assets.json).

| Detail offset | Type | Meaning |
|---:|---|---|
| `00` | u32 | String ID name |
| `04` | u32 | Sound reference group |
| `08` | datum | Sound reference (`FFFFFFFF` means absent) |
| `0C`, `10` | float pair | Random period bounds in **seconds** |
| `14` | 4 bytes | Unrecovered field; do not invent a gain interpretation |
| `18` | u32 flags | Bit 0: exclude with alternate; bit 1: exclude without alternate; bit 2: initially immediate |
| `1C`, `20` | float pair | Random yaw bounds, radians |
| `24`, `28` | float pair | Random pitch bounds, radians |
| `2C`, `30` | float pair | Random distance bounds, game world units |

Original creation `21AEB0` initializes one deadline per detail. With flag bit 2 it uses zero delay; otherwise it draws between period bounds using the game RNG. It stores a signed integer millisecond deadline from `delay_seconds*1000 + audio_clock_ms`. Engine `21B0E0` schedules only when the sound reference exists, the deadline is **strictly less than** the audio clock, the parent loop is not stopping, detail-suppression flag 4 is clear, and alternate-state flag conditions permit it. Initial zero delay is therefore eligible when the audio clock moves past the creation time. It reschedules after sound submission succeeds **or** the submission reports reason 1; other failures leave the previous deadline. [creation bytes](../analysis/campaign/audio-research-21aeb0.txt), [scheduler bytes](../analysis/campaign/audio-research-21b0e0.txt), [scheduler source](../tools/reference/halo2-decompiled/src/unknown_12a1b0.cpp).

Position helper `21CE60` draws distance, then pitch/yaw when distance is nonzero, and uses `(cos(yaw)*cos(pitch)*distance, sin(yaw)*cos(pitch)*distance, sin(pitch)*distance)`. Zero distance returns the zero offset. Preparation `21CC60` attaches spatial details to the parent source; for a nonspatial parent it selects an active local camera and positions the detail relative to that camera. These are requests through the ordinary sound submission path, not PCM samples directly mixed by the looping script host. [offset bytes](../analysis/campaign/audio-research-21ce60.txt), [detail preparation bytes](../analysis/campaign/audio-research-21cc60.txt).

Concrete Cairo metadata: `spacestation_bigroom` has **eight** details, including `hits` with period 5..12 sec, `stereo` 15..35 sec, and `distant_machine` 5..25 sec with initially-immediate flag 4. Its `ambient_clang1` and muffled explosions are also initially immediate. `spacestation_smallroom` has **five** details. Their sounds are class 33 `ambient_nature`; the `_no_detail` loop tags have zero detail records. All selected record bytes, period/angle/distance bounds, flags, and underlying sound class records are retained in [audio-research-assets.json](../analysis/campaign/audio-research-assets.json).

## Implementation boundaries still open

This research makes the above script hosts concrete. It does not recover every Xbox channel effect, ducking trigger, listener portal traversal, random permutation/detail scheduler, spatial attenuation/obstruction, device-emitted sound lifecycle, full audio restore policy, flashlight rendering, or localized pause-screen implementation. These remain separate production/runtime requirements. For verification, compare native fade state against the bounded goldens, exercise fades on an already-playing voice and a newly created voice of another class, verify phase transitions preserve classes, replay a real cinematic skip/revert with audio and objective state, and inspect actual audible/visible output. None of those establishes an entire campaign playthrough.
