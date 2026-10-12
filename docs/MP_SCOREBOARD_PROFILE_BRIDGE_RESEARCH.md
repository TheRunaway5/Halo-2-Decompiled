# Original default profile to multiplayer scoreboard appearance

The unchanged later-update routines now execute the complete default full-profile constructor, its local-profile storage binding, and its conversion to the 90-byte multiplayer profile. A separate owning case first executes the complete local-profile initializer. Both produce appearance bytes **10,0,0,10** and the same actual Lockout primary RGB words **3E828284,3ED8D8CB,3F0F8F8A**. No active-profile flag, member record, name, player binding, readiness, clock or success return is assigned. The final player-copy entry is attempted and stops at its actual missing player-pool pointer; its complete TU1.1/later correspondence establishes the field mapping without claiming that missing pool was constructed here.
[Oracle](../tools/oracle_mp_scoreboard_profile_bridge_1_5.py),
[complete report](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/report.json),
[independent verification](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/freeze-verification.json).

The authenticated later XBE is external `u5English/default.xbe`, SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. TU1.1 comparison SHA-256 is `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`; disc Lockout is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. All previous listed provider/painter pins are verified before and after both independent runs. The two reports and their 50 instruction files reproduce byte-for-byte, 52 files total. The audit retains 890 executed original instruction addresses and exact original store-site instructions. There are zero instruction patches and zero supplied game, graphics, readiness or clock results.

| Frozen artifact | SHA-256 |
| --- | --- |
| Oracle | `05cfda0f9da6b3662a7eee736b74a45c6eec5c8b0965e2344c9d43a82929d5b2` |
| Report | `ed51be6ed8cdd89bdb4c81902c996c34ce47f3aa32b98232bc8a9e90142660c8` |
| Instruction manifest | `c4ff1f3b69a3900126fe986928859024017589e6605b483cecf102ea2ca2d46b` |

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_mp_scoreboard_profile_bridge_1_5.py \
  --output analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/reproduction/report.json
