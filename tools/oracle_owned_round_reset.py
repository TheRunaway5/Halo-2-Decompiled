#!/usr/bin/env python3
"""Original authored full reset with source-joined, constructed player units.

Selected original startup/storage APIs are composition, not whole startup.
New VirtualAlloc boundaries own actual mmap-backed raw pages only. Optional
creation leaves retain the prior declared scope; reset guards every provided
gameplay/resource leaf. Sound startup stops before unprovided device/platform
owners; an independent reset on its source-produced cold prefix is explicitly
separate from successful audio initialization.
"""
import argparse
import ctypes
import json
import os
from pathlib import Path
import subprocess
import sys

import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_owned_object_deletion import OwnedTrace,ROOT,CONTEXT_OUT,PLAYER_PINS,join,sha
from oracle_mp_item_current_producers_1_1 import CurrentTrace
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_round_lifecycle import variants,compact_raw_records
from oracle_game_over import SPECS
from oracle_smg_firing_1_1 import STACK,STOP,PIN,CACHE_PIN
from oracle_mp_full_scoreboard_1_5 import Xbe,_Code

OUT=ROOT/'analysis/multiplayer/game-variants/owned-round-reset'


class ResetTrace(OwnedTrace):
    def __init__(self,data):
        self.host_backings=[];self.raw_vm=[];self.reset_stages=[];self.new_vm_next=0xb000000
        self.libc=ctypes.CDLL(None,use_errno=True)
        self.libc.mmap.restype=ctypes.c_void_p
        self.libc.mmap.argtypes=(ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_long)
        super().__init__(data)
        self.starts.update((0x15b460,0x1a2c60,0xbbda0,0x17ef90,0x127b50,
            0xd6b60,0xc1870,0xd6c40,0x126b20,0x2205c0))
    def hook(self,g,address,size,user):
        if self.physical and address==self.u(0x417ba8):
            sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(3)]
            if args!=[0x82d47000,0x3fe000,4]:
                self.new_invalid=dict(boundary='unprovided original physical address protection request; no return supplied',
                    instruction=f'{address:08x}',original_data_thunk='00417ba8',
                    return_site=f'{self.u(sp):08x}',registers=self.registers(),arguments=args)
                raise KernelBoundary()
        if address==0x33785c:
            sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(4)]
            if args[0]==0 and 0<args[1]<=0x1000000 and args[1]not in(0x1450,0x850,0x1409b,0x106470)and args[2]in(0x101000,0x1000)and args[3]==4:
                assert not self.teardown,'Unexpected allocation during reset'
                capacity=(args[1]+4095)&~4095;alias=self.new_vm_next
                self.new_vm_next+=capacity
                assert all(alias+capacity<=lo or alias>hi for lo,hi,_ in g.mem_regions())
                ctypes.set_errno(0);pointer=self.libc.mmap(None,capacity,3,0x22,-1,0)
                error=ctypes.get_errno()
                assert pointer not in(None,ctypes.c_void_p(-1).value),(error,args)
                self.host_backings.append((pointer,capacity))
                g.mem_map_ptr(alias,capacity,unicorn.UC_PROT_READ|unicorn.UC_PROT_WRITE,pointer)
                self.raw_vm.append(dict(phase=self.active,entry=f'{address:08x}',
                    original_return_site=f'{self.u(sp):08x}',arguments=args,
                    original_requested_bytes=args[1],actual_mmap_capacity=capacity,
                    actual_mmap_succeeded=True,actual_errno=error,guest_alias=f'{alias:08x}',
                    scope='actual host mmap raw backing; no source object/cache/readiness field or result supplied'))
                self.executed.add(address)
                g.reg_write(UC_X86_REG_EAX,alias);g.reg_write(UC_X86_REG_EIP,self.u(sp))
                g.reg_write(UC_X86_REG_ESP,sp+20);return
        if address==0x2205c0:
            sp=g.reg_read(UC_X86_REG_ESP)
            self.new_invalid=dict(boundary='unprovided original audio device constructor; no return supplied',
                instruction=f'{address:08x}',return_site=f'{self.u(sp):08x}',
                registers=self.registers(),arguments=[self.u(sp+4)])
            raise KernelBoundary()
        super().hook(g,address,size,user)
    def extra_state(self):
        system=self.u(0x4ed950)
        mask=self.u(0x4f4850);decal=self.u(0x4f1f20);cache=self.u(0x510c38)
        return dict(player=self.player_state(),world=self.raw_state(),
            variant_hex=bytes(self.g.mem_read(self.u(0x4edf18)+0x13c,304)).hex(),
            sound_system=f'{system:08x}',sound20c_hex=bytes(self.g.mem_read(system,0x20c)).hex()if system else None,
            sound_available=self.u(system+0x78,1)if system else None,
            static_arena=f'{self.u(0x517ff0):08x}',static_used=self.u(0x517ff4),static_crc=self.u(0x517ff8),
            mask=f'{mask:08x}',mask4201_hex=bytes(self.g.mem_read(mask,0x4201)).hex()if mask else None,
            decal_pool=f'{decal:08x}',decal_header_hex=bytes(self.g.mem_read(decal,0x4c)).hex()if decal else None,
            vertex_cache=f'{cache:08x}',vertex_cache613c_hex=bytes(self.g.mem_read(cache,0x613c)).hex()if cache else None)
    def step(self,name,entry,args=(),guard=False,regs=()):
        before=self.extra_state();self.active=name;self.new_invalid=None
        event_start=len(self.events);call_start=len(self.teardown_calls)
        self.teardown=guard
        try:
            self.run(entry,args,regs);completion='complete_original_return'
        except (unicorn.UcError,KernelBoundary):
            assert self.new_invalid is not None
            completion='stopped_original_instruction'
        finally:self.teardown=False
        result=dict(name=name,entry=f'{entry:08x}',arguments=list(args),guarded=guard,
            completion=completion,stop=self.new_invalid,
            eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',
            services=self.events[event_start:],calls=self.teardown_calls[call_start:],
            before=before,after=self.extra_state(),source_storage_after=self.storage_snapshot())
        self.reset_stages.append(result)
        return result


