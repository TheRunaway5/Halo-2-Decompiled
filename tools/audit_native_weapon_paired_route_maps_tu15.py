#!/usr/bin/env python3
"""Authenticate paired weapon-owner PC/operand maps before native routing.

Decode unchanged independently pinned MapPack1.1 and TU1.5 instructions. Every
register, width, memory operand and control target must match the authenticated
source relocation map. This is an offline map audit, not runtime execution or
whole-graph qualification. Run through tools/run_test.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from oracle_smg_firing_1_1 import Xbe,PIN,ROOT
from weapon_reference_versions import images


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);modes=parser.add_mutually_exclusive_group();modes.add_argument('--marker-matrix',action='store_true');modes.add_argument('--marker-parent',action='store_true');modes.add_argument('--marker-emission',action='store_true');modes.add_argument('--marker-emission-math',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(old_path)==PIN;old=Xbe(old_path)
    _,new,pin=next(r for r in images() if r[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True;rows=[]
    cases=[('src/gameplay/units/biped_falling_damage_generated.cpp',0xbacc0,0xbbe00,'WorldState'),
        *[('src/gameplay/units/source_damage_delivery_runtime_generated.cpp',a,b,'ObservedDamageState')
            for a,b in [(0xbacc0,0xbbe00),(0xc9a40,0xcabc0),(0x177da0,0x1791c0),(0x1781f0,0x179610)]]]
    if args.marker_matrix:
        cases=[('src/gameplay/units/source_damage_delivery_runtime_generated.cpp',0xb9c20,0xbad60,'ObservedDamageState'),
            ('src/physics/vehicles/physics_vehicle_motion_generated.cpp',0xb9c20,0xbad60,'MotionState')]
    if args.marker_parent:
        cases=[('src/physics/vehicles/physics_vehicle_motion_generated.cpp',0xb9b90,0xbacd0,'MotionState')]
    if args.marker_emission:
        cases=[('src/physics/vehicles/physics_vehicle_motion_generated.cpp',0x1dc940,0x1e1200,'MotionState')]
    if args.marker_emission_math:
        cases=[('src/physics/vehicles/physics_vehicle_motion_generated.cpp',0x143580,0x144910,'MotionState'),
            ('src/physics/vehicles/physics_vehicle_motion_generated.cpp',0x1441d0,0x145560,'MotionState')]
    for relative,entry,target,state in cases:
        source=(ROOT/relative).read_text();marker=f'static void run_{entry:x}({state} &s) {{\n';assert source.count(marker)==1
        body=source.split(marker,1)[1].split('\n}',1)[0];start=source.index('std::uint32_t selected(');end=source.index('default:throw',start)
        addresses={int(a,16):int(b,16) for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',source[start:end])}
        if state=='ObservedDamageState':
            pairs={int(a,16):int(b,16) for a,b in re.findall(r'damage_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)',body)}
        else:
            sites={int(pc,16) for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',body)};pairs={pc:pc+target-entry for pc in sites}
        assert pairs and pairs[entry]==target;assert all(b-a==target-entry for a,b in pairs.items())
        for a,b in pairs.items():
            assert a not in addresses or addresses[a]==b;addresses[a]=b
        source_bytes=bytearray();target_bytes=bytearray();instructions=[]
        for pc,mapped in sorted(pairs.items()):
            a=next(decoder.disasm(old.read(pc,15),pc,count=1));b=next(decoder.disasm(new.read(mapped,15),mapped,count=1))
            assert a.mnemonic==b.mnemonic and len(a.operands)==len(b.operands),(relative,hex(pc),a.mnemonic,b.mnemonic)
            for x,y in zip(a.operands,b.operands):
                assert x.type==y.type and x.size==y.size,(relative,hex(pc),'operand type/width')
                if x.type==1:assert a.reg_name(x.reg)==b.reg_name(y.reg),(relative,hex(pc),'register')
                elif x.type==2:
                    value=x.imm&0xffffffff;assert addresses.get(value,value)==y.imm&0xffffffff,(relative,hex(pc),'immediate',hex(value),hex(y.imm&0xffffffff))
                elif x.type==3:
                    for name in ['base','index','segment']:assert a.reg_name(getattr(x.mem,name))==b.reg_name(getattr(y.mem,name)),(relative,hex(pc),name)
                    value=x.mem.disp&0xffffffff;assert x.mem.scale==y.mem.scale and addresses.get(value,value)==y.mem.disp&0xffffffff,(relative,hex(pc),'memory address')
                else:raise ValueError('Unknown original operand kind')
            source_bytes+=a.bytes;target_bytes+=b.bytes;instructions.append(dict(canonical=pc,tu15=mapped,canonical_bytes=bytes(a.bytes).hex(),tu15_bytes=bytes(b.bytes).hex()))
        rows.append(dict(path=relative,canonical_entry=entry,tu15_entry=target,state=state,constant_pc_delta=target-entry,
            instruction_sites=len(instructions),instructions=instructions,source_body_sha256=hashlib.sha256(body.encode()).hexdigest(),
            source_file_sha256=sha(ROOT/relative),canonical_instruction_sha256=hashlib.sha256(source_bytes).hexdigest(),tu15_instruction_sha256=hashlib.sha256(target_bytes).hexdigest(),
            caller_observation_contract='canonical PCs by default; damage API also selects original TU1.5 observation PCs',required_owner_sites_remain_canonical=True))
    report=dict(passed=True,scope=__doc__,canonical_original_sha256=PIN,tu15_original_sha256=pin,owners=rows,
        total_instruction_sites=sum(r['instruction_sites'] for r in rows),generator_sha256=sha(Path(__file__)),scratch_owner=str(scratch),
        instruction_patches=[],runtime_original_code_execution=False,native_bridges_implemented=False,full_user_goal_complete=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(passed=True,paired_owners=len(rows),instruction_sites=report['total_instruction_sites'],native_bridges_implemented=False)))


if __name__=='__main__':main()
