#!/usr/bin/env python3
"""Private126640 prelude and separately declared REP/root continuations."""
import argparse,copy,hashlib,json,os,platform,random,re,shlex,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from validate_native_tu15_prt_helpers import physical_original,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT
REGIONS=((0x4f0000,0x4000),(0x61000000,0x4000),(0x62000000,8192))
UNITS={'src/physics/physics_game_arena_runtime_generated.cpp':'29de6f07a200826b98de37dc58b56b2bd9cd59f81e0c96e169e51de821af0551','src/gameplay/units/death_startup_runtime_generated.cpp':'5eaadb509c75a9fcc46a32d2865c4aeced16d65e5e79221528fa4ee66b2e6dd7'}
INPUTS=('include/halo2/native_tu15_arena_initialize_kernel.hpp','src/physics/runtime/native_tu15_arena_initialize_frame.hpp','tests/native_tu15_arena_initialize_frame_tests.cpp','tools/validate_native_arena_initialize_frame_tu15.py','tools/native_arena_rep_bulk_original.py','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def body(source):
    match=re.search(r'(?m)^.*\bvoid run_125280\([^;\n]*\)\s*\{',source)
    if not match:raise ValueError('Missing complete canonical125280 body')
    depth=1;end=match.end()
    while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]
def corpus():
    rng=random.Random(0x126640);rows=[]
    live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f','01000000000000000000','00000000000000800040','00000000000000000000','0000000000000080ffbf','00000000000000000080')
    for family in range(10):
        for variant in range(16):
            banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];sp=0x62001000+variant;gp=[rng.getrandbits(32)for _ in range(8)]
            if family==5:sp=0x4f0434;gp[1]=(gp[1]&0xffffff00)|(variant&1)
            elif family==6:sp=0x4f042c
            elif family==7:sp=0x4f2960
            elif family==8:sp=0x4f2954
            elif family==9:sp=0x4f0430
            def put(address,value,width=4):
                for(base,size),bank in zip(REGIONS,banks):
                    if base<=address<=base+size-width:bank[address-base:address-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
                raise ValueError('Arena input lacks declared backing')
            def get(address):
                for(base,size),bank in zip(REGIONS,banks):
                    if base<=address<=base+size-4:return int.from_bytes(bank[address-base:address-base+4],'little')
                raise ValueError('Arena caller word is unbacked')
            put(0x4f0430,(0,1,2,128,255,0,1,0,1,0)[family],1);put(sp,0x63000000);gp[4]=sp
            depth=(0,1,4,8)[variant%4];cw=(0x7f,0x27f,0x37f,0xf7f)[variant%4]
            rows.append(dict(id=len(rows),name=f'family-{family}-variant-{variant}',entry=0x126640,gp=gp,eflags=0x202|(0x400 if variant&1 else 0)|rng.choice((0,1,0x10,0x40,0x80,0x800,0x8d5)),matrix=get(sp+4),source=get(sp+8),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,((variant%8)<<11)|0x4500,0x1f80|((variant%4)<<13)],live_x87=list(live[:depth]),stop_before_entries=[0x21b920],max_steps=100,banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)]))
    return rows
def rep_corpus(source_rows):
    rows=[]
    for count in(1,2,3,17,256,1024):
        for variant in range(16):
            row=copy.deepcopy(source_rows[len(rows)%len(source_rows)]);row.update(id=len(rows),name=f'rep-{count}-variant-{variant}',phase='rep',entry=0x126678,stop_before_entries=[0x12667a],max_steps=count+10)
            row['gp'][0]=0;row['gp'][1]=count;row['gp'][7]=(0x61003000 if row['eflags']&0x400 else 0x61001000)+variant
            rows.append(row)
    return rows
def bulk_corpus(source_rows):
    rows=[];rng=random.Random(0x126678f3)
    for alignment in(0,1,15):
        for direction in(0,1):
            row=copy.deepcopy(source_rows[len(rows)]);row.update(id=len(rows),name=f'full-rep-align{alignment}-df{direction}',phase='bulk_rep',entry=0x126678,stop_before_entries=[0x21b990],bulk_repeat_to=0x12667a,max_steps=20)
            row['gp'][0]=0;row['gp'][1]=0xff800;row['gp'][7]=0x61000000+alignment+(0x3fe000-4 if direction else 0)
            row['eflags']=(row['eflags']&~0x400)|(direction<<10)
            row['banks'][1]['before']=rng.randbytes(0x400000).hex();rows.append(row)
    return rows
