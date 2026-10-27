# Original online postgame family adapter

Behavior authority is Xbox Halo 2 TU1.5, XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Whole producer `232D77 -> TU1.5 23975C` has matching normalized instructions
and internal branches (343 instructions). The corresponding team, player,
numeric, opponents, medals and hits row binders are recorded in
`build/test-results/multiteam-tu15/pcr-row-mapping.json`. This static whole-body
correspondence does not replace platform/widget calls or establish framebuffer
equivalence.

The original producer's family-specific numeric columns are:

| Source type | Family | First summary field | Second summary field | Score format |
| --- | --- | --- | --- | --- |
| 1 | CTF | stat16 | stat15 | integer |
| 2 | Slayer | stat13 / max(stat9 + 1, 1) | stat12 | integer |
| 3 | Oddball | stat26 | stat25 | original time |
| 4 | King | stat28 | stat27 | original time |
| 7 | Juggernaut | stat38 | stat39 | integer |
| 8 | Territories | stat43 | stat44 | original time |
| 9 | Assault | stat21 | stat20 | integer |

The native numeric publication copies the actual frozen `SourceResultStatistics`
bank and preserves the validated selected 304-byte source option buffer. It
admits these seven families; unused source type5/6 does not gain an adapter.
The backend checks exact original preset names against the 39 independently
executed option buffers, including the source engine and non-Slayer preset team
flag. Unknown custom names retain the prior Slayer-only result policy. This
does not change the original admitted identity, unanimous confirmation, guest,
competitive-history or immutability checks.
The Xbox display carrier uses 12-byte identities and tail fields10C/110; the
existing PC asset carrier uses the authenticated native 8-byte room identity
and fields108/10C. No original XUID or Live record is fabricated. The shared
original score formatter supplies the time formatting selected by159130 for
types3/4/8.

Appearance presence is separate from the retained result roster. An actual
authenticated postgame membership mask supplies live ONLINE presence. A weak
callback cannot retain the transport after Done; unknown membership displays
the original query-failure gray0.3. System Link reads its actual retained
`active_players`. A disconnected terminal report derives presence from the
completed native source's retained/non-retired player flags. None of these
paths infer presence from mute or headset settings.

The public hopper level is copied only from a known ranked admission and bound
by its actual random room identity. Existing source level1..50 to rank0..49
conversion fills display field78. Unranked, guests, System Link and absent
metadata retain NONE. This is the recorded admission level, not an invented
post-match rank update or a history write.

`online_pcr_family_carriers` covers all seven family carriers with declared
numeric inputs, sparse slots3/7, time formatting, recorded rank metadata and
current/departed/unknown appearance membership. Original whole-function numeric
controls and actual natural source-world/GPU endings are separate checks. The
real online matchmaking flow also checks that its authenticated rank metadata
reaches the report. Component inputs do not establish seven-family online
playthrough or physical Xbox framebuffer equivalence.
