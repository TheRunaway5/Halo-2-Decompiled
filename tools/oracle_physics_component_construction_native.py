#!/usr/bin/env python3
"""Observe unchanged complete component allocation/initialization in its owner."""
import argparse,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_body_slots_1_1 as prior
from oracle_mp_power_body_slots_1_1 import SlotTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_mp_spawn_biped_physics import RAW_COMPONENTS
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace

class Capture(SlotTrace):
    def __init__(self,*args,**kwargs):
        self.component_enabled=False;self.component_pending=None;self.component_done=False;self.component_stores=[]
        super().__init__(*args,**kwargs)
    def component_state(self):
        pool=self.u(0x4e78d0);heads=self.u(pool+0x44);body=self.u(heads+(self.component_object&65535)*12+8);clock=self.u(0x518444)
        return[dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())for p,n in(
            (RAW_COMPONENTS,0x1409b),(pool,0x4c),(heads,self.u(pool+0x20)*12),(body,0x382),
            (0x4e78d0,4),(0x526248,4),(0x526230,4),(self.u(0x526230),4),(0x518444,4),(clock,4))]
    def hook(self,g,address,size,user):
        if self.component_enabled and not self.component_done:
            esp=g.reg_read(reg.UC_X86_REG_ESP)
            if self.component_pending and address==self.component_pending[0]and esp>self.component_pending[1]:
                self.component_after=self.component_state();self.component_result=g.reg_read(reg.UC_X86_REG_EAX);self.component_done=True
            elif address==0x1d2aa0 and self.component_pending is None and self.active=='original_authored_scenario_crate_factory':
                self.component_object=g.reg_read(reg.UC_X86_REG_EAX);self.component_pending=(self.u(esp),esp)
                self.component_before=self.component_state();self.component_fp=dict(x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}')
        super().hook(g,address,size,user)
    def mem_write(self,g,kind,address,size,value,user):
        if self.component_pending and not self.component_done and not(STACK-0x100000<=address<STACK+0x100000):
            assert size in(1,2,4)
            self.component_stores.append(dict(instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',bytes=size,
                before_hex=bytes(g.mem_read(address,size)).hex(),after_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
        super().mem_write(g,kind,address,size,value,user)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();assert not out.exists();out.mkdir(parents=True);prior.verify()
    trace=prior.prior.prior.power.player.prepare(json.loads((OWNER/'raw-host-context.json').read_text()),trace_type=Capture)
    trace.selected_physics_fixup(0xe6f80583);trace.bridge_enabled=True;trace.slot_ready=True;trace.component_enabled=True
    result=trace.factory(0xe6f80583);trace.slot_return(STOP);assert trace.component_done
    expected=json.loads((OWNER/'report.json').read_text())['case'];keys=set(result)-{'calls','ordered_stores'}
    assert {k:result[k]for k in keys}=={k:expected[k]for k in keys}
    old_path=ROOT/'analysis/multiplayer/sandbox-source/power-body-slots/final/measured/report.json';old=json.loads(old_path.read_text())
    assert trace.slot_calls==old['calls'][:39]and trace.slot_states==old['states'][:19]
    listings=[]
    for entry in(0x16d2c0,0x1d2aa0,0x1d3340):
        body=_trace(trace.code,entry,trace.starts|{0x16d2c0,0x1d2aa0,0x1d3340});lines=[]
        for pc in sorted(body.code):
            i=trace.code.at(pc);assert trace.image.read(pc,len(i.bytes))==i.bytes
            lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
        file=f'{entry:08x}.asm';frozen_write(out/file,''.join(lines).encode());listings.append(dict(entry=f'{entry:08x}',file=file,sha256=sha(out/file),rows=len(lines)))
    report=dict(schema='physics-component-construction-original-v1',oracle_sha256=sha(Path(__file__)),
        inherited_slot_report_sha256=sha(old_path),entire_factory_and_retained_slot_sequence_identical=True,
        object=f'{trace.component_object:08x}',return_eax=f'{trace.component_result:08x}',floating_point=trace.component_fp,
        regions_before=trace.component_before,regions_after=trace.component_after,stores=trace.component_stores,
        tick_scale_bits=trace.image.read(0x465198,4).hex(),instruction_listings=listings,
        instruction_patches=[],supplied_callee_results=[],private_x86_abi_stack_excluded=True,
        scope='Complete unchanged1D2AA0 with real16D2C0/1D3340 in original crate constructor. Full retained allocation/object/clock fields, no native final body injection or world/contact readiness.')
    frozen_write(out/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(out/'report.json'),stores=len(trace.component_stores),datum=f'{trace.component_result:08x}')))

if __name__=='__main__':main()
