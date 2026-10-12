# Original control-to-locomotion-to-pose owners in TU1.5

The missing native animation seam starts with the source player command producer, before the already ported locomotion classifier. Current `Simulation::step_impl` directly passes `Controls.forward/-right` into native movement; it does not construct the source command/control/physical request chain. The multiplayer renderer samples `player.idle` for both the body and carried hand. It has no live consumer of the source primary/secondary selection, clocks or current pose. This inspection explains why the present implementation cannot express source-selected walking, firing and reload body animation. It does not capture the user's running renderer or establish its particular idle/bind-pose fallback.

This report freezes **static original instruction correspondence**, not a natural moving-player execution in TU1.5. A separate command-axis experiment will retain that distinction. The unchanged allocated-owner neutral pose and declared selector evidence remain in [MP_SPARTAN_OWNED_POSE_RESEARCH.md](MP_SPARTAN_OWNED_POSE_RESEARCH.md), [MP_COMMAND_INPUT_RESEARCH.md](MP_COMMAND_INPUT_RESEARCH.md) and [SPARTAN_EQUIPPED_ANIMATION_1_1_RESEARCH.md](SPARTAN_EQUIPPED_ANIMATION_1_1_RESEARCH.md).

## Authority and independent reproduction

The original TU1.1 image is `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. The separately authenticated owned English TU1.5 image is `/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`, SHA256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. The fresh unseeded Ghidra inventory supplies candidate starts only. User annotation names/comments are not used.

```sh
.tools/python/bin/python tools/research_mp_live_locomotion_owners_u5.py \
  --output analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/reproduction/report.json
.tools/python/bin/python tools/research_mp_live_locomotion_owners_audit_u5.py \
  --directory analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/reproduction
