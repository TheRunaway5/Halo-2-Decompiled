# PC combat tags and original executable postprocessing

**Vista's raw combat tags are not its final multiplayer tuning.** The supplied
Vista executable contains a complete multiplayer adjustment path that produces
all **41 destination values of the measured Xbox 1.5 combat postprocessor**.
An unchanged execution over actual Vista Lockout/shared metadata performs 41
stores, 38 actual changes and changes 13 tags. Installed MCC already stores the
same 41 values. The earlier 39/41 MCC result read the wrong two BR fields because
its schema repeats the name `Error Angle` at two offsets. This is not evidence
that MCC's actual BR tuning differs. [Vista execution report](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-measured/report.json),
[corrected stored-field comparison](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/corrected-sample/report.json).

## Original executable owner and caller

The supplied `Halo 2 Vista installer/halo2.exe` has SHA-256
`33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109`.
Its original function at **`0046EFDB`** first reads scenario `+10`; the
multiplayer path requires its 16-bit value to equal 1. It then resolves
`matg+16C`, the first multiplayer Runtime pointer, the Runtime Weapons block
and the Runtime Vehicles block. Weapon identities come from authored slot
indices. Melee, barrel/projectile/impact and grenade detonation references use
the actual referenced tag IDs; tag resolution masks the index to 16 bits. PC
block pointers and resident tag addresses receive their original loader-base
additions. The MP instruction path calls no other function. The complete
function also contains campaign branches, which were retained in the listing
but were not executed by this MP fixture. [Literal complete function](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-measured/0046efdb.asm),
[execution addresses and declared bindings](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-measured/report.json).

Original caller **`00428403`** iterates tags and dispatches fixups for `coll`,
`bipd`, `crea`, `vehi` and `phmo`; after that loop, `004284AC` calls `00427EDD`
and **`004284B1` calls `0046EFDB`**. This is the same structural placement as the
Xbox source caller, which invokes combat postprocessing after its tag-group
fixups and preceding correction callee. This fixture retains the complete
caller instruction listing; it executes the complete combat function's MP
path, without claiming to execute that caller, the preceding fixups or the
whole cache loader. [Original Vista caller](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-measured/00428403.asm),
[original Xbox caller and destination contract](MP_COMBAT_TAG_POSTPROCESS_UPDATES_RESEARCH.md).

## Exact BR fields and combat outputs

Vista `0046F06B` loads the literal float word **`3AE4C388`** from original image
address `007BA86C`. Instructions `0046F073` and `0046F078` store it to the first
BR barrel's **`+74/+78`**. Both values are exactly
**0.001745329238474369 radians**. The name `Error Angle` also occurs at barrel
`+5C`, which is a different field and must not be selected merely by its name.
Installed MCC's actual barrel `+74/+78` already contain `3AE4C388`. The corrected
sample and the Vista oracle were independently checked in original ordered
store order, including all other fields. [Literal Vista stores](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-measured/0046efdb.asm),
[duplicate schema names and offsets](../tools/reference/assembly/src/Assembly/Plugins/Halo2/weap.xml),
[corrected stored-field report](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/corrected-sample/report.json),
[independent verification](../analysis/multiplayer/pc-executable-postprocess-20261011/verification.json).

| Actual destination sample | Vista raw | Vista original MP output | Xbox 1.5 original output | MCC stored |
|---|---:|---:|---:|---:|
| BR first barrel `+74/+78` | 0.005235987715 / 0.01047197543 | 0.001745329238 at both | same | same |
| SMG damage upper endpoints | 4.75 | 4.625 | 4.625 | 4.625 |
| Magnum damage bounds | 8 | 5.5 | 5.5 | 5.5 |
| Frag explosion damage upper endpoints | 150 | 160 | 160 | 160 |
| Plasma explosion damage upper endpoints | 120 | 200 | 200 | 200 |

The table summarizes actual bytes in the two cited reports. Equality means the
four-byte tag values match; it does not imply that final damage application,
aiming, player simulation or every other combat field matches. The exact
ordered 41 destination comparison includes three equal-value stores, so it
represents 41 stores rather than 41 distinct changes. [Original ordered Vista
stores and Xbox comparison](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-measured/report.json).

## Execution inputs and reproducibility

The oracle maps original executable section bytes unchanged. It privately
copies each actual resident tag's original bytes at its authored relative
address plus an explicitly declared base. It assembles a resident table from
exact local entries and the corresponding exact shared entries; every shared
substitution retains matching datum, group and name. It binds the original
tag-table, tag-base, block-base, scenario and globals pointer slots. These are
declared ABI inputs, not evidence that original initialization or the PC cache
loader ran. No metadata tuning field is assigned before execution, no source
instruction is patched, and no callback or platform return is supplied. Every
executed instruction is checked against the unchanged source image.
[Oracle implementation](../tools/pc_postprocess_vista_oracle.py),
[all resident-table substitutions and global bindings](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-measured/report.json).

