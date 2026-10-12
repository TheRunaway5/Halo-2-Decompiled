#!/usr/bin/env python3
"""Original scenario reconstruction with actual looping-sound storage owners.

Selected source API composition only. Whole sound initialization retains its
physical-protection stop. An independent original audio constructor creates
its cold mixer context and stops before DirectSoundCreate without a supplied
return. Original row43/46 storage and map reset then execute. Neither stopped
audio frame is resumed, and no source readiness/gameplay result is supplied.
Whole11DE10 uses declared literal cache-index/header delivery in independent
backing; whole map I/O/loading remains excluded. Direct spawning and repeated
full resets do not establish ordinary phase/tick/match or audio startup.
Inherited fixed clock/RNG, compatibility storage and optional spawn services
retain their prior explicit boundaries.
"""
import argparse
import ctypes
import json
import os
from pathlib import Path
import subprocess
import sys

import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
import oracle_owned_round_reset as prior
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_mp_full_scoreboard_1_5 import Xbe,_Code
from oracle_smg_firing_1_1 import TAGS

ROOT=prior.ROOT
OUT=ROOT/'analysis/multiplayer/game-variants/round-reset-scenario'
PINS={
 'tools/oracle_owned_round_reset.py':'3b3b471ff9962a433afe2dc87c94f0052986b5c5c2c13fb6dddfde3a856a7435',
 'analysis/multiplayer/game-variants/owned-round-reset/report.json':'bd4bf12a36070bc45cb5e7dd7d07b4239f777377c79e38b9c833dcf2974019cb',
 'analysis/multiplayer/game-variants/round-reset-native/verification.json':'a929c95733872fe5460cb4f7be8d27390122373be2e378d208d1763e4c64ebe6',
}
class PreparationBoundary(Exception):
    def __init__(self,trace):self.trace=trace


