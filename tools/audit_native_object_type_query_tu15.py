#!/usr/bin/env python3
"""Read-only original/source provenance for the hot10DE40 type-query owner."""
import argparse,csv,hashlib,json,os,re,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/physics_live_support_tu15_generated.cpp'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    before=sha(ROOT/UNIT);image=Xbe(NEW_DEFAULT)
    if sha(Path(NEW_DEFAULT))!=NEW_PIN:raise ValueError('Original TU1.5 executable changed')
    code=_Code(image,'.text');trace=_trace(code,0x10de40,{0x10de40})
    instructions=[code.at(pc)for pc in sorted(trace.code)]
    if len(instructions)!=43:raise ValueError('Complete original type-query instruction set differs')
    source=(ROOT/UNIT).read_text();bodies=re.findall(r'^static void run_10de40\(WorldState &s\) \{.*?^\}',source,re.M|re.S)
    if len(bodies)!=1:raise ValueError('Missing/ambiguous whole type-query source body')
    sites=[int(value,16)for value in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',bodies[0])]
    if sites!=sorted(trace.code):raise ValueError('Whole source/original observation order differs')
    callbacks=[];types=[]
    for index,pointer in enumerate(struct.unpack('<13I',image.read(0x474b90,52))):
        # Runtime construction may fill inheritance-list slots. Retain these
        # original bytes as discovery only, never supplied callback outcomes.
        row=dict(type=index,descriptor=hex(pointer))
        if pointer:
            row['raw_descriptor_sha256']=hashlib.sha256(image.read(pointer,0x88)).hexdigest()
            row['raw_query_callback']=hex(struct.unpack('<I',image.read(pointer+0x4c,4))[0])
            row['raw_inheritance_list_first']=hex(struct.unpack('<I',image.read(pointer+0x84,4))[0])
            callback=int(row['raw_query_callback'],16)
            if callback:callbacks.append(callback)
            inheritance=[]
            for slot in range(16):
                descriptor=struct.unpack('<I',image.read(pointer+0x84+slot*4,4))[0]
                if not descriptor:break
                target=struct.unpack('<I',image.read(descriptor+0x4c,4))[0]
                inheritance.append(dict(slot=slot,descriptor=hex(descriptor),query_callback=hex(target)))
                if target:callbacks.append(target)
            else:raise ValueError('Original type inheritance list lacks a bounded terminator')
            row['raw_inheritance']=inheritance
        types.append(row)
    if sha(ROOT/UNIT)!=before:raise ValueError('Owning source changed during audit')
    starts_path=ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv'
    starts={int(row['address'],16)for row in csv.DictReader(starts_path.open(),delimiter='\t')}
    children=[]
    for entry in sorted(set(callbacks)):
        child=_trace(code,entry,starts);sites=sorted(child.code)
        if not sites:raise ValueError('Original type-query callback has no complete CFG')
        owned=re.findall(r'^static void run_'+format(entry,'x')+r'\(WorldState &s\) \{.*?^\}',source,re.M|re.S)
        original=[code.at(pc)for pc in sites]
        for instruction in original:
            if image.read(instruction.address,instruction.size)!=bytes(instruction.bytes):raise ValueError('Original callback instruction bytes differ')
        children.append(dict(entry=hex(entry),original_instruction_sites=len(sites),original_code_sha256=hashlib.sha256(b''.join(bytes(i.bytes)for i in original)).hexdigest(),direct_original_calls=[hex(value)for value in sorted(child.calls)],original_tail_jumps=[hex(value)for value in sorted(child.tail_jumps)],source_body_found=len(owned)==1,source_body_sha256=hashlib.sha256(owned[0].encode()).hexdigest()if len(owned)==1 else None,direct_source_owners=sorted(set(re.findall(r'\brun_([0-9a-f]+)\(s\)',owned[0])))if len(owned)==1 else [],original_has_switch_tables=bool(child.tables)))
    report=dict(passed=True,scope=__doc__,production_bound=False,original_xbe_sha256=NEW_PIN,source=UNIT,source_sha256=before,body_sha256=hashlib.sha256(bodies[0].encode()).hexdigest(),original_instruction_count=len(instructions),original_whole_cfg_sha256=hashlib.sha256(b''.join(bytes(i.bytes)for i in instructions)).hexdigest(),instructions=[dict(pc=hex(i.address),bytes=bytes(i.bytes).hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in instructions],object_type_table_address='474b90',object_root_address='4ecbd0',type_descriptors=types,original_descriptor_query_callbacks=[hex(value)for value in sorted(set(callbacks))],dispatch_site='10de95',ordered_outputs=['write32(+EBX,0) at10DE70','write8(+EDI,AL) at10DE74','optional terminal zero writes at10DEAA/10DEAE'],runtime_descriptor_construction_qualified=False,genuine_child_execution_qualified=False,retained_large_artifacts=[],owned_scratch=os.environ['HALO2_TEST_SCRATCH'])
    report.update(original_callback_owners=children,original_function_boundary_index_sha256=sha(starts_path))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(passed=True,instructions=len(instructions),callback_leads=report['original_descriptor_query_callbacks'])))
    return 0
if __name__=='__main__':raise SystemExit(main())
