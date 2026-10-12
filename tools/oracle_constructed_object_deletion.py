#!/usr/bin/env python3
"""Probe original TU1.1 teardown of an actually constructed Lockout biped.

The retained live-body preparation has explicit platform/resource/optional
creation boundaries. During teardown every inherited supplied game service is
guarded before its hook can run. Missing owners stay stopped source calls.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import csv
import struct
import sys

import unicorn
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EDX

from oracle_mp_spawn_live_body_world import (
    LiveTrace, ROOT, FROZEN as PRIOR, CONTEXT_OUT, PLACEMENT, PIN, CACHE_PIN,
)
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_firing_1_1 import STACK, STOP
from oracle_game_over import SPECS
from oracle_game_variants import Source
from oracle_mp_full_scoreboard_1_5 import Xbe, _Code, literal, complete_tokens, references
from oracle_round_lifecycle import compact_raw_records

OUT = ROOT / 'analysis/multiplayer/game-variants/constructed-object-deletion'
FIXTURE = ROOT / 'tests/fixtures/object_storage_release_original.json'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class DeletionTrace(LiveTrace):
    def __init__(self, data):
        self.teardown = False
        self.teardown_calls = []
        self.free_records = []
        self.pending_free = None
        super().__init__(data)

    def hook(self, g, address, size, user):
        if self.teardown:
            if self.pending_free and address==self.pending_free['return_address']:
                record=self.pending_free
                record['after']=self.storage_snapshot()
                del record['return_address']
                self.pending_free=None
            if address==0xbd1f0:
                assert self.pending_free is None
                sp=g.reg_read(UC_X86_REG_ESP)
                record=dict(datum=self.u(sp+4),return_address=self.u(sp),
                    before=self.storage_snapshot(),writes=[])
                self.free_records.append(record)
                self.pending_free=record
            if address in self.starts:
                sp = g.reg_read(UC_X86_REG_ESP)
                self.teardown_calls.append(dict(
                    entry=f'{address:08x}', registers=self.registers(),
                    return_address=f'{self.u(sp):08x}',
                    stack=[f'{self.u(sp+4+i*4):08x}' for i in range(6)]))
            # Check before delegating to any retained supplied-return hook.
            if address in self.services or address == 0x124f30:
                sp = g.reg_read(UC_X86_REG_ESP)
                self.new_invalid = dict(
                    boundary='unprovided teardown service; stopped before inherited hook',
                    instruction=f'{address:08x}',
                    original_return_site=f'{self.u(sp):08x}',
                    inherited_service=self.services.get(address),
                    registers=self.registers(),
                    stack=[f'{self.u(sp+4+i*4):08x}' for i in range(6)])
                raise KernelBoundary()
        super().hook(g, address, size, user)

    def storage_snapshot(self):
        pool=self.u(0x4e78d0)
        heap=self.u(0x4e58bc)
        regions=[(0x4e78d0,4),(0x4e58bc,4),(pool,0x4c),
            (self.u(pool+0x44),self.u(pool+0x20)*self.u(pool+0x24)),
            (self.u(pool+0x48),(self.u(pool+0x20)+31)//32*4),
            (heap,0x40),(self.u(heap+0x28),self.u(heap+0x2c))]
        return [dict(address=f'{address+offset:08x}',
            raw_hex=bytes(self.g.mem_read(address+offset,min(4096,size-offset))).hex())
            for address,size in regions for offset in range(0,size,4096)]

    def mem_write(self,g,access,address,size,value,user):
        if self.pending_free:
            # Only complete owned storage; the source's invocation stack is
            # outside this CPU storage contract.
            if any(int(r['address'],16)<=address and address+size<=int(r['address'],16)+len(r['raw_hex'])//2
                for r in self.pending_free['before']):
                self.pending_free['writes'].append(dict(address=f'{address:08x}',size=size,
                    before=self.u(address,size),after=value&((1<<(size*8))-1)))
        super().mem_write(g,access,address,size,value,user)

    def raw_state(self):
        pool = self.u(0x4e78d0)
        headers = self.u(pool+0x44)
        heap = self.u(0x4e58bc)
        components = self.u(0x526248)
        world = self.u(0x526234)
        return dict(
            object_pool=f'{pool:08x}',
            object_pool_header_hex=bytes(self.g.mem_read(pool,0x4c)).hex(),
            object_headers_hex=bytes(self.g.mem_read(headers,2048*12)).hex(),
            object_bitmap_hex=bytes(self.g.mem_read(self.u(pool+0x48),256)).hex(),
            object_count=self.u(pool+0x3c), object_high_water=self.u(pool+0x38),
            body_heap=f'{heap:08x}', body_heap_header_hex=bytes(self.g.mem_read(heap,0x3c)).hex(),
            first_body_pointer=f'{self.u(headers+8):08x}',
            component_pool=f'{components:08x}',
            component_pool_header_hex=bytes(self.g.mem_read(components,0x4c)).hex(),
            component_count=self.u(components+0x3c),
            physics_counter=self.u(self.u(0x526230)), world=f'{world:08x}',
            world_hex=bytes(self.g.mem_read(world,0x260)).hex())

    def probe(self, name, entry, arguments):
        before=self.raw_state()
        self.active=name
        self.new_invalid=None
        self.teardown=True
        event_start=len(self.events)
        call_start=len(self.teardown_calls)
        try:
            self.run(entry,arguments)
            completion='complete_original_return'
        except (unicorn.UcError,KernelBoundary):
            assert self.new_invalid is not None
            completion='stopped_original_instruction'
        finally:
            self.teardown=False
        return dict(name=name,entry=f'{entry:08x}',arguments=list(arguments),
            completion=completion,stop=self.new_invalid,
            stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',
            calls=self.teardown_calls[call_start:],
            services=self.events[event_start:],before=before,after=self.raw_state())


def prepare(data, contexts):
    # Same original calls/fragments as the retained LiveTrace preparation.
    # The full title/session initializer and savegame/thread I/O are excluded.
    t=DeletionTrace(data)
    t.heap_prefix(None)
    t.call('original_CRT_lock_initializer',0x32cffc)
    t.call('original_CRT_onexit_initializer',0x3274e4)
    t.register()
    t.call('original_scratch_CRT_constructor',0x37d8c0)
    t.fixup()
    t.arena_root()
    t.actual_heap=True
    for address in (0x18fce0,0x18fd70,0x246a50):t.services.pop(address,None)
    t.call('original_source_empty_player_pool',0x14cc00)
    if contexts!='cold':t.call('original_view_control_storage_initializer',0x1874d0)
    t.call('original_physics_constant_storage_initializer',0x1eef40)
    t.call('original_full_object_storage_initializer',0xb76b0)
    if contexts=='storage':
        t.call('original_contrail_storage_initializer',0x17d220)
        t.call('original_effect_storage_initializer',0x1775d0)
        t.call('original_full_AI_storage_initializer',0x1cb180)
    else:
        t.fragment('original_AI_storage_and_reset_prefix',0x1cb180,0x1cb1f4)
    t.call('original_physics_storage_initializer',0x1c5f00)
    t.call('original_empty_player_map_reset',0x14ccd0)
    if contexts!='cold':t.call('original_view_control_map_reset',0x187520)
    t.call('original_physics_constants_map_reset',0x1eefe0)
    t.call('original_object_map_reset',0xb78c0)
    if contexts=='storage':t.call('original_contrail_map_reset',0x17d2e0)
    if contexts=='storage':t.call('original_effect_map_reset',0x177690)
    if contexts!='cold':t.call('original_multiplayer_AI_map_reset',0x1cb3d0)
    t.call('original_BSP_datum_table_map_reset',0x1c5fd0)
    t.fragment('original_activation_mask_zero_prefix',0x1394b0,0x1394f0)
    t.call('original_object_map_activation',0xb7aa0)
    t.call('original_full_new_map_order',0x1c5ff0)
    t.call('original_current_cluster_masks_with_empty_player_pool',0x13a380)
    return t


def original_source_evidence(directory):
    directory.mkdir(parents=True,exist_ok=True)
    versions={}
    for name,spec in SPECS.items():
        assert sha(spec['path'])==spec['pin']
        image=Xbe(spec['path']);code=_Code(image,'.text')
        starts={int(r['address'],16)for r in csv.DictReader((ROOT/spec['tsv']).open(),delimiter='\t')}
        versions[name]=dict(image=image,code=code,starts=starts)
    # These entries are the actual direct calls of the retained round-reset
    # owner and the last direct call of deletion, verified below.
    roots={'retail':(0xb83b0,0xb8460,0xbc300),'tu11':(0xb92a0,0xb9350,0xbd1f0),
        'u5':(0xba3e0,0xba490,0xbe330)}
    lists=[];pairs=[];owners={}
    for name,t in versions.items():
        image=t['image'];code=t['code'];starts=t['starts']
        def rows(entry):return literal(code,entry,starts|{entry})[1]
        deactivate,delete,free=roots[name]
        reset={'retail':0x159e20,'tu11':0x15b460,'u5':0x15c840}[name]
        calls=[i.operands[0].imm for i in rows(reset)if i.mnemonic=='call'and i.operands[0].type==2]
        assert calls[1:3]==[deactivate,delete]
        delete_calls=[i.operands[0].imm for i in rows(delete)if i.mnemonic=='call'and i.operands[0].type==2]
        assert delete_calls[-1]==free
        free_rows=rows(free)
        erase=next(i.operands[0].imm for i in free_rows if i.mnemonic=='call')
        header_allocate={'retail':0xbc280,'tu11':0xbd170,'u5':0xbe2b0}[name]
        allocate={'retail':0x16b840,'tu11':0x16d2c0,'u5':0x16e6a0}[name]
        assert any(i.mnemonic=='call'and i.operands[0].type==2 and i.operands[0].imm==allocate
            for i in rows(header_allocate))
        # MOV absolute globals in this complete owner identify the retained
        # pool slot and body-heap slot. No relocated native address is guessed.
        globals_=[i.operands[1].mem.disp for i in free_rows if i.mnemonic=='mov'and len(i.operands)==2
            and i.operands[1].type==3 and not i.operands[1].mem.base and not i.operands[1].mem.index]
        assert len(globals_)==3 and globals_[0]==globals_[2]
        owners[name]=dict(deactivate=deactivate,delete=delete,free=free,erase=erase,
            header_allocate=header_allocate,allocate=allocate,
            object_pool_slot=globals_[0],body_heap_slot=globals_[1])
        for owner,entry in owners[name].items():
            if owner.endswith('_slot'):continue
            rr=rows(entry)
            p=directory/f'{name}-{entry:08x}.asm'
            for i in rr:assert image.read(i.address,len(i.bytes))==i.bytes
            p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rr))
            lists.append(dict(file=p.name,sha256=sha(p),rows=len(rr)))
    for name in ('retail','tu11'):
        t,later=versions[name],versions['u5']
        for owner in ('deactivate','delete','free','erase','header_allocate','allocate'):
            a=literal(t['code'],owners[name][owner],t['starts']|{owners[name][owner]})[1]
            b=literal(later['code'],owners['u5'][owner],later['starts']|{owners['u5'][owner]})[1]
            assert complete_tokens(a,t['image'])==complete_tokens(b,later['image']),(name,owner)
            pairs.append(dict(earlier=name,owner=owner,complete_shape_equal=True,
                earlier_entry=f'{owners[name][owner]:08x}',later_entry=f'{owners['u5'][owner]:08x}',
                relocated_operands=references(a,b,t['image'],later['image'])))
    return dict(owners=owners,instruction_listings=lists,complete_source_pairs=pairs)


def map_snapshot(t,snapshot):
    for region in snapshot:
        address=int(region['address'],16);raw=bytes.fromhex(region['raw_hex'])
        for page in range(address&~4095,(address+len(raw)+4095)&~4095,4096):
            if not any(lo<=page<=hi for lo,hi,_ in t.g.mem_regions()):t.g.mem_map(page,4096)
        t.g.mem_write(address,raw)


def storage_api_records(source_records,owners):
    result={}
    for name,b in owners.items():
        t=Source(SPECS[name]);writes=[];current=[]
        def write(g,access,address,size,value,user):
            if any(int(r['address'],16)<=address and address+size<=int(r['address'],16)+len(r['raw_hex'])//2 for r in current):
                writes.append(dict(address=f'{address:08x}',size=size,
                    before=int.from_bytes(g.mem_read(address,size),'little'),after=value&((1<<(size*8))-1)))
        t.g.hook_add(unicorn.UC_HOOK_MEM_WRITE,write)
        records=[]
        for n,source in enumerate(source_records):
            for poison in (False,True):
                before=[dict(r)for r in source['before']]
                pool=next(r for r in before if int(r['address'],16)==0x4e78d0)['raw_hex']
                heap=next(r for r in before if int(r['address'],16)==0x4e58bc)['raw_hex']
                # Cross-image ABI composition: actual TU1.1-created raw banks
                # are explicit API inputs. Only their two pointer-slot
                # addresses and the declared debug poison flag change.
                before=[r for r in before if int(r['address'],16)not in(0x4e78d0,0x4e58bc)]
                before.extend([dict(address=f'{b['object_pool_slot']:08x}',raw_hex=pool),
                    dict(address=f'{b['body_heap_slot']:08x}',raw_hex=heap)])
                if poison:
                    p=int.from_bytes(bytes.fromhex(pool),'little')
                    region=next(r for r in before if int(r['address'],16)==p)
                    raw=bytearray.fromhex(region['raw_hex']);raw[0x2a]|=8;region['raw_hex']=raw.hex()
                map_snapshot(t,before);current=before;writes=[]
                t.call(b['free'],(source['datum'],))
                after=[dict(address=r['address'],raw_hex=bytes(t.g.mem_read(int(r['address'],16),len(r['raw_hex'])//2)).hex())for r in before]
                if name=='tu11'and not poison:
                    assert {r['address']:r['raw_hex']for r in after}=={r['address']:r['raw_hex']for r in source['after']}
                    assert writes==source['writes']
                records.append(dict(source_record=n,datum=source['datum'],debug_poison=poison,
                    before=before,after=after,writes=list(writes)))
        # Real original allocation produces a live header before any body
        # storage exists. This exercises BD1F0's NULL-body branch without
        # inventing a header, datum, salt, bitmap or allocation return.
        if source_records:
            for poison in (False,True):
                before=[dict(r)for r in source_records[0]['before']]
                pool=next(r for r in before if int(r['address'],16)==0x4e78d0)['raw_hex']
                heap=next(r for r in before if int(r['address'],16)==0x4e58bc)['raw_hex']
                before=[r for r in before if int(r['address'],16)not in(0x4e78d0,0x4e58bc)]
                before.extend([dict(address=f'{b['object_pool_slot']:08x}',raw_hex=pool),
                    dict(address=f'{b['body_heap_slot']:08x}',raw_hex=heap)])
                current=[];writes=[]
                map_snapshot(t,before)
                pool_address=int.from_bytes(bytes.fromhex(pool),'little')
                datum=t.call(b['allocate'],registers=((UC_X86_REG_EDX,pool_address),))&0xffffffff
                u=lambda address,size=4:int.from_bytes(t.g.mem_read(address,size),'little')
                assert datum!=0xffffffff and u(u(pool_address+0x44)+(datum&65535)*12+8)==0
                if poison:t.g.mem_write(pool_address+0x2a,bytes([u(pool_address+0x2a,1)|8]))
                before=[dict(address=r['address'],raw_hex=bytes(t.g.mem_read(int(r['address'],16),len(r['raw_hex'])//2)).hex())for r in before]
                current=before;writes=[]
                t.call(b['free'],(datum,))
                after=[dict(address=r['address'],raw_hex=bytes(t.g.mem_read(int(r['address'],16),len(r['raw_hex'])//2)).hex())for r in before]
                records.append(dict(source_record=0,datum=datum,debug_poison=poison,header_only=True,
                    before=before,after=after,writes=list(writes)))
        result[name]=dict(bindings={k:b[k]for k in('object_pool_slot','body_heap_slot')},records=records)
    return compact_raw_records(dict(schema=1,versions=result,
        boundary='Original storage-release APIs on actual TU1.1-created source-body banks; other images reuse these declared ABI inputs. Two additional records per image use the original datum allocator to produce header-only storage. Debug poison is an explicit API control. Full deactivation/deletion/physics providers are not supplied by the native storage owner.'))


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=OUT/'report.json')
    ap.add_argument('--fixture',type=Path,default=FIXTURE)
    ap.add_argument('--verify-fixture',type=Path)
    ap.add_argument('--contexts',nargs='+',choices=('cold','view','storage'),default=['cold','view','storage'])
    args=ap.parse_args()
    if args.verify_fixture:assert args.fixture.resolve()!=args.verify_fixture.resolve(),'Verification must not overwrite its expected fixture'
    for path,pin in PRIOR.items():assert sha(ROOT/path)==pin,path
    args.output.parent.mkdir(parents=True,exist_ok=True)
    env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix'))
    proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env)
    cases=[];free_records=[];executed=set()
    try:
        data=json.loads(proc.stdout.readline())
        setup=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/placement-report.json').read_text())
        for contexts in args.contexts:
            flows=[([0],[0])]
            if contexts=='storage':flows.extend([([32],[0]),([0,32,0],[1,0,2])])
            for indices,order in flows:
                t=prepare(data,contexts);stages=[];created=[]
                cycles=2 if contexts=='storage'else 1
                for cycle in range(cycles):
                    handles=[]
                    for index in indices:
                        row=next(r for r in setup['normal_setup']if r['version']=='tu11'and r['authored_index']==index)
                        t.g.mem_write(PLACEMENT,bytes.fromhex(row['placement_hex']))
                        assert t.attempt('original_authored_biped_creation',0xb8a30,(PLACEMENT,))
                        handle=t.g.reg_read(UC_X86_REG_EAX);handles.append(handle)
                        created.append(dict(cycle=cycle,authored_index=index,datum=f'{handle:08x}',
                            source_placement=row,creation=t.live_stages[-1]))
                    before=t.raw_state()
                    assert before['object_count']==len(indices)
                    selected=order if cycle==0 else list(reversed(range(len(indices))))
                    for slot in selected:
                        datum=handles[slot]
                        stage=t.probe('constructed_biped_deactivate',0xb92a0,(datum,1));stage['cycle']=cycle;stages.append(stage)
                        if stage['completion']!='complete_original_return':break
                        stage=t.probe('constructed_biped_delete',0xb9350,(datum,1));stage['cycle']=cycle;stages.append(stage)
                        if stage['completion']!='complete_original_return':break
                    if contexts=='storage':
                        assert stages[-1]['completion']=='complete_original_return',stages[-1]['stop']
                        after=t.raw_state()
                        assert after['object_count']==after['component_count']==after['physics_counter']==0
                        assert after['object_high_water']==0 and int(after['first_body_pointer'],16)==0
                        assert not t.pending_free
                    else:break
                section=t.image.section('.text');original=t.image.section_bytes(section)
                assert bytes(t.g.mem_read(section.va,len(original)))==original
                executed.update(t.executed);free_records.extend(t.free_records)
                cases.append(dict(contexts=contexts,authored_indices=indices,stages=stages,
                    creations=created,preparation=t.producers,whole_guest_text_unchanged=True,
                    remaining_services={f'{k:08x}':v for k,v in t.services.items()}))
                print(json.dumps(dict(contexts=contexts,indices=indices,creations=len(created),
                    complete_deletions=sum(s['name']=='constructed_biped_delete'and s['completion']=='complete_original_return'for s in stages),
                    stops=[s['stop']for s in stages if s['stop']])),flush=True)
    finally:
        _,err=proc.communicate('x',timeout=45)
    assert proc.returncode==0,err
    for path,pin in PRIOR.items():assert sha(ROOT/path)==pin,path
    directory=args.output.parent/'instructions'
    evidence=original_source_evidence(directory)
    fixture=storage_api_records(free_records,evidence['owners'])
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    encoded=json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n'
    if args.verify_fixture:assert encoded==args.verify_fixture.read_text()
    args.fixture.write_text(encoded)
    # Dynamic original executed instructions include the real Havok deletion
    # chain, not only static direct-owner listings.
    listing=directory/'tu11-executed-original.asm'
    image=Xbe(SPECS['tu11']['path']);code=_Code(image,'.text');text=image.section('.text')
    actual=[code.at(a)for a in sorted(executed)if text.va<=a<text.va+len(image.section_bytes(text))]
    for i in actual:assert image.read(i.address,len(i.bytes))==i.bytes
    listing.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in actual))
    evidence['instruction_listings'].append(dict(file=listing.name,sha256=sha(listing),rows=len(actual)))
    dependencies={str(Path(m.__file__).resolve().relative_to(ROOT)):sha(m.__file__)for m in list(sys.modules.values())
        if getattr(m,'__file__',None)and Path(m.__file__).resolve().is_relative_to(ROOT/'tools')}
    report=compact_raw_records(dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,
        oracle_sha256=sha(__file__),source_dependencies=dependencies,prior_pins=PRIOR,
        generator_sha256=sha(CONTEXT_OUT/'generator.exe'),fixture_sha256=sha(args.fixture),
        cases=cases,source_storage_records=free_records,**evidence,
        instruction_patches=[],supplied_teardown_services=[],
        summary=dict(constructed_bipeds=sum(len(c['creations'])for c in cases),
            completed_deletions=len(free_records),storage_api_records=sum(len(v['records'])for v in fixture['versions'].values())),
        boundaries=[
            'Retained raw platform/process heap/cache/resource and optional creation boundaries remain explicit. The pinned native compatibility thread producer stays alive throughout original calls; Xbox hardware parity and whole title/session startup are excluded.',
            'Actual B8A30 creates bodies from retained original normal-setup descriptors at authored Lockout starts0/32. Optional effect178120 and looping sourceC10E0 creation return NONE through named retained services; these providers are guarded before any teardown use.',
            'Source-created empty player membership and zero active-cluster masks produce inactive unowned bipeds. No current-unit owner, loadout, active motion/contact, death/damage or ordinary multiplayer match is established.',
            'Complete deactivation/deletion includes actual registered callbacks, Havok world removal/destruction, component retirement, body-heap unlink and datum-pool deletion. Every inherited supplied game/resource return is stopped before its hook during teardown. Compatibility locks and original raw backing remain the retained platform scope.',
            'Cold/view cases retain actual missing context stops. Storage cases use full selected player/view/physics/object/contrail/effect/AI contexts and source map resets, with whole unchanged guest text. No skipped teardown callback or supplied deletion result is used.',
            'Each successful flow reconstructs and deletes again from original APIs, including linked multi-body head/middle/tail retirement. This is source API composition, not complete map/scenario/placement round reset or native multi-round admission.',
            'Cross-image storage-release fixtures reuse actual TU1.1-produced raw API banks and separately controlled poison flags. They do not claim complete retail/TU1.5 live construction/deletion or bind the native match teardown callbacks.']))
    args.output.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
    print(json.dumps(dict(report=str(args.output),sha256=sha(args.output),fixture_sha256=sha(args.fixture),summary=report['summary'])))


if __name__=='__main__':main()
