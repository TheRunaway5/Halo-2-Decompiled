# Same-phase source attachment retirement

Authority is the pinned English Xbox TU1.5 executable and the original
00a_introduction map. x01_01b_setup attaches throne_regret (name59) to Regret
(name56); x01_10_cleanup destroys the prophets before destroying names containing
throne. Original BA490 retires the attached child during parent deletion.

The native scene previously cleared only the requested parent's cached identity.
The next containing-name command therefore tried to delete an already retired
throne. The observed source header was absent while the scene slot/name remained.

campaign_scene_retirement_tu15_tests uses those two unchanged authored placements,
real source births, the actual attachment owner and actual source deletion. It
checks that both names disappear before another source tick or pose publication,
then repeats with fresh generations. It rejects another source deletion of the
retired child. The scene observes the actual source pool; it does not decide which
source children die or supply a successful deletion result.

The original failure and passing focused check are retained in
analysis/opening-cinematic-20261015/scene-retirement-integration-v1 and v2.
The passing v2 guard receipt is
aggregate-checks-v1/c28b61c3-c44f-4a11-809c-4314ef0ab235/memory-budget.json
(1.858s; unchanged60MiB hard/64MiB measured/zero swap).
The subsequent graphics run passes all three throne names and reaches
c01_intro_scene_01, then exits on panel material admission. This establishes the
cleanup continuation, not complete opening playback or rendered-frame parity.
