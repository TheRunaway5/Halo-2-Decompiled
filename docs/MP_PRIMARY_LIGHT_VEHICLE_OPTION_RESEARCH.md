# Primary Light Vehicle admission on Lockout

Parameter76 (`4C`) is the original **Primary Light Vehicle** option. The literal
`mainmenu.map` `sily` record at `038D59D4` names
`multiplayer\game_variant_settings\vehicles\primary_land_vehicle`. Its choices
are0 Map Default,1 Warthog,2 Gauss Warthog,3 Ghost,5 Spectre,6 Random,7 None;
choice4 is absent. The original built-in Slayer `goof` row at `038D3F04` has
engine0 and no overrides. The earlier independently executed original
[game-panel report](../analysis/frontend/lobby/game-panel-original.json) and
[fixtures](../tests/fixtures/frontend-lobby-game-original.tsv) retain default76=0;
the serialized `sily` value is not used as the general default authority.

The reported pregame failure was caused by an explicit saved76=1 Warthog
override. The five quick-option rows do not display this vehicle option.
The old blanket native nondefault-option rejection therefore prevented this
custom Slayer configuration from launching, even on Lockout.

## Original consumer

The base executable descriptor `237148` returns type3, byte offset`CD`, scale1.
In the game-options variant at`13C`, that is options byte`209`. The complete
untouched TU1.1 chooser `15BB10` uses the authored netgame equipment row's signed
classification at row`+14`. Its case1, `15BB1E`, reads signed options byte`209`
at`15BB24` and calls `15B780`. Classifications2..7 consume other bytes. The sole
external direct chooser call is `15C19A`, inside whole row-spawn producer
`15C110`, after testing classification1..7. This substitutes the definition for
an existing authored row; it does not synthesize a spawn location.

The authenticated owned TU1.5 image independently contains the same structure:
`15D4F0→15CEF0`, call`15D57A`; case1 at`15CEFE` reads byte`209` at`15CF04` then
calls `15CB60`. The freeze includes complete chooser, primary-light leaf and
row-spawn raw bytes/listings, their switch tables, the raw direct-call operand
inventory and absence of literal chooser-address pointers. These are static
instruction findings; the new execution suite is TU1.1 only. Original source
image hashes are pinned in the report. No human function annotation supplies
the field or branch behavior.

## Actual original execution

Lockout's unchanged scenario contains32 netgame rows with classifications
`0×14,8×16,6,7`. There is **no classification1 row**, no `vehc` collection row
or tag, and zero ordinary vehicle placements. Its vehicle palette has one row;
palette size alone is not a placement or an option-consumer test.

The new [oracle](../tools/oracle_mp_primary_light_vehicle_1_1.py) reuses the pinned
unchanged original `InventoryTrace` environment. It executes complete original
`15C110` for all32 rows independently with options byte`209` set to0 and1.
Both runs produce:

- Zero reads of the option byte, measured with an original memory-read observer.
- Fourteen identical full`C4` allocation descriptors.
- 2,350 identical ordered original writes, including actual retired SIMD bytes.
- 7,890 identical original instruction visits and all800 row-bank bytes.
- Unchanged shared seed`12345678`, an explicitly supplied fixture input.

The two full output JSON files are byte-identical. Their canonical output
digest is `a8c8955d539e71a0afff2c470dee03276a0943f748247b4a96c8ee27a11aff99`.
A second process independently reproduced every report/listing/raw-byte file.
The [freeze](../analysis/multiplayer/integration/slayer-primary-light-vehicle-20261008/report.json)
retains commands, original report and dependencies, native review snapshots,
repeat audit and a checksum manifest. Reproduce into a fresh directory:

```sh
.tools/python/bin/python tools/oracle_mp_primary_light_vehicle_1_1.py \
  --output-directory /tmp/halo2-primary-light-vehicle-repeat
```

The original request producer runs to return, but `B8A30` explicitly returns
unavailable/NONE in the inherited fixture. The isolated100-row runtime bank,
synthetic engine/options/object context and seed are declared inputs. No
ordinary session/world construction, successful allocation, vehicle gameplay,
readiness or platform success is supplied or claimed. No affected
classification1 execution or TU1.5 execution is established by this suite.

## Native integration review

Read-only review of the pinned current production snapshots found the following
contract consistent with this bounded evidence:

- `slayer_rules` retains the exact valid primary-light option, including Warthog1.
- `validate_slayer_map_options` rejects a nondefault choice if **any full imported
  netgame row has classification1**. It does not rely on collection group,
  palette size or a filtered native weapon list.
- The real frontend validates the selected map before publishing its launch;
  absence of the map validator fails closed for a nondefault primary-light choice.
- The Match constructor independently repeats the map check.

Thus Lockout can retain76=1 without changing its original equipment requests.
A map containing classification1 remains unsupported until its vehicle owner
is implemented. This conservative guard can reject a row that a later source
mode-eligibility check would exclude; no claim of exhaustive mode-sensitive
vehicle admission is made. Default0 admission preserves existing map-default
behavior and does not establish a completed native vehicle implementation.
This research changed no production/frontend file and ran no build or GPU job.
