#!/usr/bin/env python3
"""Inspect validated original charge allocation, stop and no-local-view cases.

This observes the completed source records. It does not run a weapon trigger,
advance particle cleanup, render the graph or establish physical Xbox parity.
"""
import argparse,json,struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_charged_effect_lifetime_bank_tu15 import regions
from validate_charge_effect_bank_tu15 import validate

def load(case):
    metadata=json.loads((case/'native.json').read_text())
    before=list(regions(case/'before.bin'));after=list(regions(case/'after.bin'))
    context=validate(case,metadata,before)
    original=json.loads((case/'original-comparison-v2.json').read_text())
    hardware=json.loads((case/'hardware-verification/report.json').read_text())
    assert hardware['passed'] and hardware['exit_identical_except_platform_IF']
    assert not hardware['source_instruction_patches'] and not hardware['supplied_game_services']
    assert original['matched_native_stores']==original['total_native_stores']
    assert original['final_retained_bytes_compared']==hardware['retained_bank_bytes_compared']
    assert original['original_pc']==0x60300000 and not original['services']
    return metadata,before,after,context,original,hardware

def read(rows,address,size):
    for p,b in rows:
        if p<=address and address+size<=p+len(b):return b[address-p:address-p+size]
    raise AssertionError(('Uncaptured source range',hex(address),size))

def u(rows,address,size=4):return int.from_bytes(read(rows,address,size),'little')

def record(rows,slot,datum,stride,live=True):
    pool=u(rows,slot);assert u(rows,pool+0x24)==stride and (datum&65535)<u(rows,pool+(0x38 if live else 0x20))
    address=u(rows,pool+0x44)+(datum&65535)*stride
    if live:assert u(rows,address,2)==datum>>16
    return address

def evidence(case,original,hardware):
    names=('build.json','native.json','original-comparison-v2.json','hardware-verification/report.json')
    return dict(case=str(case.resolve()),files_sha256={n:file_sha(case/n) for n in names},
        original_instructions=original['original_instructions'],retained_writes=original['matched_native_stores'],
        retained_bytes_compared=original['final_retained_bytes_compared'],
        unicorn_raw_exit_compared=original['final_abi_compared'],unicorn_raw_error=original['error'],
        hardware_exit_identical_except_platform_IF=hardware['exit_identical_except_platform_IF'],
        calls=original['original_calls'])

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--local',type=Path,required=True);ap.add_argument('--no-local-view',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();assert not a.output.exists()
    m,b,f,c,o,h=load(a.local);instance=m['returned_effect'];assert instance!=0xffffffff and m['local_view']
    assert not c['idle_charge_route'] and c['trigger_behavior']==1
    allocated=record(f,0x4f720c,instance,400);assert u(f,allocated+4)==m['effect']
    sm,sb,sf,sc,so,sh=load(a.local/'stop');assert sm['effect_instance']==instance and sc==c
    address=record(sb,0x4f720c,instance,400);raw=read(sb,address,400);stopped=read(sf,address,400)
    assert raw==(a.local/'stop/effect-before.bin').read_bytes()
    assert stopped==(a.local/'stop/effect-after.bin').read_bytes()
    assert raw[:2]==stopped[:2] and u(sf,address,2)==instance>>16
    assert stopped[2]==raw[2]|0x44 and u(sb,address+0xfc)!=0xffffffff
    systems=[];datum=u(sb,address+0x100)
    while datum!=0xffffffff:
        assert all(s['datum']!=datum for s in systems) and len(systems)<64
        p=record(sb,0x51df74,datum,0x54);r=read(sb,p,0x54);s=read(sf,p,0x54)
        flags_before=int.from_bytes(r[0xc:0xe],'little');flags_after=int.from_bytes(s[0xc:0xe],'little')
        assert flags_before&1 and flags_after==flags_before&~1
        systems.append(dict(datum=datum,address=p,flags_before=flags_before,flags_after=flags_after,before_hex=r.hex(),after_hex=s.hex()))
        datum=int.from_bytes(r[0x3c:0x40],'little')
    assert len(systems)==4
    nm,nb,nf,nc,no,nh=load(a.no_local_view);assert not nm['local_view'] and nm['returned_effect']==0xffffffff
    allocation=next(call for call in no['original_calls'] if call['name']=='effect_allocate');none_instance=allocation['return_eax'];assert none_instance!=0xffffffff
    none_record=record(nf,0x4f720c,none_instance,400,False);assert not u(nf,none_record,2)
    assert any(call['name']=='effect_free' for call in no['original_calls'])
    report=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),source_context=c,
        allocation=evidence(a.local,o,h),stop=evidence(a.local/'stop',so,sh),no_local_view=evidence(a.no_local_view,no,nh),
        local_instance=instance,stopped_instance_remains_allocated=True,
        effect_flags_before=raw[2],effect_flags_after=stopped[2],
        linked_object_location_branch_exercised=u(sb,address+8)!=0xffffffff,
        particle_systems=systems,no_local_view_allocated_then_freed=none_instance,
        particle_drain_and_later_effect_free_verified=False,gameplay_trigger_production_verified=False,
        native_rendering_verified=False,physical_xbox_verified=False,full_weapon_parity=False,passed=True)
    a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(passed=True,particle_systems_stopped=len(systems),stopped_instance_remains_allocated=True,no_local_view_allocated_then_freed=hex(none_instance))))
if __name__=='__main__':main()
