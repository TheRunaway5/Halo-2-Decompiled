#!/usr/bin/env python3
"""Separate source callback-table proof for the missing manual-reload entry.

The earlier reset correspondence report remains untouched. Entries come from
the actual E7E60 owning table, not from an annotation or identical wrapper.
"""
from pathlib import Path
import struct
import research_mp_live_combat_updates as comparison
from oracle_smg_firing_1_1 import ROOT, Xbe
from oracle_mp_combat_postprocess_updates import sha

def main():
    old,new=Xbe(comparison.OLD),Xbe(comparison.NEW_DEFAULT)
    prior=ROOT/'analysis/multiplayer/integration/scope-camera/reset-update-frozen/report.json'
    p,q=0x46ec68,0x473d28
    a,b=struct.unpack('<I',old.read(p,4))[0],struct.unpack('<I',new.read(q,4))[0]
    assert (a,b)==(0x46ea04,0x473ac4)
    left,right=struct.unpack('<III',old.read(a,12)),struct.unpack('<III',new.read(b,12))
    assert left==(0xe88b0,0xe8950,0) and right==(0xe97d0,0xe9870,0)
    evidence=dict(kind='actual_owning_action_callback_table_index0',
        original_table=f'{p:08x}',later_table=f'{q:08x}',
        original_record=f'{a:08x}',later_record=f'{b:08x}',
        original_record_hex=old.read(a,12).hex(),later_record_hex=new.read(b,12).hex(),
        earlier_report=str(prior.relative_to(ROOT)),earlier_sha256=sha(prior))
    comparison.main(groups={
        'prepared_shot_scope_reset':(0x107160,0xc9a40,0x105120),
        'manual_reload_action_lifecycle':(0xe88b0,0xe8740,0xe8950,0xe7e60,0x104a30),
        'normalized_damage_scope_reset':(0xcb410,)},
        seed_pairs={0xe88b0:(0xe97d0,evidence)},source_files=(),
        configuration_tool={'path':str(Path(__file__).relative_to(ROOT)),'sha256':sha(__file__)})

if __name__=='__main__':main()
