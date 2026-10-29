# Security Cloud remediation, 2026-10-28

The two latest completed Security Cloud runs were both on October 27:

| Repository | Scanned revision | Run |
| --- | --- | --- |
| Halo-2-Decompiled | `ec3fdb77161886514b67bcdfd6bc7ec5d6659c88` | [Game scan](https://chatgpt.com/codex/cloud/security/runs/wfr_7af0e8f58789f993626b01f23735b9bfa31968b8bbd03ce510d035af90ebf946) |
| Halo-2-PC-Website | `7bbf11624daebff410a1541f54313aecf9fe12e3` | [Website scan](https://chatgpt.com/codex/cloud/security/runs/wfr_1f3b002137af9d023ce727a9db62dd00d218021ad82ba8a91e2a093433eb7cf7) |

Both checkouts initially matched their scanned revisions and were clean. The
scan occurrence endpoint returned no findings; the complete saved findings
artifacts contained eight game findings and five website findings. Their small
JSON copies are in each checkout's `build/test-results/security-scan-20261028`.
Changes are local. Cloud status, Git history, production services and installed
launcher runtimes have not been changed.

| Game finding | Local remediation |
| --- | --- |
| `csf_51d00375964b6e54032be3b8` — shared-resource symlink escape | Xbox shared reads and the MCC resource reader use component-by-component no-follow resolution. POSIX opens each component relative to its retained directory descriptor and reads the admitted regular-file descriptor. Windows rejects reparse points and retains parent handles without delete sharing. Parent traversal, final links, intermediate links and nonregular files are rejected. |
| `csf_f1eecf3cf9f77437dec6faaa` — MCC bitmap aliases multiply work | Shared metadata admission charges tags, frames, partition references and retained inventory entries, including aliases, before block lists or retained dictionaries expand. Preflight and materialization share the budget. Unique pixel de-duplication no longer exempts repeated metadata work. |
| `csf_353e78c8b8edea5da5352254` — UI aliases multiply export work | UI bank, frame, sequence and fallback-sprite lists share aggregate admission. Export projects all compatible frame counts and decoded/output bytes before writing DDS files. Each frame has a 128 MiB decoder ceiling; file count, total bytes and free space are checked before output. |
| `csf_e53d47e617d0693aa7064123` — directory enumeration precedes quotas | Streaming `scandir` charges every entry, including empty directories and empty files, before retaining or sorting it. MCC reuses one admitted inventory for copying directories/files and one bounded final inventory for drift checks. Extracted Vista directories enforce the 4,096-member limit while enumerating. |
| `csf_e1517ef2892f7a0c9cf07a95` — tiny arithmetic streams expand before native validation | Untrusted snapshot decoders enforce a 128:1 plus 4 KiB size allowance before body allocation, physical arithmetic lookahead and operation bounds, an 8 MiB metadata ceiling, and an aggregate 8 MiB decoded budget per 33 ms window. Eight consecutive decode/native-schema failures quarantine the receiver until an explicit session reset. Snapshot encoders retain physical terminators and fund legitimate high-compression snapshots with padding; trusted low-level codec callers retain their original compact representation. |
| `csf_d03e1f68a22ce8f3afc1c3d6` — host receives excluded microphone audio | Every sender evaluates the existing mute/block/team/proximity policy against its replicated state and sends only to eligible physical peers over their existing encrypted channels. Remote preferences start disabled and are announced directly. Player hosts receive no excluded frame and never forward another microphone. Each complete frame signature binds room, epoch and recipient, preventing redistribution to a different peer. Gameplay packet role checks remain separate. Postgame follows the same direct recipient policy. |
| `csf_99eec71fac00316ee2eb7173` — transferable Steam login polling captures an account | Steam verification now pauses for explicit browser confirmation naming the verified account. The user enters a 48-bit code generated and displayed by the initiating game; the server stores only its digest, bound to that pending identity and PKCE challenge. Confirmation additionally requires the callback browser's secure, HTTP-only, same-site cookie and same-origin POST. Five incorrect attempts fail the flow. Polling alone never authorizes credentials. |
| `csf_d29ebd1238bbe1b19c9c4f81` — anonymous starts exhaust the login bank | Each source may retain four pending flows across rate-limit refills. The last 128 global slots are reserved for sources without an existing flow. Expiry reclaims occupancy; one source never evicts another's verified flow. |

The aggregate asset metadata policy admits two million references and 512 MiB
of conservatively charged retained metadata per budget. Strings are scanned and
retained up to 4,096 bytes. MCC directory admission permits 100,000 entries,
32 levels and 256 GiB of source file bytes; UI export permits 65,536 files and
16 GiB of output. These are explicit import safety limits, not recovered retail
constants or whole-process RSS guarantees. Normal original assets are preserved.

The native protocol is now **118**. The voice proof domain is `H2VOICE2`.
Deploy server and native-client changes as a qualified release: older clients do
not implement browser confirmation or the new voice proof. This work does not
activate that release. Existing DTLS/EOS peer encryption supplies confidentiality;
no new home-grown cipher or host-held group key is introduced. Host-authored
world state remains the input to team/proximity policy, under the existing
player-host simulation trust assumption.

The website changes and the additional Steam-presence fix mirrored into the
game backend are recorded in
[the website remediation report](../../../Halo-2-PC-Website/backend/eos-worker/security-scan-remediation-20261028.md).
Public identity discovery remains enabled. Public friend search returns the
same identity projection across bilateral blocks. Per the user's explicit
policy choice, denied social actions may still make a block inferable; block
lists, private presence and communication remain protected.

Validation is recorded in `build/test-results/security-scan-20261028/validation.json`:

* The full backend suite passed 388 tests. After concurrent signaling edits,
  the 29 focused Steam/friends/voice checks passed again against current source.
* Rust passed 155 core tests, two existing tests remained ignored, and strict
  all-target Clippy passed on Rust 1.96.0. Four concurrently added matchmaking
  checks also passed; they are outside this security qualification.
* Nine native checks passed: real UDP snapshot delivery, MCC resource identity,
  confined resource paths, native Steam proof generation, recipient-bound voice
  authentication/roles, voice policy and handoff, aggregate import work, and
  allocation instrumentation. The selected native build uses `-O0 -w -DNDEBUG`
  for functional checks and the normal optimized Rust bridge.
* Asset/tool suites passed 63 checks. Website validation passed 29 TypeScript
  tests, seven adversarial Python checks, type checking and the static build.
* The original snapshot netcode reproduced an actual 8 MiB allocation from a
  tiny packet. The fixed receive path makes no allocation of that size before
  rejecting it. Baseline metadata pathname swapping and full EOS outbox scans
  also have red/green evidence. The old UI bank reader accepted eight aliased
  frame references under a five-reference test budget; the new reader rejects it.

The first native failures exposed the new opener's extra `fstat` interaction
with a syscall hook, and a snapshot codec guard still capped at protocol 117.
The MCC reader now performs regular-file admission in its retained descriptor
identity check, and snapshot encode/decode guards track the selected protocol.
The original UDP fixture passes without timing changes. Temporary diagnostics
were removed.

The source receipt records 24 security files matching the final frozen native
cohort. Concurrent loading-presentation and departed-player signaling edits in
two shared C++ files were preserved; current frozen copies of both translation
units subsequently passed independent syntax compilation and matched the
workspace at completion. This is component/source evidence, without a new
launcher build, live Steam/EOS authorization, physical audio, playable WAN,
production rollout or clean Cloud rescan claim.

All ten recorded owned scratch/build roots were removed and verified. One-off
coordinators were retired. Only bounded logs, commands, source hashes and JSON
reports remain; no large test binaries or captures were retained. Unrelated
active builds, original assets and reference evidence were preserved. Cleanup
is recorded in `build/test-results/security-scan-20261028/cleanup.json`.
