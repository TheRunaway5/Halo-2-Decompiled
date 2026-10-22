# Complete Halo 2 ONLINE

## First goal — P2P security and Rust migration

User-requested priority change, 2026-10-21: **solve this security goal before
proceeding with the remaining ONLINE work.** The full existing goal and every
unfinished item below remain required. This is the first stage of that same
goal, with detailed acceptance criteria in
[ONLINE security-first goal](online-security-first-goal.md).

The checklist here is authoritative for security completion. The linked
document defines the evidence required for each stable ID; keep both in sync.
Items start open and close only against their stated evidence. Existing
authentication, bounds checks and passing gameplay tests are foundations, not
acceptance of this Rust migration.

- [ ] **S01 — Threat model and attack-surface inventory.** Trace every network
  entry point and native consumer, including hostile authenticated players and
  hosts, LAN/community joins, snapshots, migration, signaling and voice.
  Baseline completed 2026-10-21 for source inventory only: the
  [threat model and migration order](online-security-threat-model.md) account
  for all twelve inspected C++ receive funnels, SDK callbacks, native import
  and backend controls. The [verification receipt](../../.local/online-security-s01-20261021/verification.json)
  pins 59 source/declaration/reference files with no unadmitted drift and checks
  60 local links. It records actual transport/protocol defaults and explicit
  legacy-path, memory-budget, trust and containment gaps. This is not exploit,
  Rust implementation, deployed configuration or release-security acceptance.
  Reopened 2026-10-22: concurrent EOS SDK/transport, callback, session and Rust
  reassembly changes extend that historical baseline. Refresh the receive
  census, credential/sender transitions, native lifetime/dependency inventory
  and direct/relay policy before claiming current combined-source coverage.
- [ ] **S02 — Reproducible Rust build.** Integrate a pinned Rust toolchain,
  locked dependencies and disposable Cargo/CMake validation into all supported
  release platforms and packaging.
  Initial Linux Cargo foundation, 2026-10-21: the scoped
  [protocol module](../../rust/online_protocol/README.md) pins Rust 1.96.0,
  locks its dependency-free package and forbids unsafe Rust. Actual offline,
  locked tests pass all seven voice-framing groups, strict Clippy passes and
  the release library builds; see the
  [source/build receipt](../../.local/online-security-rust-20261021/initial-parser/report.json).
  Its temporary Cargo target tree is removed. CMake integration, the reviewed
  C++ bridge, supported-platform packaging and production receive-path routing
  were pending at that milestone; the following bridge milestone advances the
  Linux build and match-voice route. S02/S03/S04/S09 remain open. Parsed data grants no sender,
  speaker, generation or native-world authority.
- [ ] **S03 — Owned, typed Rust/C++ interface.** Establish a small reviewed
  bridge with explicit lifetimes, errors and cancellation; keep untrusted
  pointers, callbacks and unchecked byte buffers out of gameplay interfaces.
  Linux bridge milestone, 2026-10-21: the mandatory CMake/CXX 1.0.205 static
  library now joins the real session voice branch to the Rust parser, returning
  owned fields with no C++ decoder fallback. All existing sender/seat/role,
  block and generation checks remain in the actual session. The
  [interface and qualification](online-security-rust-bridge.md) record normal
  and C++ ASan/UBSan controls over 12,355 differential frames and 1,200 concurrent
  calls. Normal and C++ ASan/UBSan repository builds also pass the real four-peer DTLS,
  synthetic Opus/PCM and two-handoff session control, including observed
  rejection of authenticated wrong-seat and non-finite-gain voice packets plus
  successful healthy controls. Native completion/account inputs remain explicit
  fixtures. Full protocol/admission migration, platform/release checks,
  containment and independent security review are still required.
- [ ] **S04 — Rust packet decoding.** Replace the application wire decoders
  and validation used by real receive paths, preserving supported protocol
  behavior and rejecting malformed fields before native gameplay access.
  Native-request milestone, 2026-10-21: complete joins, one-to-four-seat inputs,
  leaves and configuration queries now return owned typed fields from Rust to
  the real host, loading worker and saved-input checks. The
  [implementation and qualification](online-security-native-requests.md) record
  ten passing Rust groups, strict Clippy, 337,231 native-request differential
  frames and Linux normal/C++ sanitizer plus Windows GNU/Wine bridge controls.
  All eight selected repository CTests also pass: the actual native four-seat
  malformed/replayed-request consumer, UDP input/discovery/startup and direct
  DTLS/synthetic voice handoffs. Endpoint/account/native-cut fixture limits are
  explicit. Snapshot/state and authority/archive decoding, other protocol
  families and complete security acceptance remain open; no full S04/S07/S12
  checkoff follows.
- [ ] **S05 — Rust authority archive handling.** Move snapshot reassembly,
  decompression, delta/replay decoding and schema validation behind explicit
  aggregate memory and work budgets.
  Archive milestone, 2026-10-22: the actual authority decoder now consumes an
  owned flat Rust tree after Rust framing/DEFLATE/grammar checks; its C++ wire
  reader is removed without a fallback. The
  [implementation and qualification](online-security-authority-archive.md) record
  44,719 value/grammar comparisons, hostile deep-tree teardown, synchronized
  reservations and passing normal/C++ sanitizer bridge controls. Full native
  codec qualification is in progress. Expected native-schema checks, checksum
  admission and real prepare/commit stay enforced in their C++ owners; moving
  the remaining schema, replay/delta and whole-session budget work remains open.
- [ ] **S06 — Cryptographic peer admission.** Preserve established transport
  encryption; bind admitted identity, match and connection credentials, and
  close unauthenticated public-path bypasses without creating custom crypto.
  Review the newly added EOS route independently of earlier DTLS evidence,
  including admitted PUID/pair-key binding, SDK lifetime/contracts and the
  `AllowRelays`/`ForceRelays` controls before direct-P2P release acceptance.
- [ ] **S07 — Replay protection and sender permissions.** Enforce sequence,
  membership and epoch rules for every message family, including physical-peer
  versus couch-seat ownership and host-only operations.
- [ ] **S08 — Secure migration protocol handling.** Move the network-facing
  membership, telemetry and voting state machines into Rust; reject stale or
  forged transitions and preserve native promotion/readiness gates.
- [ ] **S09 — Secure voice framing.** Move voice framing, speaker admission,
  sequencing and queue policy into Rust while preserving direct peer transport,
  mute/block rules, delivery fences and existing voice behavior.
- [ ] **S10 — Native semantic validation and containment.** Preserve the real
  C++ world prepare/commit checks and qualify containment for hostile snapshot
  import/replay; parsing success must never authorize arbitrary code or memory.
- [ ] **S11 — Abuse and resource controls.** Enforce per-peer and whole-session
  limits on rates, queued data, allocations and processing, with cancellation
  and cleanup that preserve healthy peers.
- [ ] **S12 — Adversarial and compatibility verification.** Add differential,
  fuzz, sanitizer and malicious-peer integration coverage across the Rust
  implementation, bridge and retained native consumers.
- [ ] **S13 — Dependency and hardening review.** Audit and maintain remaining
  native transport, compression and media dependencies; verify release compiler
  hardening and independently review authentication, unsafe code and lifetimes.
- [ ] **S14 — Integrated security acceptance.** Qualify the actual supported
  ONLINE paths and packaged runtime, retain reproducible evidence and resolve
  blocking findings before returning to the remaining ONLINE checklist.

**Current task:** finish the already-owned archive qualification, refresh S01
for the concurrent EOS ingress/transport changes, then continue S02/S03 and
remaining S04/S05 state/control/schema/replay work.
**Completed security items for current combined source:** none; the earlier
S01 source inventory is retained as historical evidence. **Next dependency:**
the updated ingress inventory and owned-message qualification. **Unresolved:** Rust
implementation, measured limits, containment, adversarial verification and
release acceptance. The source match-voice and native host-request routes use
Rust. Archive syntax/decompression also use Rust; native schema/authority checks
remain necessary. These changes do not install a runtime or enable migration.

Until S01–S14 are checked with their required evidence, defer new work on the
remaining ONLINE items, including M02/M04 integration and migration activation.
Finish or safely drain already-owned validation and its storage cleanup; retain
its results under their original scope. Continue only work necessary to this
security goal, preservation or cleanup. This ordering does not cancel the
existing goal, discard unfinished work or mark any downstream item complete.
Security acceptance covers currently supported runtime paths and migration
protocol rejection tests; completing M02/M07 is not a prerequisite for S14.
Later migration implementation must pass the same security controls before
enablement, so there is no circular dependency or exception for new paths.

## Existing ONLINE goal — resume after security acceptance

User-requested consolidated goal, 2026-10-13. This is the acceptance checklist
for all ONLINE work discussed in this chat, including the subsequent direct
peer architecture and modern mesh requirements. Earlier narrower completion
claims do not close the remaining items below.

The full scope is registered as the active app goal. This document is its
tracked acceptance checklist; source changes and bounded test passes do not
close requirements that still need integration or physical validation.

## Working checklist and goal completion contract

User-requested tracking update, 2026-10-19. This is the authoritative working
list for the existing active goal, whose objective already requires completing
every requirement in this document. Work includes keeping this list current
and checking off each item against its acceptance evidence. The goal must stay
open until every required item below is checked. The later progress notes and
area table preserve supporting evidence; they are not separate completion lists.

These are remaining integration, delivery and acceptance gates. An unchecked
item can already have substantial implementation and passing component tests;
it does not mean that work must be rewritten. Previously verified milestones,
including the user's real Steam login and successful two-way party voice test,
remain recorded. Checkboxes describe final acceptance, not a percentage estimate.
Steam primary login and `master.halo2forever.net` supersede the earlier identity
and hostname choices; optional verified Discord linking/social access remains.
The user's clarification, reaffirmed 2026-10-21, makes MCC imports the sole
runtime asset path for the entire port, including gameplay, audio, fonts and
every ONLINE/clan screen. Vista supplies UI reference evidence and OG Xbox
TU1.5 supplies gameplay/network behavior reference evidence; neither is an
end-user asset donor or runtime prerequisite. Extract the available authored
banks through MCC-only onboarding. Legacy Vista/Xbox asset fixtures remain
reference evidence and cannot qualify the delivered MCC-only runtime.
This supersedes the older goal registration's wording about preserving a
Vista runtime asset selection; preserve those original inputs as reference
evidence while the delivered asset selector and onboarding use MCC imports.

Asset-source reaffirmation, 2026-10-21: the user's latest instruction is
“MCC assets import are the only way forward. Vista and OG TU1.5 are just for
reference.” Apply that direction to the delivered asset path. Earlier records
of separately supplied intro/credits, button artwork or a loading-tone
exception cannot qualify an MCC-only release. Preserve those existing files
and evidence; do not delete or relabel them. Any missing MCC movie, prompt or
material owner remains an explicit delivery gap until its actual MCC source
and integration are verified. No installed runtime or preserved asset was
changed by this checklist clarification.

Missing or incompatible MCC resources must stop admission of the affected
delivered feature; do not fall back to Vista/Xbox caches, executables, loose
donor files or historical asset registries. A path inside an MCC import is not
proof of MCC origin: preserve source hashes and the extraction record for the
actual payload. Reference-only tooling and translated behavior code do not
supply runtime artwork, fonts, audio, movies or shader payloads. Keep those
reference checks separately identified from MCC runtime acceptance.

Checkoff rules:

- Keep each stable item ID when updating its description or evidence. Record
  the completion date, source/build or release identity, evidence link and
  validation scope beside a checked item. A code change alone cannot close an
  item that also requires integration, delivery or a real-user check.
- Update the current task, completed items, next dependency and blockers at
  each work milestone. Refer to these IDs in progress updates so detailed
  subsystem work stays attached to a remaining outcome.
- Keep component tests, native runtime tests, visible game-window checks,
  physical devices and independent-PC/WAN checks distinct. If required hardware
  or human participation is unavailable, record the missing check and leave
  that item open while continuing independent work.
- Reopen an affected item when later changes invalidate its evidence. Resolve
  missing original behavior with source/public evidence or an explicit user
  scope decision; do not silently substitute invented behavior or remove gates.
- Preserve TU1.5 as the gameplay/network behavior reference, Vista as the UI
  reference, MCC imports as the sole runtime asset source, direct player-hosted
  gameplay/voice, guest privacy and test-storage cleanup throughout. Delivered
  native, ONLINE/clan, fresh-install and release acceptance must use that same
  MCC-only asset path; keep legacy-asset reference checks separately labeled.

Deferred implementation after S01–S14, 2026-10-21: **M04/M02** have source candidates
for complete local Core/script-clock playback images, fresh receipt/ticket
mapping, paired actual MCC announcer publication, and stable shield/damage
state adoption. The new genuine Effects-owned auxiliary target binds Audio
to the actual four-owner Effects preparation; Effects remains the sole RNG
restore writer and must publish before Audio. This is implementation in
progress, not behavior acceptance or complete foreground readiness. The
[clock review](../../.local/mcc-playing-audio-adoption-20261021/script-clock-continuation/independent-integrated-review.md),
[damage review](../../.local/mcc-playing-audio-adoption-20261021/damage-audio-continuation/independent-review-recheck.md)
and [managed compiler receipt](../../.local/mcc-playing-audio-adoption-20261021/companion-syntax/02-complete-declared-compile-inputs/report.json)
separate source/syntax findings from the pending integrated MCC playback tests.
Actual target-native metadata capability, all audio/event/class companions,
ONLINE announcer construction and one complete foreground transaction remain
required. No M02/M04 checkoff, runtime installation or migration activation.

Current validation, 2026-10-21: the frozen local Audio/clock/announcer/native
Damage cohort now passes **10/10 normal cases together, no skips**, including
actual MCC continuation. The genuine four-birth native case retains a local
encoded cut and restores twelve active shield/damage voices, with exact
subsequent PCM/request/shared-RNG parity across four views. Its Effects owners
are cold, and Damage unit IDs are declared adapter inputs; this does not
qualify an active effects graph or network/native admission for those IDs.
The original 8/9 failure (`Native script sound stop owner unavailable`) and
the exact second-admission diagnostic are retained. The fixture now supplies
the real optional-voice stop owner required for its repeated admission;
production Clock policy and metadata were not changed to force this pass.
See the [final passing phase](../../.local/mcc-playing-audio-adoption-20261021/core-continuation/validation-normal/08-final-ten-case-cohort/certification.json)
and [initial failure](../../.local/mcc-playing-audio-adoption-20261021/core-continuation/validation-normal/01-core-announcer/tests.log).
The native fixture's separate packet-observer precondition failure is also
retained. Actual four-way adapter observations establish that its ordinary
report is silent and its hard report emits an authored cue for both species;
the positive controls now use that verified input while retaining the original
silent inputs as negatives. See the [diagnosis](../../.local/mcc-playing-audio-adoption-20261021/damage-audio-continuation/packet-observer-diagnosis.json).
Normal cleanup is verified, including all seventeen recorded process identities
and the private source/build tree; no binaries were retained. The
[independent final audit](../../.local/mcc-playing-audio-adoption-20261021/core-continuation/independent-normal-final-audit.md)
confirms the ten-case result. The exact paired ASan/UBSan run failed during
linking with `R_X86_64_PC32` relocation overflows; no CTest started. Its
[failure certification](../../.local/mcc-playing-audio-adoption-20261021/core-continuation/validation-sanitized/01-paired-core-script-announcer-damage/certification.json)
is retained without a sanitizer pass claim. Its now-idle owner was closed for
the security-first priority; [verified cleanup](../../.local/mcc-playing-audio-adoption-20261021/core-continuation/validation-sanitized/cleanup.json)
removed 15,162,448,091 logical bytes / 15,267,098,624 allocated bytes and the
private source/build parent. All 24 recorded process identities are gone;
original resource hashes match. Required changed source preimages (4,295,548
bytes) remain explicitly retained with their hashes; no binaries are retained.
That source cohort excludes newer ROOT native-publication candidates. Complete
M04, native/provider composition and runtime migration remain open and deferred.

Native publication follow-on, 2026-10-21: an actual owning takeover now has a
source candidate for retaining its exact target, Cache control block and native
World gates before timing/device holds. Independent review found two retained
bank admission gaps: an untracked execution tape could attach during exclusion,
and eligible same-backing preparation could replace a write policy and retire
its captures. The new real-memory regression reproduces both failures before
the admissions are enforced and passes all four groups afterward, including
foreign live tapes, outgoing capture retirement, registry exclusion and atomic
admission-to-hold upgrade/hold-to-admission teardown. Duplicate registry capture
retirement is also checked off the registry mutex. Two native-owner syntax checks also pass. See the
[policy handoff](../../.local/mcc-playing-audio-adoption-20261021/paired-owner/native-policy-admission/after.json).
The [final independent source review](../../.local/mcc-playing-audio-adoption-20261021/paired-owner/native-policy-admission/independent-source-review.md)
finds no remaining scoped source blocker, verifies the passing component groups
and completed probe cleanup, and records concurrent CMake drift separately.
The genuine selected-MCC takeover scope fixture is implemented with syntax
checks, but its native execution, provider/timing/Core composition and complete
foreground transaction are still pending. No readiness or migration activation
follows from this low-level result.
The next native audio seam must retain the live original Cache control block
through an incoming-only, creator-thread factory and authenticate the provider's
exact native timing owner, including rejection of another World sharing that
Cache. The [owner/callsite plan](../../.local/mcc-playing-audio-adoption-20261021/paired-owner/same-cache-incoming-plan.md)
records those remaining factories, provider/Core held binding and session
activation dependencies; it is source research, not implementation acceptance.

Following implementation candidate, 2026-10-21: the metadata provider now
offers a target binding authenticated by the actual owning native publication
and its exact Physics timing-memory owner, including its control block. Its
transferable value retains only a weak native scope; the creator-only held
value retains native exclusion before acquiring timing. Direct held witnesses
use immutable identities and epoch/eligibility checks. Four source syntax
checks pass, and a real compiled-object symbol audit keeps the generic Cache
provider free of systems/native-publication dependencies; the separate binding
implementation belongs to `halo2_systems`. Probe storage is removed. See the
[binding candidate](../../.local/mcc-playing-audio-adoption-20261021/paired-owner/provider-native-binding/after.json)
and [independent design review](../../.local/mcc-playing-audio-adoption-20261021/paired-owner/provider-native-binding/independent-binding-design-review.md).
The same-original-Cache incoming factory and genuine selected-MCC binding
controls are now implemented as source candidates. The private constructor pins
the original Cache before its public slot escapes; RuntimeAuthority admits that
exact owner and reconstructs fresh native/content owners only after its replay
worker retires. The public distinct-Cache route remains separately available.
The fixture requires another genuine World sharing Cache A to be rejected on
its actual timing owner, checks weak binding/native wrapper lifetime and
preserves original activation, twelve native ticks and direct voice assertions.
Four further C++ syntax checks and Python parsing pass, with probe storage
removed; the [final factory source review](../../.local/mcc-playing-audio-adoption-20261021/paired-owner/same-cache-incoming-implementation/independent-source-review.md)
finds no scoped source blocker. Fresh current-source eight-case CMake/native
qualification is deferred until S01–S14 security acceptance. The separate exact
paired audio sanitizer build failed to link as recorded above and has been
cleaned; no new native execution acceptance follows from these candidates.
Core native-bound restore remains deliberately fail-closed.
The complete publication still needs one shared timing/native exclusion for
gameplay and announcer devices, target-provider substitution and actual release
of outer native exclusion before deferred Core shutdown/retirement. Dropping
one held reference alone cannot establish that ordering.

