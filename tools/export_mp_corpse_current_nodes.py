#!/usr/bin/env python3
"""Export unchanged BDF80 calls with explicit fixture-only lower-owner effects.

The complete current-node phase, height correction, physical-node/hierarchy
delivery, local defaults and mask operations execute natively. Optional animation/matrix compositions run natively, with explicit original
CPU estimate and real-cache-byte delivery boundaries. Other callees remain
explicit test-only boundaries where recorded. This validates
retained output and ABI scheduling, not live gameplay composition.
"""
import argparse,bisect,json,struct
from export_mp_ragdoll_owners import spans,sha
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
NATIVE={0x10d260,0xe2ca0,0x1f0030,0x1d7310,0xb9ac0,0x16f3c0,0xc0490,0xc0b80,0xc0dc0,0xc0d30,0xc0d80,0x194ce0,0xb7670,0x1441d0}
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--native-matrices',action='store_true');ap.add_argument('--native-animation',type=Path);a=ap.parse_args()
    j=json.loads(a.report.read_text());assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN
    assert j['capture_entry']=='000bdf80'and j['failure']is None and j['frozen_prior_byte_identical']
    assert not j['instruction_patches']and not j['new_supplied_guest_fields_or_results']
    assert j['tool_sha256']==sha(Path(__file__).with_name('oracle_mp_corpse_current_nodes_1_1.py'))
    for q in j['instructions']:assert sha(a.report.parent/q['file'])==q['sha256']
    events=sorted([(q['order'],'R',q)for q in j['rebuild_reads']]+[(q['order'],'W',q)for q in j['rebuild_stores']]);orders=[q[0]for q in events]
    within=lambda c:events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]
    rows=['N '+str(int(a.native_matrices))+' '+str(int(a.native_animation is not None))];audit=[];math=[];poses={}
    if a.native_animation:
        provenance=json.loads(a.native_animation.with_suffix('.provenance.json').read_text())
        assert provenance['fixture_sha256']==sha(a.native_animation)and provenance['source_report_sha256']==sha(a.report)
        assert provenance['exporter_sha256']==sha(Path(__file__).with_name('export_mp_unit_animation_pose_runtime.py'))
        for line in a.native_animation.read_text().splitlines():
            fields=line.split()
            if fields[0]=='C':pose=poses.setdefault(int(fields[1]),[])
            elif fields[0]=='E':pose.append(line)
            elif fields[0]=='P':pose.append('D '+' '.join(fields[1:]))
    # Independent unchanged143960 execution on each actual recorded caller
    # local-node API record. Only the RCPSS instruction boundary is exported;
    # no native matrix, expected arithmetic or dependency return is supplied.
    image_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(image_path)==PIN
    image=Xbe(image_path)
    def reciprocal(c):
        pointer=int(c['stack16'][1],16);values={}
        for _,kind,q in within(c):
            if kind!='R':continue
            for i,v in enumerate(bytes.fromhex(q['raw_hex'])):
                p=int(q['address'],16)+i
                if pointer<=p<pointer+32:values.setdefault(p,v)
        node=bytes(values[pointer+i]for i in range(32))
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        base,sp,stop=0x1000000,0x2000800,0x3000000
        g.mem_map(base,0x1000);g.mem_map(0x2000000,0x1000);g.mem_map(stop,0x1000)
        g.mem_write(base,node);g.mem_write(sp,struct.pack('<III',stop,base+0x100,base))
        for reg,v in ((R.UC_X86_REG_ESP,sp),(R.UC_X86_REG_FPCW,int(c['fpcw'],16)),
            (R.UC_X86_REG_MXCSR,int(c['mxcsr'],16)),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_FPSW,0)):
            g.reg_write(reg,v)
        pair=[]
        def code(uc,p,size,user):
            if p==0x1439df:pair.append(g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff)
            if p==0x1439e3:pair.append(g.reg_read(R.UC_X86_REG_XMM1)&0xffffffff)
        g.hook_add(UC_HOOK_CODE,code);g.emu_start(0x143960,stop,count=1000)
        assert g.reg_read(R.UC_X86_REG_EIP)==stop and len(pair)in(0,2)
        math.append(dict(call=c['id'],node_hex=node.hex(),fpcw=c['fpcw'],mxcsr=c['mxcsr'],rcpss=[f'{v:08x}'for v in pair]))
        return pair
    native=NATIVE|({0x143960}if a.native_matrices else set())|({0xbea50,0x1c9e30}if a.native_animation else set())
    for c in j['rebuild_calls']:
        if c['entry']!='000bdf80':continue
        frame=int(c['entry_esp'],16)-0xf8
        def retained(q):return not 0x2000000<=int(q['address'],16)<0x2200000 and q['instruction']not in {'00278ce1','00278ce8'}
        children=[];end=0
        for q in j['rebuild_calls']:
            if not c['order']<q['order']<c['return_order']or q['order']<end:continue
            if int(q['entry'],16)in native or q['entry']=='000beb30'and int(q['stack16'][3],16)==0 and int(q['stack16'][2],16)&255==0:
                end=q['return_order']
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
        if a.native_animation:
            for q in j['rebuild_calls']:
                if c['order']<q['order']<c['return_order']and q['entry']=='000bea50':rows.extend(poses[q['id']])
        if a.native_matrices:
            for q in j['rebuild_calls']:
                if c['order']<q['order']<c['return_order']and q['entry']=='00143960':
                    pair=reciprocal(q)
                    if pair:rows.append('R '+' '.join(f'{v:08x}'for v in pair))
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        output={}
        for q in writes:
            for i,v in enumerate(bytes.fromhex(q['written_hex'])):output[int(q['address'],16)+i]=v
        for p,raw in spans(output):rows.append(f'A {p:08x} {raw.hex()}')
        for q in writes:rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
        for q in children:
            rows.append(' '.join(['P',q['entry'],q['entry_esp'],q['returned_eax']]+[q['registers'][k]for k in ('eax','ecx','edx','esi','edi','ebx')]+q['stack16']))
            for _,kind,t in within(q):
                if kind!='W':continue
                at=int(t['address'],16)
                if retained(t)or frame<=at and at+t['size']<=frame+0xf8:
                    rows.append('L '+t['instruction']+' '+t['address']+' '+t['before_hex']+' '+t['written_hex'])
        audit.append(dict(id=c['id'],stores=len(writes),retained_bytes=len(output),lower_owners=[q['entry']for q in children]))
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(rows)+'\n')
    a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_physical_nodes=True,native_matrices=a.native_matrices,native_animation=bool(a.native_animation),original_pose_fixture_sha256=sha(a.native_animation)if a.native_animation else None,original_rcpss_boundary_executions=math,instruction_patches=[],supplied_dependency_returns=[],native_expected_data_used=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
