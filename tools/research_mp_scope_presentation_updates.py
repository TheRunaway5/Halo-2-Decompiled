#!/usr/bin/env python3
"""Narrow original HUD status/mask and first-person request gate correspondence."""
from pathlib import Path
import research_mp_live_combat_updates as comparison
from oracle_smg_firing_1_1 import ROOT
from oracle_mp_combat_postprocess_updates import sha
if __name__=='__main__':
 comparison.main(groups={'scope_hud_status_and_mask':(0x13b09a,0x13b7c0,0xc9bc0,0xcd1a0,0x105120),
  'scope_first_person_model_requests':(0x167a0d,0x167996,0x3d2b0,0x169ddc,0x156eb0,0x11cf00)},
  source_files=(),configuration_tool={'path':str(Path(__file__).relative_to(ROOT)),'sha256':sha(__file__)})