Separate source candidate, 2026-10-21: the actual local peer's 54 complete
sound-class envelopes and one-shot game-over latch now live in the existing
receipt values. The runtime aliases its original writers to those fields;
the authoritative Simulation classes and once-per-frame audio elapsed update
remain with their existing owners. The composed historical receipt regression
now checks every target/current/remaining bit pattern, the latch, rejection
controls and a continued fade suffix. An explicit selected-MCC common-cut
route preserves the three-peer/twelve-couch-player cohort without an external
XBE or a missing-input skip. Three managed C++ syntax checks, Python syntax
and independent source review pass, with scratch removed; the actual new MCC
common-cut execution is still pending. Four invalid-input CLI controls also
reject with their intended admission diagnostics, without skip status 77 or native
execution; see the [admission receipt](../../.local/mcc-playing-audio-adoption-20261021/peer-envelope-continuation/admission/report.json).
See the
[source handoff](../../.local/mcc-playing-audio-adoption-20261021/peer-envelope-continuation/handoff.json).
Existing receipt preparation requires externally quiescent target values;
this change does not add an independent revision witness for arbitrary direct
value writes, qualify playing devices or close M02/M04.

Qualified earlier prerequisite, 2026-10-21: **M04/M02** now have a qualified
Core ownership/enrollment prerequisite in normal and ASan/UBSan runs. Gameplay Audio and the separate actual
MCC announcer retain their existing strong Cache owners; ownership alone leaves
legacy producer behavior unchanged. Explicit, quiescent creator enrollment
routes Core playback, completion accounting and error cleanup through typed SDL
receipts, and both fade consumers use logical elapsed time. The
[production review](../../.local/mcc-playing-audio-adoption-20261021/core-enrollment/independent-production-review.md)
records the corrected pre-publication observer compatibility and the explicit
allocation-free SDL enrollment seam. The latter avoids a checkpoint allocation
failure changing only one owner's executor policy. The new
[normal eight-case qualification](../../.local/mcc-playing-audio-adoption-20261021/core-enrollment/validation-incremental-retry2/02-poststop-and-pcm-controls/report.json)
starts and passes every requested case without skips in2.62s. This includes
actual imported MCC PCM, finite/nonzero whole-cue output, exact legacy/enrolled
selection/completion/clock/peak comparisons, authored loops, script lifetime
controls, and all six direct SDL controls. The fixed script policy tests
ownership and fade lifetime, not authentic World timing. The fixture's stop-row
expectation now follows the actual original first-match clear; reviewed
correction/failure receipts remain in the same investigation. All17 declared
inputs match before/after; the actual normal target/link census contains388
unique ROOT translation units, with its during-build sampling limit recorded.
The separately preserved Apple-only CMake change does not affect these Linux
targets. This is not a frozen whole-project or native caller qualification.
The [normal owner's cleanup](../../.local/mcc-playing-audio-adoption-20261021/core-enrollment/validation-incremental-retry2/cleanup.json)
removes its private build/scratch, empty parent and all three recorded process
identities; no large artifact is retained from that owner. The fresh
[same-eight ASan/UBSan qualification](../../.local/mcc-playing-audio-adoption-20261021/core-enrollment/validation-sanitized/report.json)
starts and passes all eight cases without skips in5.65s; actual compiler and
all four retained link commands contain address/undefined instrumentation.
The seven changed source/module pins match the normal run. Sixteen of seventeen
declared inputs remain unchanged; the preserved four-line root CMake drift only
registers a separate hologram GPU case and does not change these audio targets.
The actual build compiles388 unique ROOT translation units, with the sampled
source/header boundary recorded rather than claimed as an immutable freeze.
Its [cleanup](../../.local/mcc-playing-audio-adoption-20261021/core-enrollment/validation-sanitized/cleanup.json)
verifies removal of build/scratch, the empty parent and17 recorded process
identities. No binaries or large generated artifacts are retained. The
[stage handoff](../../.local/mcc-playing-audio-adoption-20261021/core-enrollment/handoff.md)
pins these exact qualified sources; subsequent continuation implementation
requires its own qualification. Prior
platform results below apply only to their recorded earlier pins.
Neither real construction site enrolls automatically. Complete Core snapshots,
fresh script/announcer branch adoption, target-World capabilities, damage/class
companions and the full foreground transaction remain open. Ready/default
migration stay disabled; no runtime is installed.

Earlier platform qualification, 2026-10-21: **M04**, a prerequisite of **M02**,
now has an implemented held SDL playback transaction foundation. The
[focused qualification](../../.local/mcc-playing-audio-adoption-20261021/platform-implementation/validation-final.md)
records seven normal and six ASan/UBSan cases started and passed without skips,
including the unchanged real split-producer reset compatibility control in the
normal run. Saved packet/cursor/clock state, fresh branch-qualified receipts,
callback exclusion, stale/cancel/owner guards and allocation-free publication
have focused coverage. The physical device clock stays monotonic. Both managed
scratch trees and their dedicated empty parents are removed; all13 recorded
process identities are gone and no large artifact is retained. Concurrent ROOT
CMake changes are preserved and recorded separately. This qualifies the
platform foundation, not complete playing-audio adoption, physical audibility,
MCC media provenance or full-engine/runtime ABI compatibility. The next audio
step is the complete paired game Audio/script/damage/class-envelope transaction
and the separate actual MCC announcer owner, using the
[owner inventory](../../.local/mcc-playing-audio-adoption-20261021/paired-owner/inventory.md)
and [announcer companion review](../../.local/mcc-playing-audio-adoption-20261021/paired-owner/announcer-companion.md).
Four-seat input, camera/GPU/resource borrowers and the single complete foreground
publication remain required. M02/M04 stay unchecked, the full goal stays active,
and complete Ready/default migration remain disabled. No runtime is installed.

Latest native qualification, 2026-10-21: **M02/M05** have a new
[certified native result](../../.local/mcc-online-m02-20261021/native/normal/01-current-mcc-gait-and-fullgroup/actual-result.json)
for the runtime locomotion integration and explicit MCC full-group driver.
All four selected tests pass without skips;23 exact translation units,
34 actual object records and all three fresh binaries are certified. The
rendered run observes325 accepted receipts,324 full captures,128 retained
entries and two bound/native-all cuts. Its actual16-slot locomotion owner
has eight occupied slots and no observed unavailable slot. The separate CPU
group fixture passes its420-tick/three-adapter suffix and allocation/retirement
controls; it does not represent16 simultaneously active players. Maximum local
input gap464.999ms and frame input308.735ms are observed, but frame wall505.523ms
still fails the unchanged500ms parent gate. No host/runtime error or peer drop
before completion is observed. Frozen/borrowed/consumed inputs remain unchanged;
later concurrent ROOT changes are recorded separately and are not qualified by
this result. Full foreground publication, camera/GPU/input/playing-audio owners,
whole-history memory admission and migration acceptance remain open. No runtime
is installed, and complete Ready/default migration remain disabled. This uses
the selected MCC cache without an external XBE; D01 still requires complete
MCC payload provenance and fresh delivery, including unresolved material/movie
owners. The managed owner's [verified cleanup](../../.local/mcc-online-m02-20261021/native/normal/cleanup-report.json)
removes its private source/build/MCC copy and owned processes. ROOT independently
checks all seven recorded identities and scratch references are gone and the
original registry/four core hashes are unchanged. No new large artifact is
retained; the goal and remaining acceptance gates stay open.

Earlier working checkpoint, 2026-10-20: **M05/M02** have a capture-local
material comparison candidate and a startup-candidate wait correction. The
[corrected paired component run](../../.local/mcc-frame-liveness-20261020/focused-continuity/handoff.md)
passes the same complete fixture against baseline and candidate, including all
1280 authored cases, distinct expected constant/program owners, exposed-pass
changes, COW, NaN/signed-zero and restoration controls. In eight retained
captures the candidate checks768 pass values and reuses3,145,728 successful
comparisons; the measured capture window changes108.211 to94.4519ms. Whole
CTest time increases3.47 to3.88s, so this is no general or native speedup claim.
The actual production owners build through the new focused CMake route; both
private source/build trees and their disposable helpers are removed, with no
large artifact retained. The [startup correction](../../.local/mcc-candidate-startup-20261020/review.md)
waits for the first genuine full history before attempting Arm. Strict tail
identity checks and explicit retirement remain unchanged. Managed syntax and
independent source review pass. The fresh native runs below observe two real
startup pending windows and pass the unchanged rejected-mask regression.

The [fresh native qualification](../../.local/mcc-online-m05-20261021/native/checkpoint.md)
has a [certified first result](../../.local/mcc-online-m05-20261021/native/normal/01-current-mcc-candidate-native/actual-result.json)
against its private byte-identical selected MCC import. Build/freshness pass
for16 translation units,27 actual objects and both binaries; all three selected
tests run without skips. Material and MCC input owners pass. Native records445
accepted receipts,444 full captures and128 retained entries, but zero bound
cuts before the unchanged deadline. Two actual startup pending windows pass
the rejected-mask regression. Maximum local input-observer gap is452.158ms,
maximum frame input338.337ms and frame wall471.291ms, with no observed peer
drop; these are local observations, not host-arrival or WAN acceptance.
The [failure-only diagnostic run](../../.local/mcc-online-m05-20261021/native/normal/02-continuation-first-failed-diagnostic/actual-result.json)
preserves all ten compatibility predicates and actually identifies
`configuration_binding` as the first failure. It builds exactly one updated
translation unit and a fresh native binary; the material binary stays unchanged.
All three tests run without skips: material and MCC input owners pass; native
fails at the unchanged deadline with278 accepted receipts,277 full captures,
128 retained entries and zero bound cuts. This second run observes an813.351ms
maximum local input gap and805.874ms frame wall, exceeding500ms; the first run's
lower values do not establish sustainable timing. The differing configuration
field remains unobserved. The independently reviewed one-statement
[configuration correction](../../.local/mcc-continuation-configuration-20261021/handoff.md)
keeps the actual observed weapon in presentation rules and preserves admitted
Match settings. Its [certified native result](../../.local/mcc-online-m05-20261021/native/normal/03-admitted-rules-presentation-separation/actual-result.json)
rebuilds exactly that one translation unit and a fresh native binary. The
configuration rejection clears in this exercised cohort:264 receipts,263 full
captures,128 history entries and two bound/native-all cuts are observed, with
no aborted cut. Native still fails148.95s on an actual ordinary-departure
backing gap (`88C50` → `233D57`, missing `455E48`); material and MCC input owners
pass. Maximum local input gap526.721ms and frame wall597.15ms exceed500ms;
no peer drop is observed. The [bounded name-operand repair](../../.local/mcc-name-mapping-owner-20261021/handoff.md)
now identifies and mounts eight authentic program-data owners/1,848bytes through
the existing provider. Token/string owners keep their original read-only policy;
the dispatch table keeps its actual writable policy. Independent review finds
no blocker and the focused cold component passes0.18s, checking complete raw/
semantic mappings, access, checkpoint and conflict-before-publication. Its
private build/scratch are removed. This translates reference-backed code
operands and supplies no glyph artwork or end-user reference-image dependency.
The unchanged [fourth native phase](../../.local/mcc-online-m05-20261021/native/normal/04-name-sanitizer-canonical-operands/actual-result.json)
passes all three selected tests without skips: native144.43s, input owners0.42s
and material2.44s. One provider translation unit/object and the fresh native
binary are certified; only its three approved source inputs enter the cohort.
Actual observations show268 receipts,267 full captures,128 retained entries,
two bound/native-all cuts and no host/runtime error. The ordinary authenticated
leave completes after the fixture's completion request; no peer is dropped
before completion. Maximum local input gap535.85ms and frame wall550.806ms
still exceed500ms. This exercises the formerly missing departure operand path;
dynamic private-glyph callbacks, physical/WAN play and a complete migration
transaction remain unqualified. The managed investigation's
[verified cleanup](../../.local/mcc-online-m05-20261021/native/normal/cleanup-report.json)
removes its owned source/build/private-MCC snapshot and all recorded processes;
original inputs and installed runtimes are preserved. No new large artifact
is retained and no runtime is installed. The next
prepared code step is the reviewed all16 locomotion receipt integration,
followed by the remaining timing/memory and complete-owner adoption work.
Frozen/borrowed/consumed inputs remain clean, with later ROOT changes recorded
separately. These results do
not causally isolate a speedup from older source cohorts. The fixed256/128/
two-cut/500ms gates remain; the latest native test passes while the parent timing
goal still fails. **M02/M05**, complete adoption,
whole-history memory admission and migration acceptance remain open. Direct
player-hosted gameplay/voice and disabled default migration are unchanged.

Earlier checkpoint evidence, 2026-10-20: **M05** retains the reviewed topology
change after its controlled allocation and full frozen material/native passes.
The native run captures348 complete frontiers, retains128 and binds two cuts;
its547.229-ms maximum local input gap still exceeds500ms. Sustainable timing
and the full runtime/history/preparation allocation union remain open.
The [paired-gap investigation](../../.local/mcc-frame-liveness-20261020/README.md)
now records actual command gaps with their preceding and following frames,
including work outside the existing frame timer. Its fresh frozen native
cohort is qualifying the startup and loading boundaries described below;
the earlier measurements do not qualify later concurrent runtime changes.
A separate **M02** exact-owner preflight repair
passes six real same-pointer/different-owner and null-alias cases that fail
before the repair. The permanent regression passes the fresh full material
suite. No readiness, timeout, input ordering,
capture-coverage or player-hosted transport policy is changed by this work.
The [fresh baseline result](../../.local/mcc-frame-liveness-20261020/native/normal/terminal-summary.md)
has genuine new binaries and four verified compiled translation units. Material
passes in3.56s and the registered MCC input-owner check passes in0.56s; native
startup fails in6.73s before the match loop with an immutable physics-scalar
owner conflict. Separate new `presentation/loading-v3` files also invalidate
the broad selected-cache metadata audit; pinned core map/provenance hashes
still match. Neither failure is a timing result. That closed baseline produced
no paired frame observations; the later isolated phases below supersede its
syntax/helper-only diagnostic evidence with actual native observations.
The closed owner, all six recorded process identities and its private source/
build are removed; no large artifact is retained. The complete current
[cold-data composition check](../../.local/mcc-frame-liveness-20261020/cold-data-probe/handoff.md)
now passes through 46 actual production translation units and 254 frozen code
files in 10.74s, without runtime layers or external assets. The original failed
scalar address remains unobserved; this is no old-baseline or gameplay pass.
The permanent [cold-data component regression](../../.local/mcc-frame-liveness-20261020/cold-permanent/handoff.md)
also passes its focused managed CMake build and wrapped CTest (0.24s), checking
exact typed-table/padding ownership, immutable access and checkpoint restore.
Its private build is removed. This test was added after the native cohort
froze; it is outside that cohort and closes no migration acceptance item.
The [isolated native cohort](../../.local/mcc-frame-liveness-20261020/native/isolated/handoff.md)
builds against its own byte-identical MCC reflink copy. ROOT verified all
254 cold-check code hashes, 18 critical inputs and consumed core asset hashes.
Its [actual first result](../../.local/mcc-frame-liveness-20261020/native/isolated/normal/01-current-root-paired-native/actual-result.md)
passes material (2.34s) and input owners (0.39s); native reaches rendering but
fails strict loading identity comparison (61.88s), with zero completed frames,
commands or captures. Frozen/borrowed/consumed inputs remain unchanged. The
same live owner's reviewed [prepared-host fixture correction](../../.local/mcc-frame-liveness-20261020/loading-identity/root-trace.md)
admits no unrelated ROOT updates or changed gates. Its actual phase2 confirms
equal loading fingerprints and 170 frames/277 local commands, then stops at
the original `1658FB` read of missing `4D61D8` storage, before any accepted
receipt or capture. Phase3 admits only the independently reviewed shared
saved-session provider, declaration and cold-world call. Static original
[section evidence](../../.local/mcc-frame-liveness-20261020/saved-session-loader/report.json)
and actual [component owner controls](../../.local/mcc-frame-liveness-20261020/cold-saved-session/handoff.md)
support that candidate; no live initialization is supplied. Actual phase3
clears the missing bank and accepts 81 receipts, captures 80 and binds no cuts
before the unchanged deadline. Its [paired measurements](../../.local/mcc-frame-liveness-20261020/paired-gap/phase3/analysis.md)
reconcile the 1,109.287518ms worst local command gap exactly; current-frame
scene capture dominates its pre-command interval. Scene is 99.6% of measured
whole-run capture work, while 131.145s elapses before the first completed
frame. No individual scene-child or startup-phase cause is inferred. The
actual provisional-tail rejection also needs a specific predicate. The same
owner's reviewed phase4 changes only the passive scene-child and failure-only
guard diagnostics. Its [certified actual result](../../.local/mcc-frame-liveness-20261020/native/isolated/normal/04-scene-leaf-and-tail-guard/actual-result.json)
passes material (7.34s) and input owners (1.14s); native fails (200.39s), with
64 accepted receipts, 63 full captures and no bound cuts at the unchanged
deadline. The [actual leaf attribution](../../.local/mcc-frame-liveness-20261020/scene-leaf/phase4/analysis.md)
covers all 64 completed scene calls: Material is 98.77% of scene wall time and
98.86% of its current-thread CPU time, with valid CPU samples for every leaf.
The 1,743.199427ms largest local command gap has an exact paired partition;
its current frame contains 1,010.73ms of Material capture. This is diagnostic
attribution, not a performance comparison or a passing timing result. The
[first failed predicate](../../.local/mcc-frame-liveness-20261020/tail-guard-diagnostic/phase4-context-analysis.md)
is `tail-context`; the log does not distinguish an absent context from a
different generation, and later short-circuited predicates are unobserved.
The pre-first-frame interval still takes 130.190s and is not subdivided by
these measurements. No gate is relaxed and no phase5 is authorized. The
cohort is now closed. Its [cleanup receipt](../../.local/mcc-frame-liveness-20261020/native/isolated/normal/cleanup.json)
and independent ROOT verification confirm all 12 recorded process identities,
the private source/build/MCC copy and owned scratch are gone. The disposable
coordinator is retired after exact reconstruction verification. Retained
evidence is bounded logs/reports and 28 source files (2,213,030 bytes), with no
binary, asset copy or raw bank. Removed allocated-file sums include shared
reflink extents and are not an exclusive disk-space gain. Complete native
coverage, sustained timing and migration acceptance remain pending. Next M05
dependencies are exact internal Material cost attribution,
the first Arm context/binding transition and the pre-first-frame phase budget.
Qualify each correction in a coherent new source cohort before repeating the
unchanged complete-history gate; do not substitute the older frozen runtime
for current concurrent changes or infer missing context values from this log.
**U01/D01** now have source fixes for
stale authored-prompt prerequisites in frontend admission and the rendered
ONLINE/clan wrapper. The [current source audit](../../.local/mcc-topology-sharing-20261020/mcc-online-asset-audit.md)
finds the normal frontend/menu path uses MCC owners; Vista names and explicit
reference helpers are not runtime donor evidence. New prompt-font proof must
qualify actual native MCC fonts, without relabelling historical receipts.
No additional checkbox is closed by these findings. Gameplay and voice remain
directly player-hosted, and default migration remains disabled pending its
complete acceptance gates.

The [admission controls](../../.local/mcc-topology-sharing-20261020/frontend-admission/handoff.md)
pass62 Python tests plus the direct first-run fixture check. A distinct native
MCC control-font profile is mandatory, every copied font remains hash-covered,
and historical proofs are not upgraded. The
[render-input controls](../../.local/mcc-topology-sharing-20261020/rendered-prompt-pins/handoff.md)
pass eight cases using actual copied MCC fonts and their authenticated profile.
The wrapper pins those owners instead of obsolete authored manifests; its
paired completion marker is corrected and a permanent CPU regression is
registered. All owned helper scratch is removed with no large artifact retained.
These are admission/input checks, not rendered widescreen acceptance. The new
CTest registration executes successfully in the fresh M05 baseline above; the
paired frontend C++ marker remains outside its selected native targets. No
installed runtime, cache or real proof changed through this investigation.

Latest D01 loading-route milestone, 2026-10-20: normal fresh/reuse setup,
runtime menu selection, converter packaging and frontend resource admission now
explicitly select the MCC V3 bank. The source change passes 116 managed Python
checks; the shared Linux cohort builds the native game, default decoder and two
component fixtures. All three
[genuine MCC loading checks](../../.local/mcc-animation-selector-20261020/11-genuine-default-v3-loading-bank/cohort.json)
pass without skips, including actual source preparation, offline bank repair,
canonical PCM ownership and the native V3 bridge. The private producer has no
original tone or V2 bank. Exact source/binary and scope records are in the
[independent closing summary](../../.local/mcc-animation-selector-20261020/phases10-11-independent-closing-summary.json)
and [implementation report](../../.local/mcc-loading-default-route-20261020/implementation.md).
The qualifier applies only the two owned Linux CMake changes to its frozen
baseline; concurrent Mac changes in ROOT are preserved and separately scoped.
Full fresh-cache onboarding, relocated package/launcher playback, authored
MCC loop timing, physical audio and the remaining shader/movie asset-source
gates remain open. No installed runtime or selected cache is changed, and D01
remains unchecked.

The [owned investigation cleanup](../../.local/mcc-animation-selector-20261020/cleanup-receipt.json)
is verified: the private build/source/media tree and its process identities are
gone, and 12 obsolete helpers/drafts are retired with a small reconstruction
patch. Removed files accounted for 1,474,326,528 allocated bytes in scratch;
observed filesystem free space increased only 56,295,424 bytes amid concurrent
activity, so those figures are not treated as equivalent. No large artifact is
retained. Original assets, selected cache provenance and launcher-selected
runtimes are preserved. The launcher still reports its independent MCC
frontend unavailable; these component checks do not qualify or enable Play.

Latest M05 exact-body diagnostic, 2026-10-20: the new frozen current-ROOT
[native cohort](../../.local/mcc-common-cut-association-20261020/01-01-genuine-mcc-cut-lineage/cohort.json)
builds successfully but fails the unchanged native history gate in 196.20s.
Its complete 1,786-event trace has no overflow, incomplete writer or truncation;
298 actual observations produce 297 full frontiers and zero bound cuts.
The [per-cut analysis](../../.local/mcc-common-cut-association-20261020/01-analysis/association-summary.md)
shows that native exports for ticks 91 and 1,337 exactly retain their issued
host snapshot stamps, but neither expected body was ever adopted, projected or
retained by the client. Empty, nonoverflowed reservations are rejected after
history advances beyond those ticks. The third offered body is also absent;
its native export was not observed before the test ended. This isolates a
pre-adoption delivery/admission dependency for the first two cuts, without
locating wire loss or establishing a performance change against older cohorts.
The private scheduling candidate remains unpromoted; M05/M02 remain open and
default migration remains disabled. The
[owned cleanup](../../.local/mcc-common-cut-association-20261020/cleanup-receipt.json)
verifies removal of the private source/build tree and its process identities;
no installed runtime or selected asset owner changed.

Latest untraced MCC qualification, 2026-10-20: the
[four-check cohort](../../.local/mcc-animation-selector-20261020/09-untraced-bounded-batch-and-posing/independent-results.json)
passes body selection, body overlays and material continuity. The genuine body
fixture retains its 48 existing requests and adds 12 published-clock posing
requests: two resolve actual authored clips and ten are independently confirmed
omissions, with all seven graph byte identities unchanged. The exact posing
StringID comes from the
[MCC name join and TU1.5/runtime evidence](../../.local/mcc-weapon-selector-20261020/posing-source-audit.md),
following an actual red regression. All 49 earlier mappings and the 36-entry FP
prefix remain unchanged. The full native case now captures 231 frontiers with
128 peak retained, receives three fully native-validated host offers, but binds
zero cuts and fails the unchanged deadline. No unsupported selector appears.
The bounded scheduling candidate remains private; complete cut association,
throughput and default host migration remain unaccepted. The untraced peer-drop
observation has no removal reason and is not evidence of an idle timeout.

Latest M05 input-liveness investigation, 2026-10-20: the exact
[admission timeline](../../.local/mcc-animation-input-liveness-20261020/phase05-findings.md)
records twelve genuine accepted bodies occupying 6.776 seconds before the first
command; their complete frontier captures consume 6.636 seconds. The host removes
the unseen-input peer at 5.020 seconds. A
[private bounded-batch candidate](../../.local/mcc-animation-selector-20261020/06-bounded-receipt-input-liveness/cohort.json)
completes one authentic body per frame, preserving the original ordinary input
tail and every pending body until an explicit generation fence. The host now
accepts all 153 commands and has no idle removal; the later removal is an
authenticated Leave after an unsupported animation-selector exception. Original
stale-control neutralization remains reachable, and the native gate remains red
at 51 projected receipts/50 retained frontiers/zero bound cuts. This is a private
candidate, not an installed repair or M05 acceptance. Identify the actual
selector, then qualify sustained scheduling and the unchanged native gates
without diagnostics before promotion. The coordinated V3
[setup/runtime/package integration](../../.local/mcc-loading-default-route-20261020/seam.md)
has now been implemented in source with 116 managed Python checks passing.
Shared native compilation and genuine-bank/default-route qualification were
subsequently completed in the D01 milestone above; fresh full-cache delivery,
launcher and device evidence remain required. Component evidence does not
close D01.

Latest MCC body-selector milestone, 2026-10-20: the
[untraced repair cohort](../../.local/mcc-animation-selector-20261020/03-untraced-mcc-body-selector-repair/cohort.json)
passes all three registered checks without skips. The strict compiled-to-MCC
name boundary now includes four existing body requests, retaining all45 old
rows and the first36 canonical first-person activities. Genuine MCC tests
exercise12 species/weapon/stance/dual contexts and48 requests:32 independently
named authored clips match,16 omissions are independently confirmed, four FP
bindings keep body-only activities out, and seven graph payloads stay unchanged.
This is selector/clip-identity evidence, not complete pose or migration parity.
The direct component reproduced the missing canonical reload selector before
repair; the exact earlier phase11 terminal selector remains unknown.
The [untraced full native check](../../.local/mcc-animation-selector-20261020/04-untraced-native-after-body-repair/cohort.json)
still fails with33 retained receipts and zero bound cuts. Its first input
arrives at5.681s, after peer removal at4.967s, with no runtime exception.
Input-producer admission and snapshot-batch scheduling are the next M05 seam;
the private material candidate and error instrumentation were removed before
this result. Full history/proof/deadline gates and default-disabled migration
remain unchanged. Worker-retained test logs are authoritative; the
[helper attribution correction](../../.local/mcc-animation-selector-20261020/helper-test-log-attribution-correction.json)
labels post-listing empty tails separately without changing actual results.

Earlier material-cost milestone, 2026-10-20: the
[uninstrumented candidate cohort](../../.local/mcc-material-callgrind-20261020/11-clean-schema-native-history/cohort.json)
passes six of seven actual checks without skips. Material and genuine MCC
loading components pass; native ONLINE recovery remains red with66 retained
scene/HUD receipts and no bound cuts. Its propagated runtime error is
`Unsupported compiled weapon animation selector` from the gait/footstep owner.
The separate startup predecessor rejection is caught; capture subsequently
continues. It records first input at4.003s and peer removal at21.868s. These
observations describe this cohort, not a controlled improvement over earlier
runs. A scalar schema-dispatch candidate records21.5% lower own-thread CPU in
one actual capture pair with identical current accounting, but prior-bank/cache
conditions differ; no causal speedup is claimed. The candidate is retained as
an exact patch and is not promoted to production. All history, proof, coverage
and time-limit gates remain unchanged. The
[final restored component cohort and cleanup](../../.local/mcc-material-callgrind-20261020/closure.md)
record six passing checks, original uninstrumented native/material/audio
binary restoration, removal of the private build/media and retirement of11
obsolete helpers. The [frozen failure review](../../.local/mcc-material-callgrind-20261020/phase11-native-failure-review.md)
keeps the actual thrown selector/caller explicitly unknown. M05 and
M02 remain open; no installed runtime or migration acceptance is claimed.

Earlier capture-cost milestone, 2026-10-20: [actual selected-cache measurements](../../.local/mcc-scene-capture-cost-20261020/findings.md)
attribute98.80% of scene-capture time to the material owner, predominantly its
complete graph walk. Baseline warm material captures have a363.262ms median;
the actual native test still retains33 receipts and binds no cuts before peer
loss. A fused shared-pass comparison candidate passes historical A/B-to-A/A,
same-address mutable-write and full static/readiness regressions, but demonstrates
no native performance benefit. Its slower run also has concurrent compilation/
physics load and a different capture frontier, so no causal regression is claimed.
The unproven candidate and temporary probes were removed by exact owned patches;
no production performance change or installed runtime is claimed. M05 needs
leaf-cost attribution with controlled prior-graph/load conditions. MCC loading/
shader payload provenance remains a separate D01 release blocker, and these
selected-cache measurements do not accept a fully MCC-derived runtime.

Latest owner-repair milestone, 2026-10-20: [the final untraced cohort](../../.local/mcc-native-owner-repair-20261020-retry/closure.md)
passes four of five actual checks. Genuine MCC melee-resource retention and
two independently evidenced loader-zero runtime bytes now pass real-consumer
and checkpoint regressions; the player-hosted authority reaches 1,076 ticks
without the previous host exceptions. The rendered ONLINE check still fails:
it retains33 complete receipts, offers one cut and binds none. First observed
main input arrives at11.565s; the peer is removed at4.934s before completion.
These observations exclude genuine neutral admission sends. The earlier trace
isolates7.054s of scene capture in the first receipt batch before main input;
the binding tail takes1.993ms. Complete history/proof/time-limit gates remain
unchanged. Exact source cohorts, later root drift and reference-only coverage
are separated in the receipt. Temporary tracing, both private build trees and
superseded helpers are removed; no runtime was installed. **M02** complete
adoption and **D01** whole-payload MCC provenance remain open.

Downstream resume point after security acceptance: **M05**, qualify bounded direct-peer delivery/retention of actual
candidate snapshot bodies before ordinary snapshot freshness can discard them,
and sustainable complete-frontier processing. The separately gated source
implementation now prepares every physical recipient before choosing a future
genuine broadcast, delivers its unchanged native packets through a bounded
direct-peer lane, and pins the exact completed foreground entry until the
native/common-cut owner takes over or explicitly retires. Source capture splits
the actual frame immediately after that broadcast; later native proof still
checks the original connection, body, sequence and tick. Complete-batch pacing
is enabled only by this optional qualification mode. The ordinary default path
and full migration readiness remain disabled for this contract. Final-source
delivery sanitizer checks pass. The first frozen selected-MCC native cohort
passes both requested tests: 278 complete foreground captures, 128 retained
entries and two bound native cuts, with one complete common Commit/Release.
Its measured maximum input gap is 536.201 ms, so it does not qualify the
500-ms timing target or sustainable replay. The second frozen cohort includes
the MCC-native prompt repair below. Its prompt control and delivery core pass,
and two common cuts complete Commit/Release, but the combined native test
fails its unchanged deadline with only 228 of 256 required captures. A peer
disappearance is observed before any completion request; that alone does not
distinguish live connection loss from exception teardown. The cause of the
slower capture progress remains unproved. See the [actual cohort and limits](../../.local/mcc-prepared-snapshot-20261020/README.md)
and [genuine phase-2 failure review](../../.local/mcc-prepared-snapshot-20261020/normal/phase2-review.md).
The earlier diagnostic's two native exports without exact adopted bodies remain
separate evidence; the reserved path now reaches genuine native association,
but remains optional and disabled in normal matchmaking. Preserve strict native,
receipt, foreground and all-peer checks while resolving sustained processing before
resuming the complete **M02** borrower
transaction and all-survivor qualification. D01 separately requires the actual
full-cache/package/launcher and remaining asset-source acceptance beyond its
new loading-route component evidence. M01 remains the only checked full-goal
item; default migration and full-owner readiness remain disabled.
Next M05 diagnostic: attribute runtime setup and complete-capture/frame cost,
and record the deadline/catch time alongside authenticated Leave-versus-idle
removal. The current deadline includes setup, and the terminal peer-count flag
cannot establish whether removal preceded the failure. Keep the original
256-capture, 128-entry, two-cut and 180-second requirements unchanged while
collecting that evidence; do not turn a slow run into a pass by reducing coverage.
That passive diagnostic source is now implemented, with three GCC C++20
translation-unit checks passing and their scratch removed. A new managed
current-ROOT cohort froze17,440 files and passed graphics/configure and both
target builds with every original gate unchanged. The peer delivery core passes;
the native test fails after6.44 seconds with `Original runtime regions overlap`,
after the MCC prompt control and before runtime timing/capture diagnostics.
Those metrics remain unobserved. This startup failure is distinct from the
earlier deadline failure; both remain red. Diagnose the actual conflicting
owners without weakening memory admission before collecting M05 cost evidence.
See the [active M05 run and exact scope](../../.local/mcc-history-cost-20261020/README.md).

The next exact two-file repair removes four duplicate narrow HS declarations
already covered by the unconditional complete startup provider. The focused
cold-world regression passes; the unchanged rendered MCC/core pair then both
pass, without skips:287 accepted receipts,286 complete captures,128 retained
peak, two bound native cuts, two host native-all cuts and one Commit/Release.
No peer removal occurs. Scene capture consumes68,374.7 ms wall /67,094.5 ms
thread CPU over286 samples, about98.27% of measured capture cost; this is not
the share of the entire141,340-ms runtime interval or a per-material profile.
The maximum input gap remains596.92 ms and does not qualify500 ms. **M05**
remains open for sustained catch-up, memory and input timing; the two earlier
red phases retain their original results. The next bounded experiment may
reduce schema comparison overhead while preserving every field/payload check.
See the [actual repaired cohort and limits](../../.local/mcc-history-cost-20261020/normal/02-hs-registry-owner-fix/qualifier-terminal-summary.md).

The fresh schema experiment is now retained after independent review, matched
expanded component controls against both baseline and candidate, and a passing
unchanged native/core pair. It specializes encode/compare dispatch and uses
bounded raw unsigned scalar comparisons; the complete named schema walk and
first-mismatch scanner remain. The actual MCC run reaches299 accepted receipts,
298 captures,128 peak and two bound cuts, with one host Commit/Release and no
peer drop. Longest local input-submission gap538.508 ms still exceeds500 ms.
Mean scene thread CPU is209.21 ms/sample versus234.60 in the repaired baseline,
but graph/value histories differ and total native test time grows151.32 to
158.82 seconds; no controlled or overall speedup is claimed. The first-frame
setup interval also grows43.62 to51.02 seconds. **M05** remains open. Next
work must address sustained catch-up, remaining input stalls and memory with
the same complete-history gates, then resume the complete **M02** borrower and
all-survivor adoption checks. The owned build is closed and cleanup independently
verified:37,454 files /1,281,326,066 logical bytes /1,384,538,112 allocated-byte
sum removed, all ten recorded process identities absent and helper retired.
No large artifact is retained; original/reference inputs and launcher runtimes
are preserved. Small actual results/source patches remain under the
[M05 investigation](../../.local/mcc-history-cost-20261020/README.md).
The final ROOT audit separately records later concurrent world-memory/fallback
constructor additions. They preserve this repair but were not admitted to this
frozen cohort; no whole-current-ROOT or delivered-runtime acceptance is claimed.

Next **M05** memory milestone, 2026-10-20: a managed controlled1024-root public
material-history probe measures33,860,448 requested live new/delete bytes across
128 owning cuts; zero-history storage stays constant and full retirement
returns to zero. The same baseline genuinely exceeds a24-MiB fixture-local
comparison gate. This excludes allocator overhead, malloc-only storage,
native/GPU/assets and extra distinct handoff/prepared owners; it is not a
whole-runtime bound. A private immutable-topology candidate now preserves all
fresh schema/DFS/alias/owner checks and full inherited per-cut charging.
The byte-identical probe now passes against that candidate: retained allocation
falls to5,781,544 bytes, down82.9254%, with all restore/alias/COW/retirement
controls passing, zero additional commit allocation and final retirement zero.
Single-cut allocation increases32,800 bytes and zero-history steady storage is
unchanged; this does not establish a whole-runtime memory bound or speedup.
Both managed memory owners, scratch and obsolete helpers are removed with
small source/hash/result/cleanup records retained. Expanded128-cut/real-COW/
control-block/null-alias/retirement controls are added to the permanent fixture
and their source syntax passes. The expanded fixture now passes against both
baseline and candidate, and the genuine selected-MCC native pair passes with
348 complete captures,128 retained peak and two bound native cuts. One common
Commit/Release completes; no runtime/host error occurs. The actual binaries
were verified to recompile the changed source after an identified incremental
timestamp error; both earlier startup failures retain their real outcomes.
Native test time is156.41s versus162.87s for the repaired baseline, but graph
histories/capture counts differ, so no controlled timing improvement is claimed.
Maximum local input gap547.229ms remains above500ms. Per-cut unique accounting
increases to25,280,664 peak bytes as designed; that statistic does not measure
the complete historical allocation union. Default migration and complete
readiness remain disabled; **M05/M02** stay open. See the
[active memory investigation](../../.local/mcc-topology-sharing-20261020/README.md).

The [owned cleanup](../../.local/mcc-topology-sharing-20261020/native/normal-retry1/cleanup.json)
is now complete: source/build/scratch and all12 recorded process identities
are absent, independently checked by ROOT. The obsolete qualifier is retired
after exact reconstruction verification. Removed allocated-file-byte sum is
1,407,197,184 bytes; filesystem free-space change is recorded separately amid
concurrent work. No large artifact is retained. Original/imported assets,
reference evidence, installed runtimes and79 unrelated ROOT drifts remain
preserved. This does not close the whole-goal D06 or any migration requirement.

MCC-only prompt milestone, 2026-10-20: normal frontend and gameplay HUD prompts
now use the authenticated native MCC font atlas, pixels, metrics and UVs.
Fresh import, cache reuse and package staging no longer require or select the
separately authored prompt overlay; existing cache/reference artwork is preserved.
82 focused import/reuse/package controls and seven actual-MCC font-owner
controls pass without skips. The latter is registered as
`native_mcc_prompt_font_owner` when an MCC test cache is supplied. The frozen
native phase-2 binary also passes the actual HUD constructor control for
12 selectors, 331 visible bindings and 224 colored draws, with unchanged native
source geometry, no authored-bank dependency and foreign font rejection.
These are CPU/source-closure checks, not submitted-screen or fresh portable
release acceptance. Existing overlay-based screenshots cannot establish parity
for the changed native glyphs. See the [prompt source handoff](../../.local/mcc-prepared-snapshot-20261020/native-prompts-handoff.md).
**U01/U02/D01** remain open: qualify the new glyphs in actual widescreen screens,
finish MCC-derived material/shader and movie ownership, and verify a fresh
MCC-only package and both delivered launchers. Vista and Xbox TU1.5 remain
reference inputs only throughout this work.
This qualification owner and its one-off helper are now closed and retired;
all generated source/build/scratch and recorded process identities were verified
absent. No binary, raw media or capture was retained. The [cleanup receipt](../../.local/mcc-prepared-snapshot-20261020/normal/closure.md)
records 1,355,874,304 removed allocated bytes separately from free-space gain.

Earlier selected-MCC route milestone, 2026-10-20: the explicit selected-MCC native
continuation route is implemented and exercised with no external gameplay
executable. In the [new source cohort](../../.local/scene-receipt-integration-20261020-retry-15/closure.md),
four of five actual checks pass; native continuation remains red. It renders
57 frames and retains 33 complete scene/HUD receipts, but offers/binds no native
cut and loses the peer before first input. Publication rejects missing pinned
melee resources; the host separately reports unavailable native runtime backing
at `0x51daed`. Their causal relationship is unproved. **M02** now prioritizes
genuine MCC-bound native/melee ownership before publication/adoption; **M05**
also retains the measured startup/input stalls. Source/readiness/history gates
remain unchanged. MCC format/compiled-native selection does not close the
whole-payload provenance blockers in **D01**. All generated private builds and
superseded helpers were removed. M01 remains the only checked full-goal item.
The [native-owner audit](../../.local/selected-mcc-online-history-20261020/native-owner-gaps.md)
separates the current MCC melee-resource seam from native mutable backing;
the latter is missing port-owned runtime state, not a new Xbox asset requirement.
That audit predates the real-consumer byte/resource repairs above; complete
world ownership remains unqualified. Later concurrent source changes are
explicitly distinguished from each frozen result.

Earlier **M02** integration milestones prepare world-borrower adoption using the
qualified owning receipt, runtime/event frontier and deferred-cue history.
Passive particle/model-light resource admission before renderer startup is now
qualified. Exact surviving-client decoded/scene/device/prop/collision rollback
is also qualified against a retained authentic receipt; complete prepared
adoption remains the current task. Independent paired-cut publication to every
physical recipient and nonterminal exact receiver exports are now qualified;
each recipient can now pin and agree its own exact native/foreground recovery
cut, survive ordinary history eviction, and repair a rejected later cut while
retaining the committed pair. Seven focused native/protocol tests and nine GCC
checks qualify this headless/CPU prerequisite, not complete owner readiness.
The next prerequisite now qualifies prepared restoration of the actual local
pose/FP histories, runtime receipt values, CPU effects and deferred cues, plus
incoming source-render routing/collision and paired lens staging. Eight native/
rendered tests, a nonempty Backwash residency control and ten GCC checks pass;
this remains private preparation without live migration or full readiness.
M02 remains unchecked; retaining resources and adoption inputs does not adopt
the runtime.
Exact local scenery, machine and prop checkpoints are now qualified as the
next M02 prerequisite: nine unskipped component/native/rendered checks pass
against the frozen source cohort, with private build/scratch removal verified.
They preserve completed posed geometry, lifetime/LOD/light caches, actual PRT
links and retained native prop views without cold reconstruction. The current
step connects all six scene adapters, actual callback/portal/light values and
the owning material graph to the authenticated receipt frontier. The reviewed
ONLINE admission ordering now finishes resident setup before real
all-seat admission while preserving authentic receipt/view/radar/camera bindings
and ONLINE-only operating clocks. The stage-timed diagnostic cohort passes all
15 registered checks and 18 target-configured GCC syntax checks against native
build `6f7eac6fd742`. It retains 367 complete scene receipts, the full 128-entry
history and two validated native cuts, with 35 submitted frames. These are
private diagnostic results, not live migration or final uninstrumented
acceptance. The old fixture did not record the completion-request timestamp,
so its later observed peer departure cannot yet be classified as a live loss
or normal teardown. The fixture now records that real boundary and rejects
peer loss before its successful completion request.

The diagnostic identifies static material-schema comparison as roughly
75–80% of capture cost; complete material captures take roughly 0.27–0.48
seconds each and batches still impede actual input/render cadence. Sustainable
performance and whole-history memory remain M05 work. All capture coverage,
proofs, timeouts, payload caps, 128-entry retention, 256-capture/two-cut gates
and the 180-second fixture deadline remain unchanged. Temporary timing has
been removed. The next uninstrumented source cohort passes 18 of 19 checks and
all 22 GCC
syntax checks against native build `6c9ed551007c`. All three explicit MCC-only
clan, party and matchmaking HTTP cases pass. Authored material record-array
controls also pass, including 58 byte/count/COW/tamper mutations. The rendered
client stays connected, retains 565 complete scene/HUD receipts at the full
128-entry history and submits 52 frames, but binds only one of the required
two recovery cuts before the unchanged deadline. The host validates six native
cuts. First input arrives at 2.04 seconds; the largest observed send gap is
4.46 seconds, with no peer loss. This is a real remaining cut-association/
throughput failure; combined acceptance remains red. Its exact proof, history
and timeout gates are preserved while diagnosis continues.

The following metadata-only diagnostic cohort, `59e4ccc35b8a`, passes all 21
required CTests and 24 GCC syntax checks. The reference-asset rendered fixture
retains 429 scene/HUD receipts and binds two exact cuts, but only one completes
release before return. It drops 71,306 incoming gameplay fragments at the
unchanged 512-packet queue. Three other native cuts have no accepted exact
gameplay body and abort; both successful cuts match the complete body digest,
extent, sequence, reservation and native proof. This identifies real queue
pressure without establishing why the earlier untraced run differed. Maximum
input send gap remains 3.69 seconds; reliable performance/migration stays open.
This fixture uses legacy reference inputs, so it cannot qualify the delivered
MCC-only native runtime. See the
[diagnostic findings](../../.local/cut-association-analysis-20261020/cohort13-findings.md)
and [21-test receipt](../../.local/scene-receipt-integration-20261020-retry-13/qualification.json).
Temporary tracing has been removed with exact scoped patches, preserving the
concurrent runtime changes. The private 2,052,970,305-byte source/build/scratch
tree and its processes are gone; only bounded reports and three explicitly
retained UI reference sheets remain. No runtime was installed, no full-owner
readiness was advertised, and migration stays disabled by default.

A camera audit confirms TU1.5 display clients consume actual player-host-produced
camera data, already retained by the per-peer host camera continuation. The
extra older-path camera adapter continuation has been retired and does not
qualify TU1.5 migration. Complete CPU HUD/scoreboard retention and private
preparation are integrated; GPU uploads, physical input/playing audio, the
complete owner transaction and live migration remain open.
See the
[diagnosis](../../.local/scene-receipt-integration-20261020/fixture-observation-repair.md).
This work remains private preparation, without live adoption or complete
readiness; see the
[earlier untraced 18/19 cohort receipt](../../.local/scene-receipt-integration-20261020-retry-12/qualification.json)
and [remaining owners](../../.local/scene-receipt-integration-20261020/remaining-owners.md).
See the [scene qualification](../../.local/scene-checkpoints-20261019/qualification.json)
and [remaining owners](../../.local/scene-checkpoints-20261019/remaining-owners.md).
**M01** is checked below with its dated GPU/native evidence and verified cleanup. Next
dependencies remain **M03–M06**, presentation/input/audio continuity, bounded
retention and all-survivor readiness, followed by **M07**, the actual windowed
migration path. Migration remains disabled by default until those prerequisites
and the relevant network/voice gates pass. The [next production dependency](../../.local/runtime-receipt-frontier-20261019/next-adoption.md)
and earlier [adoption inventory](../../.local/lens-deferred-retirement-20261019/next-adoption-inventory.md)
record missing owners and dependency order. The latest
[remaining owner transaction](../../.local/common-cut-pins-20261019/next-adoption.md)
identifies the complete presentation/input/playing-audio preparation and
activation still required after common-cut pinning. The
[qualification receipt](../../.local/common-cut-pins-20261019/verification.json)
applies to its frozen source cohort; later unrelated root edits are preserved
and do not inherit combined-release qualification. U01/D01 separately track the MCC-only UI
metadata/resource-bank generation and selector wiring found in the latest audit.
The [latest remaining owners](../../.local/foreground-receipt-adoption-20261019/remaining-owners.md)
identify live scenery/prop/machine caches, locomotion, HUD/camera, hardware input
and playing audio still required. The
[readiness ordering dependency](../../.local/foreground-receipt-adoption-20261019/readiness-ordering.md)
requires real pre-decision complete preparation from the committed pair;
post-decision takeover staging cannot establish pre-election or all-survivor Ready.

### M — Host migration and disconnect survival

- [x] **M01 — Deferred GPU retirement.** Prepared publication performs no
  Vulkan wait/destruction inside the locked commit; cancellation and stale
  preparations preserve old results, and retained GPU owners retire safely
  afterward on the correct executor. Qualified 2026-10-19 against native build
  `d76560c98534` (full hash/source manifest in the [completion receipt](../../.local/lens-deferred-retirement-20261019/verification.json)):
  12 focused CTests, real Vulkan publication/deferred-retirement observations,
  nine GCC syntax checks including both particle configurations, 17 matching
  owner hashes and verified scratch removal. This closes this component gate;
  actual windowed migration, active-flare continuity and hardware/WAN acceptance
  remain separate unchecked items.
- [ ] **M02 — Complete world adoption.** Prepare and atomically adopt every
  runtime borrower, source/model/particle pointer, callback, event frontier and
  cache against the exact validated world. Preserve the complete TU1.5 state,
  salted identities, inventory, RNG, timers, physics, projectiles, grenades,
  props, devices, match/results and local profiles; reject partial adoption.
  Partial milestone, 2026-10-19: exact owning decoded receipts, local slots,
  runtime/event bindings and freshly captured pending cues now share one
  proof-bound history entry. Six normal tests, four selected-owner sanitizer
  tests and nine compiler checks pass with verified cleanup; see the
  [receipt](../../.local/runtime-receipt-frontier-20261019/verification.json).
  A subsequent prerequisite admits complete supported authored weapon particle
  banks and model-light sections before renderer startup, while client native
  producers stay dormant. Two native/rendered tests and five GCC checks pass;
  the independent weapon census matches every accepted client receipt, with
  verified scratch removal. See the [resource admission receipt](../../.local/source-candidate-residency-20261019/verification.json).
  Exact surviving-client decoded and scene/device/prop/collision rollback now
  passes three normal tests, two selected-owner sanitizer tests and eight GCC
  checks, with verified scratch removal; see the
  [client adoption receipt](../../.local/client-replica-cut-20261019/verification.json).
  Complete borrower/scene/HUD/input/playing-audio adoption remains open.
  Further prerequisite qualified 2026-10-19: exact pose/FP interpolation and
  clocks, receipt values/current/previous state, deferred cues and CPU effects
  now prepare and publish together with retained retirement storage. Incoming
  native render routes/collision queries validate existing actual model/particle
  residency before mutation; the windowed caller privately stages those owners
  and paired lens preparation. Eight unskipped focused native/rendered tests,
  a separate real 240-frame Backwash control and ten GCC checks pass. See the
  [foreground preparation receipt](../../.local/foreground-receipt-adoption-20261019/verification.json).
  Full live adoption, pre-election/all-survivor readiness, delivery and physical/
  WAN validation remain open; this does not check M02 or enable migration.
  Further prerequisite qualified 2026-10-20: actual scene checkpoint/prepare/
  restore APIs preserve scenery, machine and prop poses, salted lifetimes,
  four-view caches, lighting/PRT banks and native prop section/submission state.
  Nine unskipped CTests pass against frozen native build `b55dbf868052`;
  cancellation, stale/foreign/executor/reentrant controls and normal lighting
  COW pass. The private build/scratch and one-off qualifier were removed;
  see the [scene qualification](../../.local/scene-checkpoints-20261019/qualification.json).
  Callback/material graph ownership, receipt integration and the other scene
  owners remain required; this does not establish complete scene readiness.
  Source integration candidate, 2026-10-20: the existing exact locomotion
  group is now connected from the owning cache through all16 runtime slots
  and their four metadata arrays into authentic receipt history and private
  foreground preparation/readiness. All writers invalidate preparation, and
  historical replacement/clear tokens retain outgoing adapters. The
  [eight-file candidate](../../.local/mcc-locomotion-runtime-20261021/handoff.md)
  initially passed managed generic compiler syntax only; the fresh native
  qualification below supersedes that syntax-only boundary. The windowed fixture observes
  real occupied/unavailable slots without inventing an occupied gait. Its
  separate full-group suffix driver initially used explicit offline reference
  inputs; those reference results do not qualify the MCC runtime. This candidate does not publish
  or activate a partial handoff; camera, GPU images, physical input and playing
  audio remain required before complete adoption/readiness.
  MCC component route, 2026-10-21: the
  [explicit MCC receipt/group driver](../../.local/mcc-locomotion-runtime-20261021/mcc-route/handoff.md)
  now uses the same genuine native checkpoint/host continuation/received-origin
  proofs and complete scene/HUD/pose/group fixtures without an external XBE.
  Explicit malformed, missing or non-MCC requests fail before world creation;
  the separate old reference invocation remains labeled. Source/independent
  reviews and managed syntax pass, with their scratch removed. The new
  [three-target/four-test native cohort](../../.local/mcc-online-m02-20261021/native/normal/01-current-mcc-gait-and-fullgroup/actual-result.json)
  now passes all four selected tests without skips; closed owner21732 was not
  reused. The group fixture covers all16 metadata slots with three
  occupied authored CPU adapters, including dormant ownership and a420-tick
  witness suffix; it does not assert16 active players. The windowed run observes
  eight occupied native gaits,324 proof-bound captures and unavailable mask0.
  Its505.523ms maximum frame wall still fails the500ms parent gate. No M02 checkoff,
  aggregate Ready, live activation or full timing/memory acceptance follows.
  Remaining closure groups: complete camera/GPU/material/resource borrowers;
  all four local seats and retained/released physical inputs; paired playing
  audio and its actual announcer/script/damage owners (M04); and one complete,
  validated foreground transaction over those owners. Existing focused CPU
  world/scene/HUD/pose/locomotion evidence does not close these integration
  groups. M05 timing/memory, M06 survivor readiness and M07 live activation
  remain separate subsequent acceptance gates.
- [ ] **M03 — Presentation continuity.** Complete exact scene/model/first-person,
  camera, pose, lighting, HUD, culling and active-effect history adoption.
  Verify active gameplay across the handoff; cold reset component controls
  alone do not establish continuity.
  CPU component milestone, 2026-10-20: actual authored HUD/scoreboard history
  passes 827 checks and 50 guard controls over 240 complete 1–4-seat 4:3 and
  widescreen output streams, Spartan/Elite and FFA/team. Native build
  `fabf113f7aa5` and all six component owner hashes are recorded in the
  [13/14 cohort receipt](../../.local/scene-receipt-integration-20261020-retry-10/qualification.json).
  The later stage-timed diagnostic passes 15/15 checks, including the complete
  authored HUD registry and locomotion components; final uninstrumented runtime
  integration remains incomplete after the next 18/19 cohort: exact CPU HUD
  capture succeeds, while rendered recovery binds only one required cut.
  The MCC-artwork variant was
  unregistered because its optional input pack is absent; this establishes no
  MCC UI, GPU-image, complete registry/runtime or migration acceptance.
- [ ] **M04 — Input and playing audio.** Adopt all four local seats/controllers,
  retained/released inputs, pending and already-playing game sounds and voice
  device ownership without stale-generation playback or duplicated actions.
  The [playing-audio readiness review](../../.local/mcc-playing-audio-adoption-20261021/readiness.md)
  identifies the real paired MultiplayerAudio/SDL stream owner, active packet
  cursors, fades/RNG and callback/script epochs. Its next transaction must hold
  callback exclusion through aggregate preflight/publication and retain displaced
  PCM/callbacks until after locks. Existing pending-cue snapshots and teardown
  APIs do not provide this continuity. Voice retains its separate generation
  fence and direct peer transport; microphone/codec queues must not be rewound.
  The [held-platform design](../../.local/mcc-playing-audio-adoption-20261021/platform-transaction-design.md)
  and [independent source review](../../.local/mcc-playing-audio-adoption-20261021/platform-transaction-independent-review.md)
  distinguish the monotonic physical device clock from restorable elapsed/fade
  and pause-aware presentation clocks. Epoch-bound mutations/completions, held
  preparation lifetime and paired Audio/script retirement remain mandatory;
  platform-only restoration cannot qualify aggregate readiness. The subsequent
  [held SDL implementation and qualification](../../.local/mcc-playing-audio-adoption-20261021/platform-implementation/validation-final.md)
  passes seven normal cases and six ASan/UBSan cases without skips on the exact
  reviewed four source pins. Genuine retained CTest logs, compiler/link flags,
  source comparisons and both verified cleanup receipts are recorded in the
  [result identities](../../.local/mcc-playing-audio-adoption-20261021/platform-implementation/validation-final.json).
  A subsequent [independent final audit](../../.local/mcc-playing-audio-adoption-20261021/platform-implementation/final-audit.json)
  rechecks the four current source identities, actual test logs and sanitizer
  flags, absent scratch parents and all13 recorded process identities. No owned
  path reference is found in readable procfs; permission-denied unrelated
  process surfaces and the lack of historical full descendant/group inventory
  limit that observation to the documented scope.
  Prepared target receipts are unusable before their exact publication and
  cannot revive canceled or sibling preparations. Publication performs no
  allocation, callback or PCM retirement under the held device lock; logical
  elapsed/presentation clocks restore while the physical device clock does not.
  Actual dummy-device callbacks and the real legacy producer compatibility
  route are covered; physical audibility and MCC media provenance are not.
  Complete paired Core/script/damage/class envelopes, strong cache/resource
  ownership, single-owner RNG restoration and the actual separate MCC announcer
  transaction remain required. Four-seat physical input and aggregate runtime
  adoption remain open. No migration or runtime installation is enabled.
- [ ] **M05 — Sustainable validated cuts.** Qualify bounded history/checkpoint
  memory and sustained transfer/replay catch-up under live gameplay; retain the
  exact matching native/presentation cut without unbounded backlog or memory.
  Source-bound union review, 2026-10-20: the
  [retained-owner audit](../../.local/mcc-memory-admission-20261020/handoff.md)
  identifies uncapped writable material preparation, capture staging before
  eviction, independently retained native exports/replacements, live PCM and
  resource growth. Shared committed/pending/candidate/reservation handles must
  count each actual backing once. The next memory seam is lifetime-bound
  admission through complete receipt capture/remember and the single aggregate
  preparation, including outgoing owners until their actual final retirement.
  Pending locomotion history adds its value snapshots and shared resource-bank
  lifetimes to this union. This is source evidence and an implementation
  dependency, not measured whole-runtime memory or a passing budget gate.
- [ ] **M06 — All-survivor readiness.** Advertise only actual complete native,
  presentation, input and audio readiness; agree membership/cut/epoch before
  promotion, preserve retry/cancellation, and reject stale or incomplete owners.
  Prerequisite qualified 2026-10-19: one paired publisher now sends the same cut
  to every accepted physical recipient; each independently validates/proves its
  own receipt and can export its exact paired world without stopping replay.
  Seven normal CTests, six GCC checks and verified scratch cleanup pass; see
  the [fanout/export receipt](../../.local/common-cut-fanout-20261019/verification.json).
  Further prerequisite qualified 2026-10-19: each physical recipient now stores
  its own independently validated native proof and exact foreground receipt
  before association. Fixed-membership commit/apply/release retains those
  recovery owners; rejection/Abort of an unbound later cut preserves the prior
  pair and permits a genuinely validated repair. Seven unskipped focused tests
  and nine GCC checks pass with verified scratch removal; see the
  [common-cut pin receipt](../../.local/common-cut-pins-20261019/verification.json).
  This is headless native/CPU ownership evidence. The complete prepared
  presentation/input/playing-audio barrier remains open; M06 stays unchecked.
- [ ] **M07 — Windowed runtime activation.** Connect the complete prepared
  adoption to normal host promotion and client endpoint rebind. Demonstrate an
  actual rendered match continuing after host departure with surviving players
  controlling it and hearing direct voice, then qualify default enablement.
- [ ] **M08 — Ordinary disconnect survival.** Finish nonhost leave/timeout and
  admission cancellation across supported native/custom modes. Preserve the
  remaining room, retained player/PCR rows and deferred source departures;
  resolve missing-peer confirmation/arbitration without fabricated deaths.
- [ ] **M09 — Repeated failure coverage.** Verify graceful exit, process kill,
  host silence/recovery, repeated replacement-host loss, interrupted takeover,
  couch-host departure and final-player departure across supported modes/maps.
  No stale authority, duplicated results or unintended room termination.

### N — Direct peers, host selection and mesh hardening

- [ ] **N01 — Automatic measured host policy.** Finish reciprocal reachability,
  latency/loss, upload and measured NAT compatibility across physical PCs;
  choose a host that reaches every owner and reevaluate surviving candidates.
  Keep party leader, player seat and gameplay host distinct.
- [ ] **N02 — Decentralized telemetry.** Continuously exchange bounded,
  authenticated peer matrices on the gameplay path, agree telemetry/membership
  generations and conservatively disqualify asymmetric-loss host candidates.
  Unknown NAT or path failure is not proof of cheating.
- [ ] **N03 — Host-silence quorum.** Integrate the requested greater-than-500ms
  suspicion probe, authenticated strict-majority nonhost votes, generation and
  membership fencing, replay rejection and safe stale-peer removal. Qualify
  false alarms, partitions and split-brain prevention in the real runtime.
- [ ] **N04 — Validated shadow queue.** Continuously and deterministically rank
  eligible peers by mesh latency/NAT health with stable ties; retain a complete
  validated successor and measure actual authority/transport transition time.
  An endpoint switch alone cannot satisfy seamless world continuation.
- [ ] **N05 — Outage and compatibility.** Verify direct match/voice survival
  through temporary signaling loss and master process restart, persistent
  admission and retry, negotiated protocol compatibility and old-epoch rejection.
  Confirm no default master/TURN gameplay or match-media relay is used.

### V — Voice coverage

- [ ] **V01 — Party/clan direct voice.** Complete the remaining menu voice
  transport conversion and verify joining, leaving, leadership changes and
  transition into/out of direct player-hosted match voice.
- [ ] **V02 — Original audiences.** Qualify proximity, team/FFA radio, living/dead
  audiences and death grace, mute/block preferences and postgame talk audiences
  in supported modes, including after host migration.
- [ ] **V03 — Every session phase.** Complete source-supported pregame,
  all-participant loading, in-game, postgame and LAN voice coverage.
- [ ] **V04 — Original audio behavior and link quality.** Finish supported radio
  audio/masking behavior and verify real encode/decode, bounded queues, reconnect,
  impaired links and stale-frame rejection without a master media fallback.
- [ ] **V05 — Physical couch audio.** Complete and test independent local
  microphones/headsets and per-seat routing with real controllers/audio devices;
  synthetic encoded packets do not close this gate.

### F — Native entry points, identity, friends and clans

- [ ] **F01 — Community-server joining.** Connect discovery/details to secure
  native admission and actual gameplay, with matching variant/map checks,
  capacity/errors, direct transport and clean return/requeue.
- [ ] **F02 — Quickmatch/Optimatch/Playlists.** Regress all native entry points
  through real admission, loading, play, PCR, Done and requeue with accepted
  parties and the delivered identity/transport/migration changes.
- [ ] **F03 — Identity regression.** Preserve real Steam default-browser login,
  cancellation/sign-out, hidden IDs, chosen Halo names and verified secondary
  accounts. Verify optional dual-proof Discord linking retains clan/history
  ownership and rejects collisions without enabling unauthorized social access.
- [ ] **F04 — Live Discord friends/invites.** Finish live linked-account friends,
  presence, invitation send/receive/accept, cold launch and party return with
  two real accounts; verify block changes, expiry, revocation and failed sends.
- [ ] **F05 — Party lifecycle.** Verify native leader/follower, full/scrolled
  roster, invitations, withdrawal, explicit playlist consent, launch, migration
  and requeue without losing accepted members.
- [ ] **F06 — Complete clans.** Verify create/join/invite/manage/roles/messages/
  leave/dissolve, Minor/Major lineup admission, clan matchmaking, progression
  and retained audit/history with real users. Original clan-rank policy/artwork
  also requires P05 and U03.

### U — MCC-imported ONLINE and clan UI

- [ ] **U01 — Asset and privacy audit.** Verify every ONLINE/clan screen uses
  high-resolution fonts/bitmaps imported from MCC, with Vista used only as the
  UI reference. Require the MCC-only asset pipeline with source provenance,
  `ONLINE` capitalization,
  hidden account IDs and chosen screen names, without the obstructing title logo.
  This covers Quickmatch, Optimatch, playlists, parties, friends/invites, the
  server browser, every clan screen and post-carnage reports. Each screen must
  resolve its authored images, fonts and layout data from the prepared MCC owner
  used by onboarding and the release.
  Qualify the current direct MCC v13 widget/locale/identifier and shared-MCC-bank
  ONLINE/clan route against a fresh private MCC import. Verify every required
  bitmap/font and any optional sampling manifest belongs to that same import;
  catalogue generation alone does not qualify runtime selection. Legacy Vista
  roots and sidecar importers do not satisfy MCC-only fresh-install or release
  acceptance.
  Check the actual MCC inventory for every required bank and keep missing or
  changed resources explicit; do not silently require a separate Vista donor.
  The [current source audit](../../.local/scene-receipt-integration-20261020/mcc-ui-checklist-audit.md)
  records the newer direct route and remaining delivery checks; the
  [earlier wiring audit](../../.local/runtime-receipt-frontier-20261019/mcc-ui-wiring.md)
  describes its older source cohort. Neither is visual acceptance.
  Partial inventory evidence, 2026-10-20: all 563 prepared menu images and 40
  fonts match their recorded hashes; required ONLINE banks and clan menu/panel
  owners are present. The [MCC inventory audit](../../.local/scene-receipt-integration-20261020/mcc-ui-inventory.md)
  records the missing fourth dedicated-server icon comparison state explicitly;
  the current browser skips that widget and does not request that state.
  Partial native evidence, 2026-10-20: all three explicit MCC-only clan, party
  and matchmaking HTTP cases pass against native build `6c9ed551007c` from the
  [18/19 source cohort](../../.local/scene-receipt-integration-20261020-retry-12/qualification.json).
  They use the complete private cache and its independently verified converted
  layout; the supported native layout fallback remains outside this test route.
  These synthetic loopback service/CPU checks do not establish real account or
  invitation delivery, GPU pixels, fresh raw extraction or widescreen acceptance.
  Their MCC case names identify the selected menu-data owner; they do not
  establish complete runtime payload provenance. The legacy material/loading
  payload dependencies recorded under D01 remain release blockers.
  Fresh-import, submitted-screen and release checks remain required before
  checking U01.
  Further partial evidence, 2026-10-20: clan, party and social Vulkan cases pass
  on the RTX 5080 using the selected MCC menu cache; all 54 images across six
  scenes and nine aspect ratios are represented in the reviewed contact sheets.
  They cover long/scrolled names, 100 clan members, a full 16-member party and
  invitation options, with no observed panel overflow or obstructing title logo.
  See the [visual review and limits](../../.local/scene-receipt-integration-20261020-retry-13/visual-review.md).
  These synthetic records and existing-cache captures do not close fresh import,
  complete payload provenance, actual delivered native/launcher or live social
  acceptance. U01/U02/U03/D01 remain unchecked.
  Native-font prerequisite, 2026-10-20: normal MCC frontend/HUD prompt selection
  now uses authenticated native MCC glyphs without the separately authored
  overlay. The actual native CPU control passes all331 visible bindings and
  source geometry/UV checks. Import/reuse/package and registered MCC font-owner
  controls pass; see the [source and qualification limits](../../.local/mcc-prepared-snapshot-20261020/README.md).
  Changed native glyphs require new submitted-screen/widescreen acceptance;
  prior overlay-based captures do not qualify their appearance.
  Delivered-policy follow-up, 2026-10-20: authenticated older frontend proofs
  could still select separately supplied prompt/movie/loading producers in
  normal setup/reuse/package dispatch and could admit their runtime for launch.
  The [MCC-only delivery correction](../../.local/mcc-delivered-presentation-20261020/handoff.md)
  closes both routes with a required MCC-capable runtime update. Historical
  proofs and original assets remain preserved; they are not upgraded. All128
  focused setup, admission, reuse and packaging cases pass on source-stable
  inputs, including exact-donor legacy refusal and reference-file preservation.
  A separate stale package-fixture correction preserves its cleanup, output-race
  and archive assertions. Generated scratch is removed; no binary is retained.
  These synthetic controls do not qualify an installed runtime, a fresh full
  MCC import, a relocated release or changed widescreen pixels. Genuine MCC
  material/program ownership and movie-role delivery remain D01 dependencies.
- [ ] **U02 — Widescreen and real dimensions.** Regress actual clan/party/menu/
  PCR geometry, long/scrolled rosters, hit targets and controller navigation at
  the supported aspect/scale matrix in the delivered build, using the
  MCC-imported UI banks from U01. Match the reference Vista panel dimensions
  and anchors when fitting widescreen; verify the submitted screens.
- [ ] **U03 — Remaining original rendering.** Finish matchmaking progress-ring
  materials/animation, PCR emblem shaders and original clan-rank presentation
  using MCC imports within the real screen geometry, with Vista/TU1.5 retained
  only as the applicable visual/behavior references.
  Partial MCC-owner evidence, 2026-10-20: both rank-icon sizes contain all 50
  frames in the selected MCC cache; all 100 pages match its manifest. The actual
  MCC `wigl` supplies 50 ranges, 0..48 individually and 49..127 in the final row.
  See the [bounded clan-rank audit](../../.local/mcc-frame-liveness-20261020/u03-clan-rank-audit.md).
  The authenticated catalog already supplies the selected clan's capped display
  level; preserve its existing exclusion of raw rank and clan IDs. Correct
  match/team presentation and native lobby/PCR rendering remain open; clan role
  and a profile's maximum rank are not substitutes.
  The [publication-owner follow-up](../../.local/mcc-frame-liveness-20261020/u03-publication-owner-followup.md)
  now traces the selected clan stat into the original session/lobby rank field
  and finds Xbox PCR's conditional publisher. The retained getter meaning and
  TU1.5 operands remain unqualified. Preserve PC PCR's proven NONE producer and
  raw-rank privacy; no rank renderer or U03 acceptance is claimed.

### P — Playlists, modes and competitive rules

- [ ] **P01 — Final-2010 catalog.** Restore and version the fullest primary-
  evidence-supported final Xbox Live rotations. Identify unrecovered downloaded
  configuration explicitly; obtain missing evidence or a user scope decision
  before claiming full historical restoration.
- [ ] **P02 — Original matching behavior.** Finish unresolved service ordering,
  query pagination, party/team balancing and direct-path/QoS policy against
  recovered TU1.5 code and primary explanations, including Max Hoberman's.
  Regress casual, ranked and clan gathering/launch with actual admitted parties.
- [ ] **P03 — Native mode coverage.** Complete required objective/game engines
  and selected variant settings, scoring, teams, overtime/end conditions, PCR
  and migration behavior before enabling their historical pools.
- [ ] **P04 — Custom party modes and Tombstone.** Regress authored Fiesta Slayer,
  green-zombie Infection Slayer and Action Sack under direct transport and
  migration; bundle definitions in `data/tags` and releases. Add Tombstone to
  suitable supported Slayer/Team Slayer/CTF/Fiesta and other evidenced pools
  only when the corresponding native engine is accepted.
- [ ] **P05 — Competitive completion.** Finish evidence-supported quit penalties,
  later player/clan ranking and original rank-update/display owners. Version
  policies and verify exactly-once persistent progression across disconnects,
  migration and report retries without inventing missing historical values.

### C — Four-player ONLINE couch play and privacy

- [ ] **C01 — Physical four-seat play.** Verify four local players on one PC can
  join/queue/play against another PC with correct controllers, view ownership,
  HUDs, profiles, scoreboards and widescreen layout.
- [ ] **C02 — Mixed verified/guest identities.** Verify independent secondary
  identities and guests through admission, migration, PCR and requeue. Keep all
  local PCR rows; publish no personal stats/history for unverified guest seats,
  regardless of client-supplied flags or display names.
- [ ] **C03 — Couch lifecycle.** Qualify supported local-seat join/leave,
  controller disconnect/reconnect and primary/host departure without borrowing
  another account's stats, input or voice ownership. Audio also requires V05.

### R — Organized post-carnage database and recovery

- [ ] **R01 — Complete after-match records.** Validate final admission, retained
  departures, result digests/confirmations and normalized full report details
  after normal and migrated matches; do not publish unfinished matches as final.
- [ ] **R02 — Safe retry and persistence.** Verify transactional/idempotent
  report/progression writes, missing-peer arbitration, master restart, offline
  retry and database backup/restore without duplicate or lost accepted results.
- [ ] **R03 — Future website lookup.** Preserve privacy-safe report APIs,
  pagination, stable public aliases, verified linked Discord username lookup
  and old summary-only records. Keep Steam personas separate from identity keys;
  the future website itself is outside this completion request.

### D — Delivery, real-world acceptance and cleanup

- [ ] **D01 — Coherent release build.** Build a consistent source/dependency
  cohort and run focused regressions for affected paths; package authored
  variants, official SDK dependencies and required notices. Verify fresh MCC-only
  onboarding supplies all required runtime assets, including gameplay/audio and
  ONLINE/clan UI, without Vista/Xbox installs, executables, extracted caches or
  historical donor registries. Preserve installed MCC files and source manifests;
  keep offline Vista/TU1.5 reference inputs outside the release dependency path.
  Provenance blockers found 2026-10-20 included the bundled original loading
  tone and a retained native material library with original-map/XBE donor
  provenance. The later V3 loading milestone above replaces the normal source
  path with authenticated MCC audio and passes genuine native component checks;
  its full-cache/package/launcher and physical playback gates remain open.
  Native shader/material and bundled movie asset-source gates remain unresolved.
  A strict movie-source candidate now omits the separate MP4 bank from normal
  MCC import/reuse/package/runtime selection and passes84 Python controls plus
  the native routing CTest. Its source mapping for Classic intro/credits/attract
  remains unassigned; the MCC-only directive above requires actual source
  ownership and playback qualification before delivery. This is not a
  replacement movie bank or delivered release; see
  the [candidate and limits](../../.local/mcc-movie-source-policy-20261020/README.md).
  Copying a donor payload into an MCC cache does not make it MCC-derived.
  Qualify the actual MCC-derived runtime owners before checking D01; see the
  [fresh-import dependency audit](../../.local/mcc-online-ui-rendered-20261020/fresh-import-next.md).
  The [loading-source audit](../../.local/mcc-runtime-provenance-20261020/loading-source.md)
  authenticates an available MCC loading track without claiming original-tone
  identity. The [material-source audit](../../.local/mcc-runtime-provenance-20261020/material-source.md)
  distinguishes copied payloads from port implementation code. A bounded
  [MCC HUD-family inventory](../../.local/mcc-material-owner-inventory-20261020/owner-seam.md)
  confirms four complete programs in authentic MCC shared-map allocations;
  their pass selection, state/remaps, shader semantics and direct renderer
  adapter remain unqualified. No legacy bank is relabeled as MCC-derived.
  Loading playback prerequisite qualified 2026-10-20: the new native stereo
  owner passes its component, authenticated-MCC-track and original material
  regressions in one frozen linked cohort, with no skips; see the
  [three-test receipt](../../.local/mcc-material-callgrind-20261020/03-mcc-genuine-audio-corrected/cohort.json).
  The real archive entry decodes to the pinned complete PCM hash, and the
  owner preserves rate/channels, finite overshoot, one-shot EOF/restart/mute
  and caller FP state. SDL dummy output is exercised; physical playback is
  unverified. Full decoded one-shot playback is explicit native policy, not
  evidence of MCC-authored segment/loop timing.
  The subsequent [six-check cohort](../../.local/mcc-material-callgrind-20261020/08-mcc-v3-genuine-producer-and-bridge-corrected/cohort.json)
  qualifies explicit V3 producer/verification APIs and strict V3-only idle
  DesktopLoading admission against a privately generated genuine MCC bank.
  All six registered checks run without skips; the old V2 case is separately
  labelled reference evidence. No legacy tone is present in the new private
  import root or bank. A found linked-parent traversal guard is repaired and
  covered; the private-tree reference fixture now copies its own two known
  reference files rather than weakening ownership checks. Normal preparation,
  onboarding/provenance selection and package dependency activation were
  subsequently changed to explicit V3 in the source milestone above. The
  complete fresh import, delivered package/launcher, authored loading-interval
  and physical-device acceptance remain open.
- [ ] **D02 — Both launchers.** Install the qualified native runtime, verify
  each launcher's selected paths/hashes and retained asset banks, and repeat
  startup/login/ONLINE smoke checks. Document and resolve any remaining
  gameplay-readiness prerequisite before claiming delivered playable ONLINE.
- [ ] **D03 — Safe backend deployment.** Verify retained state, private secrets,
  schema/integrity, isolated backup restoration, actual running release hashes
  and HTTPS health on `master.halo2forever.net` before/after activation.
- [ ] **D04 — Independent-PC/WAN acceptance.** Complete real matchmaking,
  community join, couch gameplay, graceful/abrupt host migration, direct voice,
  PCR and requeue across independent consumer NATs. Record latency/loss/NAT and
  host-quality results separately from localhost and VPS probes.
- [ ] **D05 — User-visible/hardware acceptance.** Verify rendered migration,
  MCC-imported ONLINE/clan widescreen screens, real social/clan flows, physical controllers
  and bidirectional headset voice in the qualified release. Keep unavailable
  required participation explicit and unchecked.
- [ ] **D06 — Test-storage cleanup.** Use the required owned scratch lifecycle;
  audit/remove unused generated executables, objects/build trees and captures
  after retaining bounded diagnostics. Verify absence and report reclaimed
  storage/explicitly retained large artifacts; preserve source, references,
  launcher runtimes and unrelated active work.
- [ ] **D07 — Final checklist audit.** Reconcile every item above with dated
  evidence and the installed/deployed release, verify earlier accepted behavior
  has not regressed, and resolve every required open gate. Only then mark the
  existing full ONLINE goal complete.

Cleanup is part of completion, including each intermediate validation run:
delete unused generated test executables, object/archive copies, private build
trees and scratch captures after retaining bounded diagnostics. Verify their
removal before reporting a step finished. Test source and required original
reference evidence remain part of the project; do not delete them to reclaim
storage. Audit older generated artifacts before removing them so active builds,
installed runtimes and concurrent work remain intact.
The user's latest continuation reaffirms this cleanup requirement for the final
delivery as well as intermediate tests; record reclaimed storage and any
explicitly retained large artifacts in the completion evidence.

Additional cleanup on this continuation: 44 unused, untracked generated ELF
test executables were removed from `build-multiplayer`, reclaiming
5,703,429,208 logical bytes (5,703,528,448 allocated bytes). Both launchers still
select the preserved `build-multiplayer/halo2_native`; its inode, size and
modification time are unchanged. The audited files had no observed live users
or active builds in that directory. No test sources, libraries, assets,
reference inputs, frozen source or concurrent scratch were removed. Evidence:
`.local/promoted-input-continuity-20261018/old-test-cleanup.json`.

## Required architecture

- **Xbox TU1.5 is the sole gameplay and networking parity target** (user confirmation,
  2026-10-14). Earlier-image decompiles are supporting evidence only where their
  correspondence to TU1.5 is established. TU1.1 construction/compatibility is
  not an ONLINE completion requirement. High-resolution ONLINE/clan images,
  fonts and layout must be imported from MCC, matching Vista UI references,
  through the MCC-only import pipeline (user clarification,
  2026-10-19). TU1.5 remains the behavior/reference target, Vista the UI reference;
  MCC imports are the only runtime asset path for all gameplay, audio and UI
  (reaffirmed 2026-10-20). Neither reference game is a release prerequisite.
- The master verifies identity, stores social/clan state, discovers servers,
  matches players, authenticates connection setup and accepts post-carnage
  reports after matches. It does not simulate or relay active match gameplay
  or match voice in the intended default architecture.
- One actual player's PC hosts the authoritative world. Other players connect
  directly over encrypted peer UDP. STUN address discovery may assist setup;
  TURN and master WebSocket media fallback are disabled in the new direct path.
- Automatic host choice tests actual lobby connectivity, latency and upload
  capacity. A host must reach every physical connection owner. Select again
  among surviving eligible PCs during migration. Do not confuse the party
  leader, matchmaking gatherer, player seat and simulation host.
- Keep original Halo 2 behavior faithful wherever recovered code or primary
  public evidence establishes it. Native replacements and unresolved original
  units/coefficients must be labeled explicitly; never fill gaps by guessing.

## Acceptance checklist

| Area | Required final behavior | Current acceptance boundary |
|---|---|---|
| Identity | Steam primary login; private account IDs and chosen Halo screen names; preserve existing Discord accounts/history through verified linking | Real Steam OpenID/account verification and the operator-controlled native frontend login/normal-browser confirmation passed on 2026-10-16; live linked social/gameplay acceptance remains open |
| Native navigation | Quickmatch, Optimatch and Playlists launch real admitted online games; server browser joins community servers | Match launch exists; community server joining remains outstanding |
| UI | ONLINE capitalization; high-resolution MCC-imported UI assets/fonts matched to Vista references throughout ONLINE and clans; readable screens without the obstructing Halo 2 logo; correct real clan-screen dimensions and widescreen bounds | Source now includes direct MCC v13 widget/locale/identifier and shared-bank selection. Qualify that route and all required images/fonts against fresh MCC-only onboarding and the delivered widescreen/native-clan screens; the optional sampling catalogue and legacy Vista donor paths do not establish acceptance |
| Social | Complete Discord friends/presence, native invitations, accepted parties and correct launch/return behavior | Existing SDK/native flows require regression and remaining live checks |
| Clans | Complete create/join/invite/manage/leave flows, matchmaking and original supported clan-rank presentation | Existing clan flows; remaining competitive/presentation coverage stays open |
| Couch players | Up to four local players per PC can queue/play together against others; verified secondary accounts retain their own stats; guests retain local PCR rows but publish no personal stats | Eight-player/two-PC native fixture exercised; physical controllers and independent voice devices are separate gates |
| Gameplay transport | Default ONLINE gameplay travels directly between players, with one player authoritative; no master media relay | v12 installed in both launchers and deployed; local native/rendered flows and one consumer-to-Vultr UDP probe passed; two consumer NATs remain unverified |
| Host quality | Select physical hosts from actual reciprocal reachability, latency and upload tests; reevaluate surviving candidates under agreed membership | Native session setup now runs bounded direct UDP upload/echo tests and broadcasts complete peer rows over authenticated DTLS. The coordinator ranks physical owners using measured reciprocal loss, upload bottleneck and RTT while retaining playlist connectivity thresholds; all-legacy setup retains its prior RTT policy. Four real socket peers exchange 768 probes beside payload traffic; two separate eight-seat native sessions select a physical host from actual measurements and retain fresh candidate lists. Five owner cases, 44 backend checks and selected owner ASan/UBSan/leak/TSan pass at the frozen protocol 86 cut. These are observed application delivery rates under bounded load, not ISP capacity or anti-cheat attestations. NAT classification, agreed telemetry/membership generations, automatic validated shadow selection, survivor reelection, full runtime delivery and WAN/hardware acceptance remain open |
| Match voice | Direct peer proximity, team/FFA radio, dead-player audiences, muting and postgame voice; complete source-supported lobby/loading, LAN and device coverage | Match/postgame voice now uses direct UDP; party/clan menu voice still uses its relay and remains to convert; LAN voice, all-participant loading voice, independent couch headsets/mics, masking and original radio audio remain open |
| Master outage | Peer simulation and voice continue through temporary signaling loss; reports retry safely afterward | Temporary master-socket outage acceptance passed:89 simulation ticks and86/88 audible frames continued, then reports uploaded; master-process-restart admission persistence remains open |
| Disconnect survival | A nonhost departure does not end the room; a graceful or abrupt host loss preserves and resumes the actual world | TU1.5 native Match and localhost UDP graceful/silent-peer departures retain roster/profile/result rows and original deferred corpse/equipment without added numeric deaths. The master explicit nonhost-leave and result-review paths preserve remaining peers and store complete admitted rows with survivor-confirmation metadata. A real native nonhost DTLS/signaling shutdown now leaves the host session running. Custom native-owned worlds, complete source admission cancellation, unannounced missing-peer result arbitration, production delivery, physical/WAN checks and host migration remain open |
| Migration | Preserve complete player/unit/weapon/inventory, allocation identities, RNG, timers, projectiles/grenades, props, devices, physics, recording/results, local profiles and voice; promote/rebind a validated survivor | Coupled TU1.5 checkpoints and the 29-operation source journal retain complete native/CPP state, typed inputs, actual returns, clocks, identities and local-view bindings. A negotiated two-cut direct-peer pipeline overlaps transfer/decode with ordered native validation. Matched cold/background fixtures validate 82 phases, reject wrong chains and forged identities with a prefetched successor, repair from full cuts and compare four complete worlds. The background retention case validates 122 phases, drains both admitted batches across actual host loss, survives worker teardown, imports/rebinds four recipient views and continues ten native ticks after old-host departure without fabricated deaths or unfinished reports. Transport normal/ASan/UBSan/TSan and selected native-owner sanitizer checks pass; current-root syntax checks pass. Normal runtime disconnect and stalled-host timeout paths retain a validated recovery world before teardown. Extended rendered scheduling shows reduced backlog but was interrupted by OS memory pressure; sustained catch-up remains unqualified. Owning private camera checkpoints also reproduce driver, passenger and death trajectories across independent native executors in the retained cohort, with complete-world equality and selected owner sanitizers. Native fatal readback now avoids premature FP publication and stale-unit command delivery. Owning host connection/input/camera sidecars also resume the same six-player world across native executors, retaining four remote couch views, queued/consumed action serials and exact complete snapshots without rebirth; normal and selected network-owner sanitizers pass against a freshly built immutable current-source cohort. A private replay pause now admits both pipeline cuts before controlled native execution; repaired current-cohort cold/background cases validate 82/122 accepted phases, with selected receiver-owner ASan/UBSan/leak checks and real UDP graceful/silent-peer departure regression. Explicit camera/host and paired complete-world wire codecs now bind the entire connection/input/camera state to room/generation/membership/world content. A real two-process direct-UDP case imports the paired state, reproduces four native phases and exact snapshots, and retains ten private ticks after old-host shutdown; normal and selected owner sanitizers pass. These fixtures retain producer view mapping, do not elect/promote a host and are not current full runtime delivery. Runtime publisher/receiver integration now negotiates paired full/replay cuts and retains validated host/input/camera state at the exact accepted native boundary. A production two-process stream verifies 70 native phases, two-cut prefetch, complete world/host equality, downgrade rejection, owning retention and ten ticks after actual old-host shutdown; normal and selected owner sanitizers pass. A fresh matched-source native rebuild also passes host-loss role rebinding: four promoted couch seats retain pending/consumed input and camera state while an existing three-seat client follows the replacement endpoint for twelve native ticks, rejecting old-host traffic and retaining native allocation/result identities. Selected owner sanitizers, paired stream and real UDP departure regressions pass; this role test supplies its generation decision and logical transport explicitly. Private hardware continuity, broader mode/caller/combat/PCR and scenery lifecycle coverage, generation-bound identity continuity, agreed membership/cut/epoch certification, actual host/transport/profile/controller/voice promotion, memory qualification, delivery and physical/WAN acceptance remain open. See [continuity evidence](player-host-continuity-progress-20261013.md). |
| Historical playlists | Final-2010 target with the fullest evidence-supported rotations/game coverage; casual/ranked matching follows recovered/public Halo 2 logic, including Max Hoberman's explanations | Final downloaded catalog not recovered; full objective engines/rotations remain open |
| Custom modes | Finish and bundle authored Fiesta Slayer and Halo-3-inspired Infection Slayer, including green zombies, under data/tags and release packaging; include Action Sack | Existing authored variants/packaging must survive transport/build changes |
| Tombstone | Include the installed map in sensible supported Slayer, Team Slayer, CTF, Fiesta and other suitable pools | Fiesta active; staged Slayer/Team Slayer and unfinished objective coverage need their native engine acceptance |
| Competitive policy | Finish supported quit penalties, later ranking behavior and original clan-rank display without inventing missing historical values | Remaining policies/evidence gates stay open |
| Post-carnage history | Upload after the match, validate admission/results/confirmations, store organized persistent report details, support later website lookup by verified Discord username, exclude guest personal history | Existing DB/history and guest tests; preserve across peer migration and master recovery |
| Delivery | Rebuild/install both native launchers, safely deploy backend with state backup/restore verification, package authored data and dependency notices | Every changed delivery needs fresh recorded verification; source presence alone is insufficient |
| Test storage cleanup | After validation, delete unused generated test binaries, objects, private build trees and scratch captures; keep small reports, commands, source hashes and failure diagnostics. Preserve test source, original assets/reference evidence, frozen source snapshots, launcher-selected runtimes and unrelated active work | User addition, 2026-10-18. Use `tools/run_tests.py` for CMake validation and `tools/run_test.py` with `HALO2_TEST_SCRATCH` for one-off probes. Verified on this migration step: 105 unused generated files and two private configure trees removed, reclaiming 5,300,504,655 bytes; new native/transfer build trees, per-test scratch and temporary snapshot caches are absent. No new large test artifacts retained. Preserve this lifecycle for subsequent validation |

## Modern mesh extension requirements

Component progress, 2026-10-18: [peer host/cut agreement](peer-host-agreement.md)
now implements generation-bound two-phase majority agreement on the replacement
PC and its exact retained cut. Partition, healthy-majority, interrupted-proposer,
duplicate/authentication, parser and concurrency cases pass, as does an actual
four-endpoint direct-UDP host-shutdown case beside player payloads. Normal,
ASan/UBSan/leak and owner TSan checks pass. Native readiness in the direct test
is declared fixture metadata; production session integration, native/controller/
voice promotion and full generation fencing remain outstanding. Disposable
test builds and compiler scratch are removed; small receipts remain under
`.local/peer-host-agreement-20261018`. The full goal stays active.

Follow-on session progress, 2026-10-18: an internal direct-session gate now
exchanges these votes over real admitted DTLS links, fences gameplay/encoded
voice until each native owner acknowledges its exact learned decision, advances
the host/epoch/physical and couch membership together, and preserves the
original native/PCR allocation roster across generations. Four real session
workers pass two host socket/signaling teardowns with 16/12/8 couch seats and
resumed payloads. Ready native worlds and role completion are declared fixtures.
Normal and selected owner ASan/UBSan/leak checks pass. Broader C-stack TSan
found an atomic/non-atomic global free-chunk counter conflict in the pinned SCTP
dependency; its live reads are now patched. A subsequent repeated run completed
one two-handoff process, then found a separate global tick read/write conflict.
Dependency-wide race qualification remains open, including the system OpenSSL
diagnostic and an intermittent ICE setup error. Ordinary matchmaking keeps the
new gate disabled until actual native/controller/device promotion, candidate
selection, negotiated compatibility, after-match admission/results and full
runtime delivery are verified. Receipts are under
`.local/peer-session-migration-20261018`; continue disposable test cleanup.
The latest broader seven-case selected-owner sanitizer run passes six cases;
the ordinary measured-host fixture reports OpenSSL allocations at child-process
exit. Its cleanup diagnosis remains open alongside the SCTP tick race. The new
two-handoff session and both old continuation transport modes pass in that run.

Subsequent teardown progress: the earlier “tick” race is now narrowed to callout
pending/active flags crossing queue and association lock domains. A real
two-thread dependency regression fails before the new timer-flag lock patch and
passes afterward. Shared last-owner RTC cleanup also resolves the reproduced
ordinary-session OpenSSL exit leak; independent rooms, rejected construction,
fresh restart and concurrent lifetimes pass. Compiled C/C++ TSan passes those
owners and three independent two-handoff session processes with uninstrumented
system libraries explicitly excluded. Actual native world/controller/headset
promotion, crypto-internal race qualification and production/default migration
remain outstanding. See `.local/peer-teardown-20261018` and the
[transport integration contract](peer-host-agreement.md).

The final broader teardown run passes all nine focused CTests with compiled
C/C++ ASan/UBSan and leak checking, including the previously leaking ordinary
session and both old transport modes. C/C++ TSan passes three focused CTests,
with the host-loss fixture repeated across three independent processes. Its
system-library exclusion stays explicit. Native world/device handoff, default
migration and delivery remain open; these checks do not close the full goal.

Native takeover progress: the receiver now consumes its exact paired native cut
only after its own session learns the replacement decision, restores the new
host's couch views/input/cameras, applies original deferred departures and keeps
media fenced until explicit activation. An owning retained-worker export can
also reconstruct that host on a different executor after the worker is destroyed.
Two actual twelve-player/three-session cases pass with complete native state,
old native world/session teardown, two-survivor agreement, four promoted seats,
an existing four-seat client, twelve subsequent connected native ticks and direct
encoded voice packets. A same-tick unrecorded native write is rejected and repaired
before readiness. The new case also found and fixed the continuation codec's
incorrect eight-group limit for FFA admission rows. Four focused CTests pass with
selected owner ASan/UBSan and leak checking, including paired replay and prior
role/session regressions. These are internally gated, in-process native/transport
checks with explicit candidate selection and activation; rendered adoption,
controller/headset and decoded-audio handoff, automatic compatibility/candidate
policy, after-match recovery and WAN/delivery acceptance remain open. See
`.local/native-peer-takeover-20261018/verification.json` and the
[native handoff contract](peer-host-agreement.md). All owned generated trees are
removed, including the final 2,662,993,859-byte supervised workspace; bounded
diagnostics and source manifests remain, with no new retained large artifacts.

Audio handoff progress: retained voice now retires its old decoder/talker,
microphone and device queues asynchronously, rejects held old-generation
microphone/receive batches and acknowledges retirement before native completion
opens the successor epoch. Real four-session/two-host-loss checks resume fresh
gameplay, voice packets and decoded synthetic stereo after each handoff without
reopening devices. Five ASan/UBSan/leak CTests and three TSan CTests pass; both
session cases repeat across three fresh processes in TSan. System shared-library
internals are excluded from that TSan run. Runtime callers now suspend source
frame work during fenced handoff phases and keep OS events responsive; those
changes have current-source syntax checks, not delivered rendered acceptance.
The complete goal remains active for renderer adoption, physical input/headsets,
automatic compatibility/candidate policy, after-match recovery, memory/WAN and
delivery. See `.local/voice-handoff-20261018/verification.json` and the
[audio retirement contract](peer-host-agreement.md). All generated test
build/scratch trees are removed; no new large artifacts are retained.

Runtime ownership progress, 2026-10-18: both runtime loops now resolve the live
Match/world/transport through `MultiplayerRuntimeAuthority`, which retains the
initial immutable map resources and owns publication/recovery lifetimes. A frame
scope rejects nested frames and role replacement, even without a recording
publisher. Private staging leaves the original role visible until the exact
native takeover activates. Two actual twelve-player native cases pass the
foreground/background-retention paths, active-frame rejection without ownership
loss, exception unwinding, resident cache lifetime, four-seat host adoption and
twelve connected native ticks/encoded voice packets. Five selected-owner
ASan/UBSan/leak CTests pass, including role, paired-stream and synthetic decoded
voice regressions. The current-root renderer passes warning-clean syntax checks;
its automatic camera/input/presentation adoption handler, default migration,
hardware/WAN, memory and delivery acceptance remain open. Receipts are under
`.local/runtime-authority-20261018/verification.json`. The owned 2,711,637,063-byte
validation workspace and compiler scratch are removed; only bounded reports,
commands and source hashes remain, with no new retained large artifacts. The
complete goal stays active.

Measured survivor progress, 2026-10-18: replacement preference now requires
native-ready cuts and fresh reciprocal measurements among the fixed survivors,
excluding only the agreed predecessor host. The gated path binds QoS, native
admissions/cuts and votes to the same membership revision. The selector does not
locally drop a second missing PC or invent speed/NAT metrics for a sole survivor.
Four root C/C++ ASan/UBSan/leak CTests pass, including normal measured-host setup
and decoded voice regression. Two root C/C++ TSan CTests pass; measured migration
runs in three independent processes, with system-library internals excluded.
Seven selected native-owner ASan/UBSan/leak CTests also pass. Both actual native
takeover paths now choose their ready PC from measurements; foreground recovery
polls until replay-worker/native-world destruction completes before releasing
the wrapper. Transport cases cover 16/12/8/4 seats across three host losses, with
native readiness declared there and actual twelve-player native worlds tested
separately. Receipts: `.local/survivor-selection-20261018/verification.json`.
The 2,731,636,383-byte native validation workspace, three other private builds
and compiler scratch are removed; bounded diagnostics remain without new large
artifacts. Automatic scheduling/retry and false-alarm/readiness lifecycle,
additional agreed membership changes, rendered/input/camera/cache adoption,
default compatibility, memory/hardware/WAN, after-match recovery and delivery
remain open. The full goal stays active.

Rendered-owner progress, 2026-10-18: the foreground authority exposes the restored
native camera and tick-idempotent retained local input. Both actual native
takeover cases send three pre-cut DTLS input pulses for four couch seats,
consume one before capture, then consume the remaining two after activation
without repeating their serials on a second poll of the same tick. Machines,
props and scenery can retire their presentation lifetimes at a completed frame
while preserving resident geometry/material slots. Original-asset CPU checks
rebind identical salted handles at an earlier cut and compare the geometry.
All ten selected-owner ASan/UBSan/leak CTests pass in the coherent native/renderer
cohort; current-root runtime syntax and critical source hashes also pass.
Receipts: `.local/rendered-handoff-20261018/verification.json`. The supervised
2,961,309,972-byte generated workspace and all six recorded compiler scratch
trees are removed; only bounded diagnostics remain. Test source, original
fixtures, frozen inputs and installed runtimes are preserved. This supplies
owner APIs, not a completed windowed adopter: hardware input suspension/drain,
interpolation/HUD/effects/GPU query retirement, automatic scheduling and the
other migration/delivery acceptance gates above remain open.

Automatic-election progress, 2026-10-18: the gated session worker now starts and
retries a measured native-ready proposal without a positive caller proposal.
Paced ballots preserve the strict 500 ms suspicion boundary, fixed physical
quorum, healthy-host veto and highest previously accepted cut; duplicate
messages cannot postpone a failed-proposer retry indefinitely. Six broader
C/C++ ASan/UBSan/leak CTests and three broader C/C++ TSan CTests pass. Measured
and decoded-audio sessions each run in three independent TSan processes, with
system-library internals excluded there. Expanded agreement checks also pass
ASan/UBSan/leaks and unsuppressed owner TSan across sixteen peers, 64 competing
preference schedules and concurrent scheduling. Seven selected-owner native
CTests pass, including both actual twelve-player takeover paths using automatic
session elections and explicit native staging/activation. Evidence:
`.local/automatic-election-20261018/verification.json`. The supervised
2,732,562,234-byte generated tree and all six root build/scratch trees are
removed, with no new large retained artifacts. Automatic readiness retention
and false-alarm lifecycle, full recovery/adoption coordination, rendered/device
integration and the remaining default/delivery/hardware/WAN gates stay open.

Native preparation/commit progress, 2026-10-18: the runtime now owns a minimal
gameplay recipe and stages its learned host or surviving-client role at a
completed frame. Foreground import uses one disposable native world, validates
the original producer mapping and full admissions, then binds recipient views;
it does not rebuild a second temporary world. Host/world ownership or the
prepared client endpoint now commits before the session publishes its epoch.
Old client destruction runs outside the session lock. Invalid/stale/foreign
endpoint preparations and refused native adoption cannot publish a new epoch.
Actual native cases also reject wrong profiles/views, retain no presentation
callback, preserve recipe selections across later menu changes, and restart a
survivor's failed old shadow. Seven final selected-owner native CTests, three
root C/C++ ASan/UBSan/leak session CTests, two root C/C++ TSan CTests with three
fresh measured/audio processes each, and current-root syntax checks pass.
Evidence: `.local/native-prepare-coordinator-20261018/verification.json`.
The supervised native workspace, both root build/scratch trees and all compiler
scratch are absent; no new large artifacts remain. The outer recipe ended with
signal status143 after the final successful CTest run while awaiting finish;
bounded passing reports and source hashes remain, but its optional final trace
copy and pre-cleanup size measurement were not produced. Native preparation is
still synchronous. Automatic readiness/false-alarm handling, rendered/input/
HUD/effects/GPU adoption and actual activation in the runtime loops, timing/
memory/device/WAN qualification, default compatibility and delivery remain open.
The local-input audit identified post-cut promoted-client commands as the next
required follow-up; the qualification below covers that owning input bank.

Promoted input carryover, 2026-10-18: the native client now retains its latest
owning input packet. Before retiring that client, host activation validates its
original session, physical owner, cookies, profiles, slots and monotonic action
serials against the independently accepted host bank. It imports newer controls,
pending presses and completed interactions without changing delivered counters,
the native world or camera checkpoint. Repeated refresh and refused adoption
preserve the same pending input. Foreign contexts/owners/slots, early consumption
and malformed input are rejected before mutation.

Seven selected-owner native ASan/UBSan/leak CTests pass. Both actual twelve-player
native recovery cases send two additional couch-input pulses after the sealed
cut, deliver all four remaining pulses once, preserve latest movement/look/zoom
and deliver one completed interaction per local seat. They continue twelve native
ticks and twelve direct encoded voice packets with an existing four-seat client.
Current-root syntax checks also pass. This uses the coherent frozen native ABI
plus hashed current owners; generated/native/RTC/system interiors remain outside
the selected-owner instrumentation. Evidence:
`.local/promoted-input-continuity-20261018/verification.json`.

The successful supervisor removed 27,199 generated private files totaling
2,738,244,560 logical bytes, including copied source, build and cache; compiler
and cleanup scratch are also absent. Together with the 44 old test executables
above, this continuation reclaimed 8,441,673,768 logical bytes. Bounded reports
remain; no new large artifacts are retained. Gameplay and match/postgame voice
remain direct peer traffic. Default migration remains gated: actual hardware
input pause/drain/release and renderer/HUD/effects/interpolation/GPU/device
adoption, automatic readiness/false-alarm handling, wider mode/generation/PCR
coverage, sustained timing/memory, delivery and physical/WAN acceptance remain
open with the rest of this full goal.

Windowed input admission, 2026-10-18: the actual runtime now suspends fresh
keyboard/controller commands throughout the transport fence and promoted-bank
drain. It handles OS/focus/device events, discards relative mouse motion, freezes
interaction hold time, and delivers the bank through `continue_local_input()`
before hardware consumption. Saved movement/look/zoom and interaction intent
cannot be overwritten by fresh polling, scripts or the current prompt. Catch-up
stops after the final carried tick so hardware is reacquired on a new frame.
Existing controller button baselines remain intact; a changed device drops only
that view's old hold state. A completed keyboard interaction survives resume and
intervening key events before the next held request starts.

The current adapter passes 287 actual SDL event-queue checks under ASan/UBSan,
and current-root syntax passes separately. A full current-source freeze, with
the latest adapter/fixture updates, builds the windowed runtime and passes its
154-tick TU1.5 input regression using normal Vulkan. It covers ordinary movement,
jump, firing, exact reserve-to-magazine reload transfer, mouse look, focus and
overlay capture; it does not trigger a rendered host migration. Khronos GPU
validation layers are unavailable. The source-bound reload trace corrected an
obsolete assertion against a separate C++ presentation timer; no gameplay reload
code or timing was changed. A first full build was interrupted with143; its
workspace was removed and its cause remains unestablished. The successful retry
and incremental qualification removed 34,345 generated private files totaling
1,063,983,811 logical bytes. Adapter build/compiler scratch are also removed;
no new large artifacts remain. Evidence:
`.local/runtime-input-handoff-20261018/verification.json`.

Default migration remains gated. Complete presentation/interpolation/HUD/effects/
GPU/device borrower adoption and native query-ring retirement, automatic readiness
and false-alarm lifetime, actual rendered migration, independent physical couch
controllers/headsets, current-ABI native migration coverage, sustained memory/
timing, delivery/WAN and the other full ONLINE requirements remain open. Matches
and match/postgame voice retain the direct player-hosted architecture.

Native/GPU lens-query ownership, 2026-10-18: incoming private hosts now retire
their predecessor's CPU query requests through unchanged original `20E30`, the
per-request primitive used by original `20700`. Count/span admission and the
native executor barrier precede mutation. This preserves unallocated bitmap
bits and high-water count, changes no non-presentation source bytes apart from
the declared caller stack, and supplies no GPU samples. Retirement is part of
native preparation before sealing its adoption boundary. The renderer query
owner can explicitly bind the retired incoming bank, allocating private state
before retiring old GPU work; failed/foreign/duplicate binds are rejected.

Sixteen unchanged-XBE differentials and eight current-source native/session
ASan/UBSan/leak CTests pass. Both actual twelve-player promotion cases carry33
declared pending CPU requests across the authenticated cut and retire them
before adoption, then preserve the existing input/voice/simulation checks.
The RTX5080 GPU assertions and Khronos validation pass for45 original quads,
visible/occluded/empty coverage, stale-result retirement, rejected binds and
fresh query-ID reuse. The GPU process CTest still fails LeakSanitizer at
shutdown. A minimal Vulkan instance/device control without Halo, SDL or query
pools independently reproduces the driver allocation families; a live-device
probe also finds allocations in NVIDIA glcore/glsi. No GPU process leak-free
claim is made. Evidence: `.local/lens-query-handoff-20261018/verification.json`.

The supervisor removed34,502 generated files totaling1,512,947,527 logical
bytes, including its full current-source copy, build, captures, validation-layer
package and extracted libraries. Compiler/control scratch are also removed.
Only bounded reports and the5,148-byte required original retirement hash
fixture remain; no new large artifacts are retained. Full rendered adoption,
other presentation/HUD/effects/device owners, automatic readiness/false-alarm
lifetime, physical/WAN and other full-goal gates remain open. Default migration
is still gated and gameplay/match voice remain player-hosted.

Dormant client lens resources and final-draw ownership, 2026-10-18: eligible
clients in the gated peer-migration path now prepare authored map flare images
before renderer material upload, while their native producers stay dormant.
The final-draw owner can bind a separately prepared retired bank at a completed
frame, preserving resident material identities and replacing its private SDK,
headers, FIFO, camera, counters and source borrower. Six rebindings over two
actual native worlds pass their assertions, including a cold owner and preserved
native adoption boundaries. This component case does not execute the normal
ONLINE client window loop or visible flares across an actual migration.

The refreshed private native executable builds. Six CPU/reference CTests pass;
both RTX5080 functional reports and Khronos validation pass. Both GPU processes
still fail unsuppressed LeakSanitizer checks with the previously reproduced
driver allocation families. Evidence:
`.local/renderer-lens-adoption-20261018/verification.json`. The supervisor removed
34,761 generated files totaling 1,519,555,707 logical bytes, including the
executable, source copy, build, captures and temporary validation layer. Disk
snapshot/compiler scratch are removed too; no new large artifacts are retained.
The ONLINE/clan Vista asset selection is unchanged. Coordinated preparation of
all presentation owners, effects/HUD histories, interpolation, source pointers,
device adoption, readiness and actual runtime activation remain outstanding.
Default migration remains gated and the full goal remains active.

HUD/event lifetime preparation, 2026-10-18: completed-frame primitives now
discard HUD shield/winning/radar/voice histories, rebase the validated device
event journal without replaying accepted requests, and retire unsubmitted
discarded-world sound cues. Resident images/materials, authored Vista shield
sampling offsets and local input bindings survive. Eleven focused CTests pass
with selected-owner ASan/UBSan/leak checks; the new HUD cases compare16 declared
world replacements,16 stale-history controls and320 complete CPU vertex streams
against independent fresh HUDs, covering both artwork paths, both species,
one through four local views and4:3/16:9. These are native presentation policies,
not recovered Xbox migration reset routines or rendered migration acceptance.
Evidence: `.local/hud-event-handoff-20261018/verification.json`.
The supervisor removed34,163 generated files totaling1,653,612,981 logical
bytes; all owned build/compiler/source-check scratch is absent. No new large
artifacts or installed runtime remain from this step. The full adopter still
must prepare all owners, preserve accepted active effects/audio, rebase source
borrowers/interpolation, invalidate radar upload ticks, and publish the role
atomically. Readiness, false-alarm, rendered/physical/WAN and full-goal gates
remain open; default migration stays gated and matches/voice remain direct.

Active effect presentation, 2026-10-18: NativeMuzzleEffects and NativeWorldEffects
now capture owning local presentation cuts and prepare replacements before an
allocation-free publication. Live particles/emitter identities, clocks, poses,
decals, lights/spatial/query caches and unsubmitted sounds survive; discarded
future state is removed. World definition links rebase into each private copy.
The caller supplies accepted-world collision callbacks and long-lived visual
RNG bindings; mismatched aliasing and foreign/consumed tokens reject. Snapshots
own their random values and exclude predecessor collision callbacks. Two TU1.5
selected-owner ASan/UBSan/leak regressions pass, including a cut with24 particles,
11 decals and1 light, forty further match ticks against an independent batched
owner, two muzzle restores and twelve continuation ticks with new allocations.
Evidence: `.local/effect-presentation-handoff-20261018/verification.json`.
The supervisor removed34,152 generated files totaling1,078,856,808 logical bytes;
all owned build/compiler/check scratch is absent and no large artifact/runtime
is retained. These local cuts still need exact accepted-native-cut association
and coordinated capture/publication with attachment caches, interpolation,
devices, source borrowers and playing audio. Readiness, rendered/physical/WAN,
delivery and all other full-goal gates remain open. Default migration stays gated.

Grouped effect/cache cuts, 2026-10-19: SourceMapEffects and
SourceCombatAttachments now retain complete local caches and reject capture
after incomplete synchronization. EffectPresentationContinuity captures them
with both effect owners and shared visual RNG, supports later binding after
asynchronous native validation, and stages all replacements before one
allocation-free publication. Exact bound context/peer/seal fields and executor/
owner/token checks reject stale or foreign publication. Six selected-owner
ASan/UBSan/leak CTests pass. A real TU1.5 native checkpoint on Turf restores
tick45 with4 map births,4 combat instances and88 particles after ten discarded
ticks, then continues twenty ticks without duplicated attachment births.
The first-person presenter is empty in this combined case; active FP coverage
remains the separate regression. Room context is fixture metadata and no live
role/GPU/device migration occurs. Evidence:
`.local/effect-cache-cut-20261019/verification.json`.
All owned scratch is removed:34,272 files and1,263,200,521 logical bytes in the
main workspace, plus compiler/check scratch. No large artifact/runtime remains.
The host sidecar already retains per-peer snapshot sequences, while accepted
gameplay snapshots may change ownership within one tick. Runtime readiness must
prove the admitted snapshot-sequence/native-cut association; tick agreement
alone is insufficient. That integration, remaining presentation/device/audio
owners, GPU/native role publication, readiness/false-alarm lifetime, delivery
and rendered/physical/WAN/full-goal acceptance remain open. Migration stays gated.

Accepted snapshot receipts, 2026-10-19: LanClient now returns one opaque owning
receipt for every accepted state, preserving its connection, generation,
membership, sequence and SHA-256 body identity. The host sidecar retains the
last sent body identity for each physical peer. A local receipt can be proved
against that exact independently authenticated/natively validated paired cut;
tick equality or a newer sequence alone cannot substitute for the matching
body. Receipt-based effect captures require this proof for later binding.
The sidecar format is now H2HOST02 and explicitly rejects H2HOST01; the current
native gameplay protocol89 is unchanged. Coherent feature negotiation and
delivery of the sidecar version remain required before activation.

Four selected-owner ASan/UBSan/leak CTests pass, including a real TU1.5 native
cut, four couch seats, a same-tick late join, an older valid body relabeled
with a newer sequence, old/foreign receipts, codec roundtrip/version rejection,
host continuation, role rebind and grouped effects. These use logical endpoint
adapters and locally validated cuts; no live DTLS/WAN/runtime/GPU migration is
established, and the new receipt case has empty effect presenters. Evidence:
`.local/snapshot-origin-20261019/verification.json`. All recorded scratch is
absent; the supervisor removed34,314 generated files/1,271,417,932 logical bytes.
No large artifact or runtime is retained. Normal runtime still needs bounded
receipt/presentation history, readiness binding and an ownership refresh for
newer same-tick states without advancing per-tick animation twice. Full owner
adoption, publication, delivery and rendered/physical/WAN acceptance remain open.
The full goal stays active; default migration stays gated. Gameplay and match/
postgame voice remain direct and player-hosted.

Native same-tick adoption, 2026-10-19: the production client now consumes each
state with its exact receipt and refreshes same-tick roster/ownership instead
of retaining only devices. Surviving interpolation endpoints remain intact;
departed/replaced owners cannot reacquire retired history. Gait resolves new
lifetimes without another channel tick, aim advances zero ticks and repeated
FP clock pairs do not dispatch another sound. Already completed shot stamps
baseline at admission and at an already-connected viewport startup.

The real TU1.5/localhost-UDP four-view renderer passes with a private Xvfb
display, RTX5080 and Khronos validation: tick2,39 receipts,36 same-tick refreshes,
28 frames,192 unchanged completed gait observations, four actual local weapon
cues without replay, and a late join/departure with the original retained report
row. Five component checks also pass with selected interpolation-owner
ASan/UBSan/leak instrumentation. A separate native late-audio regression remains
red: the FP muzzle control emits10 cues while the WORLD client submits20 from
the same weapon; the actual source effect has two equal-name locations.
TU1.5 WORLD/FP location/part/sound admission must be established before changing
either playback or expectations. Native Match currently exports clock pairs
only for producer-local views; this four-view case has no remote FP clock pairs
and does not qualify remote FP channel audio. Evidence:
`.local/replica-refresh-20261019/verification.json`.

All recorded scratch is removed, including34,710 generated files/1,528,119,727
logical bytes in the owned build workspace. No large artifact/runtime remains.
The full source freeze and explicit updates retain later unrelated root drift.
Bounded accepted presentation history/readiness, authentic remote FP ownership,
the red sound regression, full borrower/audio/GPU adoption, coherent delivery
and rendered-migration/physical/WAN/full-ONLINE acceptance remain open. Default
migration stays gated; gameplay and match/postgame voice stay player-hosted.

WORLD/FP sound admission, 2026-10-19: the native late-audio mismatch above is
resolved. The WORLD exporter had bypassed the original location iterator and
published encoded FP records as extra WORLD records. Whole unchanged TU1.5
17DCE0 replays cover65,536 node words in each of modes0/1/3 and verify the actual
17D210 caller's mode0 instructions. WORLD projection now applies that exact
admission before transform resolution, preserving FFFF and separate WORLD
records even when marker names match. No cue-count deduplication was added.
The actual native weapon has one WORLD and one FP record and now publishes one
WORLD record. Ten postjoin cues produce ten SDL submissions; the retained
prejoin cue is silent. All eight focused normal checks and six selected-owner
ASan/UBSan/leak cases pass, including the four-view renderer/continuity cases.
Evidence: `.local/world-location-admission-20261019/verification.json`.

All recorded scratch is absent:34,792 generated files/1,553,200,174 logical
bytes from the main workspace, plus compiler/oracle scratch and a duplicate
reference copy. Only the small65,536-byte original admission fixture and bounded
reports remain. Later concurrent multiplayer/header/runtime-test changes are
preserved and recorded separately from the coherent tested source cohort.
Authentic remote FP clocks/ownership, bounded presentation history and actual
readiness, full borrower/audio/GPU/native adoption, delivery and rendered-
migration/physical/WAN/full-ONLINE acceptance remain open. Default migration
stays gated; gameplay and match/postgame voice remain player-hosted.

Implement as modular C++20 owners over the existing physical peer table, with
thread-safe telemetry and vote snapshots using bounded queues, lightweight
locks/atomics, and no Discord identifiers in peer presentation.

1. **Client-side voter quorum.** More than 500 ms without host packets enters
   `SUSPECTED_DEAD` and sends a lightweight authenticated verification probe to
   the active nonhost mesh. A strict majority of the agreed nonhost membership
   must corroborate host silence before revoking that authority generation.
   Bind every vote to the match, host generation, voter and membership view;
   reject replay/duplicate/nonmember votes. Membership removal must preserve
   quorum intersection, rather than allowing each partition to shrink its own
   denominator. A timeout alone must not manufacture a permanent cheating ban.
2. **Decentralized telemetry.** During the pregame lobby, continuously probe
   other PCs on the actual gameplay path and exchange bounded latency/loss/NAT
   telemetry matrices. Analyze asymmetric loss claims and conservatively
   disqualify suspect candidates from host eligibility. NAT/path failures are
   not proof of cheating; preserve unknown NAT classifications until measured.
3. **Asynchronous shadow queue.** During gameplay, deterministically order
   eligible peers by the requested mesh-latency/NAT health policy, with stable
   private peer-ID tie-breaks and agreed membership/telemetry generations.
   Identify a shadow host in advance and retain its complete validated world.
   On certified failure, transfer authority and rebind transport within a few
   frames where actual continuation permits. Do not label an endpoint switch
   or a display snapshot seamless simulation migration.

These are new hardening requirements, not claims that the supplied descriptions
of every legacy flaw have already been verified against original Halo 2.
The user requested these after direct transport and the migration foundation.

## Evidence and completion rules

Track focused tests, full native builds, localhost native-process tests, WAN
NAT/throughput tests, visible gameplay, physical controllers/headsets and actual
multi-PC migration separately. Synthetic Discord fixtures do not establish
live authorization, and election fixtures do not establish world continuity.
Retain dirty/concurrent workspace changes and Vista ONLINE/clan behavior while
moving final asset selection to the MCC-derived bank required by U01/D01.
Do not mark the goal complete while required work or acceptance remains.

Related evidence:
[direct peer integration](direct-player-hosted-online.md),
[migration progress](player-host-continuity-progress-20261013.md),
[world inventory](native-migration-state-audit-20261013.md),
[original host behavior](halo2-host-migration-evidence.md),
[completion audit](online-completion-audit.md),
[historical target](online-default-target.json),
[couch players](online-couch-players.md).

## Direct peer delivery milestone

Both launchers now select `.local/direct-peer-20261013/runtime/halo2_native`.
The backend release is `/opt/halo2/releases/20261014T021753Z`; all36 preexisting
retained tables and the master key matched the isolated backup/restore check.
The new `match_result_consensus` table was empty. The delivery evidence is
[verification.json](/home/eric/Developer/Halo-2-Decompiled/.local/direct-peer-20261013/verification.json).
This closes the bounded direct-transport delivery stage, not the full goal or
gameplay host migration.

## Current identity and hostname directive, 2026-10-16

The user now requests Steam as the primary ONLINE login, using the existing
`STEAM_KEY` repository secret in `TheRunaway5/Halo-2-PC-Website`, and the game
service at `https://master.halo2forever.net`. This supersedes the earlier
required-Discord-login decision. Steam IDs remain private; chosen Halo screen
names and four local seats remain supported. Existing Discord accounts, clans
and history must be preserved, with explicit dual-identity proof for linking;
matching display names must never merge accounts. Discord social consent may
remain optional for its friends/invites. Guests without their own verified
identity continue to be excluded from persistent personal PCR publication.
The website apex remains on GitHub Pages. All other requirements and gates in
this goal remain in force. The migration is not accepted until the native
login/queue/social/result paths and deployed backend are verified.

Steam delivery update: the real Steam/account login and deployed new hostname
passed verification. The current full native engine and both launchers have
been built and installed with Steam identity and the bundled Discord Social SDK;
six focused CTests and nine-aspect Vista UI checks passed. The user completed
login in the native frontend and confirmed that both ONLINE and the normal
browser confirmation work. Controlled test tabs alone reproduced the browser
block; the game's default-browser route is accepted. The launcher still enforces the existing MCC gameplay-readiness
gate, and live Steam-linked friends/invites and gameplay acceptance remain open.
See `steam-identity-migration-20261016.md` for exact evidence and boundaries.


## Foreground paired-cut evidence, 2026-10-19

The receiver now exports an immutable `SourceAuthorityHostProof` with its exact
validated world/host cut through the worker status mutex. Only completed native
paired validation can create it. Foreground client receipts are checked against
that owning table's exact connection, generation, sequence and body. No native
executor/world borrower changes threads. Current proof clears on incomplete
replay, cancellation and consumption; retained historical proof survives later
validation/worker destruction, and old-generation receipts fail after rebind.
A historical proof does not assert live-world readiness or authorize promotion.

Five focused normal cases pass after correcting the stream fixture to keep its
foreground gameplay pump alive beside replay (70 actual accepted receipts).
Three native cases pass with selected receiver/fixture ASan/UBSan/leak checks;
two receipt/effect controls are uninstrumented. Real local DTLS takeover checks
both physical survivors' exact receipts across twelve admitted seats, receiver
and background-retention paths, twelve resumed native ticks and twelve encoded
voice packets. No rendered migration, physical headset/controller or WAN
acceptance is claimed. GCC C++20 syntax and source/cleanup audits pass.
Evidence: `.local/validated-host-proof-20261019/verification.json`.

The owned build removes34,386 files/1,358,022,460 logical bytes. All recorded
scratch is absent; no large artifact or runtime is retained. Concurrent source
edits, original assets/references and launcher runtimes remain intact. Bounded
completed presentation/receipt history, actual readiness for all survivors,
authentic remote FP ownership, full borrower/audio/GPU/native adoption, timing/
memory, coherent delivery and rendered/physical/WAN/full-ONLINE acceptance
remain open. Default migration stays gated; the full goal remains active.
Gameplay and match/postgame voice remain direct and player-hosted.

## Completed CPU effect receipt history, 2026-10-19

Bounded `EffectPresentationHistory` now captures completed local CPU effect
owners with exact accepted receipts. Context comes independently from the
session. Duplicate retention is idempotent; evicted/reordered/retired receipts
cannot be recaptured or replaced by another same-tick sequence/body. Failed
capture preserves older entries. The windowed runtime finishes poses/clocks
and drains queued muzzle cues for each accepted receipt, then retains128
entries under the existing internal migration gate plus one separately owned
matched cut. This is a count policy, not an Xbox constant or byte-budget claim.
Binding requires the receiver's independently validated native world/host proof;
it does not authorize or advertise migration readiness/activation.

Nine normal cases pass after correcting the new ONLINE fixture to advertise the
resolved native variant, preserving strict sidecar checks. The actual four-view
ONLINE runtime captures257/257 receipts, retains at most128 entries, binds two
real native paired cuts and renders463 frames over local DTLS. It uses fixture
identity,30Hz host pacing and10Hz snapshots; no rendered migration or physical/
WAN acceptance is claimed. Whole fixture peak RSS is7,408,412 KiB, including
collocated native worlds and renderer, so memory/catch-up qualification remains
open. Three selected history/group/fixture ASan/UBSan/leak cases and two
uninstrumented companion controls pass. Raw-LAN effect barriers and the native
late-audio regression remain green. Current-root syntax and source audits pass.
Evidence: `.local/effect-history-20261019/verification.json`.

All recorded scratch is absent:34,857 generated files/1,742,441,803 logical
bytes removed from the owned workspace, with no large artifact/runtime retained.
Concurrent changes, original assets/references and launchers are preserved.
Remaining gates include checkpoint sharing/byte budget and sustained catch-up,
authentic remote FP/event ownership, readiness for all survivors, full scene/
model/FP/device/source/playing-audio adoption, coordinated GPU/native publication,
coherent delivery and rendered-migration/physical/WAN/full-ONLINE acceptance.
Default migration stays gated; the full goal stays active. Gameplay and match/
postgame voice remain direct and player-hosted.


## Active-light checkpoint resource sharing, 2026-10-19

Effect checkpoints now share immutable BSP light geometry/atmosphere/terrain,
while light pools, spatial links, fades and query state remain independent.
A matched normal allocator probe holding128 active-light cuts reduces added
live heap from95,980,784 to40,763,552 bytes (57.53%);24 particles,11 decals and
one light retain their exact state and continuation. Four normal cases pass
before/after and three selected-owner ASan/UBSan/leak CPU checks pass. The
four-view ONLINE fixture exercises two full128-entry history turnovers and
two actual native paired-cut associations in both runs. Whole-process RSS does
not show a reduction (6,842,848 versus7,408,516 KiB); a client byte budget and
sustained catch-up/memory qualification remain open. No source/network/UI/media
behavior is changed. Evidence:
`.local/effect-light-residency-20261019/verification.json`.

All recorded scratch is absent;34,871 files/1,358,228,683 logical bytes removed.
Only bounded reports and small baseline source/hash evidence remain, with no
large artifact or installed runtime retained. Concurrent work and original
assets/references/launchers are preserved. Full readiness, authentic remote FP,
complete scene/model/device/source/playing-audio and GPU/native publication,
coherent delivery and rendered-migration/physical/WAN/full-ONLINE acceptance
remain open. Default migration stays gated and the goal stays active. Match
gameplay and voice remain direct and player-hosted.

## Staged lens presentation transaction, 2026-10-19

Query and final-draw owners now expose separate prepare/readiness/commit
operations. `SourceLensPresentationHandoffTu15` prepares both before any old
GPU retirement, then rechecks both and publishes their private state together.
Cancelled/partial preparation preserves both old owners. Executor, renderer,
incoming retired ring, owner generation and intervening frame activity are
checked again at commit, including unsubmitted frames and consumed tokens.
The enclosing adopter must still retain/quiesce the exact incoming world and
validate its complete native seal. This component does not authorize adoption.
GPU retirement retains its existing device wait; no latency bound is claimed.

The private native executable builds; 11 normal focused CTests pass. Two real
native worlds exercise 20 final-draw views and paired publication. Five rejected
or cancelled preparations preserve actual old GPU results; valid publication
retires them without changing the incoming native seal or resident materials.
The 45 original query quads and 45 final-draw callback controls pass with empty
Khronos validation logs. Current-root GCC syntax checks pass. This run does not
claim sanitizer, visible-flare migration, physical or WAN acceptance. Evidence:
`.local/lens-transaction-20261019/verification.json`.

All recorded private scratch is absent: 35,086 generated files / 1,529,265,786
logical bytes removed, with no executable, capture or installed runtime retained.
Two later unrelated source changes are recorded and preserved. Complete scene,
model, device, playing-audio and coordinated native adoption, survivor readiness,
timing/memory, delivery and actual rendered migration remain open. Default
migration stays gated; the full goal stays active. Gameplay and match/postgame
voice remain direct and player-hosted, with the Vista ONLINE/clan UI unchanged.

## Scene lifetime retirement seams, 2026-10-19

Dynamic objects, weapon projectiles and released grenades now have explicit
completed-frame retirement methods. They clear owner pose/light/LOD and model/
glow draw history while retaining authored resident banks. Together with the
existing scenery/machine/prop methods, all six scene retirements and shared
geometry history reset are now `noexcept`. The prop view entry also rejects a
fifth view before indexing its retained source cache, fixing the failure found
by the existing LOD regression.

The native executable builds. Five normal focused CTests and the same five
selected-owner ASan/UBSan/leak checks pass without suppressions. The new case
uses declared inputs with actual TU1.5 resources: four views, unchanged full
handles, frame300 to frame0, empty retired borrowed draw lists, fresh-scene
model/light/LOD/glow equality and unchanged resident material identities. This
does not prove native birth, an exact retained lighting-history checkpoint or
live ONLINE migration. Current-root GCC syntax and source audits pass; four
unrelated later Python changes are preserved. Evidence:
`.local/scene-retirement-20261019/verification.json`.

All recorded private scratch is absent; 34,926 generated files / 1,320,113,892
logical bytes removed, with no binary, capture or installed runtime retained.
The enclosing runtime still must invoke these seams within complete native/
GPU/device/audio adoption. Exact scene-history continuity, survivor readiness,
timing/memory, delivery and actual rendered/physical/WAN/full-ONLINE acceptance
remain open. Default migration stays gated and the full goal stays active.
Gameplay and match/postgame voice remain direct and player-hosted.

## Runtime presentation publication hook, 2026-10-19

Native runtime host promotion and surviving-client rebind now accept a prepared
presentation commit. It runs after decision/native/admission/audio validation
and successor allocation, before native role/endpoint changes and transport
publication. Refusal preserves the pending role for retry; successful publication
must not allocate, block or perform device I/O. Runtime reentrancy is rejected
before it can enter the locked session. Callers must capture/preflight all
owners first and leave them unchanged when returning false.

Three normal and three selected-owner ASan/UBSan/leak CTests pass. Both real
twelve-player direct-session variants exercise refusal/retry, once-only host
publication of an exact receipt-bound four-owner cold CPU effect cut, unchanged
native seal, client publication before endpoint rebind, frame/foreign-executor
rejection and runtime reentrancy rejection. Each case continues twelve native
ticks and twelve encoded direct voice packets after host loss, with no master
gameplay/media relay or unfinished report publication. This is local native
component evidence, not rendered/active-effect or physical/WAN acceptance.
The private native executable builds; current-root GCC syntax and source audits
pass. Evidence: `.local/runtime-presentation-commit-20261019/verification.json`.

All recorded scratch is absent; 34,956 generated files / 1,345,909,397 logical
bytes removed. No binary/runtime is retained or installed; six later unrelated
source changes are preserved. The windowed loop still needs complete prepared
adoption. Existing lens retirement waits for GPU work and must defer that
retirement outside the locked commit before using this hook. Active scene/FP/
effect/history, GPU/device/playing-audio adoption, all-survivor readiness,
timing/memory, delivery and rendered/physical/WAN/full-ONLINE gates remain open.
Default migration stays gated and the full goal remains active. Gameplay and
match/postgame voice remain direct and player-hosted.

## Deferred lens GPU retirement — M01 checked, 2026-10-19

Prepared renderer retirement now retains an actual device use, detaches the old
GPU query owner and cached results with swaps, and consumes eligibility without
destroying the caller's token. Query/draw/group publication retains retired CPU
and GPU owners until the caller releases the token after the enclosing native
activation returns and its session lock is released. That later executor-local
destruction still waits for real GPU work; no few-frame latency bound is claimed.
Draw/loading, resize, harvest and immediate retirement invalidate stale tokens.
Lease eligibility checks now read atomic state without taking the device gate;
new-use admission and retirement remain serialized. The renderer executor is
unconditional, including builds without optional particle graphics.

A coherent private native build and 12 focused CTests pass. Real Vulkan link
observers record zero wait/destruction/query-read calls during both renderer
and paired publication; later token destruction records one device wait, two
query-pool destructions and the expected buffer/memory/pipeline/layout cleanup.
Six rejected/cancelled paired preparations preserve actual pending GPU results.
Two actual native worlds retain their sealed boundary and resident materials;
the original 45 query quads and 45 final-draw callbacks/25 quads still pass.
The device lease regression and nine current-root GCC syntax probes pass,
including renderer builds with and without particle graphics. Three Khronos
logs are empty. This is normal real-GPU/component evidence, with no new
sanitizer, visible-flare migration, physical-controller/headset or WAN claim.
Evidence: [verification.json](../../.local/lens-deferred-retirement-20261019/verification.json).

All 17 selected owners match the qualified 15,903-input frozen source cohort;
five later unrelated root changes are recorded and preserved. Cleanup removes
35,248 generated files / 1,552,133,164 logical bytes (1,649,754,112 allocated),
with no binary, capture or installed runtime retained. All recorded private
paths/processes and the cleanup/finalizer scratch are absent. Shared filesystem
free-space change is recorded separately from removed-file size.

M01 is checked; M02 is the next task. The exact runtime/event cut, complete
scene/model/FP/camera/HUD/input/playing-audio adoption, all-survivor readiness,
catch-up/memory/timing, coherent delivery and rendered/physical/WAN/full-ONLINE
acceptance remain open. Default migration stays gated; the full goal remains
active. Gameplay and match/postgame voice stay direct and player-hosted, with
Vista ONLINE/clan assets and widescreen requirements preserved.

## Owning runtime receipt/frontier history — M02 partial, 2026-10-19

An accepted continuity receipt now retains its immutable decoded native state
and exact couch slots across later batches and client destruction. The actual
windowed loop captures that receipt, grouped CPU effects, full-salt runtime
event/binding frontiers, undelivered sound/request queues and a freshly captured
pending-effect cue checkpoint as one entry. Internal predecessor tracking and
exact native proof reject foreign, incomplete, stale or substituted receipts;
capture failure preserves older completed entries. Already-playing audio and
the complete scene/HUD/input/resource borrower set remain separate owners.

The coherent private native build and all six normal CTests pass. The actual
four-view direct-DTLS runtime accepts 259 receipts, captures 258 with one initial
unproven predecessor left unretained, peaks at 128 entries, binds two native cuts
and renders 381 frames. Whole-fixture peak RSS is 7,098,512 KiB, including native
worlds/renderer; this is not an isolated history-byte budget or catch-up proof.
The actual late-join source impulses are submitted once to SDL; physical audio
is unverified. Four unsuppressed ASan/UBSan/leak tests pass with four affected CPU
owners and their four fixture TUs instrumented; native-generated/dependency
interiors and the rendered runtime remain outside that instrumentation scope.
Nine current-root GCC syntax checks pass. Evidence:
[verification.json](../../.local/runtime-receipt-frontier-20261019/verification.json).

An initial sanitizer takeover failure was traced to the collocated fixture
leaving direct native traffic idle during background shadow warm-up. A controlled
6.002-second idle reproduces peer removal before resumed snapshots. The fixture
now services real neutral traffic through a six-second warm-up without advancing
the native tick or consuming carried input. Both variants pass normal and selected
sanitizer qualification, then continue all twelve gameplay ticks/direct voice
packets. Production timeouts are unchanged; this does not qualify migration latency.

All 17 selected owners match the 15,910-input frozen cohort. Thirty later root
changes are recorded and preserved; the isolated cohort does not qualify their
combined release. Cleanup removes 35,095 generated files / 1,602,889,260 logical
bytes (1,700,376,576 allocated), verifies all recorded private paths/processes
absent and retires nine one-off helpers (54,316 bytes) after retaining reports,
commands, source hashes and failure diagnostics. No binary/capture is retained or
installed. Shared available-space changes are recorded separately from file size.

M01 stays checked; M02 stays unchecked and the full goal stays active. Next is
complete prepared world-borrower adoption, followed by the remaining M03-M07
history/audio/input/readiness and windowed activation gates. Gameplay and
match/postgame voice remain direct and player-hosted. The user's MCC-only Vista
ONLINE/clan source requirement is recorded in U01/D01; the bounded UI audit found
catalogue-to-runtime metadata/pack and painter-selection gaps, still unaccepted.

## Exact surviving-client replica adoption — M02 partial, 2026-10-19

A client now prepares its exact owning decoded state and accepted scene,
device, prop and composed collision replica together before endpoint publication.
Opaque proof, connection/context/cut/couch identity and same-owner receipt checks
reject substituted banks. Preparation preserves the later live state; cancellation,
foreign retry and refused presentation publication preserve it too. Successful
publication restores the cut, keeps outgoing input ownership and retains replaced
storage until outside the locked session publication. The first real successor
packet establishes its own new-generation receipt.

The coherent native build and three normal CTests pass, including both native
takeover variants, decoded rollback and four-view rendered history. Two additional
ASan/UBSan/leak CTests pass without suppressions; only four core owners and two
fixture TUs are instrumented. Eight current-root GCC checks pass. The 23 selected
owners match the complete 15,936-file frozen cohort; 16 later unrelated root
changes are preserved. This does not qualify their combined release. Full results
and cleanup: [verification.json](../../.local/client-replica-cut-20261019/verification.json).
After finalization, concurrent MCC fingerprint changes landed in the two network
owners. They are preserved and recorded in the
[source-boundary receipt](../../.local/client-replica-cut-20261019/post-finalization-drift.json);
this milestone qualifies its frozen cohort, not those later combined sources.

Both private workspaces are removed: 35,194 recorded files / 1,631,359,895 logical
bytes (1,730,371,584 allocated), with no binary/capture retained or installed.
The four-view fixture renders 461 frames and retains 256 exact receipts across
two native cuts without resource growth. Its whole-process peak RSS of
7,766,220 KiB includes native worlds/renderer and is not a history-memory budget.
No rendered migration is activated. M02-M07 remain open and default migration
stays disabled. The [next proof-distribution dependency](../../.local/client-replica-cut-20261019/common-cut-next-step.md)
requires independent validation and exact local association on every surviving
PC, distinct from complete presentation/input/playing-audio readiness. Gameplay
and match/postgame voice stay direct and player-hosted. U01/U02/D01 retain the
MCC-only Vista UI source, fresh onboarding and actual widescreen/clan geometry
requirements; this milestone makes no UI acceptance claim.

## Independent paired-cut fanout and export — M02/M06 prerequisite, 2026-10-19

The live runtime now uses every accepted nonhost physical peer as a paired-cut
recipient, independently of QoS host eligibility. One recorder/encoder sends one
seal; exact per-peer/context/full-seal acknowledgements complete the group only
after every recipient validates. A collective acknowledgement has a distinct
group identity. Singleton two-cut delivery remains supported. Failed successor
streams discard their replay/delta base while retaining historical completed
group/host evidence; explicit cancellation/context replacement retires it.
Receivers and background workers export exact currently validated world/host/
admission/view values without stopping replay. Stale or foreign requests reject.

The coherent native build and seven normal CTests pass, including two independent
receiver fanout/takeover variants, original twelve-tick/direct-voice controls,
72-phase paired replay, legacy singleton/pipeline/retention and decoded-origin
checks. Six current-root GCC checks pass. All twelve selected owners match the
15,948-file frozen cohort; 24 unrelated later changes are preserved, not overlaid.
The native binary is hashed and removed, never installed. All recorded scratch/
processes and finalizer scratch are absent after removing 35,127 recorded files /
1,435,048,705 logical bytes (1,533,915,136 allocated). No large artifact is retained.
Evidence: [verification.json](../../.local/common-cut-fanout-20261019/verification.json).

The initial fixture wrongly reused a deliberately faulted recording boundary;
the correction preserves the native guard and tests explicit cancellation. Both
phases/diagnostics remain recorded. A separate audit correction classifies exactly
two small private fixture logs as outputs, preserving strict code/build hashes.
No sanitizer, GPU/rendered migration, physical-device or WAN acceptance is claimed.
Next is [pinning and agreeing the common cut](../../.local/common-cut-fanout-20261019/next-common-cut.md),
alongside complete world/presentation/input/playing-audio adoption and bounded
catch-up/memory. Historical group validation does not close M06 or enable default
windowed migration. M01 stays checked; M02-M07 stay open and the full goal remains
active. Gameplay and match/postgame voice remain direct and player-hosted. MCC-only
Vista ONLINE/clan assets, fresh onboarding and real widescreen/clan dimensions
remain required and unaccepted by this native-only milestone.
