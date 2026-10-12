# Original sound admission CPU owner

`source_sound_admit` replaces the missing native `128380` admission callback with the original CPU decision sequence. It preserves availability gates, definition/codec checks, one ordered local-RNG advance, the skip comparison, actual listener selection, attenuation and gain checks, and the optional rejection-reason store. It does not initialize audio, supply a local listener, allocate a voice, or connect live reload playback.

The new implementation is [sound_admission.hpp](../include/halo2/sound_admission.hpp) and [sound_admission.cpp](../src/sound_admission.cpp). Its memory provider must retain actual source-owned buffers under the common execution gate. Missing memory or scene occlusion throws at its source phase. No missing callback becomes false, clear-world, initialized, or successful.

## Source authority and native contract

The disc1.1 image is SHA256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`; the authenticated user-owned1.5 image is `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. Seventeen complete bodies and all internal branches pair uniquely, totaling971 instruction rows per image. The core mappings are128380→129740,128320→1296E0,126760→127B20,129450→12A810,129570→12A930,12C370→12D730,126930→127CF0 and223EB0→2287B0. The constructor14CC00→14DFE0 also pairs. [Complete bodies](../analysis/multiplayer/integration/sound-admission-native/paired-v2/report.json), [literal instructions](../analysis/multiplayer/integration/sound-admission-native/paired-v2/instructions/).

`SoundAdmissionState` requires the actual sound system, literal tag table, active matg and ugh, class-fade storage, local RNG storage, alternate codec flag address, first-person/local-player table and current scene. These are mutable owners, rather than immutable executable defaults. The module uses the previously imported eleven `SoundCreateConstants` words and sixteen `SoundDriverParameterConstants` words. Its one additional immutable operand is the occlusion threshold:1.1 VA4651CC,1.5 VA46A26C, bytes00008043. The scene query24BE30 remains a separately required complete provider on the original mismatching-cluster branch. [Exact operand packet](../analysis/multiplayer/integration/sound-admission-native/paired-v2/providers.json).

The entry receives the full raw request address and actual local/reason output addresses. It checks system bytes+78,+79,+7A before looking up the sound. The accepted codec predicate is `(snd[5]==1 && snd[4]!=2) || (snd[4]==2 && actual_alternate_flag!=0)`. The skip test advances local RNG+4 by the original unsigned LCG exactly once. It interpolates the authored ugh+0C row's +0C/+10 bounds using request+10 and multiplies the authored playback-row+08. The random high16 value is scaled on the x87 stack. The threshold comparison is strict and preserves the original control word; a rounded binary32 substitute is not used.

After that test,128320 requires authored pitch-count byte+0A and a nonzero first range permutation count.126760 selects maximum range from actual request flags, sound/class fields or playback metadata.129450 chooses from the actual active listener bytes at system+8E with48h stride, compares the original distance values in order, executes the original x87sqrt/store/divide sequence and applies223EB0. Nonpositional requests scan the four actual local-player slots at table+0C. The first non-NONE slot wins; no native view count or player position is substituted. Positional override flags100h/800h retain their original fallback behavior.12C370 preserves the reversed range/minimum normalization inputs, epsilon gate and separately rounded SSE ratio/product.126930 retains the authored volume interpolation and class-fade clock consumers. The final gain sum includes the authored playback+10 again, in its original addition order. The result flags100h/800h preserve the original gain-gate bypass.

Reason DWORD values are retained as code outputs:5 before the availability/codec gates,1 after RNG/skip entry,4 after a passing skip test and0 only on successful admission. The local output is written only after the original listener call. It remains untouched on earlier rejection. A zero optional reason pointer causes no store.

## What actually executed

The original source startup/class constructors are reexecuted with the unchanged previously measured compatibility context and clock schedule. Full14CC00 then executes its actual allocator/pool and130h-byte local-table constructor. It returns table82D4A804 with four source-written NONE slots. Its57 observed nonstack writes and214 executed preparation instructions are retained.14CCD0 was explored but did not return under the inherited dependency boundary; it is **not** part of the canonical preparation or claimed reset closure. No table pointer, local binding or readiness value is assigned. [Actual constructor preparation](../analysis/multiplayer/integration/sound-admission-native/measured-v5/preparation.json), [executed original bytes](../analysis/multiplayer/integration/sound-admission-native/measured-repeat/instructions/actual14cc00-preparation.asm).