```

## Executed storage and copy order

The first case begins with literal original image/BSS bytes. The second first calls whole `192380`, which clears the four local-profile records and executes its unchanged subordinate initialization. Both then call whole `1AA360` with EDX pointing to declared raw output storage, whole `192931(0, full_profile, NONE)`, and whole `192BAA(0, wire_profile)` with EAX pointing to a separate 12-byte identity output. Local index0 and profile handle NONE are explicit API inputs; the fixture does not assert a frontend-selected account, signed-in user or online handle.

`1AA360` clears all 1E0 bytes through its actual REP STOSD. It writes its default appearance at +118..11B, ORs profile flags DWORD+4 with1, and writes control preference byte+102=3. Its name at +8 is naturally empty. The immediate10 is original `B1 0A` at `1AA36C` (TU1.1 `1A40BC`); the actual stores at `1AA36E/1AA374/1AA37A/1AA380` use CL/AL from that literal and the preceding XOR EAX,EAX. These exact instructions establish the small default appearance import seam. Other profile fields must come from their owning original constructor or explicit profile input, rather than treating a four-byte default as a complete profile.
[Complete default constructor](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-001aa360.asm),
[complete TU1.1 constructor](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/tu11-001a40b0.asm).

`192931` uses local record base `55D770 + user*C74`. Its actual first-activation branch calls `14DF40` to produce the 12-byte identity from the retained platform MAC fields and user index, then calls `65690` to allocate/clear an input-member record in the original four-record table at `4DA128`, stride D0. For this cold context the source assigns member index0. The retained platform MAC bytes are literal cold-image zeros; they are not claimed as a real signed-in machine identity. Source `1929A4` copies 78 DWORDs to local full-profile base `55D788`, `1929A6` sets active flag10, and `1929B2` stores the declared profile handle NONE. Whole `193A4C`, `193CD1`, `192CB7`, `53840` and `54FB0` then execute their actual branches and return. Their session/online gates retain the cold owning context; none is supplied a successful online/session result.
[Local storage owner](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-00192931.asm),
[identity producer](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-0014df40.asm),
[input-member allocation](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-00065690.asm).

`193CD1`'s actual active, offline branch copies full-profile name+8 to local display-name `55E38C + user*C74`, with capacity32. `192BAA` then copies this name to wire+0 with capacity32 and copies four DWORDs from local `55D8A0/+4/+8/+C` to wire+40/+44/+48/+4C. Those local source fields are full-profile+118/+11C/+120/+124. The report retains complete literal before/after 1E0,90,C74 and D0 records, actual ABI, register values, store order and calls. Both completed cases retain an empty source name; they do not invent `Player` for the live roster.
[Display-name owner](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-00193cd1.asm),
[complete converter](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-00192baa.asm),
[bounded UTF16 copy](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-001425a0.asm).

The source-created wire appearance is passed directly by pointer to `830C0`, with explicit ordinary FFA arguments `(team=NONE, override=false)`. It is not re-created by fixture writes. The preceding original cache-loader tail `125434..125482` resolves actual scenario/matg header datums using the previously declared imported tag table and saved source frame; full disk-loading ownership remains separate. The four real palette lookups return rows10,0,0,10. The first actual RGB result is used by the full-scoreboard row builder. This permits a native default-profile adapter to use the imported source default appearance plus the actual cache palette, while a chosen frontend profile must provide its actual stored appearance.
[Profile/palette provider contract](MP_SCOREBOARD_PROVIDERS_RESEARCH.md),
[source-produced palette calls and bytes](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/report.json).

## Final player fields and remaining owners

Complete original `14D600` and later `14E9E0` have equal full instruction/branch shape, with all relocated data/call operands retained. They first preserve old player+44, copy the supplied90-byte profile to player+D4, copy it to scratch, sanitize scratch through `14D3D0`/later `14E7B0`, then copy that scratch90-byte record to player+44. Therefore wire appearance+40 lands at player+84; wire representation+44 lands at player+88. In ordinary options state2 the source sanitizer maps representation1/3 to3 and other representations to2. It does not change primary palette byte+40. This fixture's whole later player-copy attempt stops at `14EA04`, reading NULL+44 from absent `[4F54F4]`; no pool/header/record or binding is supplied to bypass it. The preceding genuine TU1.1 source-player allocation and complete copy are independently frozen with their earlier declared creation profile input.
[Complete later player copy](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-0014e9e0.asm),
[complete sanitizer](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-0014e7b0.asm),
[previous actual player allocation/copy](MP_PLAYER_VISIBILITY_RESEARCH.md).

Roster membership is signed byte player+C0. Local view/user is signed16 player+28; input/controller user is DWORD player+24. Exact complete source mappings `14DFF0→14F3D0`, `1508B0→151C90`, and `150950→151D30` establish their ownership. The local-key owner matches player+1A against player-global+98, calls the view binder with player+1C, and calls the controller binder with player+20. The view binder writes player+28, player-global+0C+4*view and its signed16 count+8; the controller binder writes player+24, player-global+1C+4*controller and its signed16 count+A. These are static owning contracts, not successful local bindings executed by this suite. Source profile member index0 from `65690` is not by itself a player local-view assignment.
[Complete local-key owner](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-0014f3d0.asm),
[view binder](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-00151c90.asm),
[controller binder](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-00151d30.asm).

Actual frontend default-name owner `21D170` reads language selector `[56B120]`, calls `21E430`, invokes `1AA360`, copies at most31 UTF16 units to full-profile+8 and appends a1E4-byte wrapper to the profile pool. It uses an original localized literal, rather than a guessed label. The owning pool constructor `21C290` requests **6BF00 bytes** from its incoming allocator and initializes that record. The independent whole constructor attempt stops at its first `MOV EAX,[ECX]` with ECX zero because no owning allocator is available in this narrow cache/profile context. Thus profile-pool allocation, language-selector startup, actual selected frontend name/profile handle and selected-profile-to-local-user flow remain unexecuted. The executed default local-storage bridge closes default appearance ownership; it does not claim those frontend or match/session producers.
[Frontend default-name owner](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-0021d170.asm),
[literal language branches](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-0021e430.asm),
[actual allocator stop and retained records](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/report.json).
