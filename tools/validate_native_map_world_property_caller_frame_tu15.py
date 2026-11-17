#!/usr/bin/env python3
"""Compare the private 11E490/1DB800 composition with unchanged TU1.5 i386."""
import argparse,copy,hashlib,json,os,platform,shlex,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from validate_native_projectile_function_query_frame_tu15 import boundary_packet
from validate_native_tu15_prt_helpers import PIN,physical_original
from validate_native_map_world_property_query_frame_tu15 import corpus as leaf_inputs

ROOT=Path(__file__).resolve().parents[1]
FRAME='src/gameplay/objects/native_tu15_map_world_property_caller_frame.hpp'
TEST='tests/native_tu15_map_world_property_caller_frame_tests.cpp'
ENTRY=0x11e490
CHILD=0x1db800
REGIONS=((0x46a000,4096),(0x4ec000,4096),(0x52b000,4096),(0x61000000,16384),(0x62000000,4096))

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def corpus(image):
    rows=leaf_inputs(image)
    for n,row in enumerate(rows):
        banks={bank['address']:bytearray.fromhex(bank['before'])for bank in row['banks']}
        for base,size in REGIONS:banks.setdefault(base,bytearray(size))
        def get(address):
            for base,data in banks.items():
                if base<=address<=base+len(data)-4:return int.from_bytes(data[address-base:address-base+4],'little')
            raise ValueError('Caller seed address escaped bank')
        def put(address,value):
            for base,data in banks.items():
                if base<=address<=base+len(data)-4:
                    data[address-base:address-base+4]=(value&0xffffffff).to_bytes(4,'little');return
            raise ValueError('Caller fixture address escaped bank')
        gp=row['gp'];sp=gp[4];record=gp[6];key=gp[1]
        first=gp[2];second=get(sp+4)
        # The child reads its byte-output pointer from the caller's pushed
        # copy after publishing the scalar, not from the caller argument.
        if first==sp+4:first=sp-12
        datum_index=(0,1,31,65535)[n%4];attached_index=(0,2,127,65535)[n//4%4]
        datum=0x91280000|datum_index
        attached=0xffffffff if n%7==6 else 0x84cd0000|attached_index
        put(0x4ecbd0,0x61000100);put(0x61000144,0x61000300-datum_index*12)
        put(0x61000308,0x61000400);put(0x610004b4,attached)
        put(0x52be5c,0x61000200);put(0x61000244,record-attached_index*160)
        put(sp,0x63000000);put(sp+4,datum);put(sp+8,key);put(sp+12,first);put(sp+16,second)
        row.update(entry=ENTRY,matrix=datum,source=key,name=f'map-world-caller-{n}',max_steps=120)
        row['banks']=[dict(address=base,before=banks[base].hex())for base,_ in REGIONS]
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--prefixes',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=(FRAME,TEST,'include/halo2/native_tu15_map_world_property_caller_kernel.hpp',
        'tools/validate_native_map_world_property_caller_frame_tu15.py','tools/validate_native_map_world_property_query_frame_tu15.py',
        'tools/validate_native_projectile_function_query_frame_tu15.py',
        'tools/validate_native_tu15_prt_helpers.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py',
        'tools/observe_native_campaign_ai_vector_math_tu15.cpp')
    pins={name:sha(ROOT/name) for name in names}
    headers={path.resolve():sha(path) for tree in ('include','src','tests') for path in (ROOT/tree).rglob('*') if path.suffix in ('.hpp','.h','.inc')}
    report=dict(passed=False,production_bound=False,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        image=Xbe(a.xbe);code=common._Code(image,'.text');initial=common.metadata
        def metadata(image):
            result=initial(image)
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in ('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;common.ENTRIES=(ENTRY,CHILD)
        sites=metadata(image)['sites']
        calls=[site['instruction']for site in sites if code.at(site['instruction']).mnemonic=='call']
        if len(sites)!=64 or calls!=[0x11e4da] or code.at(0x11e4da).operands[0].imm!=CHILD:
            raise ValueError('Original 11E490 to 1DB800 closure changed')
        traces=[dict(entry=site['instruction'],arguments=0,gp=True,xmm=True) for site in sites if site['operands'] or site['instruction']==CHILD or
                code.at(site['instruction']).mnemonic in ('push','call')]
        rows=corpus(image);normal=len(rows)
        def observe(input_rows,folder):
            return physical_original(image,input_rows,folder,entries=(ENTRY,CHILD),regions=REGIONS,
                trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        observed,authority=observe(rows,scratch);report['authority']=authority
        if a.prefixes:
            stopped=[]
            for row,actual in zip(rows[:30],observed['cases'][:30],strict=True):
                for phase,key in (('before_read','reads'),('before_write','writes')):
                    for ordinal in range(len(actual[key])):
                        candidate=copy.deepcopy(row);candidate.update(id=len(stopped),name=row['name']+f'-{phase}-{ordinal}',
                            fault=dict(phase=phase,ordinal=ordinal));stopped.append(candidate)
            folder=scratch/'provider-prefixes';folder.mkdir()
            prefix,receipt=observe(stopped,folder);rows+=stopped;observed['cases']+=prefix['cases'];report['prefix_authority']=receipt
        coverage=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            if actual['completed']!=('fault' not in row):raise ValueError('Original completion changed '+row['name'])
            coverage.update(actual['executed_sites'])
            for packet in actual['calls']:
                if packet.pop('arguments')!=[]:raise ValueError('Unexpected original call arguments')
            pc=actual['boundary']['instruction'] if 'fault'in row else actual['calls'][-1]['entry']
            row.update(expected=boundary_packet(actual,row,pc),reads=actual['reads'],writes=actual['writes'],calls=actual['calls'])
            for bank,after in zip(row['banks'],actual['banks'],strict=True):bank['after']=after['after']
        if coverage!={site['instruction'] for site in sites}:raise ValueError('Original 11E490/1DB800 site coverage incomplete')
        report.update(original_sites=len(sites),covered_sites=len(coverage),original_returns=normal,
                      original_provider_prefixes=len(rows)-normal,genuine_child_entries=sum(
                          packet['entry']==CHILD for row in rows for packet in row['calls']))
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(rows,separators=(',',':')));report['compilers']=[]
        for compiler in a.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                *('-I'+str(ROOT/tree) for tree in ('include','src','tests')),str(ROOT/TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True,timeout=600)
            if built.returncode:raise ValueError('MapWorld query compile failed: '+built.stderr[-7000:])
            for item in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                path=Path(item).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=1200)
            if result.returncode:raise ValueError('MapWorld query comparison failed: '+(result.stdout+result.stderr)[-9000:])
            report['compilers'].append(dict(compiler=compiler,command=command,result=json.loads(result.stdout)))
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name) for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