The canonical source experiment uses the actual ready/full-reload/empty-reload raw requests and full23-map backing from the immutable creation study. Old source-created startup/class/local-table buffers are explicitly retained as ABI inputs in both images. The normal map prefix is still stopped at the real absent-driver operation. System availability bytes remain000100 and the four local slots remain NONE. This is not whole1.5 startup or an admitted local-player sound.

The3,864 controls comprise276 complete128380 cold returns,138 explicitly bounded1283F4..12846A/128524 RNG/chance slices and3,450 direct queries. The direct query controls include authored range/codec/gain/class data, declared distance/linear-gain inputs and caller-owned location-type bytes. They do not activate a listener, change a source clock or assign a seed. The chance slice executes unchanged instructions with explicit register/frame ABI and the actual retained RNG; it deliberately starts after availability/codec checks and must never be cited as a positive full admission. No complete positive128380 execution is claimed.

Native normal and GCCASan/UBSan tests match552 ordered original stores and1,881,423,264 complete compared bytes across46 image/map banks. Independent source repeats reproduce every report/listing byte, including a newly executed complete14CC00 preparation. Independent exporters reproduce the exact fixture/meta bytes. Fresh isolated builds preserve strict floating-point options and captured x87CW027F/MXCSR1F80. The production CMake target uses the same actual fixtures; its observed source/archive snapshots and logs are retained separately without claiming a reproducible entire checkout. [Original report](../analysis/multiplayer/integration/sound-admission-native/measured-v5/report.json), [fixtures](../analysis/multiplayer/integration/sound-admission-native/fixtures/fixtures.json), [validation](../analysis/multiplayer/integration/sound-admission-native/validation-v2/native/commands.json).

## Distance-gain correction,2026-10-11

A new dense comparison found a native operand error in12C370's final distance
factor. Original12C450 loads distance from `[ESP+18]`; after `POP ESI`,12C45A
loads minimum distance from `[ESP+0C]`. The factor is
`minimum / max(minimum,distance)`, multiplied by the separately normalized
maximum-to-minimum envelope. The previous native implementation used
`maximum / max(maximum,minimum)`, losing the inverse-distance component.

`tools/oracle_sound_distance_gain.py` executes the unchanged numeric tail in
both owned updates, with no intercepted callee. Authored bounds from all2,575
resident sound definitions across Lockout/Triplicate supply dense interval and
one-ULP boundary controls; additional zero/reversed/infinite controls are
explicit numeric ABI inputs.3,190 of3,543 cases failed before the correction;
all3,543 match bit-for-bit afterward, with identical independently repeated
original reports. The prior broad case counts did not establish this interval
coverage. Frozen checkpoints above remain historical evidence.

The corrected `source_sound_distance_gain` is shared with the full native
admission/query owner. It is registered as `original_sound_distance_gain` with
the paired original fixture and provenance in `tests/fixtures`. This corrects
the CPU calculation; the separate live `MultiplayerAudio` stereo adapter still
requires its actual spatial listener/emitter and platform routing integration.
It is not evidence of complete positional playback or physical speaker parity.

## Genuine original game admission captures2026-10-11

The earlier unavailable-constructor fixtures above remain historical evidence.
A separate experiment now captures initialized game state from the owned base
Xbox executable running in xemu0.8.136. Hardware breakpoints stop the actual
game at126C30 and its actual return address. Each memory page is read from the
stopped guest only when needed. An independent whole-call replay verifies every
executed instruction against the base executable, without instruction patches,
intercepted callees, guest memory writes, supplied readiness, listener bindings
or RNG seeds. Its return value and retained outputs/RNG writes agree with the
game's own subsequent execution.

Six actual requests are retained:

| Original request | Actual result |
|---|---|
| Lockout particulate wind detail | Accepted; positional source |
| Lockout off-in-the-canyon detail | Rejected with reason1; local output untouched |
| SMG idle posing variation | Accepted; local source |
| SMG firing, triggered by normal controller input | Accepted; local source |
| Chief cement footstep, triggered by walking | Accepted; local source |
| Frag grenade metal bounce, triggered by throwing | Accepted; positional source |

The captured shot and footstep requests retain coordinates but have source
type0, so the original chooses a local player and distance0. Wind detail and
grenade bounce use type1 and actual listener distance. Coordinates alone must
not turn a local source into a world source. These captures reach either the
local branch or the same-cluster positional branch; they do not establish the
separate24BE30 mismatching-cluster provider.

