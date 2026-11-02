#!/usr/bin/env python3
"""Unchanged191848/191855 calls with actual disjoint private options.

Clock/seed derivation before19183A is an explicit input boundary. Both actual
CALL instructions, full430-site child closure and real RETs execute physically.
No ID, callee result, return instruction or options result is supplied.
"""
import argparse,copy,hashlib,json,os,struct,subprocess
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_transition_participants_tu15 as original
import validate_native_geometry_tu15 as geometry
sha=original.sha
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);meta=original.metadata(image);code=_Code(image,'.text');prefix=[];registers={r:n for n,r in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};pc=0x19183a
    while pc<0x19185a:
        ins=code.at(pc);operands=[]
        for op in ins.operands:
            if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
            base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index);operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
        meta['sites'].append(dict(instruction=pc,bytes=ins.bytes.hex(),operands=operands));prefix.append(dict(pc=hex(pc),bytes=ins.bytes.hex(),text=ins.mnemonic+' '+ins.op_str));pc+=ins.size
    assert pc==0x19185a
    source=original.inputs(image);selected=[copy.deepcopy(next(r for r in source if r['name']==name))for name in('capture_0_True','capture_1_True','capture_2_True','capture_16_True','capture_17_True','private_prepare_True')]
    for depth in range(8):
        r=copy.deepcopy(selected[1]);r['name']='caller_live_'+str(depth);r['before_fp'][1]=(depth<<11)|0x241;r['live_x87']=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3).hex()for k in range(depth)];selected.append(r)
    for index,r in enumerate(selected):
        r['id']=index;r['entry']=0x19183a;r['gp'][4]=geometry.SP-0x4000+(index%4)*4;r['options']=r['gp'][4]+0x350;r['gp'][0]=r['options'];r['private_options']=True
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=original.prepare(image,meta,scratch);observer=ROOT/'tools/observe_native_campaign_transition_participants_callers_tu15.cpp';command=['c++','-std=c++20','-O2',str(observer),'-o',str(owned/'observer')];r=subprocess.run(command,capture_output=True,text=True);assert not r.returncode,r.stderr
    rows=geometry.physical(owned,selected,'actual-callers');packets=[]
    for r in rows:
        assert r['completed']and r['live_x87_unchanged'];entries=r['callee_entries'];assert len(entries)==2
        first,second=entries;sp=r['gp'][4];options=sp+0x350
        assert first['entry']==0x154f80 and first['gp'][0]==options and first['gp'][4]==sp-4
        assert struct.unpack('<I',bytes.fromhex(first['stack_hex']))==(0x19184d,)
        assert second['entry']==0x155190 and second['gp'][1]==options and second['gp'][4]==sp-8
        assert struct.unpack('<II',bytes.fromhex(second['stack_hex']))==(0x19185a,options)
        assert first['fp']==second['fp']==r['before_fp']and r['boundary_gp'][4]==sp
        for n in(3,5,6,7):assert r['boundary_gp'][n]==r['gp'][n]
        out=bytes.fromhex(r['options_after']);assert len(out)==0x1118
        packets.append(dict(profile=r['name'],initial_gp=r['gp'],options_address=options,private_options_bytes=len(out),actual_callee_packets=entries,returned_gp=r['boundary_gp'],before_fp=r['before_fp'],after_fp=r['after_fp'],older_values=len(r['live_x87']),older_values_preserved=True,nonvolatile_gp_preserved=True,return_sp_preserved=True,options_after_sha256=hashlib.sha256(out).hexdigest(),participant_identity_examples=[out[0x2e2+i*0xe4:0x2ee+i*0xe4].hex()for i in range(2)],original_instructions=r['original_instructions'],persistent_reads=len(r['reads']),persistent_stores=len(r['writes'])))
    fixture=ROOT/'tests/fixtures/native_campaign_transition_participants_tu15.json.gz';report=dict(passed=True,scope=__doc__,original_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',instruction_patches=[],supplied_callee_returns=[],supplied_participant_outcomes=[],profiles=len(rows),actual_calls=2*len(rows),original_instructions=sum(r['original_instructions']for r in rows),source_prefix=prefix,packets=packets,canonical_fixture=dict(path=str(fixture.relative_to(ROOT)),bytes=fixture.stat().st_size,sha256=sha(fixture)),source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),str(observer.relative_to(ROOT)):sha(observer),'tools/oracle_native_campaign_transition_participants_tu15.py':sha(original.__file__)},limits=['Starts after original clock/seed derivation; no clock or full191750 execution claimed.','Private options and source pools are genuine disjoint caller profile; corrupted private stack/source alias is not claimed.','Stops19185A before further completion/profile/session dependencies.','No native component or binding was imported or used for original expectations.'],owned_scratch=str(scratch),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,separators=(',',':'))+'\n');print(json.dumps(dict(profiles=len(rows),actual_calls=2*len(rows),instructions=report['original_instructions'],report=str(a.report),bytes=a.report.stat().st_size,sha256=sha(a.report))))
if __name__=='__main__':main()
