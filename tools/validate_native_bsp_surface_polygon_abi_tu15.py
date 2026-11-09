#!/usr/bin/env python3
"""Whole1E6E80 shared polygon DATA and retained raw ABI on unchanged i386."""
import argparse
import copy
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
from generated_routine_ownership import function_bodies
from validate_native_tu15_prt_helpers import PIN,physical_original
from native_bsp_surface_polygon_replacements import canonical_body,replace_bsp_surface_polygon

ROOT=Path(__file__).resolve().parents[1]
COPY='src/runtime/source_map_world_support_tu15_generated.cpp'
REGIONS=((0x1000000,0x5000),(0x44a000,8192),(0x62000000,8192))


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def metadata(image,original):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    result=original(image);code=common._Code(image,'.text')
    for site in result['sites']:
        ins=code.at(site['instruction'])
        if ins.mnemonic.startswith('prefetch'):site['operands']=[]
        elif ins.mnemonic=='test':
            for operand in site['operands']:operand.update(read=True,write=False)
    return result


def rows(image):
    from validate_native_bsp_surface_query_tu15 import cases
    old=[row for row in cases(image)if row['entry']==0x1e6e80];rng=random.Random(0x1e6e80);result=[]
    live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f',
        '0100000000000080ff7f','0000000000000080ff7f','0000000000000080ffff','01000000000000000000')
    for n,source in enumerate(old):
        data=bytes.fromhex(source['banks'][0]['before']);stack=bytearray(rng.randbytes(8192));sp=0x62001000+n%4
        arguments=source['arguments'];struct.pack_into('<6I',stack,sp-0x62000000,0x63000000,*arguments)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp;gp[0]=source['entry_registers']['eax'];gp[2]=source['entry_registers']['edx']
        cw=(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10);status=((n%8)<<11)|(n%64)|((n*0x100)&0x4700)
        mxcsr=0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[(n//4)%4]|(n%64)
        result.append(dict(id=n,name=source['name'],entry=0x1e6e80,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
            matrix=arguments[0],source=arguments[1],xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
            before_fp=[cw,status,mxcsr],live_x87=list(live_values[:(0,1,4,8)[n%4]]),max_steps=10000,
            banks=[dict(address=0x1000000,before=data.hex()),dict(address=0x44a000,before=image.read(0x44a000,8192).hex()),dict(address=0x62000000,before=stack.hex())]))
    def extra(mode):
        row=copy.deepcopy(result[90]);row.update(id=len(result),name=mode)
        banks=[bytearray.fromhex(bank['before'])for bank in row['banks']];sp=row['gp'][4]
        def put(at,value,width=4):
            for bank,region in zip(banks,row['banks'],strict=True):
                base=region['address']
                if base<=at and at+width<=base+len(bank):bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Polygon alias fixture escaped backing')
        d,surface,edges,vertices=0x1000100,0x1001800,0x1002000,0x1002800
        if mode.startswith('point-stack-'):put(sp+20,sp+int(mode.removeprefix('point-stack-')))
        elif mode.startswith('descriptor-'):
            offset=int(mode.removeprefix('descriptor-'));d=sp-offset;row['gp'][2]=d
            put(d+0x2c,surface);put(d+0x34,edges);put(d+0x3c,vertices)
            if offset==0x40:row['gp'][7]=surface;row['gp'][3]=edges
            if offset==0x44:row['gp'][6]=edges
            if offset==0x48:row['gp'][7]=edges;row['gp'][3]=vertices
            if offset==0x50:row['gp'][7]=vertices
        elif mode=='vertices-stack':
            vertices=sp-64;put(d+0x3c,vertices)
            for index,values in enumerate(((0,0,0),(0x3f800000,0,0),(0x3f800000,0x3f800000,0),(0,0x3f800000,0))):
                for lane,value in enumerate(values):put(vertices+index*16+lane*4,value)
        elif mode.startswith('surface-stack-'):
            surface=sp+int(mode.removeprefix('surface-stack-'));put(d+0x2c,surface)
            if surface==sp-16:row['gp'][6]=1
            else:put(surface+2,0,2)
        elif mode=='wrapped-edge-scratch':
            first=vertices&0xffff;row['source']=1;put(sp+8,1);put(surface+8+2,first,2)
            put(d+0x34,(sp-12-first*12)&0xffffffff);row['gp'][3]=0x00010000
        elif mode.startswith('surface-index-'):
            index=int(mode.removeprefix('surface-index-'));row['source']=index;put(sp+8,index);put(d+0x2c,(surface-index*8)&0xffffffff)
            for index_edge in range(4):put(edges+index_edge*12+10,index,2)
        elif mode.startswith('table-'):
            _,axis,winding=mode.split('-');axis=int(axis);winding=int(winding);put(sp+12,axis);put(sp+16,winding)
            signed=axis if axis<0x8000 else axis-0x10000;table=(0x44ac8c+(winding+signed*2)*6)&0xffffffff
            put(table,0,2);put(table+2,1,2);put(table+4,2,2)
        elif mode.startswith('material-'):
            _,material,limit,allowed=mode.split('-');material=int(material);limit=int(limit);allowed=int(allowed);row['gp'][0]=0x1003300
            put(surface+4,8,1);put(surface+5,material,1);put(sp+4,limit,2);row['matrix']=(row['matrix']&0xffff0000)|(limit&0xffff)
            put(0x1003300+(material>>5)*4,(1<<(material&31))if allowed else 0)
        else:raise ValueError('Unknown polygon directed case')
        for target,value in zip(row['banks'],banks,strict=True):target['before']=value.hex()
        result.append(row)
    for offset in (-24,-20,-16,-12,-8,-4,4,12,20):extra('point-stack-'+str(offset))
    for offset in (0x40,0x44,0x48,0x50):extra('descriptor-'+str(offset))
    extra('vertices-stack')
    for offset in (-16,-4,4):extra('surface-stack-'+str(offset))
    extra('wrapped-edge-scratch')
    for index in (256,65535,0x80000000):extra('surface-index-'+str(index))
    for axis,winding in ((0xffff,0),(0xffff,1),(3,0),(0,2),(2,255)):extra('table-'+str(axis)+'-'+str(winding))
    for material in (0,31,32,255):
        for limit in (0x8000,0xffff,0,31,64,256,32767):
            for allowed in (0,1):extra(f'material-{material}-{limit}-{allowed}')
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--sanitizers',action='store_true');p.add_argument('--compiler',default='c++');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in ('x86_64','amd64','i386','i686'):print('SKIP physical BSP polygon requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong unchanged original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,candidate_unbound=not args.require_binding,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        source=(ROOT/COPY).read_text();body=next(body for match,body in function_bodies(source)if match[1]=='1e6e80')
        original_body=canonical_body(body);raw=source.replace(body,original_body,1);bound=replace_bsp_surface_polygon(raw);rejected=0
        if args.require_binding and bound!=source:raise ValueError('Native polygon source prefix missing')
        if replace_bsp_surface_polygon(bound)!=bound:raise ValueError('Polygon binding not idempotent')
        literal=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original_body)
        for shape in (original_body,literal):
            candidate=raw.replace(original_body,shape,1);replace_bsp_surface_polygon(candidate)
            for version in (candidate,replace_bsp_surface_polygon(candidate)):
                target=next(b for m,b in function_bodies(version)if m[1]=='1e6e80');changed=target.replace('s.gp[1]*8u','s.gp[1]*4u',1)
                if changed==target:raise ValueError('Polygon body mutation witness missing')
                try:replace_bsp_surface_polygon(version.replace(target,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Accepted changed complete polygon body')
        changed=raw.replace('{if(image==SourcePhysicsContactImage::MapPack11)','{if(p==0)return 1u;if(image==SourcePhysicsContactImage::MapPack11)',1)
        try:replace_bsp_surface_polygon(changed)
        except ValueError:rejected+=1
        else:raise ValueError('Accepted changed complete polygon resolver')
        selected=next(line for line in source.splitlines()if line.startswith('std::uint32_t selected('))
        mini=replace_bsp_surface_polygon(selected+'\n'+original_body)
        (scratch/'bsp_surface_polygon_bodies.inc').write_text('namespace reference {\n'+selected+'\n'+original_body+'\n}\nnamespace bound {\n'+mini+'\n}\n')
        header=ROOT/'src/physics/runtime/native_tu15_bsp_surface_polygon.hpp'
        sites=sorted({int(value,16)for value in re.findall(r'0x(1e6[0-9a-f]+)u',header.read_text())if 0x1e6e80<=int(value,16)<=0x1e6fac})
        from oracle_smg_firing_1_1 import Xbe
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        original=common.metadata;common.metadata=lambda image:metadata(image,original)
        image=Xbe(args.xbe);inputs=rows(image)
        from specialize_native_operands import specialize,BSP_SURFACE_POLYGON_ACTUAL_OWNER_IMAGES,ALLOCATOR_OWNER_FRESH_CASE,ALLOCATOR_OWNER_FRESH_BODY_PIN
        primary=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text();_,receipt=specialize(primary)
        if receipt.get('native_bsp_surface_polygon_actual_owner_entries')!=['1e6e80']:raise ValueError('Polygon actual owner dispatcher binding absent')
        for entry,(length,pin)in BSP_SURFACE_POLYGON_ACTUAL_OWNER_IMAGES.items():
            if hashlib.sha256(image.read(entry,length)).hexdigest()!=pin:raise ValueError('Whole original polygon image drift')
        dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',primary)[0]
        for replacement in (dispatch.replace('{','{if(target==0)return;',1),dispatch.replace('s.required(site,target,tail)','s.required(site,target^1u,tail)',1)):
            try:specialize(primary.replace(dispatch,replacement,1))
            except ValueError as error:
                if str(error)!='Complete13FE50 dispatcher drift':raise
            else:raise ValueError('Altered actual dispatcher did not trigger complete PageAllocate rejection')
            rejected+=1
        seam='case 0xfa670u:run_fa670(s);return;'
        if dispatch.count(seam)!=1:raise ValueError('Fresh dispatcher insertion seam changed')
        fresh_dispatch=dispatch.replace(seam,ALLOCATOR_OWNER_FRESH_CASE+seam,1);fresh_primary=primary.replace(dispatch,fresh_dispatch,1)
        _,fresh_receipt=specialize(fresh_primary)
        owner_groups=('native_allocator_actual_owner_entries','native_tick_conversion_actual_owner_entries',
            'native_physical_epoch_actual_owner_entries','native_bsp_surface_2d_actual_owner_entries',
            'native_bsp_surface_polygon_actual_owner_entries','native_datum_delete_actual_owner_entries')
        if fresh_receipt.get('native_actual_owner_dispatch_variant')!='fresh-fa5d0'or any(fresh_receipt[group]!=receipt[group]for group in owner_groups):raise ValueError('Exact fresh dispatcher did not preserve every imported owner group')
        from native_halfword_bit_query_replacements import canonical_body as halfword_body,installed_entries as halfword_entries
        fa_body=next(b for m,b in function_bodies(primary)if m[1]=='fa5d0')
        canonical_fa_sha=hashlib.sha256(halfword_body(fa_body).encode()).hexdigest()
        if canonical_fa_sha!=ALLOCATOR_OWNER_FRESH_BODY_PIN:raise ValueError('Fresh dispatcher FA5D0 canonical continuation drift')
        changed_fa=fa_body.replace('s.gp[1]),2)','s.gp[1]),4)',1)
        if changed_fa==fa_body:raise ValueError('FA5D0 body mutation witness missing')
        mutations=[fresh_primary.replace(fa_body,changed_fa,1),
            fresh_primary.replace(ALLOCATOR_OWNER_FRESH_CASE,'case 0xfa5d0u:run_fa670(s);return;',1),
            fresh_primary.replace(ALLOCATOR_OWNER_FRESH_CASE,'case 0x123456u:run_fa5d0(s);return;'+ALLOCATOR_OWNER_FRESH_CASE,1),
            fresh_primary.replace(ALLOCATOR_OWNER_FRESH_CASE,ALLOCATOR_OWNER_FRESH_CASE*2,1),
            fresh_primary.replace(fresh_dispatch,fresh_dispatch.replace('s.required(site,target,tail)','s.required(site,target^1u,tail)',1),1)]
        for index,mutation in enumerate(mutations):
            try:specialize(mutation)
            except ValueError as error:
                expected='Complete FA5D0 specialization body drift'if index==0 else'Complete13FE50 dispatcher drift'
                if str(error)!=expected:raise
            else:raise ValueError('Changed fresh dispatcher/FA5D0 body did not trigger its complete-source rejection')
            rejected+=1
        observed,proof=physical_original(image,inputs,scratch,entries=(0x1e6e80,),regions=REGIONS,
            trace_calls=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=0)for pc in sites],trace_extended=True)
        cases=[]
        for row,actual in zip(inputs,observed['cases'],strict=True):
            if not actual['completed']or not actual['live_x87_unchanged']:raise ValueError('Original polygon failed normal/live80 contract')
            case=copy.deepcopy(row)
            for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes'):case[key]=actual[key]
            case['packets']=[{key:packet[key]for key in ('entry','gp','eflags','xmm','fp','live_x87')}for packet in actual['calls']]
            for before,after in zip(case['banks'],actual['banks'],strict=True):before['after']=after['after']
            cases.append(case)
        read_sites=sorted({e['instruction']for case in observed['cases']for e in case['reads']})
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases,explicit_read_sites=read_sites),separators=(',',':')))
        binary=scratch/'polygon-tests';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_bsp_surface_polygon_abi_tests.cpp'),
            *[str(ROOT/'src/physics'/name)for name in ('native_tu15_array_allocator.cpp','native_tu15_stack_arena.cpp','native_tu15_main_heap_release.cpp','native_tu15_main_heap_allocate.cpp','native_tu15_raw_allocator.cpp','native_tu15_physical_epoch.cpp')],'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        compiled=subprocess.run(command,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Polygon test compile failed: '+compiled.stderr[-5000:])
        run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=90)
        if run.returncode:raise ValueError('Polygon native ABI failed: '+run.stdout[-1000:]+run.stderr[-6000:])
        result=json.loads(run.stdout);executed=sorted({pc for row in observed['cases']for pc in row['executed_sites']})
        if len(executed)!=97:raise ValueError('Whole97-site polygon coverage incomplete')
        forced=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=90,env=dict(os.environ,HALO2_REFERENCE_BSP_SURFACE_POLYGON='1',HALO2_BSP_POLYGON_REFERENCE_TEST='1'))
        if forced.returncode:raise ValueError('Polygon forced-reference failed: '+forced.stderr[-3000:])
        reference=json.loads(forced.stdout)
        if not reference['native_refused']or reference['callbacks']<=0 or reference['last_pc']!=cases[0]['packets'][-1]['entry']or reference['returned_eax']!=cases[0]['boundary_gp'][0]:raise ValueError('Missing measured polygon reference evidence')
        controls=[]
        for control in ('HALO2_REFERENCE_RAW_ALLOCATOR','HALO2_REFERENCE_TICK_CONVERSION','HALO2_REFERENCE_PHYSICAL_EPOCHS','HALO2_REFERENCE_BSP_SURFACE_2D','HALO2_REFERENCE_DATUM_DELETE'):
            independent=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=90,env=dict(os.environ,**{control:'1'}))
            if independent.returncode or json.loads(independent.stdout)!=result:raise ValueError('Unrelated control changed polygon owner '+control+': '+independent.stderr[-2000:])
            controls.append(control)
        paths=(COPY,'include/halo2/native_tu15_bsp_surface_polygon_kernel.hpp','src/physics/runtime/native_tu15_bsp_surface_polygon.hpp',
            'src/physics/native_tu15_bsp_surface_query.cpp','tools/validate_native_bsp_surface_query_tu15.py',
            'tools/validate_native_bsp_surface_polygon_abi_tu15.py','tests/native_tu15_bsp_surface_polygon_abi_tests.cpp','tools/validate_native_tu15_prt_helpers.py',
            'tools/native_bsp_surface_polygon_replacements.py','tools/specialize_native_operands.py','tools/generate_physics_live_support_tu15.py',
            'src/physics/runtime/native_tu15_allocator_owner_dispatch.hpp','src/physics/physics_live_support_tu15_generated.cpp')
        report.update(result,physical_original=proof,executed_sites=executed,full_original_sites=len(executed),compiler_command=command,
            source_sha256={path:sha(ROOT/path)for path in paths},memory_fault_prefix_abi_qualified=False,
            binding_drift_rejections=rejected,actual_owner_entry='1e6e80',independent_reference_controls=controls,forced_reference=reference,
            fresh_dispatcher_imported_groups_verified={group:fresh_receipt[group]for group in owner_groups},
            fresh_dispatcher_fa5d0_native_bindings=halfword_entries(primary),
            fresh_dispatcher_fa5d0_canonical_body_sha256=canonical_fa_sha,
            fresh_dispatcher_fa5d0_continuation='Exact canonicalized7-site continuation and ordered shift; native execution qualified separately',
            fresh_dispatcher_authentication_is_execution_proof=False)
        print(run.stdout.strip())
    except BaseException as error:report['error']=str(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0


if __name__=='__main__':raise SystemExit(main())