def tail_corpus(source_rows):
    rows=[]
    for charge in(0,1,4,0xffffffff,0xb40bbe37,0x5a05df1b,0x2d02ef8d):
        for variant in range(16):
            row=copy.deepcopy(source_rows[variant]);row.update(id=len(rows),name=f'tail-charge{charge:x}-variant{variant}',phase='tail',entry=0x1266b2,stop_before_entries=[0x166a80],max_steps=100)
            row['gp'][7]=charge;row['gp'][6]=0x61000800+variant
            for bank in row['banks']:
                if bank['address']==0x4f0000:
                    data=bytearray.fromhex(bank['before']);struct.pack_into('<I',data,0x2950,0x61001000);struct.pack_into('<I',data,0x2954,(0,0x1288,0xfffffff0,0x7fffffff)[variant%4]);bank['before']=data.hex()
            rows.append(row)
    return rows
def final_corpus(source_rows):
    rows=[]
    for family in range(6):
        for variant in range(16):
            row=copy.deepcopy(source_rows[variant]);sp=0x62001000+variant
            row.update(id=len(rows),name=f'final-root{family}-variant{variant}',phase='final',entry=0x1266e6,max_steps=30)
            row.pop('stop_before_entries');row['gp'][4]=sp;row['gp'][6]=(0,0x61001000+variant,sp,sp+4,sp+8,0x51df2c)[family]
            row['banks'].append(dict(address=0x51d000,before=(bytes([0x5a])*8192).hex()))
            for bank in row['banks']:
                if bank['address']==0x62000000:
                    data=bytearray.fromhex(bank['before']);struct.pack_into('<I',data,sp-0x62000000+12,0x63000000);bank['before']=data.hex()
            rows.append(row)
    return rows
def middle_corpus(source_rows):
    rows=[]
    for variant in range(32):
        row=copy.deepcopy(source_rows[variant]);row.update(id=len(rows),name=f'reservation-return-variant{variant}',phase='reservation',entry=0x126669,stop_before_entries=[0x126678],max_steps=20)
        row['gp'][0]=(0,1,0x61001000,0xfffffffc)[variant%4]+variant//4;rows.append(row)
    for family in range(6):
        for variant in range(16):
            row=copy.deepcopy(source_rows[variant]);sp=(0x62001000+variant,0x4f2954,0x4f2958,0x4f2960,0x4f0434,0x4f2964)[family]
            row.update(id=len(rows),name=f'metadata-return-family{family}-variant{variant}',phase='metadata',entry=0x12667f,stop_before_entries=[0x166a80],max_steps=40)
            row['gp'][4]=sp
            def put(address,value):
                for bank in row['banks']:
                    offset=address-bank['address'];data=bytearray.fromhex(bank['before'])
                    if 0<=offset<=len(data)-4:struct.pack_into('<I',data,offset,value&0xffffffff);bank['before']=data.hex();return
                raise ValueError('Middle continuation word is unbacked')
            put(0x4f2950,0x61001000);put(0x4f2954,(0,0x1288,0xfffffff0,0x7fffffff)[variant%4])
            put(sp,0x63000000);put(sp+4,row['matrix']);put(sp+8,row['source']);rows.append(row)
    return rows
