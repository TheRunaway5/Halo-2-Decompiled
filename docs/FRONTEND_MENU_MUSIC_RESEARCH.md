# Original frontend music selection and fading

The previous native title path decoded only permutation zero of the loop sound
and passed that same PCM to `AudioEngine` at every boundary. The retail menu
loop has four complete ASF containers. This omitted every other permutation and
repeated the first section indefinitely while the menu stayed active.

The failing native reproduction is
`analysis/frontend/profile-regression/music-before.cpp` and its
`music-before.log`: after the complete decoded intro and loop zero, the next
4,096 mixed frames repeat loop zero exactly. This reaches the old frontend's
actual decoding/mixing pattern.

## Original authority

`assets/disc/default.xbe` SHA-256:
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
`assets/disc/maps/mainmenu.map` SHA-256:
`1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.

The menu `lsnd` is `sound\ui\main_menu_music\main_menu_music`, flags zero,
one track with flags two, gain −6 dB, fade-in zero and fade-out five seconds.
The intro sound `E5280308` has one permutation. The loop `E5290309` has four,
all with zero skip chance and one complete ASF chunk. There are no alternate
or outro sounds. The original sound-class flags retain permutation history;
bit `0x400` is clear, selecting fade curve one.

These executable/decompiled routines establish the behavior:

- `21B940` draws the initial fixed pitch, then selects the initial sound through
  `219110`.
- `21C190` transitions from the intro to the loop using `219110`.
- `21C400` selects another permutation after the final loop chunk, preserving
  the used mask and previous permutation through `21CB90`/`21CBE0`.
- `219110` draws with the secondary LCG, rejects used permutations, and clears
  a completed bank while retaining the last choice to prevent an immediate
  repeat. This is a random order, not a fixed sequential playlist.
- `146240`/`1462B0` initialize the secondary seed as `time(0) ^ rand() ^
  GetTickCount()`. Original CRT `321F75` uses multiplier `0x343FD`, increment
  `0x269EC3`, and returns the high fifteen bits. `3260DA` initializes new CRT
  thread storage to state one.
- `21BD00` requests the authored fade-out, choosing the curve from the sound
  class. `12A810` converts relative fade times on the first update by adding
  the current audio time plus one. Curve one interpolates power using a square
  root, converts back to dB, and clamps at −64 dB. The five-second request
  therefore has a 5,001 ms endpoint in the tested integer-clock scenario.
- `1293F0` retires a fading voice when fade gain plus caller gain is at most
  −64 dB. The 5,001 ms retirement check uses a controlled unit-gain voice;
  the menu's −6 dB caller gain can cross that threshold earlier on the same
  authored curve.
- `235B9C` records initial playback and each subsequent transition. Unless
  its stored timestamp is zero, another change requires signed elapsed time
  strictly greater than 6,000 ms. Movie exit must request playback through
  this scheduler rather than starting it unconditionally.

Independent full-container probes are saved in `menu-music-containers.json`.
The intro ASF duration is 29.689 seconds. Loop containers are 56.486, 35.390,
75.505 and 149.936 seconds. Here the resident WMA `samples` field equals the
container byte length; it must not be used to truncate decoded PCM by treating
it as an audio sample count.

## Implementation and verification

`MenuMusic` prepares every complete authored loop once and keeps source
selection state across stop/restart. Its prepared callback selects the next
PCM at the mixer boundary without decoding or file I/O. The existing menu
screen policy now requests the authored fade; movie entry retains immediate
stop behavior. Attract thresholds have not been altered by this change.

`tools/oracle_menu_music.py` executes unchanged original instructions with
declared input state. `tools/verify_menu_music.py` checks:

- 1,281 selector calls, including chained retail choices, the intro and 1,024
  controlled bounded count/chance/mirror/history probes.
- All 12,956 ordered LCG stores, selected permutations, used masks and seeds.
- 128 original CRT/secondary-seed initializer cases with supplied platform
  time and CRT thread storage; original integer rand/tick routines execute.
- 6,027 byte-exact fade results and mutated times, including every millisecond
  of the authored fade and 1,024 controlled fade cases.
- A complete native intro, all four complete decoded loop files before a
  repeat, five song boundaries, fade continuity and controlled unit-gain
  retirement at 5,001 ms.
  Every mixed sample is compared with the same complete PCM through the
  reference native mixer and authored gain.
- `tools/oracle_menu_music_scheduler.py` executes unchanged `235B9C` over
  1,036 cases. Only its start/stop backend calls are intercepted. Native checks
  match the original action, logical playing state and stored timestamp,
  including initial playback, early movie skip, exact 6,000/6,001 ms boundaries,
  zero-time initialization and signed counter wrapping.

## Remaining boundaries

The frontend initializes a dedicated secondary stream with the source formula
and host clock inputs. Xbox and host clock values differ; unrelated consumers
and their global ordering have not been reconstructed. The bounded selector
proof establishes identical outputs for identical supplied inputs, not the
same first song on independently timed launches.

FFmpeg decodes the complete original containers. Original Xbox WMA decoder
waveform parity, its driver update cadence, hardware mixing and audible output
have not been compared. Native fades sample the proven integer-clock kernel
against the mixer clock; a full original hardware audio comparison remains
required. Full frontend timing and navigation remain separate comparisons.

The known Choose Profile and virtual-keyboard transitions now resolve channel
five as the retained title (nine) or main menu (six). Original `147F4F` reads
`windows_5[4]`; `149EF3` creates profile ten on channel three, and
`238C21`/`238C69` create keyboard 167 on channel two through `149F49` and
`23784F`/`147F6D`. Those overlays therefore do not replace the timing screen.
The original Create New Profile keyboard disposes profile channel three; the
native navigation stack may retain that profile as history while the resolver
skips it to find channel five. This does not assert that both original overlays
are simultaneously visible.
`223976` still requests the music fade at 69,500 ms and the attract movie at
75,000 ms after the most recent input.
The old native test asserting indefinite profile music was contradicted by
the executable and has been replaced with those exact boundary checks.
`profile-idle-red-test.log` records the failing old behavior.

The original caller at `147AAD` calls `223976`, then `223A21` when due, then
updates music at `147AC0`. It supplies no additional dialog gate. The resolver
walks only the known profile and keyboard overlays to find title or main menu;
an unknown intervening screen is explicitly unresolved. Regression checks
cover both bases, the keyboard over the profile, immediate music continuity,
and each threshold's preceding millisecond.

The traced movie entry (`156090`) and playback end (`1565A0`) contain no UI
window disposal; `147AF1` skips UI drawing while a movie is active. Native movie
playback therefore continues to retain its menu stack. The live original
`original-movie-idle-channels.json` and `original-after-movie-B-channels.json`
captures additionally show keyboard channel two and title channel five retained
through attract playback and its skip. Other movie/context callbacks remain
outside this bounded comparison.
