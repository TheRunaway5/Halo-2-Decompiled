#!/usr/bin/env python3
"""Independent physical2EA380 with genuine native/original array-pool growth.

All fixtures are declared ordinary DATA over authenticated original allocator
before-inputs. No allocator/geometry result, GP image or after-state is supplied
to the native algorithm. Run through run_test.py; all large outputs are owned.
"""
import argparse,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as array
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

STAGE=ROOT/'analysis/physics/native-broad-phase-edges-stage-20261102/staged'
UI=array.UI;BROAD=UI+0x100;NODES=UI+0x1000
EDGES=[UI+0x2000,UI+0x3000,UI+0x4000]
HANDLES,RECORDS,BOUNDS=UI+0x5000,UI+0x5100,UI+0x5200
CREATED,REMOVED,CREATE_DATA,REMOVE_DATA=UI+0x6000,UI+0x6020,UI+0x6100,UI+0x6800
SPECIAL,SPECIAL_DATA=UI+0x7000,UI+0x7400
FIELDS=((8,10),(0,4),(2,6))
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def quant(v,upper=False):
    n=(bits(65536.+min(65532.,max(0.,v)))>>7)&0xffff
    return n|1 if upper else n&0xfffe

def cases():
    rows=[]
    def make(name,initial,target,axis=None,special=False,capacity=16,other_gap=False,fp=None,w=None):
        old=[[[10.,20.]for _ in range(3)],[[10.,20.]for _ in range(3)]]
        new=copy.deepcopy(old)
        if axis is None:
            for a in range(3):old[0][a]=initial[0][:];old[1][a]=initial[1][:];new[0][a]=target[:]
        else:
            old[0][axis]=initial[0][:];old[1][axis]=initial[1][:];new[0]=copy.deepcopy(old[0]);new[0][axis]=target[:]
        if other_gap:
            a=(axis+1)%3;old[0][a]=[50.,60.];new[0][a]=[50.,60.]
        words={}
        def put(p,v,n=4):
            p-=UI
            for k in range(n):words[p+k]=(v>>(8*k))&255
        for off in(0x10,0x20):
            for lane in range(4):put(BROAD+off+lane*4,0)
        for lane in range(4):put(BROAD+0x30+lane*4,bits(1.))
        put(BROAD+0x40,NODES);put(BROAD+0x78,SPECIAL)
        for a,(lo,hi)in enumerate(FIELDS):
            edges=[(0,0),(65535,0)]
            for node,box in enumerate(old,1):edges.extend([(quant(box[a][0]),node),(quant(box[a][1],True),node)])
            edges.sort();put(BROAD+(0x4c,0x58,0x64)[a],EDGES[a])
            for i,(value,node)in enumerate(edges):
                put(EDGES[a]+i*4,value|(node<<16))
                field=hi if value&1 else lo;put(NODES+node*16+field,i,2)
        put(NODES+16+12,0x11223344)
        put(NODES+32+12,1 if special else 0x55667788)
        put(HANDLES,RECORDS);put(RECORDS,1)
        for lane in range(4):
            put(BOUNDS+lane*4,bits(new[0][lane][0])if lane<3 else(0 if w is None else w))
            put(BOUNDS+16+lane*4,bits(new[0][lane][1])if lane<3 else(0 if w is None else w))
        for header,data in((CREATED,CREATE_DATA),(REMOVED,REMOVE_DATA)):
            put(header,data);put(header+4,0);put(header+8,0x80000000|capacity)
        overlap=all(old[0][a][0]<=old[1][a][1]and old[1][a][0]<=old[0][a][1]for a in range(3))
        put(SPECIAL+4,SPECIAL_DATA);put(SPECIAL+8,int(overlap));put(SPECIAL+12,0x80000000|capacity)
        put(SPECIAL_DATA,1,2)
        packed={}
        for p,v in words.items():packed[p&~3]=packed.get(p&~3,0)|(v<<((p&3)*8))
        c=dict(name=name,entry=0x2ea380,receiver=BROAD,arguments=[HANDLES,BOUNDS,1,CREATED,REMOVED],broad=BROAD,handles=HANDLES,bounds=BOUNDS,edge_count=1,created=CREATED,removed=REMOVED,user_words=sorted(packed.items()))
        if fp is not None:c['before_fp']=fp
        rows.append(c)
    motions=[('lower_left',([30.,40.],[10.,20.]),[5.,40.]),('upper_right',([10.,15.],[20.,30.]),[10.,25.]),('upper_left',([10.,40.],[20.,30.]),[10.,15.]),('lower_right',([5.,15.],[10.,20.]),[25.,35.])]
    for axis in range(3):
        for name,old,new in motions:
            for cap in(0,16):make(f'axis{axis}_{name}_cap{cap}',old,new,axis=axis,capacity=cap)
            make(f'axis{axis}_{name}_separated',old,new,axis=axis,other_gap=True)
    for name,old,new in motions:
        for cap in(0,16):make(f'x_special_{name}_cap{cap}',old,new,axis=0,special=True,capacity=cap)
    make('stationary',([10.,20.],[15.,25.]),[10.,20.])
    make('all_axes_expand',([40.,50.],[20.,30.]),[5.,60.],capacity=0)
    make('all_axes_shrink',([10.,40.],[20.,30.]),[10.,15.],capacity=0)
    for rounding in range(4):
        for top in range(8):
            make(f'fp_round{rounding}_top{top}',([10.,15.],[20.,30.]),[10.000001,25.000001],axis=rounding%3,fp=[0x27f|rounding<<10,(top<<11)|0x320,0x1f80|rounding<<13])
    for w in(0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'fourth_lane_{w:x}',([10.,15.],[20.,30.]),[10.,25.],axis=0,w=w)
    for count in(0,0xffffffff,0x80000000):
        c=copy.deepcopy(rows[0]);c['name']=f'count_skip_{count:x}';c['arguments'][2]=count;c['edge_count']=count;rows.append(c)
    for direction in ('upper_left','lower_right'):
        seed=next(x for x in rows if x['name']==f'x_special_{direction}_cap16')
        for variant,values in [('scan_found',(2,1)),('scan_absent',(2,3)),('empty',()),('negative_count',())]:
            c=copy.deepcopy(seed);c['name']=f'special_{direction}_{variant}';w=dict(c['user_words']);w[SPECIAL+8-UI]=0x80000001 if variant=='negative_count'else len(values)
            if values:w[SPECIAL_DATA-UI]=values[0]|(values[1]<<16)
            c['user_words']=sorted(w.items());rows.append(c)
    for direction in ('lower_left','upper_right'):
        c=copy.deepcopy(next(x for x in rows if x['name']==f'axis0_{direction}_cap16'));c['name']='two_handles_'+direction;c['arguments'][2]=2;c['edge_count']=2;w=dict(c['user_words']);w[HANDLES+4-UI]=RECORDS+4;w[RECORDS+4-UI]=1
        for offset in range(0,32,4):w[BOUNDS+32+offset-UI]=w[BOUNDS+offset-UI]
        c['user_words']=sorted(w.items());rows.append(c)
    return rows

