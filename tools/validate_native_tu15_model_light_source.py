#!/usr/bin/env python3
"""Compare native object-lighting sources and RGB channels with physical TU1.5."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import re
import struct
import subprocess
import sys
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parent.parent
REGIONS=((0x46a000,4096),(0x474000,4096),(0x4ec000,4096),(0x4f0000,4096),(0x515000,4096),
         (0x52b000,4096),(0x61000000,65536),(0x62000000,8192))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--native',type=Path)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--require-binding',action='store_true')
    parser.add_argument('--sanitizers',action='store_true')
    args=parser.parse_args()
    if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):
        print('SKIP original model light source requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing owned English TU1.5');return 77
    digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    if digest(args.xbe)!=PIN:raise ValueError('Wrong English TU1.5 original')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image=Xbe(args.xbe);rng=random.Random(0xbc2001d8c80)
    native=args.native or scratch/'native-light-source'
    if not args.native:
        command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_model_light_source_tests.cpp'),'-o',str(native)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        subprocess.run(command,check=True)
    cases=[]
    for chain in (0,1,3):
        for scenario in ('none','negative','beyond','default','channels','channel-default'):
            for outputs in range(4):cases.append((0xbc200,chain,scenario,outputs,'separate'))
    for scenario in ('none','default','channels','channel-default'):
        for mode in ('output-overlap','source-overlap','scenario-overlap','global-default-overlap',
                     'saved-primary','saved-secondary','saved-secondary-earlypop','first-output-argument'):
            for outputs in (1,3):cases.append((0xbc200,2,scenario,outputs,mode))
    for entry in (0x1d8c80,0x1d8d80):
        for scenario in ('channels','channel-default'):
            for mode in ('separate','source-overlap','global-default-overlap','saved-primary','descriptor-overlap'):
                for index in (0,1,127):cases.append((entry,index,scenario,3,mode))
    for count in (-32768,-1,0,1,23,24,47,48,72,32767):
        for index in ((-1,0x7fffffff) if count<0 else (-1,0,1,2,3,0x7fffffff)):
            cases.append((0xbcd80,count,0x200,index,'rgb-row'))
    for displacement in (-32768,-1,0,1,0x200,32767):
        for index in (0,1):cases.append((0xbcd80,48,displacement,index,'rgb-row'))
    for mode in ('source-same','source-forward','source-backward','saved-bx','row-saved-bx',
                 'count-saved-bx','offset-saved-bx','output-count','output-pool'):
        for index in (0,1):cases.append((0xbcd80,48,0x200,index,mode))
    for scenario in ('no-light','tag-off','tag-on','forced'):
        for count in (0,1,2,4):
            for mode in ('publish','unaligned','source-nan','source-infinite'):
                cases.append((0x3dd00,scenario,0,count,mode))
    for mode in ('packed-source','scratch-source','counter-source','argument-output',
                 'saved-register-output','object-light-output','light-pool-output'):
        for count in (1,2,4):cases.append((0x3dd00,'forced',0,count,mode))
    input_path,expected,actual=(scratch/name for name in ('source.in','source.expected','source.actual'))
    rows=[];receipts=[]
    with input_path.open('wb') as inputs:
        for ordinal,(entry,chain,scenario,outputs,mode) in enumerate(cases):
            banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
            def put(at,value,width=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-width:
                        bank[at-base:at-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
                raise ValueError(f'Light-source fixture escaped0x{at:x}')
            gp=[rng.getrandbits(32) for _ in range(8)];stack=0x62001000;gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10)
            status=((ordinal%8)<<11)|(0x4000 if ordinal&8 else 0)|((ordinal%8)<<8)|((ordinal>>4)&63)
            live=list(('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')[:(0,1,4)[ordinal%3]])
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            pool=0x61000100;table=0x61001000;root_object=0x61003000
            resource_pool=0x61000400;resource=0x610041e0
            source_rows=0x61005000+ordinal%15;mirror_rows=0x61005400+ordinal%15
            descriptor=0x61006000;mirror_descriptor=0x61006200
            data=0x61007000+ordinal%15;mirror_data=0x61007400+ordinal%15
            defaults=0x61007800;new_defaults=0x61007a00
            put(0x4ecbd0,pool);put(pool+0x44,table)
            if entry==0xbc200:
                gp[0]=0x12340000 if chain else 0xffffffff
                if not chain:put(pool+0x44,table-65535*12);put(table+8,root_object)
                else:
                    for link in range(chain):
                        obj=root_object-(chain-1-link)*0x200
                        put(table+link*12+8,obj);put(obj+0x14,0xffffffff if link==chain-1 else 0x12340000+link+1)
            index=chain if entry not in (0xbc200,0xbcd80,0x3dd00) else 1
            put(root_object+0xb4,0xffffffff if scenario=='none' else 0x12340003)
            put(root_object+0xc1,1 if scenario=='default' else 0,1)
            put(0x52be5c,resource_pool);put(resource_pool+0x44,resource-3*160)
            put(resource+0x18,0x80 if scenario=='negative' else (7 if scenario=='beyond' else index),1)
            put(resource+0x74,3 if scenario=='beyond' else 128)
            put(resource+0x70,source_rows)
            put(source_rows+index*96+0x40,descriptor);put(mirror_rows+index*96+0x40,mirror_descriptor)
            put(descriptor+0x40,1 if scenario=='channel-default' else 0,1);put(descriptor+0x3c,data)
            put(mirror_descriptor+0x40,0,1);put(mirror_descriptor+0x3c,mirror_data)
            put(0x474d04,defaults)
            first_word={'scenario-overlap':mirror_rows,'global-default-overlap':new_defaults,
                        'first-output-argument':0x61007c00}.get(mode,rng.getrandbits(32))
            for base in (root_object+0x88,root_object+0x94,data+0x40,data+0x50,defaults):
                put(base,first_word);put(base+4,rng.getrandbits(32));put(base+8,rng.getrandbits(32))
            for base in (mirror_data+0x40,mirror_data+0x50,new_defaults):
                for component in range(3):put(base+component*4,rng.getrandbits(32))
            primary,secondary=0x61007b00,0x61007c00
            primary={'source-overlap':root_object+0x8c,'scenario-overlap':resource+0x70,
                     'global-default-overlap':0x474d04,'saved-primary':stack-16,
                     'first-output-argument':stack+4,'descriptor-overlap':descriptor+0x3c}.get(mode,primary)
            secondary={'output-overlap':primary+4,'source-overlap':root_object+0x90,
                       'saved-secondary':stack-12,'saved-secondary-earlypop':stack-16}.get(mode,secondary)
            gp[1]=primary if outputs&1 else 0
            if entry==0xbc200:put(stack+4,secondary if outputs&2 else 0)
            else:
                gp[0]=index;gp[2]=resource
                if mode=='source-overlap':gp[1]=data+(0x44 if entry==0x1d8c80 else 0x54)
            if entry==0xbcd80:
                count,displacement,index=chain,scenario,outputs
                count_rows=(count&0xffffffff)//24
                object=0x61008000 if displacement<0 else 0x61002000
                if mode=='row-saved-bx':
                    object=stack+0x200;displacement=(stack-4)-object-(count_rows+index)*12
                if mode=='count-saved-bx':object=stack-0x128
                if mode=='offset-saved-bx':object=stack-0x12a
                put(0x4ecbd0,pool);put(pool+0x44,table);put(table+3*12+8,object)
                put(object+0x124,count,2);put(object+0x126,displacement,2)
                row=object+displacement+(count_rows+index)*12
                if 0<=index<count_rows:
                    for component in range(3):put(row+4*component,rng.getrandbits(32))
                output={'source-same':row,'source-forward':row+4,'source-backward':row-4,
                    'saved-bx':stack-12,'output-count':object+0x124,'output-pool':0x4ecbd0}.get(mode,0x6100f000)
                gp[0]=0x81230003;gp[6]=output;gp[7]=index&0xffffffff
            if entry==0x3dd00:
                scenario,count=chain,outputs;object=0x61002000;record=0x61008100
                if mode=='argument-output':record=stack-0xa4
                if mode=='saved-register-output':record=stack-0xc4
                if mode=='object-light-output':record=object+0x24
                if mode=='light-pool-output':record=0x515e6c
                if mode=='unaligned':record+=1
                if mode=='scratch-source':object=stack+0x300
                pool=0x61000100;table=0x61001000;light_pool=0x61000600;tag_table=0x61000a00;tag=0x61000700
                put(0x4ecbd0,pool);put(pool+0x44,table);put(table+3*12+8,object)
                put(object+0xcc,0xffffffff if scenario=='no-light' else 0x81230001)
                put(object,0x12340005);put(0x4f0414,tag_table);put(tag_table+5*16+8,tag)
                put(tag+0x1c,1 if scenario=='tag-on' else 0,1)
                put(0x515f14,light_pool);put(light_pool+0x44,record-256)
                secondary=object+0x200+count*12
                if mode=='packed-source':secondary=record+0xa8
                if mode=='counter-source':secondary=record+0xb0
                if mode=='scratch-source':secondary=stack-12
                displacement=secondary-object-count*12
                put(object+0x124,count*24,2);put(object+0x126,displacement,2)
                for channel in range(count):
                    for component,value in enumerate((0x3d909091,0,0x3c40c0c1)):
                        if mode=='source-nan':value=0x7f812345+component
                        if mode=='source-infinite':value=0x7f800000 if component&1 else 0xff800000
                        put(secondary+channel*12+component*4,value)
                put(0x46a148,0x437f0000)
                put(stack+4,0x81230003);put(stack+8,1 if scenario=='forced' else 0)
            put(stack,0x63000000)
            inputs.write(struct.pack('<I12I',entry,*gp,flags,mxcsr,cw,status));inputs.write(struct.pack('<I',len(live)))
            for value in live:inputs.write(bytes.fromhex(value))
            for lane in xmm:inputs.write(struct.pack('<4I',*lane))
            inputs.write(b''.join(banks))
            def at_stack(offset):return struct.unpack_from('<I',banks[-1],stack+offset-REGIONS[-1][0])[0]
            rows.append(dict(id=ordinal,name=f'{entry:x}-{chain}-{scenario}-{outputs}-{mode}',entry=entry,gp=gp,eflags=flags,
                matrix=at_stack(4),source=at_stack(8),max_steps=1000,xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,
                banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)]))
            receipts.append(dict(entry=entry,chain=chain,scenario=scenario,outputs=outputs,alias=mode))
    physical,proof=physical_original(image,rows,scratch,entries=(0xbc200,0x1d8c80,0x1d8d80,0xbcd80,0x3dd00,0x1349e0),regions=REGIONS,
        trace_calls=[dict(entry=entry,arguments=0,gp=True,xmm=True) for entry in (0xbcd80,0x1349e0)])
    with expected.open('wb') as out:
        for receipt,case in zip(receipts,physical['cases'],strict=True):
            if not case['completed'] or not case['live_x87_unchanged']:raise ValueError('Original lighting source incomplete')
            start=out.tell();out.write(struct.pack('<12I',*case['boundary_gp'],case['boundary_eflags'],case['after_fp'][2],case['after_fp'][1],case['after_fp'][0]))
            for lane in case['boundary_xmm']:out.write(struct.pack('<4I',*lane))
            out.write(b''.join(bytes.fromhex(bank['after']) for bank in case['banks']));out.write(struct.pack('<I',len(case['writes'])))
            for event in case['writes']:
                before,written=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                out.write(struct.pack('<III',event['instruction'],event['address'],len(written))+before.ljust(4,b'\0')+written.ljust(4,b'\0'))
            out.write(struct.pack('<I',len(case['calls'])))
            for call in case['calls']:
                out.write(struct.pack('<I12I',call['entry'],*call['gp'],call['eflags'],call['fp'][2],call['fp'][1],call['fp'][0]))
                for lane in call['xmm']:out.write(struct.pack('<4I',*lane))
            receipt.update(stream_start=start,stream_bytes=out.tell()-start,stores=len(case['writes']))
    with input_path.open('rb') as inputs,actual.open('wb') as out:subprocess.run([str(native)],stdin=inputs,stdout=out,check=True)
    if digest(actual)!=digest(expected):
        left,right=actual.read_bytes(),expected.read_bytes();first=next((i for i,(a,b) in enumerate(zip(left,right)) if a!=b),min(len(left),len(right)))
        receipt=next((r for r in receipts if r['stream_start']<=first<r['stream_start']+r['stream_bytes']),{})
        raise ValueError(f'Lighting source mismatch{first} local{first-receipt.get("stream_start",0)} {receipt}: '
            f'{left[first:first+16].hex()} vs{right[first:first+16].hex()}')
    rejected=0
    if args.require_binding:
        from model_light_source_native_replacements import replace_model_light_source,PINS,prefix
        for path in ('src/rendering/models/source_model_render_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp',
                     'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp',
                     'src/physics/physics_live_support_tu15_generated.cpp'):
            source=(ROOT/path).read_text()
            if replace_model_light_source(source)!=source:raise ValueError('Lighting source native binding absent')
            for entry in PINS:
                match=re.search(rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}',source,re.S)
                if not match:continue
                body=match[0]
                for shape in (body,re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)):
                    rewritten=source.replace(body,shape,1)
                    if replace_model_light_source(rewritten)!=rewritten:raise ValueError('Lighting body shape not idempotent')
                    for candidate in (shape,shape.replace(prefix(entry),'',1)):
                        changed=candidate.replace(',4)',',2)',1)
                        if changed==candidate:raise ValueError('Lighting source mutation witness absent')
                        try:replace_model_light_source(source.replace(body,changed,1))
                        except ValueError:rejected+=1
                        else:raise ValueError('Lighting source binder accepted body drift')
        import model_color_publish_native_replacements as publish
        for path in ('src/physics/physics_live_support_tu15_generated.cpp',
                     'src/runtime/source_map_world_support_tu15_generated.cpp',
                     'src/gameplay/units/source_combat_action_tu15_generated.cpp'):
            source=(ROOT/path).read_text();bind=publish.replace_model_color_publish
            if bind(source)!=source:raise ValueError('Color-publish parent native binding absent')
            body=re.search(r'static void run_3dd00\(WorldState &s\) \{.*?\n\}',source,re.S)[0]
            for shape in (body,re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)):
                rewritten=source.replace(body,shape,1)
                if bind(rewritten)!=rewritten:raise ValueError('Color-publish body shape not idempotent')
                for candidate in (shape,shape.replace(publish.PREFIX,'',1)):
                    changed=candidate.replace(',4)',',2)',1)
                    if changed==candidate:raise ValueError('Missing color-publish mutation witness')
                    try:bind(source.replace(body,changed,1))
                    except ValueError:rejected+=1
                    else:raise ValueError('Color-publish binder accepted body drift')
    paths=('src/rendering/models/native_tu15_model_light_source.hpp','src/rendering/models/native_tu15_model_color_publish.hpp',
           'src/rendering/models/native_tu15_model_color.hpp','tests/native_tu15_model_light_source_tests.cpp',
           'tools/validate_native_tu15_model_light_source.py','tools/validate_native_tu15_prt_helpers.py',
           'tools/model_light_source_native_replacements.py','tools/native_selected_operand_binding.py',
           'tools/model_color_publish_native_replacements.py',
           'tools/generate_model_render_tu15.py','tools/generate_ai_frame_tu15.py','tools/generate_scene_device_constructor_tu15.py',
           'tools/generate_physics_live_support_tu15.py','src/physics/physics_live_support_tu15_generated.cpp',
           'tools/generate_combat_action_tu15.py','tools/generate_map_world_support_tu15.py',
           'cmake/NativeTu15ModelViewSlots.cmake','src/rendering/models/source_model_render_tu15_generated.cpp',
           'src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp',
           'src/runtime/source_map_world_support_tu15_generated.cpp','src/gameplay/units/source_combat_action_tu15_generated.cpp',
           'src/rendering/models/native_tu15_prt_helpers.hpp','src/rendering/models/native_tu15_prt_projection.hpp',
           'src/physics/runtime/physics_contact_runtime_internal.hpp')
    sites=dict(hierarchy=0xbc214,object_defaults=0xbc2ee,active_defaults=0xbc299,first_channel=0x1d8c80,
               second_channel=0x1d8d80,first_authored=0x1d8ca6,first_defaults=0x1d8cbb,
               second_authored=0x1d8da6,second_defaults=0x1d8dbb,rgb_channel_accepted=0xbcdc1,
               rgb_channel_rejected=0xbcddd,color_publish_pack=0x3dd94,color_publish_tag_check=0x3dd48)
    coverage={label:sum(site in case['executed_sites'] for case in physical['cases']) for label,site in sites.items()}
    if not all(coverage.values()):raise ValueError(f'Lighting source branch not reached: {coverage}')
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,cases=receipts,physical_original=proof,original_xbe_sha256=PIN,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),native_instruction_callbacks=0,
        rejected_semantic_mutations=rejected,source_sha256={p:digest(ROOT/p) for p in paths},
        branch_cases=coverage,all_production_copies_qualified=args.require_binding,
        exact_child_entry_abi_snapshots=sum(len(case['calls']) for case in physical['cases']),
        scratch_owner=str(scratch),
        memory_fault_prefix_abi_verified=False,ordered_read_fault_semantics_verified=False),indent=2,sort_keys=True)+'\n')
    print(f'PASS {len(cases)} complete native BC200/channel cases');return 0


if __name__=='__main__':raise SystemExit(main())
