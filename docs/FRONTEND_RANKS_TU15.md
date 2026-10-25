# Frontend rank presentation

TU1.5's `14C9EA` maps a signed session rank byte through `wigl+158` in
first-match order. The disc table has fifty entries: `0..48` map individually,
and `49..127` map to the final page. Negative values have no matching range.
`2B9EB2 -> 2B9EC5` sets or clears item bit `40`; `2B9D59` hides the rank
child for NONE and otherwise calls the actual bitmap-page setter `2BA733`.
`tools/oracle_frontend_rank_tu15.py` executes these original instructions with
the literal disc table for all 256 possible signed bytes and all fifty pages.

The native service's public levels are one-based (`1..50`), so presentation
subtracts one before selecting these pages. Private raw skill, XP, identity,
party size, voice state and player names never select a rank icon.

`GET /v1/matchmaking/ticket?progress=1&ranks=1` opts into nullable `level`
fields for the admitted gathering or proposed match roster. The coordinator's
actual player identities bind each level, even when chosen names are identical.
Ranked clan matches use the admitted clan level. Casual and guest rows have no
display level. The earlier response schema remains available without the opt-in.

The matchmaking UI uses the rank bitmap inside its actual authored list grid,
including its original placement and imported pixels. Before a gathering exists,
the sole local searching row can use its authenticated selected-playlist level.
Missing levels remain hidden. Social voice parties have no selected-playlist
rank owner and continue to hide ranks. Idle, failed and cancelled searches do
not retain rank artwork.

The ordinary lobby and matchmaking rank draws share the group-bitmap path.
That path retains the selected bitmap frame, including nonzero stripe pages;
the former explicit page-zero override discarded the roster's choice.

Evidence belongs in `analysis/frontend/ranks-tu15-20261023/` and
`build/test-results/ranks-tu15-*`. Native checks compare the submitted texture
against the admitted source page, exercise rank 37 and casual hiding through
authenticated HTTP, and compare rank-on/rank-off GPU pixels when DISPLAY is
available. Original-code execution and native GPU proof do not establish a
physical Xbox framebuffer match. Server rollout and launcher/runtime delivery
are separate from these local source changes.
