#!/usr/bin/env python3
"""Original joined-player construction, ownership and deletion investigation.

Retained probes and their evidence are read-only. Original join, construction,
assignment and deletion execute; inherited supplied gameplay/resource services
are guarded before any teardown hook can return a result.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from unicorn.x86_const import UC_X86_REG_EAX

from oracle_constructed_object_deletion import DeletionTrace, sha
from oracle_mp_spawn_player_visibility import (
    PlayerTrace, join, FROZEN as PLAYER_PINS, CREATION,
)
from oracle_mp_item_current_producers_1_1 import CurrentTrace
from oracle_mp_spawn_live_body_world import ROOT, CONTEXT_OUT, PIN, CACHE_PIN
from oracle_round_lifecycle import compact_raw_records
from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import Xbe,_Code

OUT=ROOT/'analysis/multiplayer/game-variants/owned-object-deletion'


class OwnedTrace(DeletionTrace,PlayerTrace):
    def __init__(self,data):
        super().__init__(data)
        # Observers only: several genuine callback entries are absent from
        # the old function inventory. No guest instruction/state changes.
        self.starts.update((0x154440,0x14e240,0x153a90,0x189490))


def prepare(data):
    t=OwnedTrace(data)
    t.heap_prefix(None)
    t.call('original_CRT_lock_initializer',0x32cffc)
    t.call('original_CRT_onexit_initializer',0x3274e4)
    t.register()
    t.call('original_scratch_CRT_constructor',0x37d8c0)
    t.call('original_engine_table_CRT_callback',0x37eb40)
    t.fixup();t.arena_root();t.actual_heap=True
    for a in (0x18fce0,0x18fd70,0x246a50,
              0x111490,0x111500,0x1dc8f0,0x1dc940,0xfb560,0xefee0,0xc9a40):
        t.services.pop(a,None)
    initializers=(
        ('player',0x14cc00),('view_control',0x1874d0),('engine',0x158ba0),
        ('physics_constants',0x1eef40),('object',0xb76b0),
        ('unit_queue',0x10d490),('trigger_volume',0x1efe10),
        ('contrail',0x17d220),('effect',0x1775d0),
        ('full_AI',0x1cb180),('physics',0x1c5f00))
    for name,entry in initializers:t.call('original_'+name+'_storage_initializer',entry)
    t.call('original_empty_player_map_reset',0x14ccd0)
    t.call('original_view_control_map_reset',0x187520)
    source=ROOT/'analysis/multiplayer/integration/spawn-source/smg-producers-report.json'
    t.default_variant=json.loads(source.read_text())['original_default_slayer_multiplier_cases'][0]['default_variant_hex']
    t.g.mem_write(t.u(0x4edf18)+0x13c,bytes.fromhex(t.default_variant))
    assert t.stage('original_engine_map_reset',0x158c10)
    resets=(('physics_constants',0x1eefe0),('object',0xb78c0),
        ('unit_queue',0x10d4f0),('trigger_volume',0x1efe70),
        ('contrail',0x17d2e0),('effect',0x177690),('multiplayer_AI',0x1cb3d0),
        ('BSP_datum_table',0x1c5fd0))
    for name,entry in resets:t.call('original_'+name+'_map_reset',entry)
    t.fragment('original_activation_mask_zero_prefix',0x1394b0,0x1394f0)
    t.call('original_object_map_activation',0xb7aa0)
    t.call('original_full_new_map_order',0x1c5ff0)
    t.call('original_current_cluster_masks_with_empty_player_pool',0x13a380)
    t.track_players=True
    CurrentTrace.weapon_resources(t)
    return t


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=OUT/'report.json')
    ap.add_argument('--slots',type=int,nargs='+',default=[0,3,15])
    ap.add_argument('--activation',choices=('inactive','motion'),nargs='+',default=['inactive','motion'])
    ap.add_argument('--cycles',type=int,default=2)
    args=ap.parse_args()
    for path,pin in PLAYER_PINS.items():assert sha(ROOT/path)==pin,path
    args.output.parent.mkdir(parents=True,exist_ok=True)
    proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
        env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix')))
    assert args.cycles>0 and all(0<=s<16 for s in args.slots)
    cases=[];executed=set()
    try:
        data=json.loads(proc.stdout.readline())
        for activation,slot in ((a,s)for a in args.activation for s in args.slots):
            t=prepare(data);player=join(t,slot)
            before=t.player_state();stages=[];creations=[]
            print(json.dumps(dict(activation=activation,slot=slot,stage='original_join_returned')),flush=True)
            for cycle in range(args.cycles):
                assert t.stage('original_normal_player_spawn',0x1509c0,(player,))
                assert t.g.reg_read(UC_X86_REG_EAX)&255==1
                p=t.u(t.u(0x4f01f4)+0x44)+(player&65535)*0x21c
                unit=t.u(p+0x2c)
                creations.append(dict(cycle=cycle,unit=f'{unit:08x}',player=t.player_state(),world=t.state()))
                assert t.u(p+0x218,2)==cycle+1
                if activation=='motion':
                    assert t.prefix_stage('original_player_update_through_cluster_writer',0x14eae0,0x14ed0c,(0,))
                    assert t.stage('original_current_cluster_masks_and_callbacks',0x13a380)
                    for name,entry in (('object_prephysics',0xb7e00),('world_physics',0x1c7660),
                            ('object_postphysics',0xb7f50),('object_newflags',0xb8040)):
                        assert t.stage('original_'+name,entry)
                assert t.u(p+0x2c)==unit
                for name,entry in (('owned_unit_deactivate',0xb92a0),('owned_unit_delete',0xb9350)):
                    result=t.probe(name,entry,(unit,1));result['cycle']=cycle
                    result['player_after']=t.player_state();stages.append(result)
                    assert result['completion']=='complete_original_return',result['stop']
                    assert t.u(p+0x2c)==t.u(p+0x30)==t.u(p+0x174)==0xffffffff
                after=t.raw_state()
                assert after['object_count']==after['object_high_water']==after['component_count']==after['physics_counter']==0
                assert t.u(t.u(0x4f01f4)+0x3c)==1
                assert not t.pending_free
            section=t.image.section('.text');raw=t.image.section_bytes(section)
            assert bytes(t.g.mem_read(section.va,len(raw)))==raw
            executed.update(t.executed)
            cases.append(dict(slot=slot,activation=activation,player=f'{player:08x}',
                before=before,creations=creations,stages=stages,creation_input_hex=t.creation_input_hex,
                default_variant_hex=t.default_variant,preparation=t.producers,
                player_stages=t.player_stages,player_calls=t.player_calls,
                player_and_object_stores=t.player_stores,services=t.events,
                storage_release_records=t.free_records,
                whole_guest_text_unchanged=True))
            print(json.dumps(dict(slot=slot,activation=activation,player=f'{player:08x}',
                completions=[s['completion']for s in stages],
                stops=[s['stop']for s in stages if s['stop']])),flush=True)
    finally:
        _,error=proc.communicate('x',timeout=45)
    assert proc.returncode==0,error
    for path,pin in PLAYER_PINS.items():assert sha(ROOT/path)==pin,path
    directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
    image=Xbe(SPECS['tu11']['path']);code=_Code(image,'.text');section=image.section('.text')
    rows=[code.at(a)for a in sorted(executed)if section.va<=a<section.va+len(image.section_bytes(section))]
    for i in rows:assert image.read(i.address,len(i.bytes))==i.bytes
    listing=directory/'tu11-executed-original.asm'
    listing.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rows))
    dependencies={str(Path(m.__file__).resolve().relative_to(ROOT)):sha(m.__file__)
        for m in list(sys.modules.values())if getattr(m,'__file__',None)
        and Path(m.__file__).resolve().is_relative_to(ROOT/'tools')}
    report=compact_raw_records(dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,
        oracle_sha256=sha(__file__),source_dependencies=dependencies,prior_pins=PLAYER_PINS,
        generator_sha256=sha(CONTEXT_OUT/'generator.exe'),cases=cases,
        instruction_patches=[],supplied_teardown_services=[],
        executed_instruction_listing=dict(file=listing.name,sha256=sha(listing),rows=len(rows)),
        summary=dict(joined_players=len(cases),constructed_owned_bipeds=sum(len(c['creations'])for c in cases),
            completed_object_deletions=sum(len(c['storage_release_records'])for c in cases),
            reconstructed_flows=sum(len(c['creations'])>1 for c in cases)),
        boundaries=[
            'Retained selected original storage/CRT/world preparation and live native compatibility producer; whole title/session startup, Xbox kernel/hardware parity and original input/action tail are excluded.',
            'Join consumes the retained original-produced default Slayer variant and host-effective property packet. Zero-key nonlocal identity/sign-in/transport delivery is an explicit input boundary; no player/current-unit/PVS field is supplied.',
            'Original normal1509C0 chooses from unchanged authored Lockout starts, constructs the actual biped and SMG, binds ownership and executes source loadout/attachment. Retained optional effect, looping sound and equip audio creation boundaries remain recorded.',
            'Motion cases execute the retained current-cluster prefix and complete original PVS/prephysics/world/postphysics/newflags passes. These are source API compositions, not normal player command input, a whole match tick, or gameplay/contact parity.',
            'Deactivation and deletion guard all inherited provided gameplay/resource leaves before any hook can return. Actual assignment/status/view callbacks, recursive attached-weapon deletion, physics/component/body-heap retirement run to original returns.',
            'The surviving actual player record has current/previous/watched NONE after every teardown; the same original player then spawns again with source spawn-count and fresh salted object handles. Whole round-phase/reset/scenario/placement policy, native binding and complete normal matches remain unproved.']))
    args.output.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
    print(json.dumps(dict(report=str(args.output),sha256=sha(args.output))))


if __name__=='__main__':main()
