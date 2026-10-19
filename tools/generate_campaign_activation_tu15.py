#!/usr/bin/env python3
"""Recover whole original map/BSP activation owners and missing BSP callbacks.

Callbacks consume the retained original registration table and complete caller
ABI. Resource publication, actual native storage and other external owners are
required inputs; recovery alone never supplies map readiness or load success.
"""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ENTRIES=(0xd7aa0,0x2255f0,0x191cf0,0x191df0,0x13a780,0x13a7e0,0x13a8b0,0x13a920,0x11e8e0,0x13a820,0x13a880,
         0x126830,0x22680,0x171f80,0x3f2a0,0x128430,0xd7370,0x1cb220,0x14e1d0,
         0x15b50,0x1e1a0,0x36aa0,0x1a7170,0x11f5d0,0xd7b80,0x191260,0x126730,
         0x1888f0,0x692d0,0x11e550,0x1f38d0,0x22570,0x171f40,0x219300,
         0x2282b0,0x1281a0,0x18b660,0x14a38b,0x1410b6,0xd7360,0x1378c0,0x22c9c0,
         0x86bd0,0x887b0,0x86c30,0x19a430,0x219f90,0x1f290,0x128900,0x197230,0x2260e0)
BSP_CALLBACKS={16:0x126830,31:0x22680,36:0x171f80,41:0x3f2a0,
               44:0x128430,57:0xd7370,59:0x1cb220,60:0x14e1d0}
NEW_MAP_CALLBACKS={2:0x11f5d0,13:0x1a59d0,16:0x126730,20:0x1888f0,23:0x692d0,
    24:0x11e550,25:0x1f38d0,31:0x22570,36:0x171f40,42:0x219300,43:0x2282b0,
    44:0x1281a0,46:0x18b660,50:0x14a38b,51:0x1410b6,57:0xd7360,
    64:0x1378c0,66:0x22c9c0}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--directory',type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists() or args.directory.exists():raise ValueError('Preserve existing recovery evidence')
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Expected original English TU1.5 executable')
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    calls={0x191cf4:0x191ca0,0x191df7:0x13a780,0x191e10:0x13a920,0x191e1f:0x11e8e0,
           0x13a7a0:0x13a8b0,0x11e96a:0x13a880,0x11ea75:0x13a820,
           0x22680:0x15b50,0x22685:0x1e1a0,0x13a8e5:0x1a7170,0xd7391:0xd7b80,
           0xd7ca8:0x191260,0x6932f:0x86bd0,0x86bff:0x887b0,0x6933b:0x86c30,
           0x69340:0x19a430,0x22598:0x1f290,0x128279:0x128900,0x12827e:0x197230,
           0x12893e:0x2260e0}
    for pc,target in calls.items():
        instruction=code.at(pc)
        if instruction.mnemonic!='call' or instruction.operands[0].imm!=target:
            raise ValueError('Original activation call correspondence differs')
    instruction=code.at(0x11e567)
    if instruction.mnemonic!='jmp' or instruction.operands[0].imm!=0x219f90:
        raise ValueError('Original new-map reset tail correspondence differs')
    for pc,table in ((0x13a7b0,0x44aed8),(0x13a860,0x44aee0),(0x11e940,0x44aee8)):
        instruction=code.at(pc)
        if instruction.mnemonic!='mov' or instruction.operands[1].mem.disp!=table:
            raise ValueError('Original activation callback column differs')
    for pc,table in ((0x191d10,0x45f940),(0x191d70,0x45f944)):
        instruction=code.at(pc)
        if instruction.mnemonic!='mov' or instruction.operands[1].mem.disp!=table:
            raise ValueError('Original resource release/acquire callback column differs')
    for row,target in BSP_CALLBACKS.items():
        if int.from_bytes(image.read(0x44aee0+row*0x24,4),'little')!=target:
            raise ValueError('Original BSP callback registration differs')
    for row,target in NEW_MAP_CALLBACKS.items():
        if int.from_bytes(image.read(0x44aed8+row*0x24,4),'little')!=target:
            raise ValueError('Original new-map callback registration differs')
    if int.from_bytes(image.read(0x45f940+5*8,4),'little')!=0x2255f0:
        raise ValueError('Original row5 resource release registration differs')
    if image.read(0x2255f0,20).hex()!='a1fcb8480085c0740ac705fcb8480000000000c3':
        raise ValueError('Original row5 release complete body differs')
    sites,instructions,addresses,routines={},{},{},[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES))
        if not body.code or any(width not in (1,4) for _,width,_ in body.tables):
            raise ValueError('Unexpected original activation control flow')
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p,count=n) for p,width,n in body.tables if width==4],
            byte_tables=[dict(address=p,width=width,count=n,raw_hex=image.read(p,width*n).hex())
                         for p,width,n in body.tables if width==1]))
        for pc in sorted(body.code):
            instruction=code.at(pc)
            if image.read(pc,instruction.size)!=bytes(instruction.bytes):raise ValueError('Original instruction differs')
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+instruction.size]=pc+instruction.size
            for operand in instruction.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    correspondence=dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,instruction_map=instructions,
        address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    args.directory.mkdir(parents=True);manifest=args.directory/'bodies.json'
    manifest.write_text(json.dumps(correspondence,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=args.output,manifest=args.directory/'generation.json',
        correspondence=manifest,additional_entries=Path(__file__)),correspondence,
        runtime_header='halo2/source_campaign_activation_tu15.hpp',runtime_function='source_run_campaign_activation_tu15')
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_campaign_activation_tu15.py').replace('source_run_physics_live_support_tu15',
        'source_run_campaign_activation_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_campaign_activation_tu15_owns_entry')
    args.output.write_text(source)
    path=args.directory/'generation.json';report=json.loads(path.read_text())
    report.update(generated_cpp_sha256=sha(args.output),module_generator_sha256=sha(Path(__file__)),scope=__doc__,
        original_call_sites={f'{pc:08x}':f'{target:08x}' for pc,target in calls.items()},
        original_bsp_callbacks={str(row):f'{target:08x}' for row,target in BSP_CALLBACKS.items()},
        original_new_map_callbacks={str(row):f'{target:08x}' for row,target in NEW_MAP_CALLBACKS.items()})
    path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(len(ENTRIES),'whole original activation bodies;',len(instructions),'instructions')


if __name__=='__main__':main()