The function returns normally after **382 original instructions** with its
original stack cleanup. Both runs retain full raw/processed buffers of every
changed tag, ordered stores, exact source/caller listings and input hashes.
Both whole report files have SHA-256
`d26cf8a26248f3c55711060722ab1a627bb4604682a2814f386cc6d5ed98b691`;
every retained evidence file repeats byte-for-byte. Verification also matches
all 41 raw Vista values and processed outputs independently against the
corrected PC sample. [Repeated report](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-repeat/report.json),
[verification](../analysis/multiplayer/pc-executable-postprocess-20261011/verification.json).

```sh
.tools/python/bin/python tools/pc_postprocess_vista_oracle.py \
  --output analysis/multiplayer/pc-executable-postprocess-20261011/vista-measured
.tools/python/bin/python tools/pc_postprocess_vista_oracle.py \
  --output analysis/multiplayer/pc-executable-postprocess-20261011/vista-repeat
.tools/python/bin/python tools/pc_postprocess_verify.py \
  --output analysis/multiplayer/pc-executable-postprocess-20261011/verification.json
```

## MCC executable and remaining boundaries

The inspected installed MCC `halo2/halo2.dll` is a PE32+ x64 image with SHA-256
`de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946`.
A bounded literal constant/reference scan was retained, but it does not recover
or execute MCC's complete map loader or prove absence of later transformations.
Its missing literal Xbox BR constant in this executable is not evidence that
MCC changes those two fields: the actual map data already contains it.
[MCC scan scope and source hash](../analysis/multiplayer/pc-executable-postprocess-20261011/mcc-literal-constant-scan.json),
[actual corrected MCC metadata](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/corrected-sample/report.json).

The initial frozen execution concerns the original routine's **Lockout-visible
41 combat destinations**. The separate all-map extension below verifies the
same routine across every supplied Vista multiplayer scenario. Neither result
establishes complete Vista/MCC/Xbox tag equivalence. Vista's
campaign branch, other map-load corrections, MCC executable postprocessing,
Cartographer's external runtime modifications and whole live PC/Xbox gameplay
remain outside this execution fixture. The supplied Vista distribution contains
Cartographer components; the hashed executable and cache bytes are the source
for this claim, without asserting pristine distribution provenance. Original
Xbox maps plus original Xbox 1.5 consuming code remain this project's parity
authority. [Corrected comparison inputs](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/corrected-sample/report.json),
[original Xbox source scope](MP_COMBAT_TAG_POSTPROCESS_UPDATES_RESEARCH.md).


## Separate execution across every supplied Vista multiplayer map

A separate oracle now executes unchanged `0046EFDB` over **all 23 supplied Vista
multiplayer maps**. It discovers scenarios by the actual scenario datum in the
tag-index header at `+0C`, validates its exact datum and `scnr` group, and
selects the signed scenario type `+10 == 1`. It does not select a scenario by a
hardcoded map or tag path. Campaign, menu and shared scenario inventory records
remain in the summary as excluded inputs. The original Lockout tool, source
helper, reports and buffers remain unchanged. [All-map implementation](../tools/pc_postprocess_vista_allmaps.py),
[complete input inventory and measured summary](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-measured/report.json).

Every selected map returns normally after **382 original instructions**, makes
**41 metadata stores and 38 actual value changes**, and changes **13 shared
tags**. Across 23 independent executions this is **8,786 instructions, 943
stores and 874 changes**. Every ordered destination value matches the original
Xbox 1.5 combat routine's recorded field contract. This includes Vista-only
`needle` and `street_sweeper`, which have no corresponding supplied Xbox maps
in this comparison. Source services and callback returns supplied: **zero**.
[Measured summary and per-map report paths](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-measured/report.json),
[all-map verification](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-verification.json).

Canonical per-map reports are under
`analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-measured/<map>/report.json`;
repeated reports use `vista-allmaps-repeat/<map>/report.json`. Each ordered store
retains its original instruction, absolute real owner-file path, integer file
offset and tag-relative offset, size, before bytes and after bytes. The records
support applying **only actual metadata stores in original order** to private
input views, while checking each recorded before value. All measured stores in
these supplied maps belong to their actual `shared.map`; the report keeps the
owner explicit rather than assuming that storage arrangement. Full raw and
processed buffers are retained for every changed tag. [Example per-map
report](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-measured/ascension/report.json),
[owner-offset and processed-buffer checks](../tools/pc_postprocess_verify_allmaps.py).

Both top-level reports have SHA-256
`4abc95c9e8663bc06e344344aa977458f5a6c0f75bcd8ccab67f698a9ada99e1`.
All **668 retained files per run** match byte-for-byte. The separate verifier
checks all 943 raw owner-file offsets independently and reconstructs every
processed tag buffer from its ordered stores. [Repeated summary](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-repeat/report.json),
[verification manifest](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-verification.json).

```sh
.tools/python/bin/python tools/pc_postprocess_vista_allmaps.py \
  --output analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-measured
.tools/python/bin/python tools/pc_postprocess_vista_allmaps.py \
  --output analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-repeat
.tools/python/bin/python tools/pc_postprocess_verify_allmaps.py \
  --output analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-verification.json
```

This extension increases actual source-execution coverage of combat
postprocessing. The PC cache loader, preceding fixups, other load-time
corrections, campaign execution, MCC executable transforms and live gameplay
remain outside these fixtures. Metadata and executable inputs remain read-only.