```

| Frozen artifact | SHA256 |
|---|---|
| [Correspondence tool](../tools/research_mp_live_locomotion_owners_u5.py) | `82db82bdbbf29b269fdfca6a95cf798d41c591d98f400e5d956c01901bc1627d` |
| [Independent original-byte auditor](../tools/research_mp_live_locomotion_owners_audit_u5.py) | `40acd5255a1ba4e9456202a36b09298fdf14fbd3ddcced960567bbd9a7411547` |
| [Complete measured correspondence](../analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/report.json) | `104075c71753a770ba82863a63aa793248b5bad418e397a76eb363c619b82fa8` |
| [Literal-byte audit](../analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/audit.json) | `104be72d73ba62c40296c1f4892cd8f2fd5ed2acfb3eb69bbb9c0415ef44fa31` |

All27 complete owners have exactly one full normalized instruction/internal-branch-topology match. The audit verifies12,392 literal original instruction rows,183 absolute image-memory operands,303 direct call sites and all23 targets of3 actual jump tables. Every operand retains actual old/new address, size and initial bytes. Every call retains actual old/new site and target; calls to another independently mapped owner must agree. External helper equivalence is not inferred. The adjacent manifest records byte-identical fresh-process report, audit and54 instruction-file reproduction. Earlier reports outside `complete/` are excluded drafts and retain their bytes.

Some genuine entries are omitted by heuristic Ghidra starts. The tool therefore searches actual encoded opcode prefixes, masking only image-address fields, then verifies the entire routine and internal branches. This finds `DF880`, `1F9BF0` and `114750`; it does not use an annotation label to assert their identity. `30BD0` has two prefix candidates but one complete match.

## Exact owner chain and retained ABI

| Owner | TU1.1 | TU1.5 |
|---|---|---|
| Current-player/unit movement admission | `151A20` | `152E00` |
| Flattened aim/fallback basis | `151980` | `152D60` |
| Command pair to motion vector | `151AA0` | `152E80` |
| Whole player command to unit control | `151C90` | `153070` |
| Literal control copier | `C8140` | `C92C0` |
| Normal unit action wrapper | `C7E90` | `C9010` |
| Biped motion/request/animation owner | `DE960` | `DF880` |
| Unit motion parameter owner | `E1800` | `E2720` |
| Physical request builder | `1E9540` | `1EDE60` |
| Physical result and activity consumer | `E3B20` | `E4A40` |
| Physical result producer | `1E8F60` | `1ED880` |
| Physical-state classifier dispatch | `1E94E0` | `1EDE00` |
| Measured state1 locomotion classifier | `1F52F0` | `1F9BF0` |
| Classification to activity request | `E44C0` | `E53E0` |
| Request and channel clock owner | `112E10` | `113D30` |
| Channel update | `1CF100` | `1D39C0` |
| Unit local-pose callback | `113830` | `114750` |

The report additionally maps `E7B10/E5680/30BD0/1D10B0` and six channel clock/selection leaves. [Complete old/new listings and call/operand tables](../analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/report.json).

`151C90/153070` receives EAX=full player datum and stack `(unit_datum, command5C_pointer)`, RET8. It produces a look vector with original x87 cosine/sine from command+4/+8. It passes the actual command+0C/+10 pair to `151AA0/152E80`, then builds the7C control and calls `C8140/C92C0`. The latter copies control+14/+18/+1C to unit+1B0/+1B4/+1B8. The source request is therefore an admitted control vector, not native world velocity.

`151AA0/152E80` receives EAX=player datum, EBX=command+0C pair, stack `(aim12, world12_output, local12_output)`, RET12. Its true admission path resolves the player's current unit and uses the actual unit forward basis at+70. It flattens/normalizes the aim and basis, forms a side vector `{-Y,X,0}`, projects the command pair through that basis and recomputes local X/Y. It clamps a pair whose squared magnitude exceeds original1.0 using its own x87 square-root/division path. Its false admission branch retains the original raw command pair. All instruction ordering and signed-zero operations are retained in the listings; no reconstructed expected vector is asserted by this static work.

`151A20/152E00` admits that transformation only when the player current datum exists, the object type selects bit0, the unit parent+14 is NONE, unit+3DC byte is1, and genuine `E5680/E65A0` returns false. `151980/152D60` sets Z=0 and executes actual `30BD0` x87 normalization; its zero/unordered branch tries the actual unit forward and then the source fallback vector. These are state/pose producers and actual branch gates. Native `Body.grounded` is not established as equivalent to physical state1.

## Upstream deadzone that the standalone classifier does not own

The full biped owner reads unit+1B0/+1B4/+1B8, squares each scalar with SSE and accumulates `(X²+Y²)+Z²`. At TU1.1 `DEBF5`, and TU1.5 `DFB15`, it loads the original binary32 word **`3C23D70B`** from **`4654C0 → 46A560`**. It compares threshold against that squared magnitude with COMISS. The following JBE preserves the vector for magnitude at/above the threshold and unordered comparison; the other branch copies the actual source zero vector through its source pointer into all three unit fields. [Full DE960/DF880 listings](../analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/instructions/000de960-u5.asm).

The standalone native classifier's absence of an epsilon remains correct for its leaf. A live composition that passes Controls directly to that leaf would omit this owning deadzone. Its threshold must come from the supported executable and its zero-vector identity/stores from the actual owner; a guessed cutoff or world-velocity-to-clip shortcut would not reproduce this chain.

## Minimum production seam and explicit missing inputs

The next bounded native owner should accept the literal source command axes/aim plus actual source player/current-unit admission, unit forward basis and required physical mode fields. It should emit the source control7C and preserve `C8140` store ownership. The following biped owner must apply its original deadzone before constructing the physical request and dispatching the existing classifier. Activity request, primary/secondary graph selection, channel clock, local sampler and current matrices remain consecutive source producers; the renderer should consume their shared pose output for body, hand and posed collision.

Current `Controls` provides host forward/right/yaw/pitch/button values. The natural original device conversion, local member ownership, scheduler/map readiness and original non-neutral command admission are still open. An explicit declared command-axis fixture is useful but does not prove those host fields' exact original sign/scaling/quantization ownership. Current `Simulation::move_unit` uses native triangle/capsule support state; it cannot manufacture original unit+3DC from `Body.grounded` or replace a missing physical state with a convenient1.

The source-owned neutral freeze establishes actual physical state1, idle185 frame0→1,19 changed local channels and all33 changed current matrices on its allocated owner. Older declared-control freezes establish source fire/reload clips and clocks, with different readiness/weapon fixture boundaries. None of this static report executes a new TU1.5 player startup, natural non-neutral device input, finite full-frame clock, positive Havok contact, successive admitted frames or player-versus-player damage. Production and all prior freezes remain unchanged.
