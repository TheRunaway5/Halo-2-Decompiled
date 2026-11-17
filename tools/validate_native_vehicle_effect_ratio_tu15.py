#!/usr/bin/env python3
"""Private TU1.5 D3750 against unchanged i386 instructions and full ABI packets."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import shlex
import struct
import subprocess
import sys

from native_mopp_ray_dense_original import physical_original_dense
import oracle_native_campaign_screen_lifecycle_tu15 as original
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ROOT = Path(__file__).resolve().parents[1]
ENTRY, STOP = 0xd3750, 0x63000000
REGIONS = ((0x1000000, 4096), (0x46a000, 4096), (0x4ec000, 4096),
           (0x4f0000, 4096), (0x62000000, 4096))
TEST = 'tests/native_tu15_vehicle_effect_ratio_tests.cpp'
FRAME = 'src/physics/vehicles/native_tu15_vehicle_effect_ratio_frame.hpp'
OWNER = 'src/physics/vehicles/physics_vehicle_update_generated.cpp'
OBSERVER = 'tools/observe_native_campaign_ai_vector_math_tu15.cpp'
KERNEL = 'include/halo2/native_tu15_vehicle_effect_ratio_kernel.hpp'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0] if isinstance(value, float) else value


def corpus(image, smoke):
    rng = random.Random(ENTRY)
    qnan, snan, inf = 0x7fc12345, 0x7f812345, 0x7f800000
    profiles = [
        ('inactive-zero', 0., 0., 2., 1.), ('inactive-negative', -1., 0., 2., 1.),
        ('initial', 1., 0., 2., 1.), ('saturated', 2., -0., 2., 1.),
        ('baseline-equal', 2., 2., 4., 2.), ('baseline-active', 3., 1., 4., 2.),
        ('elapsed-unordered', snan, 0., 2., 1.), ('baseline-unordered', 3., qnan, 4., 2.),
        ('initial-unordered-limit', 2., 0., qnan, 1.), ('initial-signaling-limit', 2., 0., snan, 1.),
        ('baseline-signaling', 3., snan, 4., 2.), ('initial-infinite-limit', 2., 0., inf, 1.),
        ('baseline-zero-duration', 3., 1., 4., 0.), ('baseline-negative-duration', 3., 1., 4., -0.),
        ('baseline-signaling-duration', 3., 1., 4., snan), ('baseline-unordered-duration', 3., 1., 4., qnan),
        ('negative-baseline', 1., -2., 2., 3.), ('positive-subnormal', 1, 0., 2, 1),
        ('subnormal-baseline', 0x00800000, 1, 2, 0x00800000),
        ('overflow-difference', 0x7f7fffff, 0xff7fffff, 0., 2.),
        ('underflow-result', 0x00800000, 0., 0x7f7fffff, 1.),
    ]
    rows = []
    for name, elapsed, baseline, limit, duration in (profiles[:8] if smoke else profiles):
        for mode in range(1 if smoke else 8):
            n = len(rows)
            data, stack = REGIONS[0][0], REGIONS[-1][0]
            pool, object_rows, body, tags, definition = (data+x for x in (0x100, 0x200, 0x500, 0x900, 0xb00))
            if mode == 5:
                definition = body  # Independent member offsets in the same bank object.
            elif mode == 6:
                tags = object_rows  # Both table lookups still execute independently.
            elif mode == 7:
                body = stack + 0x100  # Keep caller and body data in the same bank.
            # Distinct handles make the 12-byte and 16-byte index scales observable.
            obj, datum = 3+n%9, 17+n%7
            sp = stack+0xe00+4*(n%4)
            banks = {base: bytearray(rng.randbytes(size)) for base, size in REGIONS}
            def put(address, value):
                base = next(base for base,size in REGIONS if base <= address <= base+size-4)
                struct.pack_into('<I', banks[base], address-base, bits(value))
            put(0x4ecbd0, pool); put(pool+0x44, object_rows); put(object_rows+obj*12+8, body)
            put(body, 0x53000000|datum); put(0x4f0414, tags); put(tags+datum*16+8, definition)
            put(body+0x334, elapsed); put(body+0x338, baseline)
            put(definition+0x1d4, limit); put(definition+0x1dc, duration)
            put(0x46a0d0, int.from_bytes(image.read(0x46a0d0, 4), 'little'))
            matrix, source = rng.getrandbits(32), rng.getrandbits(32)
            put(sp, STOP); put(sp+4, matrix); put(sp+8, source)
            gp = [rng.getrandbits(32) for _ in range(8)]; gp[0]=(gp[0]&0xffff0000)|obj;gp[4]=sp
            depth = (0, 1, 4, 8)[mode%4]
            live = [struct.pack('<QH', 0x8000000000000000|(n<<24)|(slot<<32),
                                0x3fff|((slot%2)<<15)).hex() for slot in range(depth)]
            cw = (0x037f, 0x007f, 0x027f, 0x0f7f)[mode%4]
            status = ((n%8)<<11)|(0x4500 if mode%2 else 0)|(1 if mode==4 else 0)
            mxcsr = 0x1f80|((mode%4)<<13)|(0x8040 if mode>=4 else 0)|(4 if mode==4 else 0)
            rows.append(dict(id=n,name=f'{name}-{mode}',entry=ENTRY,gp=gp,eflags=0x202|(n%4)*0x400,
                xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],matrix=matrix,source=source,
                before_fp=[cw,status,mxcsr],live_x87=live,
                banks=[dict(address=base,before=bank.hex()) for base,bank in banks.items()]))
    return rows


def defects():
    def change(path, old, new):
        text = (ROOT/path).read_text()
        if text.count(old) != 1:
            raise ValueError('Effect ratio defect seam changed: '+old)
        return path, text.replace(old, new, 1)
    return {
        'lost-AH-ownership': change(FRAME, 's_.set(0,s_.flags.bits()|2u,1,8);', 's_.gp[0]=s_.flags.bits()|2u;'),
        'skip-live-baseline-reread': change(FRAME, 'const auto baseline=scalar(0xd37d9u,s_.gp[1]+0x338u);',
            'at(0xd37d9u);const auto baseline=s_.xmm[1];'),
        'unordered-is-zero': change(FRAME, 'at(0xd3798u);return !s_.flags.pf;', 'at(0xd3798u);return s_.flags.zf;'),
        'incorrect-duration-field': change(FRAME, 'scalar(0xd37e1u,s_.gp[2]+0x1dcu)', 'scalar(0xd37e1u,s_.gp[2]+0x1d4u)'),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--smoke',action='store_true')
    parser.add_argument('--prefixes',action='store_true')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--mutations',action='store_true')
    parser.add_argument('--fixture-only',action='store_true',help='Keep the original fixture inside the current owner for a linked test')
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):
        return 77
    if not args.xbe.is_file():
        print('SKIP missing owned English TU1.5 executable')
        return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    before={path.resolve():sha(path) for tree in ('src','include','tests','tools') for path in (ROOT/tree).rglob('*')
            if path.suffix in ('.hpp','.h','.cpp','.inc','.def','.py') and path.is_file()}
    deps={ROOT/name for name in (FRAME,KERNEL,OWNER,OBSERVER,TEST,'tools/validate_native_vehicle_effect_ratio_tu15.py')}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[])
    try:
        if sha(args.xbe)!=NEW_PIN:
            raise ValueError('Original TU1.5 image drift')
        image=original.Xbe(args.xbe)
        original.ENTRIES=(ENTRY,); old_metadata=original.metadata
        def metadata(source):
            result=old_metadata(source);code=original._Code(source,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic=='ret':
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        original.metadata=metadata
        body=metadata(image); all_sites=[site['instruction'] for site in body['sites']]
        io_sites={site['instruction'] for site in body['sites'] if site['operands']}
        if len(all_sites)!=34 or len(set(all_sites))!=34:
            raise ValueError('Original D3750 body no longer has exactly 34 sites')
        rows=corpus(image,args.smoke); authorities=[]
        def measure(cases,label):
            folder=scratch/label;folder.mkdir()
            result,authority=physical_original_dense(image,cases,folder,entries=(ENTRY,),regions=REGIONS,
                trace_calls=[dict(entry=pc,gp=True,xmm=True) for pc in all_sites],trace_extended=True,preserve_x87_prefix=True)
            authorities.append(authority)
            return result['cases']
        observed=measure(rows,'normal')
        if args.prefixes:
            prefixes=[]
            for name in ('inactive-zero','initial','saturated','baseline-equal','baseline-active'):
                for mode in ((0,) if args.smoke else (0,3,5)):
                    index=next(n for n,row in enumerate(rows) if row['name']==f'{name}-{mode}')
                    for ordinal in range(len(observed[index]['reads'])):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefixes),
                            name=row['name']+f'-before-read-{ordinal}',fault=dict(phase='before_read',ordinal=ordinal))
                        prefixes.append(row)
            rows+=prefixes;observed+=measure(prefixes,'prefixes')
        for module in tuple(sys.modules.values()):
            if name:=getattr(module,'__file__',None):
                path=Path(name).resolve()
                if path in before and path.is_relative_to(ROOT/'tools'):deps.add(path)
        covered=set()
        for row,result in zip(rows,observed,strict=True):
            calls=result['calls'];covered.update(result['executed_sites'])
            if result['writes'] or result['completed']!=('fault' not in row):
                raise ValueError('Unexpected original effect ratio write/termination '+row['name'])
            for packet in calls:
                if packet.pop('arguments')!=[]:raise ValueError('Childless effect ratio acquired arguments')
            row['expected']=dict(entry=calls[-1]['entry'],gp=result['boundary_gp'],eflags=result['boundary_eflags'],
                xmm=result['boundary_xmm'],fp=result['after_fp'],live_x87=result['boundary_live_x87'])
            row.update(reads=result['reads'],sites=calls,operands=[packet for packet in calls if packet['entry'] in io_sites])
            for bank,actual in zip(row['banks'],result['banks'],strict=True):bank['after']=actual['after']
        if covered!=set(all_sites):
            raise ValueError('Effect ratio corpus missed original sites: '+str(sorted(set(all_sites)-covered)))
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')))
        mutants=defects() if args.mutations else {}
        for compiler in (() if args.fixture_only else args.compiler or ('g++','clang++')):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
                '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(ROOT/TEST),'-o',str(binary),'-MMD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180)
            if built.returncode:raise ValueError('Effect ratio proof compile failed: '+built.stderr[-7000:])
            for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(name).resolve()
                if path.is_relative_to(ROOT):deps.add(path)
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
            if result.returncode:raise ValueError('Effect ratio comparison failed: '+(result.stdout+result.stderr)[-7000:])
            record=dict(compiler=compiler,command=command,result=json.loads(result.stdout),compiled_defects=[])
            report['compilers'].append(record)
            for label,(path,text) in mutants.items():
                overlay=scratch/('defect-'+label)
                target=overlay/path.removeprefix('src/').removeprefix('include/')
                target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text)
                broken=scratch/(Path(compiler).name+'-'+label)
                compile_defect=[command[0],'-I'+str(overlay),*command[1:]]
                compile_defect[compile_defect.index('-o')+1]=str(broken)
                compile_defect[compile_defect.index('-MF')+1]=str(broken.with_suffix('.d'))
                compiled=subprocess.run(compile_defect,cwd=ROOT,capture_output=True,text=True,timeout=180)
                if compiled.returncode:raise ValueError('Effect ratio defect compile failed: '+compiled.stderr[-4000:])
                failed=subprocess.run([str(broken),str(fixture)],capture_output=True,text=True,timeout=120)
                diagnostic=failed.stdout+failed.stderr
                if failed.returncode!=1 or 'effect ratio' not in diagnostic or 'differs' not in diagnostic:
                    raise ValueError('Effect ratio defect survived '+label+': '+diagnostic[-4000:])
                record['compiled_defects'].append(dict(name=label,sha256=sha(target),exit_code=failed.returncode,
                    diagnostic=diagnostic[-1600:]))
        report.update(passed=True,physical_paths=len(rows),source_sites=sorted(hex(pc) for pc in covered),
            xbe_sha256=NEW_PIN,original=authorities,fixture_only=args.fixture_only,fixture_sha256=sha(fixture),
            scope='Private actual D3750. Dense logical x87, masked controls, physical scalar SSE and readonly GP/SIMD/policy providers. Original execution is unchanged; no source/default/parent/other-image qualification.')
    except BaseException as error:
        report['error']=repr(error)
        raise
    finally:
        report['inputs_before']={str(path.relative_to(ROOT)):before.get(path) for path in sorted(deps)}
        report['inputs_after']={str(path.relative_to(ROOT)):sha(path) for path in sorted(deps)}
        if report['inputs_after']!=report['inputs_before']:report.update(passed=False,input_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
