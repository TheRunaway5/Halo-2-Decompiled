#!/usr/bin/env python3
"""Observe original object physical constructors without replacing lower calls."""
import argparse,hashlib,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_body_slots_1_1 as prior
from oracle_mp_power_body_slots_1_1 import SlotTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_mp_spawn_biped_physics import RAW_HAVOK,RAW_COMPONENTS
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace

ENTRIES={0x1c7050,0x1c6f50,0x1c72e0,0xbbe70,0xc0d10}
LOWER={0x148360,0x27f190,0x1d2aa0,0x1d2b10,0x1d4f30}
PIN='d3564d22b7b5046c400ab038c3e14f26df0ebd6d61049876d0a0eecc14553e2c'

class Capture(SlotTrace):
    def __init__(self,*args,**kwargs):
        self.owner_enabled=False;self.owner_object=None;self.owner_pending=[]
        self.owner_lower=[];self.owner_calls=[];self.owner_stores=[];self.owner_states=[];self.owner_ids={}
        self.owner_order=0;self.owner_done=False;super().__init__(*args,**kwargs)
    def owner_state(self):
        pool=self.u(0x4e78d0);heads=self.u(pool+0x44);body=self.u(heads+(self.owner_object&65535)*12+8)
        regions=[(RAW_HAVOK,0x106470),(RAW_COMPONENTS,0x1409b),(pool,0x4c),(heads,self.u(pool+0x20)*12),
            (body,0x382),(0x4e78d0,4),(0x526248,4),(0x486560,1),(0x526230,4),(self.u(0x526230),4)]
        rows=[dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())for p,n in regions]
        key=hashlib.sha256(encoded(rows)).hexdigest()
        if key not in self.owner_ids:
            self.owner_ids[key]=len(self.owner_states);self.owner_states.append(dict(id=len(self.owner_states),regions=rows,sha256=key))
        return self.owner_ids[key]
    def hook(self,g,address,size,user):
        esp=g.reg_read(reg.UC_X86_REG_ESP)
        if self.owner_enabled and not self.owner_done:
            if self.owner_pending and address==self.owner_pending[0][0]and esp>self.owner_pending[0][1]:
                assert not self.owner_lower
                self.owner_final=self.owner_state();self.owner_done=True
            if not self.owner_done:
                for pending in self.owner_lower[:]:
                    ret,sp,row=pending
                    if address==ret and esp>sp:
                        row.update(after_state=self.owner_state(),return_eax=f'{g.reg_read(reg.UC_X86_REG_EAX):08x}',return_order=self.owner_order)
                        self.owner_order+=1;self.owner_lower.remove(pending)
                if not self.owner_pending and address==0x1c7050 and self.active=='original_authored_scenario_crate_factory':
                    self.owner_object=self.u(esp+8);self.owner_initial=self.owner_state()
                    self.owner_root=dict(entry='001c7050',root=f'{self.u(esp+4):08x}',object=f'{self.owner_object:08x}')
                    self.owner_pending=[(self.u(esp),esp)]
                if self.owner_pending and address in LOWER and not self.owner_lower:
                    obj=g.reg_read(reg.UC_X86_REG_EAX)if address==0x1d2aa0 else 0
                    component=g.reg_read(reg.UC_X86_REG_EAX)if address==0x1d2b10 else g.reg_read(reg.UC_X86_REG_ECX)if address==0x1d4f30 else 0
                    row=dict(id=len(self.owner_calls),entry=f'{address:08x}',instruction=f'{self.u(esp)-5:08x}',
                        object=f'{obj:08x}',component=f'{component:08x}',before_state=self.owner_state(),order=self.owner_order)
                    self.owner_order+=1;self.owner_calls.append(row);self.owner_lower.append((self.u(esp),esp,row))
        super().hook(g,address,size,user)
    def mem_write(self,g,kind,address,size,value,user):
        pc=g.reg_read(reg.UC_X86_REG_EIP)
        if self.owner_enabled and self.owner_pending and not self.owner_done and not self.owner_lower and not(STACK-0x100000<=address<STACK+0x100000):
            assert size in(1,2,4,8)
            self.owner_stores.append(dict(order=self.owner_order,instruction=f'{pc:08x}',address=f'{address:08x}',bytes=size,
                before_hex=bytes(g.mem_read(address,size)).hex(),after_hex=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
            self.owner_order+=1
        super().mem_write(g,kind,address,size,value,user)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    prior.verify();old_path=ROOT/'analysis/multiplayer/sandbox-source/power-body-slots/final/measured/report.json'
    assert sha(old_path)==PIN
    context=json.loads((OWNER/'raw-host-context.json').read_text())
    trace=prior.prior.prior.power.player.prepare(context,trace_type=Capture)
    trace.selected_physics_fixup(0xe6f80583);trace.bridge_enabled=True;trace.slot_ready=True;trace.owner_enabled=True
    result=trace.factory(0xe6f80583);trace.slot_return(STOP)
    assert trace.owner_done and not trace.owner_lower
    expected=json.loads((OWNER/'report.json').read_text())['case'];keys=set(result)-{'calls','ordered_stores'}
    assert {k:result[k]for k in keys}=={k:expected[k]for k in keys}
    old=json.loads(old_path.read_text())
    assert trace.slot_calls==old['calls'][:39]
    assert trace.slot_states==old['states'][:19]
    listing=[]
    for entry in sorted(ENTRIES):
        body=_trace(trace.code,entry,trace.starts|ENTRIES|LOWER)
        lines=[]
        for pc in sorted(body.code):
            i=trace.code.at(pc);assert trace.image.read(pc,len(i.bytes))==i.bytes
            lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
        name=f'{entry:08x}.asm';frozen_write(out/name,''.join(lines).encode());listing.append(dict(entry=f'{entry:08x}',file=name,sha256=sha(out/name),rows=len(lines)))
    report=dict(schema='object-physics-construction-original-v1',oracle_sha256=sha(Path(__file__)),
        inherited_slot_report_sha256=PIN,actual_factory_identical=True,initial_state=trace.owner_initial,
        final_state=trace.owner_final,root=trace.owner_root,calls=trace.owner_calls,stores=trace.owner_stores,
        states=trace.owner_states,instruction_listings=listing,instruction_patches=[],supplied_lower_results=[],
        scope='Unchanged original whole selected crate wrapper execution. Retained fields are inputs to upper-caller comparison only; no native lower-body/world allocation completion.')
    frozen_write(out/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(out/'report.json'),calls=len(trace.owner_calls),stores=len(trace.owner_stores),states=len(trace.owner_states))))

if __name__=='__main__':main()
