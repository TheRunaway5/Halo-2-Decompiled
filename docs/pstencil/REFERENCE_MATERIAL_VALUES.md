# Original material values and current assets

The read-only [extractor](../../tools/pstencil/extract_reference_environment_parameters.py)
uses the primary 2003 tool schema and original 00037CA0/00038120 cache ABI.
It reads the validated 71-cache inventory and selects only resident environment
passes whose complete 240-byte program matches the recovered default program
SHA-256 `40c792b9a3b656e2c1e83bc5ca27fe3e319c7404dc95b5d1dd4cdd5732cd4cac`.

For each exact current/original shader path, it retains explicit V10/V11 RGBA
and combiner midrange constants. Animated overrides, indirect unresolved values,
multiple implementations or missing roles are rejected. All accepted records
for a path must agree. [Evidence](../../analysis/pstencil/reference-environment-parameters/evidence.json)
contains original file hashes, cache build labels, lighting-model/pass selections,
shader/implementation/constant byte offsets and the complete values.

There are **159 unanimous static reference paths**, zero conflicting paths and
zero rejected records in this selection. This is a reference parameter bank,
not proof that all 159 are rendered by the current adapter. The current bounded
independent tangent-space family intersects **12** of those paths: Chief armor
and first-person arms; Elite head/helmet/arms/torso/legs; Banshee; Needler;
plasma pistol; plasma rifle; and the carbine's shiny material. The recognized
current combined visor also uses the original facing/glancing strengths; its
tint and response math remain with the current combined consumer. Other
families remain reference data until their shader interface is supported.

The mode applies these values only to an exact path and a supported selected
pass/control shape. Unknown/later materials retain their authored current
values with the shared 2003 response. No historical alias or faction preset is
invented. Current geometry, base/detail/bump/environment images and player
colors remain current assets. Reference reflection tint is separate from the
base/team color.

Chief armor and arms have identical original values across 70 caches for the
separate tangent-space response: facing RGBA `(.85,.85,.69,.2)`, glancing
`(.43,.57,.62,.05)`, midrange factor `1A000000`, midrange RGB `006E919E`.
Both lighting-model variants agree. These differ from the retained current
material's `.8/.3` brightness. Elite's original facing tint is warm and its
glancing tint purple; these are literal source values, not a red point light.

Reproduce with the local primary build intact:

```sh
.tools/python/bin/python tools/pstencil/extract_reference_environment_parameters.py --generate
```

The generated [bank](../../src/pstencil/reference_environment.inc) preserves
float32 bits explicitly. Tests compare every native record with the independent
offset-bearing extraction and retain the original shader control/camera words.
This recovers static environment values only; original direct-light utility,
complete combined visor math, complete intermediate target production and an
original framebuffer comparison remain separate work.