The native owner matches all six admission decisions,13 ordered semantic
stores,27 observed internal numeric returns and782,336 complete input/output
page bytes under the captured floating-point control words. Original private
callee-stack and argument scratch are excluded from the semantic native ABI;
caller outputs, RNG and complete retained input pages are compared. The original
game's accept/reject decision is also independently checked at its real return.
The grenade capture makes the previous distance operand error fail at12C370
(`3D5FD900` instead of original `3C9ADFD8`). The corrected owner passes both normal
and ASan/UBSan builds. This extends the numerical regression to an actual game
request rather than an invented listener or request.

`original_live_sound_admission` is registered with the frozen fixture and
provenance in `tests/fixtures/live_sound_admission.original.*`. The capture,
export, complete base-to1.1 body mapping and isolated validator tools are
`capture_live_sound_admission.py`, `export_live_sound_admission.py`,
`research_live_sound_admission_base.py` and `validate_live_sound_admission.py`.
The complete body mapping covers16 owners/888 instruction rows; its unexecuted
scene-occlusion callee remains an explicit external boundary. Evidence is under
`analysis/audio-parity-20261011/live-admission-original-v{4,6,7,8,9,10}`,
`base-admission-paired-v1`, `live-admission-fixture-final` and
`live-admission-validation-final`. Original controller events and GDB commands
are retained in `xbox-spatial-session`.

All14 selected audio CTests pass in the isolated source build. This is positive
original admission evidence, not original speaker output, native spatial mixing,
the complete event/ambience scheduler or a rendered local/LAN playback pass.
The live multiplayer adapter still discards world emitter positions, and the
captured wind detail's schedule remains unported. Those remain required work.

## Remaining live reload handoffs

The shortest complete ownership chain remains the actual local FP clock event→1687C2→18AA90/18AA40/18ACD0→18AD70/18DDE0→18BA10→21F220→128380→128610→127750/cache→ordinary127290/129A20/12AD30 voice assignment→source driver parameter/packet/completion owners→the real SDL backend. The event seam and authored reload sample are available; the source CPU modules do not imply that this whole chain is installed.

The FP semantic request deliberately retains only48 location bytes and40 marker bytes.18BA10's actual rawA0 producer additionally copies20 location bytes and preserves other caller backing. Those bytes must come from the original owning caller, rather than zero-extension.18DDE0 still requires the real weapon/location/PVS/current-node/velocity phase. Complete portable startup must reach its genuine native platform dependency and original commit; complete map activation and real local/listener binding must occur before full128380 can admit. These gates cannot be derived from opening SDL stereo, decoding a reload, or the existence of a native view. Static/shared voice arbitration, routing, packet scheduling and completion remain distinct owners. This checkpoint does not make reload audible or change the launcher/runtime/shaders.

## Reproduce

Use new output directories; preserve the immutable checkpoint:

```sh
.tools/python/bin/python tools/research_sound_admission_u5.py --output <new-paired-directory>
.tools/python/bin/python tools/oracle_sound_admission_native.py --output <new-original-directory>
.tools/python/bin/python tools/export_sound_admission_fixtures.py --report <new-original-directory>/report.json --output <new-fixture-directory>
.tools/python/bin/python tools/validate_sound_admission_native.py --directory <new-validation-directory> --fixture-directory <new-fixture-directory>
sound_admission_tests <absolute-fixtures.tsv>
```

The oracle intentionally consumes the separately pinned canonical paired/source-creation authority. The actual production `original_sound_admission`, `original_sound_create` and `original_sound_submit` CTests pass in both normal and GCCASan/UBSan builds. [Production command and archive record](../analysis/multiplayer/integration/sound-admission-native/production/commands.json), [normal output](../analysis/multiplayer/integration/sound-admission-native/production/normal-ctest.stdout), [sanitizer output](../analysis/multiplayer/integration/sound-admission-native/production/sanitizer-ctest.stdout).

The [final audit](../analysis/multiplayer/integration/sound-admission-native/audit.json) checks12,984 protected prior pins,91 exact repeat file pairs and26,260 original instruction rows with zero mismatches. The [immutable manifest](../analysis/multiplayer/integration/sound-admission-native/final-manifest.json) retains the exact tools, original inputs, snapshots, repeatability and unavailable boundaries. Preliminary incomplete mapping attempts are preserved outside the selected canonical evidence. Source and archives are observed focused-build artifacts; this is not a reproducible full-tree or audible-playback claim.
