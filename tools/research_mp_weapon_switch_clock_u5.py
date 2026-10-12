#!/usr/bin/env python3
"""New focused full-body mapping for switch clear and actual channel clock."""
import research_mp_inventory_switch_u5 as original
from pathlib import Path
PIN='fe51d55efa84a5e6bc4809bedf424fa9d56aa6320ac2cdeed7ed520e77f1c219'
assert original.sha(Path(original.__file__))==PIN
original.DEPENDENCIES={**original.DEPENDENCIES,
 'tools/research_mp_inventory_switch_u5.py':PIN,
 'tools/research_mp_weapon_switch_clock_u5.py':original.sha(__file__)}
original.ENTRIES=(0xe9a90,0x1ca2b0,0x1ca090)
original.OUT=original.ROOT/'analysis/multiplayer/integration/inventory-source/weapon-switch-clock-u5'
if __name__=='__main__':original.main()
