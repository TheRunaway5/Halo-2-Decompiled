# Stock SMG muzzle alpha endpoint

The local-four assertion that every submitted muzzle sprite has alpha greater
than zero was too strict. The actual Lockout age curve produces zero alpha
before and at normalized age one. Birth alpha is one. No particle lifetime,
fade curve, emitter definition or production rendering behavior was changed
for this investigation.

Evidence is retained in
[`muzzle-alpha-20261010`](../analysis/multiplayer/integration/muzzle-alpha-20261010/).
`original-detailed.json` and its independent repeat contain 26 untouched
original sprite calls: disc-base `224550` and disc1.1 `2265C0`, each with
13 declared age controls. Only the final graphics submission is intercepted;
the original value completion, property evaluation and color packing execute.
Each row retains the complete 88-byte submission and the actual 16 bytes
passed to the color packer. The two reports reproduce byte-identically.

The authored chain is Lockout `effe E59E0429` normal SMG fire, first-person
system0, `prt3 E5A0042B` center burst, with two emitters. Both emitter alpha
properties are literal constant one. The sprite alpha property reads value
slot0 (particle age). `authored.json` retains the complete property and curve
bytes; `extract_source.py` independently reproduces them. The actual source
sprite bodies are already paired in
[`static-complete/report.json`](../analysis/multiplayer/integration/spawn-source/smg-particle-render/static-complete/report.json).

| Declared normalized age | Original evaluated alpha before final pack | Packed alpha byte |
| --- | --- | --- |
| 0 | 1 | 255 |
| 0.25 | 0.9970588088035583 | 254 |
| 0.5 | 0.9411764740943909 | 240 |
| 0.75 | 0.686274528503418 | 175 |
| 0.9998999834060669 | 0 | 0 |
| 1 | 0 | 0 |

The zero is already present before final ARGB packing; final conversion alone
does not explain it. Original1.1 `22662E` evaluates the alpha property,
`22663A` multiplies the incoming emitter alpha, `2266C3..2266F9` clamps it,
and `226746` calls `1335C0`. The latter multiplies by the original255 word
and uses x87 integer conversion. Original aging owner `249210` statically
compares the newly stored age to one at `2492C2..2492CE`; `JBE` bypasses the
expired-age branch at equality. This is static whole-body instruction evidence,
not a new execution of the complete aging owner.

The isolated native-owner probe loads the actual Cache, SMG effect, material,
authored model/default node pose and muzzle marker. It supplies a native
visual seed, owner identity, identity world/view basis and a normal30Hz clock;
it never assigns particle age, lifetime or alpha. One ordinary effect request
produces two sprites. Its timeline and independent repeat match exactly:

- tick0/fraction0: both alpha1;
- tick2/fraction0.75: both alpha0.290196;
- tick2/fraction1 and tick3/fraction0: both alpha0, still submitted;
- tick3/fractions greater than0: no sprite submission;
- tick4: no retained particles.

The native probe is linked against preserved source-v2 archives and their
matching copied headers in `lan-shot-audio-20261010/late-join-red`, not a fresh
full-tree build. It selects the authenticated user-owned u5 image through
`OriginalData`; this does **not** establish direct execution or complete
correspondence of the u5 sprite/aging routines. The direct original calls here
are base and1.1 only, with declared completed value-cache inputs and an
intercepted graphics leaf. No GPU, shader, pixel or complete world-render
claim is made. These probes were not sanitizer builds.

The preserved failing v3 log contains no particle age, so the exact age of
that failure remains unknown. The evidence proves a valid counterexample to
the strict assertion. A correct runtime regression can accept finite packed
alpha in0..1 while separately requiring visible submissions on multiple real
ticks and eventual expiry. The existing600-lifetime check tests birth and
retirement, so its pass does not contradict a transparent endpoint.

Run `reproduce.py <new-output-directory>` with `.tools/python/bin/python` for
independent CPU reproduction. `manifest.json` pins the new artifacts, source
snapshots, owned inputs and prior archive bank; the two audit files verify
those pins without changing earlier freezes.
