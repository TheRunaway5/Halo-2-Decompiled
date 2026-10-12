# Original multiplayer display clock and text

The native match stores the source display clock before its world update and
replicates that signed-short value over native LAN protocol 6. HUD time text
uses this stored value. It no longer recomputes seconds from the later frame
tick; unlimited matches display elapsed seconds.

The arithmetic authority is the frozen [Slayer accounting suite](MP_SLAYER_ACCOUNTING_RESEARCH.md#score-limit-and-time-limit-outcomes):
17 original time samples include timed expiry, negative elapsed and the
unlimited 32767-second cap. `15D490` converts ticks with separate single-precision
integer conversion/multiplication and truncating integer conversion. Remaining
ticks, score ending and world-phase order retain the separate documented limits.

The [clock-text oracle](../tools/oracle_mp_clock_text_1_1.py) executes untouched
TU `1600E0` and `163280`, including their Unicode library routines, with no
replacement formatter. Its 16 explicit signed-short inputs exercise negative
values, zero, seconds, minute boundaries and 32767. The original caller clamps
nonpositive values to zero. The formatter writes a blank for zero minutes:
59 seconds becomes ` :59`, 60 becomes `1:00`, and 32767 becomes `546:07`.

Reproduce with `.tools/python/bin/python tools/oracle_mp_clock_text_1_1.py`.
Frozen hashes:

| Artifact | SHA-256 |
| --- | --- |
| oracle | `be0062d7c85f71888fd0796368070cadf40513d2e314e61235404cc4f5cd91bc` |
| ignored report | `cb557916448e79a079b5bc80813f53238c98c171aee4ff59e433f4c5a7d82e55` |

[Original report](../analysis/multiplayer/integration/spawn-source/clock-text-report.json)
and [instruction listing](../analysis/multiplayer/integration/spawn-source/clock-text-instructions.txt)
retain input hashes and raw evidence locally. TU `1600E0` takes code in EAX,
buffer capacity in ECX and destination pointer in EDX. This operand order is
verified at `1600ED..1600EF` and the `1412E0` formatting helper; inferred source
parameter order does not override those instructions.

[Native differential](../tools/differential_slayer_accounting.py) compares the
frozen clock samples and all 16 strings, plus timed and unlimited match/actual
UDP timelines. Text-cache production, dirty-engine replication, announcements,
round transitions and final Xbox framebuffer parity remain separate work.