def original(image,banks,rows,scratch):
    array.OWNERS=tuple(dict.fromkeys((0x2ea380,*array.OWNERS)))
    text=inspect.getsource(array.original).replace('observe_native_array_growth_tu15.cpp','observe_native_broad_phase_edges_tu15.cpp')
    text=text.replace("index=all_cases.index(case);fp=[0x27f|((index%4)<<10),((index%8)<<11)|((index%4)<<8)|(index%32),0x1f80|((index%4)<<13)]", "index=all_cases.index(case);fp=case.get('before_fp',[0x27f|((index%4)<<10),((index%8)<<11)|((index%4)<<8)|(index%32),0x1f80|((index%4)<<13)])")
    text=text.replace("('fault','mutations')", "('fault','mutations','readonly_range','after_store')")
    # Actual Linux mprotect makes the original INC/DEC perform its read phase
    # and fault at the hardware write. No preinstruction surrogate for RMW.
    text=text.replace("prepare=''", "prepare=''\n        if 'readonly_range'in case:\n            r=case['readonly_range'];prepare=f'mov $125,%eax\\n mov ${r[\"address\"]},%ebx\\n mov ${r[\"bytes\"]},%ecx\\n mov $1,%edx\\n int $0x80\\n test %eax,%eax\\n jne 0\\n'")
    namespace=dict(array.__dict__);exec(text,namespace)
    return namespace['original'](image,banks,rows,scratch)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report={'passed':False,'owned_scratch':str(scratch),'retained_large_artifacts':[],'allocator_outcomes_supplied':False}
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,provenance=array.reference_inputs(image);banks[0x41c000]=image.read(0x41c000,0x1000)
        rows=cases();fixture_dir,authority=original(image,banks,rows,scratch)
        baseline=len(rows);code=_Code(image,'.text');extras=[];seen_reads=set();seen_writes=set();seen_post=set()
        def clean(c):return {k:copy.deepcopy(v) for k,v in c.items() if k not in ('banks','reads','writes','executed_sites','allocator_calls','after_fp','original_instructions','return_word','declared_fault','poststore_stop','mutations_applied')}
        for c in rows[:baseline]:
            occurrences={}
            for r in c['reads']:
                key=(r['address'],r['bytes']);occurrences[key]=occurrences.get(key,0)+1
                if r['instruction'] in seen_reads:continue
                seen_reads.add(r['instruction']);x=clean(c);x['name']=f'read_fault_{r["instruction"]:x}';x['fault']=dict(address=r['address'],bytes=r['bytes'],occurrence=occurrences[key],write=False);extras.append(x)
            occurrences={}
            for w in c['writes']:
                key=(w['address'],len(w['written'])//2);occurrences[key]=occurrences.get(key,0)+1
                pc=w['instruction'];ins=code.at(pc)
                if pc not in seen_post:
                    seen_post.add(pc);x=clean(c);x['name']=f'poststore_{pc:x}';x['after_store']=dict(instruction=pc,occurrence=1);extras.append(x)
                if pc in seen_writes or ins.mnemonic not in ('mov','movaps','movups'):continue
                seen_writes.add(pc);x=clean(c);x['name']=f'write_fault_{pc:x}';x['fault']=dict(address=w['address'],bytes=key[1],occurrence=occurrences[key],write=True);extras.append(x)
        for c in rows[:baseline]:
            if c['name'].startswith('axis')and c['name'].endswith('cap16')and any(n in c['name']for n in ('upper_right','upper_left')):
                x=clean(c);x['name']='hardware_readonly_'+c['name'];x['readonly_range']=dict(address=NODES,bytes=4096);extras.append(x)
        # Publication callbacks modify DATA only after a genuine source store.
        for name,pc,address,value in [('pair_payload_after_count',0x2ea662,NODES+28,0xabcdef00),('growth_header_count',0x23328f,CREATED+4,0),('special_reread_count',0x2ea6a1,SPECIAL+8,3)]:
            seed=next(c for c in rows if c['name']==('x_special_upper_right_cap16' if name.startswith('special') else 'axis0_upper_right_cap0'))
            x=clean(seed);x['name']='mutation_'+name;x['mutations']=[dict(instruction=pc,address=address,written=struct.pack('<I',value).hex())];extras.append(x)
        extra_root=scratch/'prefixes';extra_root.mkdir();extra_fixture,_=original(image,banks,extras,extra_root)
        for c in extras:
            for b in c['banks']:
                for k in ('before_file','after_file'):
                    source=extra_fixture/b[k];target=fixture_dir/b[k]
                    if not target.exists():shutil.copyfile(source,target)
        rows+=extras;fixture=fixture_dir/'original.json';fixture.write_text(json.dumps({'image_sha256':NEW_PIN,'oracle':'unchanged-tu15-broad-phase-edges-real-growth-v1','cases':rows},separators=(',',':')))
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        sources=['src/physics/native_tu15_broad_phase_edges.cpp','src/physics/native_tu15_array_growth.cpp','src/physics/native_tu15_array_allocator.cpp']
        files={}
        for name in ['include/halo2/native_tu15_broad_phase_edges.hpp',*sources]:
            src=STAGE/name if(STAGE/name).exists()else ROOT/name;out=project/name;out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,out);files[str(src.relative_to(ROOT))]=sha(src)
        test=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text().replace('#include "halo2/native_tu15_array_allocator.hpp"','#include "halo2/native_tu15_array_allocator.hpp"\n#include "halo2/native_tu15_broad_phase_edges.hpp"').replace('unchanged-tu15-physical-array-and-real-pool-v1','unchanged-tu15-broad-phase-edges-real-growth-v1')
        # The integer-only parent fixture has no SIMD override. Preserve each
        # authentic MOVAPS read as one architectural16B access in this model.
        needle='  void write(std::uint32_t p,std::span<const std::byte>in)override'
        assert test.count(needle)==1
        test=test.replace(needle,'  void read_vector(std::uint32_t p,std::span<std::byte,16>out)const override{read(p,out);}\n'+needle)
        needle='if(entry==0x2e3250u)native_tu15_grow_array(array,args[1],fp,growth,&activity);';assert test.count(needle)==1
        test=test.replace(needle,'if(entry==0x2ea380u)native_tu15_update_broad_phase_edges(memory,{c.at("broad"),c.at("handles"),c.at("bounds"),c.at("edge_count"),{c.at("created")},{c.at("removed")}},fp,growth,observe);\n        else '+needle)
        # Fault runs observe physical memory writes without synthetic before-image
        # reads; normal/poststore runs retain exact Source-PC callback events.
        test=test.replace('struct Fault{};struct Required{};','struct Fault{};struct Required{};struct ObserverStop{};')
        test=test.replace('J fault;mutable unsigned fault_count{};bool log_reads{};', 'J fault,readonly_range;J raw_stores=J::array();mutable unsigned fault_count{};bool log_reads{},record_raw{};')
        old='const auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);'
        new='if(!readonly_range.is_null()&&std::uint64_t(p)+in.size()>readonly_range.at("address").get<std::uint32_t>()&&p<std::uint64_t(readonly_range.at("address").get<std::uint32_t>())+readonly_range.at("bytes").get<std::uint32_t>())throw Fault{};const auto i=at(p,in.size());if(record_raw){const auto&bank=banks.at(i->first);raw_stores.push_back(J{{"address",p},{"before",hex(std::span(bank).subspan(p-i->first,in.size()))},{"written",hex(in)}});}std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);'
        assert test.count(old)==1;test=test.replace(old,new)
        test=test.replace('memory.log_reads=read_pass;', 'memory.log_reads=read_pass;memory.readonly_range=c.value("readonly_range",J{});memory.record_raw=c.value("declared_fault",false);')
        test=test.replace('if(!read_pass)observe=', 'if(!read_pass&&!c.value("declared_fault",false))observe=')
        test=test.replace('++index;}};', '++index;}if(c.contains("after_store")&&event.instruction==c.at("after_store").at("instruction"))throw ObserverStop{};};')
        test=test.replace('if(read_pass&&!c.value("mutations",J::array()).empty())continue;', 'if(read_pass&&(!c.value("mutations",J::array()).empty()||c.contains("after_store")))continue;')
        test=test.replace('bool failed=false,required=false;', 'bool failed=false,required=false,observer_stopped=false;')
        test=test.replace('catch(const Fault&){failed=true;}', 'catch(const ObserverStop&){observer_stopped=true;}catch(const Fault&){failed=true;}')
        test=test.replace('check(failed==c.value(', 'check(observer_stopped==c.value("poststore_stop",false),name+" poststore stop differs");\n      check(failed==c.value(')
        test=test.replace('else{if(actual_stores!=c.at("writes"))', 'else{if(failed){actual_stores=memory.raw_stores;for(auto&w:actual_stores)w["instruction"]=0;auto expected=c.at("writes");for(auto&w:expected)w["instruction"]=0;check(actual_stores==expected,name+" fault write phase differs");actual_stores=c.at("writes");}if(actual_stores!=c.at("writes"))')
        (project/'tests.cpp').write_text(test)
        units=' '.join(sources)+ ' tests.cpp'
        (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(native_broad_edges LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(native_broad_edges '+units+')\ntarget_include_directories(native_broad_edges PRIVATE include)\ntarget_compile_features(native_broad_edges PRIVATE cxx_std_20)\ntarget_compile_options(native_broad_edges PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(native_broad_edges PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME native_broad_edges_original_tu15 COMMAND native_broad_edges "'+str(fixture)+'")\n')
        results=[]
        for cc in('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/cc),'--target','native_broad_edges','--filter','^native_broad_edges_original_tu15$','--jobs','1','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+cc];r=subprocess.run(command,check=False);results.append({'compiler':cc,'exit_code':r.returncode,'command':command});assert r.returncode==0,'Native2EA380 differs; inspect '+str(a.output/cc/'tests.log')
        negative=[]
        for name,before,after in (
            ('quantized-upper-parity','((hi[axis]>>7u)&0xffffu)|1u','((hi[axis]>>7u)&0xfffeu)'),
            ('neighbor-edge-direction','m_.move_index(right.other_low,other+a.low_index,-1)','m_.move_index(right.other_low,other+a.low_index,1)'),
            ('pair-growth-stride','grow(header,8)','grow(header,4)')):
            target=project/'src/physics/native_tu15_broad_phase_edges.cpp';prior=target.read_text();assert prior.count(before)==1,name;target.write_text(prior.replace(before,after,1))
            try:
                command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/('negative-'+name)),'--target','native_broad_edges','--filter','^native_broad_edges_original_tu15$','--jobs','1','--test-jobs','1'];r=subprocess.run(command,check=False)
                log=(a.output/('negative-'+name)/'tests.log').read_text();assert r.returncode!=0 and any(x in log for x in ('array store differs','complete array/pool bank differs','array read order differs','actual allocator request order differs')),name+' not discriminated by independent original'
                negative.append(dict(name=name,rejected=True,command=command))
            finally:target.write_text(prior)
        executed={pc for c in rows for pc in c['executed_sites']};body=__import__('generate_physics_live_support_tu15').complete_body(_Code(image,'.text'),0x2ea380)
        report.update(passed=True,compiler_results=results,original_cases=len(rows),full_returns=sum(not c["declared_fault"]and not c.get("poststore_stop",False)for c in rows),fault_prefixes=sum(c["declared_fault"]for c in rows),poststore_stops=sum(c.get("poststore_stop",False)for c in rows),observer_mutations=sum(c["mutations_applied"]for c in rows),hardware_rmw_faults=sum("readonly_range"in c for c in rows),source_sites=len(body.code),executed_source_sites=len(executed&set(body.code)),missing_source_sites=[dict(address=hex(pc),instruction=f"{code.at(pc).mnemonic} {code.at(pc).op_str}")for pc in sorted(set(body.code)-executed)],negative_controls=negative,hardware_fault_instructions=[hex(c["fault_instruction"])for c in rows if "readonly_range"in c],ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),native_allocator_calls=sum(len(c['allocator_calls'])for c in rows),source_hashes=files,authority=authority,input_provenance=provenance,fixture_sha256=sha(fixture),remaining=['Actual genuine input/World binding/live','Other hardware fault/alias domains'])
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(q.stat().st_blocks*512 for q in scratch.rglob('*')if q.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