class ScenarioTrace(prior.ResetTrace):
    full_tag_postprocess=False
    def __init__(self,data):
        self.new_preparation=[];self.cold_device=None;self.tag_postprocess=None
        self.tag_index_address=None;self.tag_index_writes=[];self.tag_index_delivery=None
        super().__init__(data)
        self.starts.update((0x223960,0x2239b0,0x18a200,0x18a2a0,0x18a9c0,0x383dd7))
    def run(self,address,argv=(),regs=()):
        if address not in(0x11de10,0x15b460):return super().run(address,argv,regs)
        self.count=0;self.w(prior.STACK,prior.STOP)
        for i,value in enumerate(argv):self.w(prior.STACK+4+i*4,value)
        self.g.reg_write(UC_X86_REG_ESP,prior.STACK)
        for reg,value in((UC_X86_REG_FPCW,0x027f),(UC_X86_REG_MXCSR,0x1f80),
            (UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff),*regs):self.g.reg_write(reg,value)
        self.g.emu_start(address,prior.STOP,count=10000000)
        assert self.g.reg_read(UC_X86_REG_EIP)==prior.STOP,(hex(address),hex(self.g.reg_read(UC_X86_REG_EIP)),self.count)
    def fixup(self):
        if not self.full_tag_postprocess:return super().fixup()
        c=self.cache;index=c.u(16);count=c.u(index+24)
        table=c.u(index+8)-(c.u(index)-(index+32))
        # Retain the same literal cache-index view as the earlier selected
        # iterator API. The original whole dispatcher owns all tag selection.
        self.g.mem_write(TAGS,bytes(c.data[table:table+count*16]))
        old_header=bytes(self.g.mem_read(index+c.magic,32)).hex()
        # The inherited metadata alias no longer contains the file's index
        # header. Deliver the exact authored32-byte iterator header in its own
        # actual backing, rather than substituting a chosen tag count.
        capacity=4096;alias=self.new_vm_next;self.new_vm_next+=capacity
        assert all(alias+capacity<=lo or alias>hi for lo,hi,_ in self.g.mem_regions())
        ctypes.set_errno(0);pointer=self.libc.mmap(None,capacity,3,0x22,-1,0)
        error=ctypes.get_errno();assert pointer not in(None,ctypes.c_void_p(-1).value),(error,capacity)
        self.host_backings.append((pointer,capacity))
        self.g.mem_map_ptr(alias,capacity,unicorn.UC_PROT_READ|unicorn.UC_PROT_WRITE,pointer)
        file_header=bytes(c.data[index:index+32]);self.g.mem_write(alias,file_header)
        self.w(0x54fa50,alias);self.tag_index_address=alias
        header_before=bytes(self.g.mem_read(alias,32)).hex()
        self.tag_index_delivery=dict(file_offset=f'{index:08x}',raw32_hex=file_header.hex(),
            inherited_metadata_alias=f'{index+c.magic:08x}',inherited_header_hex=old_header,
            inherited_header_matching_file_offset=c.data.find(bytes.fromhex(old_header)),
            actual_mmap_succeeded=True,actual_errno=error,guest_header_alias=f'{alias:08x}',capacity=capacity,
            scope='declared literal cache-index header API delivery in independent actual host backing; whole original map I/O/loading remains excluded')
        tags=[t for t in c.tags.values()if t['group']in('bipd','crea','coll','phmo','vehi')]
        before=[dict(tag=t,raw_hex=bytes(self.g.mem_read(t['offset']+c.magic,t['size'])).hex())for t in tags]
        self.active='original_whole_map_physics_tag_postprocess';start=len(self.events);self.new_invalid=None
        try:self.run(0x11de10);complete=True
        except (unicorn.UcError,KernelBoundary):
            assert self.new_invalid is not None;complete=False
        after=[dict(tag=t,raw_hex=bytes(self.g.mem_read(t['offset']+c.magic,t['size'])).hex())for t in tags]
        self.tag_postprocess=dict(entry='0011de10',cache_tag_count=count,
            original_iterator_index=0,before=before,after=after,
            complete_original_return=complete,stop=self.new_invalid,
            eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',services=self.events[start:],
            observed_instructions=self.count,instruction_budget=10000000,
            file_header_hex=bytes(c.data[index:index+32]).hex(),
            actual_header_before_hex=header_before,
            actual_header_after_hex=bytes(self.g.mem_read(alias,32)).hex(),
            tag_index_writes=self.tag_index_writes,
            scope='whole original physics tag dispatcher and its tails; literal cache index view remains a declared delivery boundary')
        if not complete:raise PreparationBoundary(self)
    def mem_write(self,g,access,address,size,value,user):
        if self.tag_index_address is not None and self.active=='original_whole_map_physics_tag_postprocess'and(
            self.tag_index_address<=address<self.tag_index_address+32 or address==0x54fa50):
            self.tag_index_writes.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
                address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),
                after_hex=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
        super().mem_write(g,access,address,size,value,user)
    def hook(self,g,address,size,user):
        if address==0x383dd7:
            sp=g.reg_read(UC_X86_REG_ESP)
            self.new_invalid=dict(boundary='unprovided original DirectSoundCreate; no return supplied',
                instruction=f'{address:08x}',return_site=f'{self.u(sp):08x}',
                arguments=[self.u(sp+4+i*4)for i in range(3)],registers=self.registers())
            raise KernelBoundary()
        if address==0x2205c0:
            # Bypass only the older observation stop at constructor entry;
            # execute the real constructor until its first device dependency.
            prior.OwnedTrace.hook(self,g,address,size,user);return
        super().hook(g,address,size,user)
    def audio_state(self):
        context=self.u(0x526474);pool=self.u(0x4f485c);classes=self.u(0x5098f0)
        return dict(mixer=f'{context:08x}',mixer2ad8_hex=bytes(self.g.mem_read(context,0x2ad8)).hex()if context else None,
            looping_pool=f'{pool:08x}',looping_header_hex=bytes(self.g.mem_read(pool,0x4c)).hex()if pool else None,
            looping_rows_hex=bytes(self.g.mem_read(self.u(pool+0x44),0x400*0x18)).hex()if pool else None,
            looping_context=f'{self.u(0x4f4858):08x}',
            looping244_hex=bytes(self.g.mem_read(self.u(0x4f4858),0x244)).hex()if self.u(0x4f4858)else None,
            classes=f'{classes:08x}',classes360_hex=bytes(self.g.mem_read(classes,0x360)).hex()if classes else None)
    def independent_device_prefix(self):
        self.active='independent_original_audio_device_constructor_to_DirectSoundCreate'
        self.new_invalid=None;start=len(self.events);before=self.audio_state()
        try:self.run(0x2205c0,(0x451220,));completion='complete_original_return'
        except (unicorn.UcError,KernelBoundary):
            assert self.new_invalid is not None;completion='stopped_original_instruction'
        self.cold_device=dict(entry='002205c0',arguments=[0x451220],
            original_argument_producer='00126c8b',
            original_argument_hex=self.image.read(0x451220,12).hex(),
            completion=completion,stop=self.new_invalid,
            eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',services=self.events[start:],
            before=before,after=self.audio_state(),independent_later_reset_not_a_resumption=True)
    def call(self,name,address,args=(),regs=(),blocked=None):
        if name=='original_full_AI_storage_initializer':
            self.independent_device_prefix()
            super().call('original_object_looping_sound_storage_initializer',0x18a200)
            self.new_preparation.append(self.producers[-1])
        result=super().call(name,address,args,regs,blocked)
        if name=='original_effect_storage_initializer':
            super().call('original_sound_class_storage_initializer',0x223960)
            self.new_preparation.append(self.producers[-1])
        if name=='original_effect_map_reset':
            super().call('original_sound_class_map_reset',0x2239b0)
            self.new_preparation.append(self.producers[-1])
            super().call('original_object_looping_sound_map_reset',0x18a2a0)
            self.new_preparation.append(self.producers[-1])
        return result


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=OUT/'report.json')
    ap.add_argument('--tag-postprocess',choices=('selected-biped','full'),default='full')
    ap.add_argument('--cycles',type=int,default=2)
    args=ap.parse_args()
    assert 1<=args.cycles<=3
    for path,pin in PINS.items():assert prior.sha(ROOT/path)==pin,path
    args.output.parent.mkdir(parents=True,exist_ok=True)
    proc=subprocess.Popen(['wine',str(prior.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
        env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(prior.CONTEXT_OUT/'wine-prefix')))
    try:
        data=json.loads(proc.stdout.readline());prior.ResetTrace=ScenarioTrace
        ScenarioTrace.full_tag_postprocess=args.tag_postprocess=='full'
        try:t=prior.prepare(data,True);prepared=True
        except PreparationBoundary as boundary:t=boundary.trace;prepared=False
        player=None;reset=None;before=None;after=None;cycles=[]
        if prepared:
            player=prior.join(t,0)
            for cycle in range(args.cycles):
                assert t.stage('original_Elimination_normal_player_spawn',0x1509c0,(player,))
                before=t.audio_state();free_start=len(t.free_records)
                reset=t.step('unchanged_Elimination_full_object_map_reset',0x15b460,guard=True)
                observed=t.count;after=t.audio_state()
                cycles.append(dict(index=cycle,reset=reset,audio_before=before,audio_after=after,
                    storage_releases=len(t.free_records)-free_start,
                    observed_instructions=observed,instruction_budget=10000000,
                    constructed_world_after=t.state()if reset['completion']=='complete_original_return'else None))
                if reset['completion']!='complete_original_return':break
        section=t.image.section('.text');raw=t.image.section_bytes(section)
        assert bytes(t.g.mem_read(section.va,len(raw)))==raw
        if reset:assert reset['before']['variant_hex']==reset['after']['variant_hex']==t.variant['raw_hex']
    finally:
        _,error=proc.communicate('x',timeout=45)
    assert proc.returncode==0,error
    for path,pin in PINS.items():assert prior.sha(ROOT/path)==pin,path
    directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
    code=_Code(t.image,'.text')
    rows=[code.at(a)for a in sorted(t.executed)if section.va<=a<section.va+len(raw)]
    for i in rows:assert t.image.read(i.address,len(i.bytes))==i.bytes
    listing=directory/'tu11-executed-original.asm'
    listing.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rows))
    deps={str(Path(m.__file__).resolve().relative_to(ROOT)):prior.sha(m.__file__)
        for m in list(sys.modules.values())if getattr(m,'__file__',None)
        and Path(m.__file__).resolve().is_relative_to(ROOT/'tools')}
    report=prior.compact_raw_records(dict(schema=1,source_sha256=prior.PIN,cache_sha256=prior.CACHE_PIN,
        oracle_sha256=prior.sha(__file__),source_dependencies=deps,prior_pins=PINS,
        generator_sha256=prior.sha(prior.CONTEXT_OUT/'generator.exe'),
        prepared=prepared,variant=getattr(t,'variant',None),source_player=f'{player:08x}'if player is not None else None,preparation=t.producers,
        tag_postprocess=t.tag_postprocess,
        tag_index_delivery=t.tag_index_delivery,
        new_preparation=t.new_preparation,sound_initializer=getattr(t,'sound_prefix',None),
        independent_device_constructor=t.cold_device,player_stages=t.player_stages,
        cycles=cycles,raw_vm=t.raw_vm,requested_cycles=args.cycles,
        storage_release_records=t.free_records,whole_guest_text_unchanged=True,
        instruction_patches=[],new_supplied_gameplay_returns=[],
        executed_instruction_listing=dict(file=listing.name,sha256=prior.sha(listing),rows=len(rows)),
        boundary=__doc__))
    args.output.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
    print(json.dumps(dict(report=str(args.output),sha256=prior.sha(args.output),
        prepared=prepared,completion=reset['completion']if reset else 'stopped_preparation',
        stop=reset['stop']if reset else t.tag_postprocess['stop'],deleted=len(t.free_records))),flush=True)


if __name__=='__main__':main()
