#!/usr/bin/env python3
"""Focused original melee command/admission/impact body correspondence only."""
from pathlib import Path
import research_mp_inventory_switch_u5 as original
PIN='fe51d55efa84a5e6bc4809bedf424fa9d56aa6320ac2cdeed7ed520e77f1c219'
assert original.sha(Path(original.__file__))==PIN
original.DEPENDENCIES={**original.DEPENDENCIES,
 'tools/research_mp_inventory_switch_u5.py':PIN,
 'tools/research_mp_melee_admission_u5.py':original.sha(__file__)}
original.ENTRIES=(0x150f00,0xe7f30,0xeb530,0xeb080,0xeb000,0xeb700,0xeb830,0xd0830)
original.OUT=original.ROOT/'analysis/multiplayer/integration/inventory-source/melee-admission-u5'
if __name__=='__main__':original.main()
