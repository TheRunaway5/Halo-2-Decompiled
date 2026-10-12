#!/usr/bin/env python3
"""Export unchanged DE960 calls with explicit fixture-only lower-owner effects.

The native compositions incrementally include admission, packed animation
input, action/recovery/marking, velocity and the complete ragdoll world owners.
Any remaining lower calls are explicit fixture-only boundaries. Independent
original CPU estimates and real cache bytes remain explicit inputs. No native
expected result is exported. This validates scheduling and memory effects on
the declared component bank, not live gameplay composition.
"""
import argparse,bisect,json
from export_mp_ragdoll_owners import spans,sha
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
from pathlib import Path
NATIVE={0xbbe30,0x30bd0,0xde900,0xe5680,0xe6280,0xe7e60,0xe43b0,0xe52c0,0x1e9480,0x1f0940,0xe3b20,0xdf5e0,0xdee00,0xe44c0,0xe23f0,0x1e8f60,0x1e94e0,0x1f52f0,0x1fe870,0x1efde0,0x1fffe0,0x1d42d0,0xdd970,0x1f0080,0xbbcb0,0x2e5510,0x112e10,0x110bf0,0x114f00,0x114f70,0x1cf100,0x1cf070,0x1ca2b0,0x1ca090,0x1c9f50,0x1ca3a0,0x1ca5a0,0x1c9e30,0x1de890,0x1cae40,0x1e13c0,0x1e1230,0x1de9b0,0x1150d0,0x1dcd10,0x10f430,0x1102b0,0x1105e0,0xe7e30,0x1d0a60,0x1d0590,0x1cee00,0x1c9e60,0x1cacf0,0xb7650,0x1d06c0}
NATIVE |= {0x1d0790,0x1de7d0,0x1cfde0,0x1cfce0,0x1deb60,0x1df670,0x1dea60,0x1e02b0,0x29c6a0,0x1e0f50,0x1deb10,0x1e16f0,0x1e1660,0x1e1580,0x1ceda0,0x1e1020,0x1e0fc0,0x1de890,0x1de9b0,0x1e13c0,0x1e1230,0x124e40,0x124dd0,0x16d2c0,0x1d1b70}

NATIVE |= {0xb8250,0xbb240,0xc83d0,0x111b60,0x114bd0,0x115af0,0x1c9e60,0x1c9f50,0x1ca3a0,0x1cee00,0x1cef10,0x1cefe0,0x1cf360,0x1cf3c0,0x1cf4c0,0x1cfb90,0x1dcc30,0x1dce20,0x1cec20,0x1cfa90,0x1c9de0}

