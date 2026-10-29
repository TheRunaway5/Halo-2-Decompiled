#!/usr/bin/env python3
"""Lower twenty-five authenticated TU1.5 texture-camera CPU producer bodies.

The generated C++ preserves all original branches/calls and floating-point
semantics. It neither allocates GPU targets nor supplies object/marker/script
engine results. Rendering, copy completion and view scheduling are separate.
The retired2F7E0 packet helper is an external native required-owner boundary;
its null-observer default projection remains an explicit unqualified capability.
"""
import argparse,json,sys
from pathlib import Path
from types import SimpleNamespace

ENTRIES=(0x255c0,0x255e0,0x256d0,0x25800,0x25940,0x2acc20,0x2acc70,0x229a90,0x2f5e0,0x2f620,0x2f690,0x2c470,0x14cd8d,0x14cd61,0x14aa8c,0x23590b,0x2b770,0x2b9f0,0x2b700,0x2f950,0x2fb50,0x2fd70,0x3d2b0,0x3ea50,0x13c00)
CALLS=((0x2acc20,0x2acc54,0x255e0),(0x25800,0x25819,0x256d0),
       (0x256d0,0x257d1,0x255c0),(0x229ef0,0x22a000,0x229a90),
       (0x229a90,0x229b44,0x2f5e0),(0x229a90,0x229b5f,0x2f620),
       (0x229a90,0x229b7d,0x2f690),(0x229a90,0x229b95,0x2f7e0),
       (0x229a90,0x229c30,0x2f7e0),
       (0x229a90,0x229b9e,0x2c470),(0x229a90,0x229acf,0x14cd8d),
       (0x2b770,0x2b94f,0x2b9f0),(0x2b9f0,0x2bbfb,0x132b0),(0x2b9f0,0x2bab7,0x2fd70),(0x2fd70,0x2fd79,0x2fb50),(0x13c00,0x13c40,0x132b0),(0x14cd8d,0x14cd8d,0x14cd61),(0x14cd61,0x14cd78,0x14aa8c),(0x14aa8c,0x14aa9a,0x23590b))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',type=Path,default=Path(__file__).resolve().parents[1])
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--directory',type=Path,required=True)
    args=parser.parse_args();source=args.source_root.resolve(strict=True)
    if args.output.exists() or args.directory.exists():raise ValueError('Preserve generated source and evidence')
    sys.path[:0]=[str(source/'tools'),str(source/'tools/reference/halo2-decompiled/tools')]
    from xbe import Xbe
    from functions import _Code,_trace
    from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
    from generate_physics_live_support_tu15 import generate,sha
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Expected authenticated original English TU1.5 image')
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    for owner,site,target in CALLS:
        body=_trace(code,owner,{owner});ins=code.at(site)
        if site not in body.code or ins.mnemonic!='call' or ins.operands[0].imm!=target:
            raise ValueError('Original whole-owner call boundary changed')
    sites,instructions,addresses,routines={},{},{},[]
    external=[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES))
        if not body.code or any(width not in (1,4) for _,width,_ in body.tables):
            raise ValueError('Original complete CFG unsupported')
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n) for p,w,n in body.tables if w==4],
            byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex()) for p,w,n in body.tables if w==1]))
        external.extend(dict(owner=f'{entry:08x}',site=f'{p:08x}',target=f'{t:08x}',kind=k)
                        for p,t,k in sorted(body.sites) if t not in ENTRIES)
        for pc in sorted(body.code):
            ins=code.at(pc)
            if image.read(pc,ins.size)!=bytes(ins.bytes):raise ValueError('Original instruction identity differs')
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
            for operand in ins.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},
        body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    args.directory.mkdir(parents=True,exist_ok=False);correspondence=args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    generate(SimpleNamespace(output=args.output,manifest=args.directory/'generation.json',correspondence=correspondence,
        additional_entries=Path(__file__)),paired,code_override=code)
    text=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_texture_camera_producer_tu15.py')
    text=text.replace('halo2/physics_live_support_tu15.hpp','halo2/source_texture_camera_producer_tu15.hpp')
    text=text.replace('source_run_physics_live_support_tu15','source_run_texture_camera_producer_tu15')
    text=text.replace('source_physics_live_support_tu15_owns_entry','source_texture_camera_producer_tu15_owns_entry')
    # With2F7E0 absent fromENTRIES, both authenticated CALLs naturally use
    # the existing external dispatch's required-owner transport. Never emit
    # its former lifted helper or silently restore the null default profile.
    if 'run_2f7e0' in text or 0x2f7e0 in ENTRIES:
        raise ValueError('Retired2F7E0 was reintroduced as a local lifted helper')
    camera_packet_boundaries=[]
    for site in (0x229b95,0x229c30):
        if not any(row['site']==f'{site:08x}' and row['target']=='0002f7e0' for row in external):
            raise ValueError('Authenticated native camera packet boundary was not externalized')
        ins=code.at(site)
        camera_packet_boundaries.append(dict(site=f'{site:08x}',target='0002f7e0',
            original_bytes=ins.bytes.hex(),original_return=f'{site+ins.size:08x}',
            transport='existing external dispatch -> required_owner',
            null_default_projection_qualified=False))
    original_call='if(s.services.instruction){s.services.instruction(0x258f7u);} s.push(0x258f7u,0x258fcu);run_25940(s);'
    required_call='if(s.services.instruction){s.services.instruction(0x258f7u);} s.push(0x258f7u,0x258fcu);s.required(0x258f7u,0x25940u,false);'
    ins=code.at(0x258f7)
    if ins.mnemonic!='call' or ins.bytes.hex()!='e844000000' or ins.operands[0].imm!=0x25940 or text.count(original_call)!=1:
        raise ValueError('Exact source258F7 native required-owner boundary changed')
    # Preserve the original complete static body and all generated address
    # metadata. Only publish this one native service boundary; no arithmetic,
    # original instruction, call return, observer or memory read is replaced.
    text=text.replace(original_call,required_call)
    producer_call='if(s.services.instruction){s.services.instruction(0x258e9u);} s.push(0x258e9u,0x258eeu);run_2b770(s);'
    producer_boundary='if(s.services.instruction){s.services.instruction(0x258e9u);} s.push(0x258e9u,0x258eeu);s.required(0x258e9u,0x2b770u,false);'
    if text.count(producer_call)!=1:raise ValueError('Exact258E9 deferred scene boundary changed')
    text=text.replace(producer_call,producer_boundary)
    # The source2B770/2B9F0 graphics call graph is NOT admitted as a completed
    # World entry. A separate typed prefix API below suspends only at the
    # original132B0 backend boundary and retains its real continuation ABI.
    hidden=(0x2b770,0x2b9f0,0x13c00)
    for entry in hidden:
        line=f'case 0x{entry:x}u:run_{entry:x}(s);break;'
        if text.count(line)!=1:raise ValueError('Exact private prefix entry changed')
        text=text.replace(line,'')
    marker='bool source_texture_camera_producer_tu15_owns_entry'
    start=text.index(marker)
    head,tail=text[:start],text[start:]
    for entry in hidden:
        token=f'case 0x{entry:x}u:'
        if tail.count(token)!=1:raise ValueError('Exact private prefix admission changed')
        tail=tail.replace(token,'')
    text=head+tail
    fragment=Path(__file__).with_name('texture_camera_scene_prefix_fragment.inc').read_text()
    closing='} // namespace halo2\n'
    if not text.endswith(closing):raise ValueError('Generated module closing changed')
    text=text[:-len(closing)]+fragment+closing
    text=text.replace('#include "halo2/source_texture_camera_producer_tu15.hpp"',
        '#include "halo2/source_texture_camera_producer_tu15.hpp"\n#include "halo2/source_texture_camera_scene_prefix_tu15.hpp"')
    args.output.write_text(text)
    report_path=args.directory/'generation.json';report=json.loads(report_path.read_bytes())
    report.update(module_generator_sha256=sha(Path(__file__)),generated_cpp_sha256=sha(args.output),
        compiled_original_entries=[f'{p:08x}' for p in ENTRIES],
        admitted_original_entries=[f'{p:08x}' for p in ENTRIES if p not in hidden],
        private_typed_prefix_entries=[f'{p:08x}' for p in hidden],
        authenticated_caller_chain=[dict(owner=f'{p:08x}',site=f'{q:08x}',target=f'{r:08x}') for p,q,r in CALLS],
        retired_native_packet_helper='0002f7e0',native_packet_boundaries=camera_packet_boundaries,
        required_native_boundary=dict(site='000258f7',target='00025940',original_bytes='e844000000',original_return='000258fc',arithmetic_or_call_ABI_changed=False,scope='Required native owner only; no-capture World dispatch executes the same complete local25940 body'),
        external_engine_calls=external,source_tools_root=str(source),source_scope=__doc__,
        GPU_allocation_or_rendering_supplied=False,runtime_opcode_decode_or_interpretation=False)
    report_path.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(passed=True,complete_bodies=len(ENTRIES),instruction_sites=len(instructions),
        output=str(args.output),output_sha256=sha(args.output),external_engine_calls=external)))
if __name__=='__main__':main()
