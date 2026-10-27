# Original party privacy contract

Authority is the unchanged owned Xbox TU1.5 executable
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
MCC supplies prepared runtime data and does not determine this behavior.

| Behavior | Original source owner | Replacement binding |
|---|---|---|
| Modes0/1/2 are Open / Invite Only / Closed | 19A148/19A161 → TU1.5 194936/19494F; actual screen25 strings04000201/0F000202/06000203 | Strict integer0..2, absent capability remains unknown |
| Fresh hosted party starts Open | 59BD0 →5A880 writes session+498C=0 | New service parties start0 only when the native admission kernel is installed |
| Existing invitation-only service parties do not silently become Open | Previous service had no Open join route | Independent privacy table migrates existing parties to1 |
| Only the established party leader changes privacy | 5BDE0 →5CAA0; 5DAC0 →5E7B0 validates the actual authenticated remote member equals leader | Current accepted membership and leader authorization; opaque membership context, roster version and independent privacy revision |
| A remote leader need not be the network host | 5BDE0 sends parameter request to the actual host;5DAC0 authenticates the leader sender | Account service is the parameter publisher; it never equates network host with party leader |
| Changing privacy leaves countdown and roster consent intact | Privacy branch changes value498C/update counter; no stop-countdown, mode or membership call | No `changed()` call; ticket, roster version, readiness and published countdown remain intact |
| Open allows a normal join with sufficient public slots | 64D70 →65DF0 advertises mode0/public slots;6F2B0 →72EE0 selects them | Authenticated `join_friend` uses an accepted friend public identity and real seat capacity |
| Invite Only requires an actual invitation and sufficient private slots | 6F200 →72E30 records actual invite flag;6F4B0 →730E0 carries it through the actual QoS description to6F2B0, then clears it | Existing separately targeted, unexpired invitation is required; no caller-supplied invited boolean |
| Closed rejects even an invited join | 6F2B0 rejects modes other than0/invited1; host62FE0 →64010 rejects privacy>1 before existing-member/capacity checks | Both direct Open joins and explicit native/Discord invitation acceptance recheck current mode |
| Real source joining state can override privacy/capacity errors | 6F2B0: description states6/7/8→12,5→13 | Actual service match/transport admission remains a separate guard; no fabricated source state is supplied |
| Full/private failures use original join dialog routes | 199CFC →1944FB maps errors7/8 to UI7/8;24F927 →255B67 selects dialogs0x41/0x42 | UI may bind actual service denial; no made-up successful admission |
| Privacy X handler has no countdown gate | 2BBED0 →2C5A6B: type5,param2 and non-local session;2BB9CE →2C553B loads25 through239026 | Supported Online Setup opens exact25; async input waits for the actual matching command acknowledgement |

The original selected-friend action2B55AE requires an online same-title friend,
the actual joinable flag or current session membership, and source join gates.
The service publishes its own observed `party_joinable` only for an accepted,
unblocked, online native friend and actual Open capacity. It does not claim to
be an Xbox Live presence flag. Private Discord/Steam account subjects, display
names, party database IDs and caller assertions do not grant joining authority.
Existing bilateral friend blocks, explicit party invitation blocks, SDK target
invitations, account expiry and actual match admission remain enforced.
The new Open join also refuses an actual local or target queue/proposal:
19A250 suppresses source local states5..8 and72EE0 rejects target6/7/8.
Refusal preserves the real ticket and party consent; entering a social party
does not silently cancel another party's in-progress search.
The selected friend's observed accepted-membership context also travels in
the new join command. This replaces the original selected XNKID/6F200 target
ownership without revealing a private party database ID. A friend who moves
to another party invalidates the old selection. The native worker verifies
the actual accepted roster contains that selected context before acknowledging
success; the existing membership observer then enters the lobby.

Supported snapshots are opt-in on the existing party read/command routes:
`privacy={mode,revision,context}`. Context is the authenticated member's existing
opaque accepted-membership handle, and the parser checks it against the actual
self roster row. `privacy` commands carry that context plus the observed roster
and privacy revisions. A delayed request cannot mutate a different newly
accepted party with coincident numeric revisions. Repeated selection advances
the independent privacy revision; retry of the same request ID is idempotent.
Local typed command sequence/completion fields distinguish actual acknowledgement
from a later unrelated snapshot; no successful UI state is predicted.

Static complete normalized body/internal-branch correspondence is retained in
`build/test-results/og-matchmaking-complete-20261025/privacy-footer-tu15-correspondence.json`
and `privacy-join-tu15-correspondence.json`. The original validator executes in
120 unchanged-XBE controls in `privacy-join-original-controls.json`, without
instruction patches or callee stubs, including the real cached completed-logon
lookup. The24 privacy/capacity controls are checked through the Rust ABI in
`tests/fixtures/tu15_party_privacy_original.json`. These controls do not execute
Xbox Live, transport handshake or original hardware rendering.