def prepare(data,sound):
    t=ResetTrace(data);t.heap_prefix(None)
    t.call('original_CRT_lock_initializer',0x32cffc)
    t.call('original_CRT_onexit_initializer',0x3274e4)
    t.register();t.call('original_scratch_CRT_constructor',0x37d8c0)
    t.call('original_engine_table_CRT_callback',0x37eb40)
    t.fixup();t.arena_root();t.actual_heap=True
    for a in(0x18fce0,0x18fd70,0x246a50,0x111490,0x111500,
             0x1dc8f0,0x1dc940,0xfb560,0xefee0,0xc9a40):t.services.pop(a,None)
    initializers=(('player',0x14cc00),('view_control',0x1874d0),('engine',0x158ba0),
        ('physics_constants',0x1eef40),('object',0xb76b0),('unit_queue',0x10d490),
        ('trigger_volume',0x1efe10),('mask',0x1858e0),('decal',0x17ec90),
        ('contrail',0x17d220),('effect',0x1775d0))
    for name,entry in initializers:t.call('original_'+name+'_storage_initializer',entry)
    t.sound_prefix=None
    # Sound row44 belongs before AI58/physics59. The exact original static
    # reservation fragment has EBX=0 at entry, set by its caller12001. The
    # preceding disk/partition call and later startup remain excluded.
    if sound:
        t.active='original_process_static_arena_reservation_fragment'
        t.g.reg_write(UC_X86_REG_ESP,STACK);t.w(STACK,STOP)
        t.g.reg_write(UC_X86_REG_EBX,0)
        t.g.emu_start(0x12008,0x12025,count=200000)
        assert t.g.reg_read(UC_X86_REG_EIP)==0x12025
        t.producers.append(dict(name=t.active,original_fragment=['00012008','00012025'],
            entry_ebx=0,entry_ebx_producer='00012001',scope='separate original reservation fragment; whole12000 excluded'))
        # Cannot use step/player_state until queue/engine map resets below.
        t.active='original_sound_initializer_to_dependency';t.new_invalid=None
        start=len(t.events)
        try:t.run(0x126b20);completion='complete_original_return'
        except (unicorn.UcError,KernelBoundary):
            assert t.new_invalid is not None;completion='stopped_original_instruction'
        system=t.u(0x4ed950)
        t.sound_prefix=dict(entry='00126b20',completion=completion,stop=t.new_invalid,
            eip=f'{t.g.reg_read(UC_X86_REG_EIP):08x}',services=t.events[start:],
            sound_system=f'{system:08x}',available=t.u(system+0x78,1)if system else None,
            independent_later_reset_not_a_resumption=True)
    t.call('original_full_AI_storage_initializer',0x1cb180)
    t.call('original_physics_storage_initializer',0x1c5f00)
    for name,entry in(('player',0x14ccd0),('view_control',0x187520)):
        t.call('original_'+name+'_map_reset',entry)
    t.variant=variants()['7']
    assert t.variant['index']==7 and t.variant['file_offset']=='038d3f90'
    t.g.mem_write(t.u(0x4edf18)+0x13c,bytes.fromhex(t.variant['raw_hex']))
    t.default_variant=t.variant['raw_hex']
    assert t.stage('original_Elimination_engine_map_reset',0x158c10)
    for name,entry in(('physics_constants',0x1eefe0),('object',0xb78c0),('unit_queue',0x10d4f0),
        ('trigger_volume',0x1efe70),('mask',0x185960),('decal',0x17ed50),
        ('contrail',0x17d2e0),('effect',0x177690),('multiplayer_AI',0x1cb3d0),('BSP_datum_table',0x1c5fd0)):
        t.call('original_'+name+'_map_reset',entry)
    t.fragment('original_activation_mask_zero_prefix',0x1394b0,0x1394f0)
    t.call('original_object_map_activation',0xb7aa0);t.call('original_full_new_map_order',0x1c5ff0)
    t.call('original_current_cluster_masks_with_empty_player_pool',0x13a380)
    t.track_players=True;CurrentTrace.weapon_resources(t)
    return t


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=OUT/'report.json')
    ap.add_argument('--sound',choices=('none','prefix','both'),default='both')
    args=ap.parse_args()
    for path,pin in PLAYER_PINS.items():assert sha(ROOT/path)==pin,path
    args.output.parent.mkdir(parents=True,exist_ok=True)
    proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
        env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix')))
    cases=[];executed=set()
    try:
        data=json.loads(proc.stdout.readline())
        for sound in([False,True]if args.sound=='both'else[args.sound=='prefix']):
            t=prepare(data,sound);p=join(t,0)
            assert t.stage('original_Elimination_normal_player_spawn',0x1509c0,(p,))
            assert t.g.reg_read(UC_X86_REG_EAX)&255==1
            assert t.u(t.u(0x4edf18)+0x184)&8
            result=t.step('unchanged_Elimination_full_object_map_reset',0x15b460,guard=True)
            section=t.image.section('.text');raw=t.image.section_bytes(section)
            assert bytes(t.g.mem_read(section.va,len(raw)))==raw
            executed.update(t.executed)
            assert result['before']['variant_hex']==t.variant['raw_hex']
            assert result['after']['variant_hex']==t.variant['raw_hex']
            cases.append(dict(sound_prefix_included=sound,variant=t.variant,
                source_player=f'{p:08x}',creation_input_hex=t.creation_input_hex,
                sound_initializer=t.sound_prefix,preparation=t.producers,
                player_stages=t.player_stages,reset=result,
                raw_vm=t.raw_vm,storage_release_records=t.free_records,
                whole_guest_text_unchanged=True))
            print(json.dumps(dict(sound=sound,completion=result['completion'],stop=result['stop'],
                sound_initializer=t.sound_prefix,
                deleted=len(t.free_records))),flush=True)
    finally:
        _,error=proc.communicate('x',timeout=45)
    assert proc.returncode==0,error
    directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
    image=Xbe(SPECS['tu11']['path']);code=_Code(image,'.text');section=image.section('.text')
    rows=[code.at(a)for a in sorted(executed)if section.va<=a<section.va+len(image.section_bytes(section))]
    for i in rows:assert image.read(i.address,len(i.bytes))==i.bytes
    listing=directory/'tu11-executed-original.asm'
    listing.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rows))
    dependencies={str(Path(m.__file__).resolve().relative_to(ROOT)):sha(m.__file__)
        for m in list(sys.modules.values())if getattr(m,'__file__',None)
        and Path(m.__file__).resolve().is_relative_to(ROOT/'tools')}
    storage_rows={}
    for index in(19,20,22,25,27,28,30,34,37,39,40,43,44,46,58,59):
        address=0x447428+index*36;raw=image.read(address,36)
        storage_rows[str(index)]=dict(table_address=f'{address:08x}',record_hex=raw.hex(),
            initialize=f'{int.from_bytes(raw[:4],"little"):08x}',
            map_reset=f'{int.from_bytes(raw[8:12],"little"):08x}')
    report=compact_raw_records(dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,
        oracle_sha256=sha(__file__),source_dependencies=dependencies,prior_pins=PLAYER_PINS,
        generator_sha256=sha(CONTEXT_OUT/'generator.exe'),cases=cases,
        original_storage_rows=storage_rows,
        unresolved_object_looping_sound_pool=dict(global_slot='004f485c',
            allocate='0018a9c0',initializer='0018a200',map_reset='0018a2a0',
            table_index=46,source_name_address='0045ad5c',source_name_hex=image.read(0x45ad5c,22).hex(),
            scope='source-listed next owner; initialization/map reset are not executed or supplied'),
        instruction_patches=[],new_supplied_gameplay_returns=[],
        executed_instruction_listing=dict(file=listing.name,sha256=sha(listing),rows=len(rows)),boundary=__doc__))
    args.output.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
    print(json.dumps(dict(report=str(args.output),sha256=sha(args.output))))


if __name__=='__main__':main()