NATIVE |= {0x110b00,0x1cf310,0x1d1bd0}

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--native-admission',action='store_true');ap.add_argument('--native-input',type=Path);ap.add_argument('--native-phase',type=Path);ap.add_argument('--native-velocity',action='store_true');ap.add_argument('--native-world',action='store_true');a=ap.parse_args()
    if a.native_world:assert a.native_admission and a.native_input and a.native_phase and a.native_velocity
    j=json.loads(a.report.read_text());assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN
    assert j['capture_entry']=='000de960'and j['failure']is None and j['frozen_prior_byte_identical']
    assert not j['instruction_patches']and not j['new_supplied_guest_fields_or_results']
    assert j['tool_sha256']==sha(Path(__file__).with_name('oracle_mp_biped_prephysics_1_1.py'))
    exception_chain_sites=set()
    for q in j['instructions']:
        listing=a.report.parent/q['file'];assert sha(listing)==q['sha256']
        for line in listing.read_text().splitlines():
            if 'mov dword ptr fs:[0],'in line:exception_chain_sites.add(line.split()[0])
    events=sorted([(q['order'],'R',q)for q in j['rebuild_reads']]+[(q['order'],'W',q)for q in j['rebuild_stores']]);orders=[q[0]for q in events]
    within=lambda c:events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]
    rows=['N '+str(int(a.native_admission))+' '+str(int(a.native_input is not None))+' '+str(int(a.native_phase is not None))+' '+str(int(a.native_velocity))+' '+str(int(a.native_world))];audit=[];inputs={}
    phase_entries={0xe1800,0xe4f40,0xdf7c0,0x142e80,0x143f60,0x1d1290}
    for fixture,tool,roots in ((a.native_input,'export_mp_biped_input_runtime.py',{0xddbc0}),(a.native_phase,'export_mp_biped_phase_runtime.py',phase_entries)):
        if not fixture:continue
        provenance=json.loads(fixture.with_suffix('.provenance.json').read_text())
        assert provenance['fixture_sha256']==sha(fixture)and provenance['source_report_sha256']==sha(a.report)
        assert provenance['exporter_sha256']==sha(Path(__file__).with_name('export_mp_unit_animation_pose_runtime.py'))
        assert provenance['configuration_tool_sha256']==sha(Path(__file__).with_name(tool))
        assert provenance['capture_entry']=='000de960'and provenance['roots']==[f'{entry:08x}'for entry in sorted(roots)]
        for line in fixture.read_text().splitlines():
            fields=line.split()
            if fields[0]=='C':packet=inputs.setdefault(int(fields[1]),[])
            elif fields[0]=='E':packet.append(line)
            elif fields[0]=='P':packet.append('D '+' '.join(fields[1:]))
    native=NATIVE|({0xddbc0}if a.native_input else set())|(phase_entries|{0xbc910,0xc04f0,0xe8050,0xe6670,0xe63c0}if a.native_phase else set())|({0xe2520,0x1f08e0,0x13a710,0x110ee0,0x1e13c0,0x1f0e20,0x185700,0x259b0,0xe2430,0x1effe0,0x1d6fc0,0x3190e0,0x319090,0x31c570,0x31c520,0xb7690}if a.native_admission else set())
    if a.native_velocity:
        native.add(0x1c8430)
        if a.native_admission:native.update({0xbaa80,0x1d4a00})
    if a.native_world:native.update({0xe2ce0,0xe30b0})
    for c in j['rebuild_calls']:
        if c['entry']!='000de960':continue
        frame=int(c['entry_esp'],16)-0x17c
        # Normal native C++ owners use their host exception ABI. The original
        # FS:[0] chain is thread/platform storage, not a biped/world field.
        # Keep the frozen trace unchanged and declare every excluded source
        # site, as the separate original world-owner comparisons already do.
        excluded=exception_chain_sites if a.native_world else {'00278ce1','00278ce8'}
        def retained(q):return not 0x2000000<=int(q['address'],16)<0x2200000 and q['instruction']not in excluded
        children=[];end=0
        for q in j['rebuild_calls']:
            if not c['order']<q['order']<c['return_order']or q['order']<end:continue
            if int(q['entry'],16)in native:
                if int(q['entry'],16)not in {0xe6280,0xe7e60,0xe2520,0xe43b0,0xe52c0,0x1e9480,0xe3b20,0xe44c0,0x1e8f60,0x1e94e0,0x1f0080,0x1d42d0,0xdd970,0x112e10,0x110bf0,0x114f00,0x1cf100,0x1ca2b0,0x1ca090,0x1c9f50,0x1ca3a0,0x1ca5a0,0x1c9e30,0x1de890,0x1cae40,0x1e13c0,0x1e1230,0x1de9b0,0x1d0a60,0x1cee00,0x1cacf0,0x1d06c0,0x1d0790,0x1de7d0,0x1cfde0,0x1cfce0,0x1deb60,0x1df670,0x1dea60,0x1e02b0,0x29c6a0,0x1e0f50,0x1deb10,0x1e16f0,0x1e1660,0x1e1580,0x1ceda0,0x1e1020,0x1e0fc0,0x1de890,0x1de9b0,0x1e13c0,0x1e1230,0x124e40,0x124dd0,0x16d2c0,0x1d1b70,0xb8250,0xbb240,0xc83d0,0x111b60,0x114bd0,0x115af0,0x1c9e60,0x1c9f50,0x1ca3a0,0x1cee00,0x1cef10,0x1cefe0,0x1cf360,0x1cf3c0,0x1cf4c0,0x1cfb90,0x1dcc30,0x1dce20,0x1cec20,0x1cfa90,0x1c9de0,0x110b00,0x1cf310,0x1d1bd0}:end=q['return_order']
                continue
            children.append(q);end=q['return_order']
        initial={};current={};writes=[]
        for _,kind,q in within(c):
            at=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']if kind=='R'else q['before_hex'])
            for i,v in enumerate(raw):
                if at+i not in current:initial[at+i]=current[at+i]=v
                assert current[at+i]==v
            if kind=='W':
                if retained(q):writes.append(q)
                for i,v in enumerate(bytes.fromhex(q['written_hex'])):current[at+i]=v
        rows.append(' '.join(['C',str(c['id']),c['stack16'][0],c['entry_esp'],c['fpcw'],c['mxcsr'],c['returned_eax']]))
        for q in j['rebuild_calls']:
            if c['order']<q['order']<c['return_order']and q['id']in inputs:rows.extend(inputs[q['id']])
        if a.native_world:
            world_roots=[q for q in j['rebuild_calls']if c['order']<q['order']<c['return_order']and q['entry']in {'000e2ce0','000e30b0'}]
            for root in world_roots:
                for value in j['math']:
                    if root['order']<value['order']<root['return_order']:
                        assert value['instruction']in {'001dc782','001d2151','001d21bf','001d2225','001d2288','001d1d48','001d1daf','001d1e12','001d1e78'}
                        rows.append('V '+value['operand_word']+' '+value['result_word'])
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        output={}
        for q in writes:
            for i,v in enumerate(bytes.fromhex(q['written_hex'])):output[int(q['address'],16)+i]=v
        for p,raw in spans(output):rows.append(f'A {p:08x} {raw.hex()}')
        for q in writes:rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
        for q in children:
            rows.append(' '.join(['P',q['entry'],q['entry_esp'],q['returned_eax']]+[q['registers'][k]for k in ('eax','ecx','edx','esi','edi','ebx')]+q['stack16']))
            # Reached lower owners may write their native caller's output
            # locals. Retain only owned locals, excluding incidental private
            # prologs and outgoing argument slots of the original callee.
            locals=[(frame+0x18,frame+0x17c)]
            for parent_call in j['rebuild_calls']:
                if parent_call['entry']=='000e3b20' and parent_call['order']<q['order']<parent_call['return_order']:
                    movement_frame=int(parent_call['entry_esp'],16)-0xe0
                    locals.append((movement_frame+0x18,movement_frame+0xe0))
            for parent_call in j['rebuild_calls']:
                if parent_call['order']<q['order']<parent_call['return_order']:
                    if parent_call['entry']=='001d06c0':
                        base=int(parent_call['entry_esp'],16)-0x2c
                        locals.append((base+0xc,base+0x44))
                    elif parent_call['entry']=='001d0a60':
                        base=int(parent_call['entry_esp'],16)-0x48
                        locals.append((base+0x13,base+0x60))
                    elif parent_call['entry']=='001cee00':
                        base=int(parent_call['entry_esp'],16)
                        locals.append((base+8,base+12))
                    elif parent_call['entry']=='001cae40':
                        base=int(parent_call['entry_esp'],16)-0x34
                        locals.append((base+0xc,base+0x34))
                    elif parent_call['entry']=='001ca090':
                        base=int(parent_call['entry_esp'],16)-0x1c
                        locals.append((base+0x10,base+0x34))
                    elif parent_call['entry']=='00112e10':
                        base=int(parent_call['entry_esp'],16)-0x90
                        locals.append((base+0x11,base+0x90))
                    elif parent_call['entry']=='00110bf0':
                        base=int(parent_call['entry_esp'],16)-0x14
                        locals.append((base+0x10,base+0x14))
                    elif parent_call['entry']=='001e8f60':
                        base=((int(parent_call['entry_esp'],16)-4)&~15)-0xa0
                        locals.append((base+0x14,base+0xa0))
                    elif parent_call['entry']=='001f0080':
                        base=((int(parent_call['entry_esp'],16)-4)&~15)-0xe0
                        locals.append((base+0x14,base+0xe0))
            for _,kind,t in within(q):
                if kind!='W':continue
                at=int(t['address'],16)
                if retained(t)or any(lo<=at and at+t['size']<=hi for lo,hi in locals):
                    rows.append('L '+t['instruction']+' '+t['address']+' '+t['before_hex']+' '+t['written_hex'])
        audit.append(dict(id=c['id'],stores=len(writes),retained_bytes=len(output),lower_owners=[q['entry']for q in children]))
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(rows)+'\n')
    a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_admission=a.native_admission,native_velocity=a.native_velocity,native_world=a.native_world,excluded_platform_exception_chain_store_sites=sorted(excluded),native_input_fixture_sha256=sha(a.native_input)if a.native_input else None,native_phase_fixture_sha256=sha(a.native_phase)if a.native_phase else None,native_expected_data_used=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
