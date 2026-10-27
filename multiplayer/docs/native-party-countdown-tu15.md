# Native party-A countdown

Behavior authority is the original Xbox Halo 2 TU1.5 XBE, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The native authenticated party service publishes the recovered session parameter;
each frontend observes that publication rather than starting a private UI timer.

| Original owner | Recovered behavior |
| --- | --- |
| retail `25106C -> 19A78E`, TU1.5 decision block `194FBD` | Leader Start begins at 10; another A/Start subtracts one only above 3. Active A/Start precedes focused-button dispatch (`250B1E`). |
| retail `6E360`, TU1.5 `71D70` | After signed 32-bit elapsed milliseconds reaches 1000, subtract once and store the current clock. A delayed pump does not catch up several ticks. |
| retail `5DF30`, TU1.5 `5EC20` | Parameter fields are active flag `49A8`, seconds `49AC`, mode `49B0`, watermark `49B4`, requester identity `49B8`. A lower ordinary publication clears mode 1 and its requester. |
| retail `19A7E9` | Leader X stops the countdown. Member X publishes the actual tuple `(start=true, mode=1, value=10, requester)`; it does not stop the leader's countdown. |
| retail `64B00`, `22D80B`, `24FFC9`, `19ABE4` | Mode-1 requester resolves to a real roster name for token E414 and source delayed-countdown status. The public party handle adapts the original XUID lookup. |
| retail `6E410`, `19B1C9` | Zero stops the active parameter and enters matchmaking mode 6, presented as screen 210. |
| retail `250B1E`, `2511B6`, `25132E`, `251345`, `199E3C` | Active leader B cancels and stays. Other B routes ask with 8B. Confirm leaves both sessions; the two-member host variant kicks its sole remote before leaving. |

`tools/oracle_tu15_party_countdown.py` executes unchanged TU1.5 timer and press
decision blocks and the complete parameter writer. It locates unique masked
retail counterparts and records instruction visits. Timer execution stops before
the network publication call, and the press block accepts previously evaluated
status/host/getter conditions. The fixture has 40 controls and 1,083 instruction
visits. It does not claim original Live transport, an original Xbox framebuffer,
or physical-controller coverage. The independent C++ and Python tests consume
that fixture.
The live service calls the Rust native press/tick exports through the trusted
scalar C ABI. Python owns authentication, the shared clock and publication, with
no Python decision fallback. The Python fixture test therefore checks the Rust
implementation through the same ABI used by real party requests. Missing native
kernels produce an explicit 503 response.

The replacement transport uses the existing authenticated accepted party and
real selected hopper revision. Initial Start validates actual compatible queue
sessions and member consent. Roster or consent-version changes discard the
countdown. Start, shorten, cancel, and member-request commands leave the accepted
party version and consent unchanged. One service-owned clock publishes the
countdown to every member, including a late observer. Completed serials remain
observable after the leader consumes zero, and each native menu consumes a
serial once. Existing match readiness and content gates still decide admission.
The separate `leave_lobby` command implements confirmed 8B departure, including
the two-member leader variant. The frontend waits for authoritative departure
before returning to the main menu. A real member departure cancels the remaining
party's countdown, clears consent, and presents the source AE notice; a canceled
confirmation leaves membership intact. Rejoining requires a fresh invitation,
acceptance, and hopper agreement and does not automatically queue.

`GET /v1/party/countdown` is a small authenticated view with an independent
600-read/minute account budget. Native account workers poll the accepted
parameter at 100 ms while active and 500 ms while idle, while keeping the
configured social-list cadence. Membership changes force a full roster refresh.
Neither a failed read nor a paused render loop advances a private timer.

The real-process regression in `tests/online_match_launch_tests.cpp` uses retained
`FrontendSession` services and actual `run_frontend` draws. SDL inputs cover
Start, leader X cancel/restart, delayed member observation, repeated A with Game
Setup focused, member X, shared stages 3/2/1 and zero-to-search, real native match,
all original PCR tabs, Done, prior report review, and the next search. Service,
kernel, and rendered-flow outcomes are separate evidence levels; a passing
component test does not establish the rendered flow.

Raw frames live only in the test's owned scratch. With the explicit absolute
`HALO2_ONLINE_FLOW_EVIDENCE` directory, the HTTP test retains selected actual
frames as lossless PNGs (at most 1 MiB each and 8 MiB total), a contact sheet,
pixel dimensions, hashes, actual menu panes, actual service serial/value, and SDL
input receipts. With that variable unset, no frames are retained. The contact
sheet is labeled resampling of the captured pixels, not a generated UI image.
