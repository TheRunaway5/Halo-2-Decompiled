# Original TU1.5 fog draw selection

`tools/oracle_fog_draw_selection_tu15.py` executes the unchanged31-instruction
25D30 body from English OG Xbox TU1.5, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The81 cases supply classification at491F6C and original control bytes492295
and492080. They stop before external GPU submission/state cleanup, or run to
RET for suppression. No instruction, service return or GPU result is replaced.
Original EAX at25D78 identifies the selected25E30 atmospheric or26500 sky owner.
Classification2/3 selects sky unless force-atmospheric is nonzero. Suppression
precedes all selection. 26500 admits only positive491FD4 sky opacity and draws
at maximum W with LEQUAL. The native sky program retains that depth contract.

Both retained Waterworks vehicle next-view headers have classification3 and
zero491FC8..491FD4 sky RGBA, so the selected sky pass emits nothing. Native's
previous unconditional atmospheric composite added fog over their surfaces.
The original GPU history, field hashes and source selector evidence are in
`analysis/shader-parity-20261013/fog-draw-matched-frame-evidence-v1.json`.

This proves the selection branch and the recorded vehicle condition; full fog
aggregation, other source controls and final pixel equality remain separate gates.
