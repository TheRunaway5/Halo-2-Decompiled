# Source local and hierarchy pose arithmetic

The original cold bind path `BD090 -> 16D940 -> 1421F0 -> 142A60` consumes the
authored local quaternion without normalizing it first. `16D940` copies the
four raw quaternion words from model node offsets `18..24`, translation from
`C..14`, and the literal scale1. `BD090` can subsequently replace these locals
through its animation branch; that controller and sampling chronology is a
separate boundary. The local arithmetic helper accepts the actual resulting
local bank, so it does not choose or modify an animation phase.

`1421F0` computes the squared quaternion norm as `(x*x+z*z)+(y*y+w*w)`.
At or below the original float threshold `38D1B717`, it emits identity rotation
and preserves scale and translation. Otherwise it runs `RCPSS`, one Newton
step, doubles the resulting inverse norm, then builds the basis with literal
SSE products, sign-bit XORs, and subtraction grouping. In particular, its
diagonals use `(1-term)-term`; converting a normalized quaternion through GLM
changes this rounding. Scale remains a separate word rather than being folded
into the basis.

`source_model_local_transform(p)` uses binary32 rounded `1/norm` as its
**explicit Unicorn/QEMU reference convention**. The original oracle executes
the original `RCPSS` instruction unchanged. This convention does not prove the
physical Pentium III approximation seed. The overload accepting
`reciprocal_seed` preserves that input explicitly and applies the original
post-instruction arithmetic, allowing a captured or separately established
hardware seed to be consumed without changing the pose kernel.

`source_model_world_nodes` composes the resulting Transform bank with original
`142A60`. It retains unscaled basis columns and separately multiplies scales;
basis sums are `X+(Y+Z)`, while translated points use `(X+Y)+Z`. It accepts the
literal model-order parent bank and rejects nonpreceding parents. Generic
animation remapping may need an explicitly resolved order in its caller.
Roots remain direct locals unless the caller supplies an actual root Transform.
No identity matrix multiplication, scale division, or node-center proxy is
introduced.

The untouched differential runs1,109 cases /7,091 nodes /191,457 exact words.
Its403 literal cold bind models cover4,119 nodes across all21 supplied maps;
original `16D940` verifies32,952 copied words. Controlled cases cover signed
zeros, nonunit/negative scales, small quaternions, an explicit root bank and
640 explicit post-`RCPSS` register-bank nodes. The latter enter untouched
`142273` with declared inputs and prove that arithmetic seam, rather than
claiming those seeds came from hardware. Original function/services and output
values are never replaced. The frozen fixture and provenance are
`tests/fixtures/retail_source_model_pose.tsv` and its adjacent JSON; the
independent runner is `tools/differential_source_model_pose.py`.
The separate raw asset fixture verifies that `Cache::load_model(mode,false)`
preserves all32,952 original copied bind words, plus names and parents, across
the403 models. Native loader normalization must be disabled at the bind-node
call; animation codec normalization sites are outside that narrow correction.
The same asset fixture compares the production `pose_model_nodes` bank against
53,547 original WORLD Transform words for all4,119 nodes. This tests the actual
loader and bind hierarchy together without supplying an animation phase.

The earlier read-only production audit found2,025 literal local transforms
and2,183 hierarchy transforms differing from source arithmetic even when the
same native local Transform inputs were supplied. This helper fixes the
identified producers. Production loader, pose and skin integration is owned by
the main task; the raw asset/WORLD regression directly checks its bind path.
Neither these arithmetic fixtures nor
the narrow earlier4,118 single-node controls establish every live animation,
original scheduling/registration state, physical reciprocal result or GPU
frame equality.
