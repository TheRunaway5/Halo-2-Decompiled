#!/usr/bin/env python3
"""Observe complete unchanged original rebuild callers and actual lower ABI."""
import argparse,hashlib,json
from pathlib import Path
from unicorn import x86_const as reg
import oracle_object_physics_construction_native as prior
from oracle_mp_power_body_slots_1_1 import SlotTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP

class Capture(prior.Capture):
    def hook(self,g,address,size,user):
        if self.owner_enabled and not self.owner_done:
            esp=g.reg_read(reg.UC_X86_REG_ESP)
            if self.owner_pending and address==self.owner_pending[0][0]and esp>self.owner_pending[0][1]:
                assert not self.owner_lower
                self.owner_final=self.owner_state();self.owner_done=True
            for pending in self.owner_lower[:]:
                ret,sp,row=pending
                if address==ret and esp>sp:
                    row.update(after_state=self.owner_state(),return_eax=f'{g.reg_read(reg.UC_X86_REG_EAX):08x}',return_order=self.owner_order)
                    self.owner_order+=1;self.owner_lower.remove(pending)
            if not self.owner_pending and address==0x1d2b10 and self.active=='original_authored_scenario_crate_factory':
                self.rebuild_datum=g.reg_read(reg.UC_X86_REG_EAX)
                self.owner_object=self.u(self.u(self.u(0x526248)+0x44)+(self.rebuild_datum&65535)*160+8)
                self.owner_initial=self.owner_state();self.owner_pending=[(self.u(esp),esp)]
                self.owner_root=dict(entry='001d2b10',datum=f'{self.rebuild_datum:08x}',object=f'{self.owner_object:08x}')
            if self.owner_pending and address in prior.LOWER and not self.owner_lower:
                component=g.reg_read(reg.UC_X86_REG_ESI)
                if address==0x1da570:component=g.reg_read(reg.UC_X86_REG_EDI)
                if address==0x1d90e0:component=g.reg_read(reg.UC_X86_REG_EAX)
                args=[self.u(esp+4+i*4)for i in range(4 if address==0x1d9330 else 2 if address==0x1c8430 else 0)]
                row=dict(id=len(self.owner_calls),entry=f'{address:08x}',instruction=f'{self.u(esp)-5:08x}',
                    component=f'{component:08x}',object=f'{g.reg_read(reg.UC_X86_REG_EAX):08x}'if address==0x1c8430 else'00000000',
                    linear=f'{g.reg_read(reg.UC_X86_REG_EBX):08x}'if address==0x1c8430 else'00000000',
                    angular=f'{args[0]:08x}'if address==0x1c8430 else'00000000',arguments=[f'{v:08x}'for v in args],
                    before_state=self.owner_state(),order=self.owner_order)
                self.owner_order+=1;self.owner_calls.append(row);self.owner_lower.append((self.u(esp),esp,row))
        # Retain its state/write observer, with this separate root/callee set.
        SlotTrace.hook(self,g,address,size,user)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    prior.LOWER={0x1da570,0x1d90e0,0x1da690,0x1d9330,0x1c8430}
    prior.prior.verify()
    trace=prior.prior.prior.prior.power.player.prepare(json.loads((OWNER/'raw-host-context.json').read_text()),trace_type=Capture)
    trace.selected_physics_fixup(0xe6f80583);trace.bridge_enabled=True;trace.slot_ready=True;trace.owner_enabled=True
    result=trace.factory(0xe6f80583);trace.slot_return(STOP);assert trace.owner_done and not trace.owner_lower
    expected=json.loads((OWNER/'report.json').read_text())['case'];keys=set(result)-{'calls','ordered_stores'}
    assert {k:result[k]for k in keys}=={k:expected[k]for k in keys}
    old_path=ROOT/'analysis/multiplayer/sandbox-source/power-body-slots/final/measured/report.json';old=json.loads(old_path.read_text())
    assert trace.slot_calls==old['calls'][:39]and trace.slot_states==old['states'][:19]
    report=dict(schema='physics-component-rebuild-original-v1',oracle_sha256=sha(Path(__file__)),
        inherited_slot_report_sha256=sha(old_path),entire_factory_and_retained_slot_sequence_identical=True,
        root=trace.owner_root,initial_state=trace.owner_initial,final_state=trace.owner_final,states=trace.owner_states,
        calls=trace.owner_calls,stores=trace.owner_stores,instruction_patches=[],supplied_callee_results=[],
        scope='Complete original1D2B10/1D9090 with real lower grouping/current-body velocity calls. Captured lower states are only test evidence; no native body/world readiness.')
    frozen_write(out/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(out/'report.json'),calls=len(trace.owner_calls),stores=len(trace.owner_stores),states=len(trace.owner_states))))

if __name__=='__main__':main()
