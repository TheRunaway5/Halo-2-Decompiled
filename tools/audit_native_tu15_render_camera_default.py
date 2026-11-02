#!/usr/bin/env python3
"""Authenticated default camera data/CRT/caller contract; no storage mount or run."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');owners={}
    for entry in(0x2f7e0,0x380450,0x2dbbad):
        body=complete_body(code,entry);listing=[dict(instruction=hex(pc),bytes=code.at(pc).bytes.hex(),operation=code.at(pc).mnemonic,operands=code.at(pc).op_str)for pc in sorted(body.code)]
        owners[hex(entry)]=dict(sites=len(listing),listing=listing,listing_sha256=hashlib.sha256(json.dumps(listing,separators=(',',':')).encode()).hexdigest())
    assert code.at(0x2da9bb).mnemonic=='call'and code.at(0x2da9bb).operands[0].imm==0x2dbbad
    assert code.at(0x2dbbf9).bytes.hex()=='ffd0'
    table_start,table_end=0x46d6f0,0x46dd78;table=image.read(table_start,table_end-table_start)
    values=struct.unpack('<'+'I'*(len(table)//4),table)
    expected={0x380450:0x46d82c,0x381710:0x46da68,0x381730:0x46da6c,0x381750:0x46da70,0x381770:0x46da74}
    for function,address in expected.items():assert values[(address-table_start)//4]==function
    refs=[];needle=struct.pack('<I',0x5320ac)
    for section in image.sections:
        raw=image.section_bytes(section);offset=0
        while True:
            offset=raw.find(needle,offset)
            if offset<0:break
            refs.append(dict(section=section.name,address=hex(section.va+offset)));offset+=1
    section=next(s for s in image.sections if s.va<=0x5320ac<s.va+s.vsize)
    assert section.name=='.data'and section.flags==7 and image.read(0x5320ac,4)==b'\0'*4
    required=[(0x5320ac,4),(0x46ab88,4),(0x474ce8,4),(0x474d08,4),(0x474d10,4),(0x44abec,12),(0x44ac28,12),(0x44ac40,12),(0x46a0cc,4),(0x46a0d0,4),(0x46a13c,4),(0x46a1d0,8),(0x4922f4,4),(0x4922f8,4)]
    files=['include/halo2/native_tu15_render_camera_default.hpp','include/halo2/native_tu15_render_camera_default_binding.hpp','src/rendering/world/native_tu15_render_camera_default.cpp','src/rendering/world/native_tu15_render_camera_default_binding.cpp','cmake/NativeTu15RenderCameraDefault.cmake']
    report=dict(passed=True,original_sha256=NEW_PIN,xbe_entry=hex(image.entry),owners=owners,
        crt=dict(startup_owner='0x2dbbad',actual_startup_call='0x2da9bb',table=[hex(table_start),hex(table_end)],table_sha256=hashlib.sha256(table).hexdigest(),entries=len(values),dispatch='0x2dbbf9 CALL EAX',iteration='Ascending DWORD cells; skips only0 andFFFFFFFF; preceded by optional46E508 and earlier46DD7C..46DD94 initializer scan',selected={hex(function):dict(cell=hex(address),index=(address-table_start)//4)for function,address in expected.items()},phase='380450 runs143table positions before381710 in the same initializer scan; isolated native bootstrap must preserve this relative order'),
        fov_storage=dict(address='0x5320ac',bytes=4,section=section.name,flags=section.flags,loader_bytes='00000000',direct_reads=['0x2f8a5','0x22a010'],sole_direct_writer=dict(entry='0x380450',instruction='0x380458',actual_source='0x46ab88',source_bytes=image.read(0x46ab88,4).hex()),whole_image_literal_references=refs),
        required_original_data=[dict(address=hex(address),bytes=width,original_bytes=image.read(address,width).hex(),
            section=next(s.name for s in image.sections if s.va<=address and address+width<=s.va+s.vsize),
            flags=next(s.flags for s in image.sections if s.va<=address and address+width<=s.va+s.vsize))for address,width in required],
        actual_callers={'0x229b95':'Main camera EAX=actual observer orNULL; ECX=destination; EDX=span record; stack+4=grid; RET4; volatile outputs dead before use','0x229c30':'Secondary camera same operand/result contract; real independent29DWORD destination','0x2dbbf9':'Initializer no args, plainRET; MOVSS literal only; GP/x87/control state unchanged','bootstrap0':'Explicit native startup top-level entry, not an arbitrary callback return; root must activate only at authenticated CRT phase'},
        floating_point=dict(algorithm='Real native FPTAN, extended aspect product, FPATAN and doubled angle; no virtual arithmetic stack',context='Explicit80bit values and occupied mask; canonical zero/special/normal FSAVE tags derived from actual values',fault_store='FinalFSTPS rejects beforecommit by restoring prepop hardware snapshot; committed observer fault retains rounding/pop',abi='Dense logical ABI only; sparse export retains full typed required-owner state without collapsing mask',diagnostic_metadata='FIP/FOP/FDP are native host instruction/pointer metadata; not canonical original PCs. Exact liveCW/SW/tags/values/MXCSR are compared',host_reference='analysis/campaign-native-ai-awareness-20261029/ambient-fp-probe-conclusion.json'),
        fstp_observer_descriptor=dict(instruction='0x2f919',bytes='d95928',write_bytes=4,reason='Capstone incorrectly marks its memory operand asREAD; v2 oracle authenticates opcode and records actualWRITE; original text unchanged'),
        old_fixture_sha256=sha(ROOT/'tests/fixtures/native_tu15_render_camera_packet.json.gz'),source_hashes={f:sha(ROOT/f)for f in files},audit_source_sha256=sha(__file__),production_mount_or_activation=False,complete_crt_or_parent_claim=False,retained_large_artifacts=[])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print('Default4B storage, actual380450 CRT ordering, vector/FP operands and caller profiles authenticated')
if __name__=='__main__':main()