def crc_table_alias_proof(image,scratch,source_rows,metadata):
    # This is an independently declared genuine child entry. Its returned DI
    # is not injected as the outcome of the outer CRC update or arena caller.
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=(0x166ae0,);sites=metadata(image)['sites'];code=common._Code(image,'.text')
    table=0x56e258;regions=(*REGIONS,(0x56e000,4096));rows=[]
    for saved_index in(None,254,255):
        for variant in range(16):
            row=copy.deepcopy(source_rows[variant]);row.update(id=len(rows),name=f'crc-table-savedDI-{saved_index}-variant{variant}',entry=0x166ae0,max_steps=30000)
            row.pop('stop_before_entries');row['banks'].append(dict(address=0x56e000,before=(bytes([0xa5])*4096).hex()))
            row['gp'][2]=table;row['gp'][4]=0x62001000+variant if saved_index is None else table+saved_index*4+8
            row['saved_index']=saved_index;row['matrix']=0x11223344;row['source']=0x55667788
            for bank in row['banks']:
                base=bank['address'];data=bytearray.fromhex(bank['before']);offset=row['gp'][4]-base
                if 0<=offset<=len(data)-12:struct.pack_into('<III',data,offset,0x63000000,row['matrix'],row['source']);bank['before']=data.hex()
            rows.append(row)
    io=[dict(entry=site['instruction'],gp=True,xmm=True)for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')]
    folder=scratch/'crc-table-original';folder.mkdir();original,authority=physical_original(image,rows,folder,entries=(0x166ae0,),regions=regions,trace_calls=io,trace_extended=True,preserve_x87_prefix=True)
    polynomials=[]
    for byte in range(256):
        remainder=byte
        for _ in range(8):remainder=(remainder>>1)^((remainder&1)*0xedb88320)
        polynomials.append(remainder)
    records=[]
    for row,observed in zip(rows,original['cases'],strict=True):
        if not observed['completed']:raise ValueError('Genuine CRC table child failed to return')
        index=row['saved_index'];expected=row['gp'][7]if index is None else polynomials[index]
        if observed['boundary_gp'][7]!=expected:raise ValueError('Actual saved-DI alias return differs')
        table_bank=next(bank for bank in observed['banks']if bank['address']==0x56e000);after=bytes.fromhex(table_bank['after']);actual=list(struct.unpack_from('<256I',after,table-0x56e000))
        if actual!=polynomials:raise ValueError('Actual table bytes differ from reflected polynomial definition')
        if len(observed['writes'])!=258:raise ValueError('Actual CRC table store count differs')
        records.append(dict(saved_DI_table_index=index,input_DI=row['gp'][7],returned_DI=observed['boundary_gp'][7],returned_SI=observed['boundary_gp'][6],returned_SP=observed['boundary_gp'][4],complete_returned_gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],table_sha256=hashlib.sha256(after[table-0x56e000:table-0x56e000+1024]).hexdigest()))
    return dict(scope='Independent genuine166AE0 whole-function returns with its saved DI aliased to actual writable table words254/255. No full166A80 alias return or arena child outcome is supplied/claimed.',physical_paths=len(rows),physical_authority=authority,covered_sites=[hex(pc)for pc in sorted({pc for observed in original['cases']for pc in observed['executed_sites']})],pre_io_packets=sum(len(observed['calls'])for observed in original['cases']),alias_returns=sum(row['saved_index']is not None for row in rows),records=records)
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--rep-only',action='store_true');parser.add_argument('--bulk-only',action='store_true');parser.add_argument('--tail-only',action='store_true');parser.add_argument('--middle-only',action='store_true');parser.add_argument('--final-only',action='store_true');parser.add_argument('--all-phases',action='store_true');parser.add_argument('--crc-table-alias-only',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');args=parser.parse_args()
    if sum((args.rep_only,args.bulk_only,args.tail_only,args.middle_only,args.final_only,args.all_phases))>1 or(args.bulk_only and args.prefixes):parser.error('Each declared-entry/aggregate scope runs separately')
    if args.semantic_mutants and not(args.tail_only or args.all_phases):parser.error('The live-charge mutation requires the declared root continuation')
    if args.crc_table_alias_only and(args.rep_only or args.bulk_only or args.tail_only or args.middle_only or args.final_only or args.all_phases or args.prefixes or args.semantic_mutants):parser.error('The genuine CRC table entry is a separate scope')
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={p:sha(ROOT/p)for p in (*INPUTS,*UNITS)}
    headers={p.resolve():sha(p)for directory in('include','src')for p in(ROOT/directory).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    scope=('Separately declared original entry1266B2 through actual166A80 child-entry stop, with liveDI root charge and physical instruction-prefix exceptions; no supplied CRC result or whole-CRC alias return claim.'if args.tail_only else 'Separate declared REP entry/count continuations or initialized full returns/genuine21B920 entry prefixes; full cold allocation/savegame/CRC/root construction and production admission remain open.')
    if args.final_only:scope='Separately declared original entry1266E6 after actual second-CRC return, complete root/global/vtable publication and real saved-frame/RET reads. No CRC result is supplied to a whole parent.'
    if args.middle_only:scope='Separately declared original entries126669 and12667F: actual reservation-resume publication through REP entry, and metadata acquisition/accounting through genuine166A80 entry. No child outcome is supplied to a whole parent.'
    if args.all_phases:scope='Union of genuine initialized returns/actual first-child stops and separately declared reservation, REP, metadata, post-first-CRC and post-second-CRC entries. Every own opcode can be covered without supplying any whole-parent child outcome; complete cold real-owner returns are a separate composition proof.'
    result=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[],scope=scope)
    try:
        import native_arena_initialize_replacements as binding
        pins['tools/native_arena_initialize_replacements.py']=sha(ROOT/'tools/native_arena_initialize_replacements.py')
        for unit,pin in UNITS.items():
            source=(ROOT/unit).read_text()
            if unit=='src/physics/physics_game_arena_runtime_generated.cpp':source=binding.remove_arena_initialize(source)
            if hashlib.sha256(body(source).encode()).hexdigest()!=pin:raise ValueError('Original complete arena body pin differs: '+unit)
        if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
        sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
        image=Xbe(args.xbe);import oracle_native_campaign_screen_lifecycle_tu15 as common
        original_metadata=common.metadata
        def metadata(image):
            data=original_metadata(image);code=common._Code(image,'.text')
            for site in data['sites']:
                if code.at(site['instruction']).mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return data
        common.metadata=metadata
        if args.crc_table_alias_only:
            result.update(crc_table_alias_proof(image,scratch,corpus(),metadata),passed=True)
            return 0
        common.ENTRIES=(0x126640,);sites=metadata(image)['sites'];code=common._Code(image,'.text')
        io=[site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')]
        writes=[site['instruction']for site in sites if any(op['write']for op in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
        normal=corpus();rows=middle_corpus(normal)if args.middle_only else final_corpus(normal)if args.final_only else tail_corpus(normal)if args.tail_only else bulk_corpus(normal)if args.bulk_only else rep_corpus(normal)if args.rep_only else normal;folder=scratch/'original';folder.mkdir()
        regions=((0x4f0000,0x4000),(0x61000000,0x400000),(0x62000000,8192))if args.bulk_only else(*REGIONS,(0x51d000,8192))if args.final_only else REGIONS
        if args.all_phases:
            rows=normal+middle_corpus(normal)+tail_corpus(normal)+final_corpus(normal)+rep_corpus(normal);regions=(*REGIONS,(0x51d000,8192))
            for variant in range(16):
                row=copy.deepcopy(normal[variant]);row.update(name=f'savegame-call-entry-variant{variant}',phase='savegame',entry=0x12667a,stop_before_entries=[0x21b990],max_steps=5);rows.append(row)
            for index,row in enumerate(rows):
                row['id']=index
                if not any(bank['address']==0x51d000 for bank in row['banks']):row['banks'].append(dict(address=0x51d000,before=(bytes([0x5a])*8192).hex()))
        if args.bulk_only:
            from native_arena_rep_bulk_original import physical_bulk_rep
            original,authority=physical_bulk_rep(image,rows,folder,regions)
        else:original,authority=physical_original(image,rows,folder,entries=(0x126640,),regions=regions,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
        prefix_authority=None
        if args.prefixes:
            prefixes=[]
            for index in ((0,1,15,48,49,63,80,81,95)if args.rep_only else(0,1,16,17,80,81,96,97,111)if args.tail_only else(0,1,16,17,32,33,64,65,80,81,95)if args.final_only else(0,1,16,17,32,33,48,49,64,65,96,97,112,113,127)if args.middle_only else(0,1,16,17,80,81,112,113,160,161,192,193,224,225,256,257,287,288,289,304,305,368,369,384,385,399,400,401,416,417,432,433,464,465,480,481,495,496,497,512,513,544,545,576,577,591)if args.all_phases else(0,1,16,17,80,81,112,113)):
                count=len(original['cases'][index]['writes'])
                for ordinal in sorted({0,count//2,count-1}):
                    if ordinal<0:continue
                    for phase in('before_write','after_write'):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefixes),name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal));prefixes.append(row)
            folder=scratch/'prefix';folder.mkdir()
            observed,prefix_authority=physical_original(image,prefixes,folder,entries=(0x126640,),regions=regions,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
            rows+=prefixes;original['cases']+=observed['cases']
        for row,observed in zip(rows,original['cases'],strict=True):
            if not observed['completed']and 'fault'not in row:raise ValueError('Original did not reach declared return/child boundary')
            row.update({key:observed[key]for key in('calls','boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes')})
            row['child_stop']='fault'not in row and observed.get('stopped_at')in(0x21b920,0x21b990,0x166a80)
            row['expected_pc']=observed['boundary']['instruction']if'fault'in row else 0x126678 if row.get('phase')in('rep','reservation')else observed['stopped_at']if row['child_stop']else 0x1266f9
            for before,after in zip(row['banks'],observed['banks'],strict=True):before['after']=after['after']
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows,write_sites=writes),separators=(',',':')))
        compilers=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_arena_initialize_frame_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Arena private compile failed: '+done.stderr[-8000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path.is_relative_to(ROOT)and path.is_file():pins[str(path.relative_to(ROOT))]=headers.get(path,sha(path))
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
            if done.returncode:raise ValueError('Arena private native mismatch: '+(done.stdout+done.stderr)[-9000:])
            entry=dict(compiler=compiler,command=command,result=json.loads(done.stdout))
            if args.semantic_mutants:
                overlay=scratch/(Path(compiler).name+'-stale-charge');header=overlay/'halo2/native_tu15_arena_initialize_kernel.hpp';header.parent.mkdir(parents=True)
                original_header=(ROOT/'include/halo2/native_tu15_arena_initialize_kernel.hpp').read_text();needle='const auto charge=view.root_charge_bytes();'
                if original_header.count(needle)!=1:raise ValueError('Live-charge mutation anchor differs')
                mutated=original_header.replace(needle,'const auto charge=root_bytes;');header.write_text(mutated)
                mutant=scratch/(Path(compiler).name+'-stale-charge-mutant');mutant_command=command.copy();mutant_command[1:1]=['-I'+str(overlay)];mutant_command[mutant_command.index('-o')+1]=str(mutant);mutant_command[mutant_command.index('-MF')+1]=str(mutant.with_suffix('.d'))
                compiled=subprocess.run(mutant_command,capture_output=True,text=True)
                if compiled.returncode:raise ValueError('Stale-charge mutant failed to compile: '+compiled.stderr[-4000:])
                rejected=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True);diagnostic=rejected.stdout+rejected.stderr
                if rejected.returncode==0 or 'actual read/store sequence differs'not in diagnostic:raise ValueError('Stale fixed-four root charge was not rejected semantically: '+diagnostic[-4000:])
                entry['compiled_mutants']=[dict(name='stale-fixed-four-root-charge',rejected=True,exit_code=rejected.returncode,diagnostic=diagnostic[:4000],mutated_header_sha256=sha(header))]
            compilers.append(entry)
        covered=sorted({pc for row in original['cases']for pc in row['executed_sites']})
        if args.all_phases and set(covered)!={site['instruction']for site in sites}:
            result.update(compilers=compilers,covered_parent_sites=[hex(pc)for pc in covered])
            raise ValueError('Declared phase union coverage differs: missing='+repr([hex(pc)for pc in sorted({site['instruction']for site in sites}-set(covered))]))
        result.update(passed=True,physical_paths=len(rows),normal_paths=sum('fault'not in row for row in rows),prefix_paths=sum('fault'in row for row in rows),physical_authority=authority,physical_prefix_authority=prefix_authority,parent_sites=len(sites),covered_parent_sites=[hex(pc)for pc in covered],covered_parent_site_count=len(covered),compilers=compilers,complete_body_pins=UNITS)
    except BaseException as error:result['error']=repr(error);raise
    finally:
        result['source_after']={p:sha(ROOT/p)for p in pins}
        if pins!=result['source_after']:result.update(passed=False,source_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(result,indent=2)+'\n')
    return 0 if result['passed']else 1
if __name__=='__main__':raise SystemExit(main())
