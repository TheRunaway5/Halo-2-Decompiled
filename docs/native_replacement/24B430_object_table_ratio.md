# CombatAction object table ratio 24B430

The unchanged TU1.5 routine has 34 sites and no children. It resolves
the object through the datum table, takes its signed row index at `+0x17e`,
and substitutes one for a zero state word at `+0x244`. A row index of -1
returns zero. A set bit 3 selects the live constant; the other path scales
the nonnegative row count by the ratio of the live scale to the signed state
word. The original `1655F0` caller uses this routine only when the live
mode at `+0x180` is 9 and has a real CALL at `165602` and return to `165607`.

- [x] Authenticate complete current/fresh CombatAction in raw and
  prepared forms, including the real 1655F0 caller. Owner removed.
  `native-24b430-object-table-ratio-owner-audit-20261110/report.json`.
- [x] Match all 34 original sites under GCC and Clang sanitizers: 192
  normal returns, 324 stopped prefixes and 8,088 pre-I/O packets per
  compiler, plus four rejected defects each. Owner removed.
  `native-24b430-object-table-ratio-physical-both-20261110/report.json`.
- [x] Trace the unchanged original parent: 96 returns, 72 genuine child
  calls and returns, 1,215 packets and all 42 combined sites. Owner removed.
  `native-1655f0-object-table-ratio-original-20261110/report.json`.
- [x] Close retained source and exact parent/child compositions under both
  sanitizers. The leaf passes 4,608 returns, 11,164 prefixes and 329,344
  packets per compiler. Both parent compositions pass 2,304 returns,
  9,022 prefixes, 257,438 packets and 1,296 actual child calls and returns
  each. Five defects per compiler; owners removed.
  `native-24b430-1655f0-object-table-ratio-source-both-20261110/report.json`.
- [x] Qualify and install both exact CombatAction defaults. Their private
  binding passes four whole contexts and 12 structural refusals each.
  The live inverse preserves all 451 neighboring bodies, including
  165590 and Projectile, and fresh generation produces the exact owners.
  `native-24b430-1655f0-object-table-ratio-binding-private-20261110/report.json`,
  `native-24b430-1655f0-object-table-ratio-insertion-20261110/report.json`.
- [x] Close nine installed and adjacent sanitizer CTests, exact selector,
  four-owner raw/prepared catalog and focused operand checks. Owner removed.
  `native-24b430-1655f0-object-table-ratio-installed-20261110/report.json`.
- [x] Trace both unchanged 1046C0 call chains. Each of the four 165590
  and 1655F0 call sites returns 48 times; the real 24B430 child returns
  96 times. Owner removed.
  `native-1046c0-combat-ratio-original-routes-20261110/report.json`.
- [x] Compare both actual compiled APIs in five modes: 288 returns,
  12 stopped prefixes, 192 authentic Primary required calls and 27,296
  complete events per mode. Each own-reference control is isolated.
  The exact rebuilt CombatAction object uses independently copied R31
  libraries. Owner removed.
  `native-24b430-1655f0-object-table-ratio-linked-r31-overlay-20261110/report.json`.
- [x] Replay the actual World dispatcher with the exact installed World
  and CombatAction objects against independently copied R31 archives.
  Each of five modes matches 96 unchanged original returns. The four
  inherited calls return 48 times each, the real 24B430 child returns
  96 times, and each observed mode matches 1,248 ordered writes and
  all returned ABI and declared memory changes. Own-reference switches
  disable only their matching entry. Owner removed.
  `native-24b430-combat-ratio-live-world-r2-20261111/comparison.json`.
- [ ] Qualify a coherent runtime containing the installed defaults.
