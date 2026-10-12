#!/usr/bin/env python3
"""Narrow original prepared-shot/manual-reload scope reset correspondence."""
from pathlib import Path
import research_mp_live_combat_updates as comparison
from oracle_smg_firing_1_1 import ROOT
from oracle_mp_combat_postprocess_updates import sha
if __name__=='__main__':
 comparison.main(groups={'prepared_shot_scope_reset':(0x107160,0xc9a40,0x105120),
  'manual_reload_action_lifecycle':(0xe88b0,0xe8740,0xe8950,0xe7e60,0x104a30)},
  source_files=(),configuration_tool={'path':str(Path(__file__).relative_to(ROOT)),'sha256':sha(__file__)})
