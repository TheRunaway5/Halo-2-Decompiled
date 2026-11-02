#!/usr/bin/env python3
"""Physical TU1.5 actual2F83B0 (base GSK convex agent factory) qualification.

Derived from validate_native_convex_agent_factory_actual_tu15.py (2E4EB0) and
its physical chain. The unchanged English TU1.5 XBE is linked at its real
addresses with the actual02 native-pair BEFORE banks into a private i386 ELF
and single-stepped under tools/observe_native_array_growth_tu15.cpp. The
observer copy gets recorded private patches: sorted retained-region lookup,
bank dumps limited to the touched pages, return ESP, and replay of the ORIGINAL
FuncInfo unwind funclets after a declared fault (the FS chain, FH3 thunk,
FuncInfo and unwind map are read from the traced process; only the dispatch
order is modelled, the funclets and release run physically). Every original
read must hit XBE section bytes or a captured bank; nothing is zero-filled or
supplied.

The candidate native_tu15_create_gsk_contact_agent and the frozen cohort units
it links against are privately copied (compiler-derived header closure) and
compared under GCC and Clang: touched banks, ordered stores, ordered reads
(including formal stack arguments), allocator calls, return value and
x87/SSE state, for the four real class-8 pairs and declared fault/unwind
controls. The 256B 2E4EB0 kernel and targeted kernel mutants must go red on a
comparison (not on a build error). Run only under tools/run_test.py; ELFs,
banks, fixture and builds stay under HALO2_TEST_SCRATCH. Small receipts go to
--report-dir. --root selects the checkout whose evidence/helpers are used.
"""
import argparse,bisect,hashlib,inspect,json,lzma,os,shutil,struct,subprocess,sys,tarfile,time,traceback
from pathlib import Path
def _root():
    for i,arg in enumerate(sys.argv):
        if arg=='--root' and i+1<len(sys.argv):return Path(sys.argv[i+1]).resolve()
        if arg.startswith('--root='):return Path(arg.split('=',1)[1]).resolve()
    return Path(__file__).resolve().parents[1]
ROOT=_root();sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import ROOT as HELPER_ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import validate_native_array_growth_tu15 as array_base
assert HELPER_ROOT==ROOT,(HELPER_ROOT,ROOT)

ARCHIVE_DIR=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-flip-controller-original-v1465-20261101/iteration-02-native-pair-owned-release-input-vehicle-camera/required-native-pair-consumer-before'
ARCHIVE=ARCHIVE_DIR/'actual-native-before.tar.zst'
ARCHIVE_PIN='05ce4baa6c8d8a71326a629ce93a25eca8259a5b52edd9f51fc62fd5b3cd90dd'
DEFAULT_CANDIDATE=ROOT
DEFAULT_REPORT=ROOT/'build/test-results/native-gsk-agent-factory-actual-tu15'
ORACLE='unchanged-tu15-physical-gsk-factory-actual02-v1'
ENTRY,USER=0x2f83b0,0x10082380
# Real root cdBody payloads of class-8 rigid bodies in actual02 (claude-pair-factory.md section 6).
PAIRS=((0x1007bd6c,0x1007d32c),(0x1007d32c,0x1007bd6c),(0x1007bd6c,0x1007c758),(0x1007c758,0x1007bd6c))
DISPATCHER,DISPATCHER_VTABLE,TABLE_OFFSET=0x100680b0,0x41c7c4,0x18c
CLASS8_SHAPE_VTABLES=(0x461e58,0x461fe0)
STACK,SP,STOP,RESUME=0x2000000,0x200f000,0x5000000,0x5000100
UI,UI_BYTES=0x4000000,0x10000  # native-only private DATA; never an original input
FS_BASE,FRAME_HANDLER=0x71000000,0x32c193
IMAGE_FIRST,IMAGE_END=0x10000,0x800000
PRIVATE_SOURCE_BANKS=(0x60000000,0x70000000)  # source control/TLS DATA is never an input
ARGUMENTS_REGION=(SP+4,16)
OWNERS=(0x2f83b0,0x2f8300,0x2e4550,0x309060,0x2e4690,0x2e47b0,0x3077c0,0x3077f0,0x1cb030,0xad390,
        0x149da0,0x233250,0x149e00,0x233420,0x37ea00,0x37ea20,0x2f0380,0x2f7a10)
UNITS=('native_tu15_convex_agent_factory.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_inverse_geometry.cpp',
       'native_tu15_contact_cache.cpp','native_tu15_contact_shape.cpp','native_tu15_geometry.cpp','native_tu15_activation_services.cpp',
       'native_tu15_agent_cleanup_children.cpp','native_tu15_agent_cleanup.cpp','native_tu15_array_allocator.cpp','native_tu15_array_growth.cpp')
KERNEL='src/physics/native_tu15_convex_agent_factory.cpp'
KERNEL_HEADER='include/halo2/native_tu15_convex_agent_factory.hpp'
GSK_SIGNATURE='NativeTu15ContactAgent native_tu15_create_gsk_contact_agent(SourcePhysicsContactMemory&memory,'
CTOR_STATE0,FACTORY_STATE0=0x37ea00,0x37ea20
# name, access, selector, original unwind actions derived from FuncInfo 46BBC0/46BBE4.
FAULT_POINTS=(
    ('pool_entry_read','read',lambda r,b:r['instruction'] in b[0x233250],()),
    ('size_word_write','write',lambda w,b:w['instruction']==0x2f83d8,()),
    ('ctor_user_store_write','write',lambda w,b:w['instruction']==0x2f832a,(FACTORY_STATE0,)),
    ('first_shape_read','read',lambda r,b:r['instruction']==0x2f8341,(FACTORY_STATE0,)),
    ('inverse_entry_read','read',lambda r,b:r['instruction'] in b[0x2e4550],(CTOR_STATE0,FACTORY_STATE0)),
    ('shape_class_read','read',lambda r,b:r['instruction']==0x2f8360,(CTOR_STATE0,FACTORY_STATE0)),
    ('center_entry_read','read',lambda r,b:r['instruction'] in b[0x3077c0],(CTOR_STATE0,FACTORY_STATE0)),
    ('support_entry_read','read',lambda r,b:r['instruction'] in b[0x3077f0],(CTOR_STATE0,FACTORY_STATE0)),
    ('empty_count_write','write',lambda w,b:w['instruction']==0x2f838c,(CTOR_STATE0,FACTORY_STATE0)),
    ('final_store_write','write',lambda w,b:w['instruction']==0x2f8394,(CTOR_STATE0,FACTORY_STATE0)),
)

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def sha_bytes(b):return hashlib.sha256(b).hexdigest()
def save(p,j):Path(p).write_text(json.dumps(j,indent=1,sort_keys=True)+'\n')

class Intervals:
    def __init__(self,pairs):
        merged=[]
        for a,n in sorted(pairs):
            if merged and a<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],a+n)
            else:merged.append([a,a+n])
        self.ranges=merged;self.starts=[a for a,_ in merged]
    def covers(self,a,n):
        i=bisect.bisect_right(self.starts,a)-1
        return i>=0 and a+n<=self.ranges[i][1]

def merge_adjacent(banks):
    """Exactly contiguous captured banks become one run (no gap is ever filled)."""
    runs=[]
    for a,raw in sorted(banks.items()):
        if runs and runs[-1][0]+len(runs[-1][1])==a:runs[-1][1].extend(raw)
        else:
            assert not runs or runs[-1][0]+len(runs[-1][1])<a,hex(a)
            runs.append([a,bytearray(raw)])
    return runs

def load_actual02():
    assert sha(ARCHIVE)==ARCHIVE_PIN,'actual02 archive pin differs'
    capture_raw=(ARCHIVE_DIR/'capture-native-before.json').read_bytes();capture=json.loads(capture_raw)
    hashes=json.loads(lzma.decompress((ARCHIVE_DIR/'all-file-hashes.json.xz').read_bytes()))
    wanted={r['before_file']:r for r in capture['regions']};banks={};verified=0
    stream=subprocess.Popen(['zstd','-dc',str(ARCHIVE)],stdout=subprocess.PIPE)
    with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
        for member in tar:
            key=Path(member.name).name
            if key in wanted:
                raw=tar.extractfile(member).read();r=wanted[key];assert len(raw)==r['size'],key
                h=hashes.get(key)
                if h is not None:assert h['bytes']==len(raw) and h['sha256']==sha_bytes(raw),key;verified+=1
                banks[r['address']]=raw
    assert stream.wait()==0 and len(banks)==len(capture['regions'])
    fp=capture['input']['floating_point']
    provenance=dict(archive=str(ARCHIVE.relative_to(ROOT)),archive_sha256=ARCHIVE_PIN,capture_manifest_sha256=sha_bytes(capture_raw),
        regions=len(banks),bank_bytes=sum(map(len,banks.values())),member_hashes_verified=verified,boundary=capture.get('boundary'),
        schema=capture.get('schema'),captured_after_consumed=capture.get('captured_after_consumed'),floating_point=fp)
    return banks,(fp['x87_control_word'],fp['x87_status'],fp['mxcsr']),provenance

def build_observer(scratch):
    text=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();base_sha=sha_bytes(text.encode())
    unwind='''bool unwinding=false;std::vector<std::pair<std::uint32_t,std::uint32_t>> actions;std::size_t next_action=0;std::uint32_t unwind_stack=0;J unwind_log=J::array();
  const std::uint32_t resume=m.contains("unwind")?m.at("unwind").at("resume").get<std::uint32_t>():0u;
  auto word32=[&](std::uint32_t p){const auto raw=memory(child.pid,p,4);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(raw[i])<<(8u*i);return v;};
  auto poke32=[&](std::uint32_t p,std::uint32_t v){for(unsigned off=0;off<4;){const auto at=std::uintptr_t(p+off)&~std::uintptr_t(7);auto w=std::uint64_t(request(PTRACE_PEEKDATA,child.pid,reinterpret_cast<void*>(at)));for(unsigned lane=unsigned((p+off)&7u);lane<8&&off<4;++lane,++off)w=(w&~(0xffull<<(lane*8u)))|(std::uint64_t((v>>(off*8u))&0xffu)<<(lane*8u));request(PTRACE_POKEDATA,child.pid,reinterpret_cast<void*>(at),reinterpret_cast<void*>(w));}};
  auto collect_actions=[&](std::uint32_t fault_esp){const auto&u=m.at("unwind");const auto fs=u.at("fs_base").get<std::uint32_t>(),handler=u.at("frame_handler").get<std::uint32_t>();unwind_stack=(fault_esp-u.at("stack_margin").get<std::uint32_t>())&~15u;unsigned frames=0;
    for(std::uint32_t node=word32(fs);node!=0xffffffffu;node=word32(node)){check(++frames<64,"Original unwind chain too long");check(node>=fault_esp&&node-fault_esp<0x10000u,"Original registration node outside the active stack");
      const auto h=word32(node+4);const auto thunk=memory(child.pid,h,10);check(std::to_integer<unsigned>(thunk[0])==0xb8u&&std::to_integer<unsigned>(thunk[5])==0xe9u,"Original frame handler is not an FH3 thunk");
      std::uint32_t info{},rel{};for(unsigned i=0;i<4;++i){info|=std::to_integer<unsigned>(thunk[1+i])<<(8u*i);rel|=std::to_integer<unsigned>(thunk[6+i])<<(8u*i);}check(h+10u+rel==handler,"Original frame handler target differs");
      check(word32(info)==0x19930520u,"Original FuncInfo magic differs");const auto states=word32(info+4),map=word32(info+8);auto state=std::int32_t(word32(node+8));J frame{{"node",node},{"handler",h},{"funcinfo",info},{"state",state},{"actions",J::array()}};
      while(state!=-1){check(state>=0&&std::uint32_t(state)<states,"Original unwind state outside its FuncInfo");const std::int32_t next=std::int32_t(word32(map+std::uint32_t(state)*8u));const std::uint32_t action=word32(map+std::uint32_t(state)*8u+4u);if(action){actions.emplace_back(action,node+12u);frame["actions"].push_back(J{{"state",state},{"action",action},{"frame",node+12u}});}state=next;}
      unwind_log.push_back(frame);}};
  auto start_action=[&]{auto r=regs;r.rip=actions[next_action].first;r.rbp=actions[next_action].second;r.rsp=unwind_stack-4u;poke32(unwind_stack-4u,resume);request(PTRACE_SETREGS,child.pid,nullptr,&r);++next_action;};'''
    patches=[
        ('auto retained=[&](std::uint32_t p,std::uint32_t n){for(const auto&r:m.at("regions")){const auto first=r.at("address").get<std::uint32_t>();if(p>=first&&std::uint64_t(p-first)+n<=r.at("bytes").get<std::uint32_t>())return true;}return false;};',
         'std::map<std::uint32_t,std::uint32_t> retained_regions;for(const auto&r:m.at("regions"))check(retained_regions.emplace(r.at("address").get<std::uint32_t>(),r.at("bytes").get<std::uint32_t>()).second,"Duplicate retained region");\n  {std::uint64_t end=0;for(const auto&[first,bytes]:retained_regions){check(first>=end,"Overlapping retained regions");end=std::uint64_t(first)+bytes;}}\n  auto retained=[&](std::uint32_t p,std::uint32_t n){auto i=retained_regions.upper_bound(p);if(i==retained_regions.begin())return false;--i;return std::uint64_t(p-i->first)+n<=i->second;};'),
        ('J before=J::array();for(const auto&r:m.at("regions"))before.push_back(','const J dump_regions=m.value("dump",m.at("regions"));J before=J::array();for(const auto&r:dump_regions)before.push_back('),
        ('std::set<unsigned> applied;std::set<std::uint32_t> executed;','std::set<unsigned> applied;std::set<std::uint32_t> executed;\n  '+unwind),
        ('const auto pc=std::uint32_t(regs.rip);if(pc==stop)break;','const auto pc=std::uint32_t(regs.rip);if(pc==stop)break;\n    if(unwinding&&pc==resume){if(next_action>=actions.size())break;start_action();continue;}'),
        ('    if(declared_fault)break;','    if(declared_fault&&!unwinding){if(!m.contains("unwind"))break;unwinding=true;collect_actions(registers[4]);if(actions.empty())break;start_action();continue;}'),
        ('J banks=J::array();for(const auto&r:m.at("regions"))banks.push_back(','J banks=J::array();for(const auto&r:dump_regions)banks.push_back('),
        ('check(site!=sites.end(),"Original array escaped authenticated complete bodies");','check(site!=sites.end(),("Original array escaped authenticated complete bodies pc="+std::to_string(pc)).c_str());'),
        ('{"declared_fault",declared_fault},','{"declared_fault",declared_fault},{"unwind",unwind_log},{"unwind_actions_run",next_action},{"final_esp",std::uint32_t(regs.rsp)},'),
    ]
    for old,new in patches:
        assert text.count(old)==1,old[:60];text=text.replace(old,new)
    owned=scratch/'observer';owned.mkdir();source=owned/'observe_gsk_tu15.cpp';source.write_text(text)
    built=subprocess.run(['c++','-std=c++20','-O2',str(source),'-o',str(owned/'observer')],capture_output=True,text=True)
    assert built.returncode==0,('observer build failed',built.stderr[-4000:])
    return owned/'observer',text,dict(base='tools/observe_native_array_growth_tu15.cpp',base_sha256=base_sha,patched_sha256=sha_bytes(text.encode()),patches=len(patches))

def site_metadata(image):
    source=inspect.getsource(array_base.metadata);needle="assert not op.mem.segment or ins.reg_name(op.mem.segment)=='es'"
    assert source.count(needle)==1
    source=source.replace(needle,"if op.mem.segment and ins.reg_name(op.mem.segment)=='fs':continue\n                    "+needle)
    namespace=dict(array_base.__dict__,OWNERS=OWNERS);exec(source,namespace)
    m,authority=namespace['metadata'](image,{});return m['sites'],authority

def link_original(owned,image_file,high_files,pair,fp):
    owned.mkdir();first,second=pair
    stack=bytearray(0x10000);struct.pack_into('<5I',stack,SP-STACK,STOP,first,second,0,USER);(owned/'stack.bin').write_bytes(stack)
    environment=bytearray(28);struct.pack_into('<H',environment,0,fp[0]);struct.pack_into('<H',environment,4,fp[1]);struct.pack_into('<H',environment,8,0xffff);(owned/'env.bin').write_bytes(environment)
    sections=['.image 0x%x : { *(.image) }'%IMAGE_FIRST,'.stack 0x%x : { *(.stack) }'%STACK,'.fs 0x%x : { *(.fs) }'%FS_BASE,'.bridge 0x70000000 : { *(.bridge) }']
    assembly=[f'.section .image,"awx"\n.incbin "{image_file}"',f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"','.section .fs,"aw"\n.zero 4096']
    for i,(address,path) in enumerate(high_files):
        assembly.append(f'.section .bank{i},"aw"\n.incbin "{path}"');sections.append(f'.bank{i} 0x{address:x} : {{ *(.bank{i}) }}')
    assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 cld
 mov $123,%eax
 mov $1,%ebx
 mov $descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 jne setup_failed
 mov $7,%eax
 mov %ax,%fs
 movl $0xffffffff,%fs:0
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${SP},%esp
 xor %eax,%eax
 xor %ecx,%ecx
 xor %edx,%edx
 xor %ebx,%ebx
 xor %ebp,%ebp
 xor %esi,%esi
 xor %edi,%edi
 int3
 jmp *entry
setup_failed:
 mov $1,%eax
 mov $99,%ebx
 int $0x80
descriptor:.long 0,{FS_BASE},0xfffff,0x51
entry: .long {ENTRY}
sse_control: .long {fp[2]}
environment: .incbin "{owned/'env.bin'}"
''')
    (owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
    subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True)
    subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
    (owned/'reference.o').unlink();return owned/'reference'

def observe(observer,elf,metadata,owned,label):
    meta=owned/f'{label}.metadata.json';out=owned/f'{label}.output.json'
    meta.write_text(json.dumps(metadata,separators=(',',':')))
    r=subprocess.run([str(observer),str(elf),str(meta),str(out),hex(STOP)],capture_output=True,text=True,timeout=900)
    assert r.returncode==0,(label,r.stdout[-2000:],r.stderr[-2000:])
    result=json.loads(out.read_text());meta.unlink();out.unlink();return result

def dump_extents(accesses,regions):
    starts=[a for a,_ in regions];pages=set()
    for a,n in accesses:
        for page in range(a>>12,((a+n-1)>>12)+1):pages.add(page)
    pieces=[]
    for page in sorted(pages):
        lo,hi=page<<12,(page+1)<<12;i=max(0,bisect.bisect_right(starts,lo)-1)
        while i<len(regions) and regions[i][0]<hi:
            a,n=regions[i];s,e=max(lo,a),min(hi,a+n)
            if s<e:pieces.append([s,e])
            i+=1
    merged=[]
    for s,e in sorted(pieces):
        if merged and merged[-1][1]==s:merged[-1][1]=e
        else:merged.append([s,e])
    return [dict(address=s,bytes=e-s)for s,e in merged]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,default=ROOT,help='checkout whose evidence/helpers are used (parsed before import)')
    p.add_argument('--output',type=Path,required=True,help='owned output directory inside HALO2_TEST_SCRATCH')
    p.add_argument('--report-dir',type=Path,default=DEFAULT_REPORT,help='small retained receipts')
    p.add_argument('--candidate-source',type=Path,default=DEFAULT_CANDIDATE)
    p.add_argument('--compilers',nargs='+',default=['g++','clang++'])
    p.add_argument('--jobs',type=int,default=4)
    p.add_argument('--discover-only',action='store_true',help='original discovery/classification only; no native verdict')
    a=p.parse_args();started=time.time();assert a.root.resolve()==ROOT
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();output=a.output.resolve();assert output.is_relative_to(scratch),'outputs must stay in owned scratch'
    output.mkdir(parents=True,exist_ok=False);report_dir=a.report_dir.resolve();report_dir.mkdir(parents=True,exist_ok=True);candidate=a.candidate_source.resolve()
    report=dict(passed=False,validator=str(Path(__file__).resolve()),validator_sha256=sha(__file__),argv=sys.argv,root=str(ROOT),owned_scratch=str(scratch),
        candidate_source=str(candidate),backend='physical i386 ELF single-stepped by ptrace (not Unicorn)',supplied_original_outcomes=False,retained_large_artifacts=[],
        helper_sha256={f:sha(ROOT/f)for f in('tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tools/generate_physics_live_support_tu15.py',
            'tools/oracle_smg_firing_1_1.py','tools/research_mp_sandbox_update_functions.py','tools/run_tests.py','tools/run_test.py','tools/validate_native_convex_agent_factory_actual_tu15.py')})
    def flush():save(report_dir/'report.json',report)
    shutil.copyfile(__file__,report_dir/Path(__file__).name)
    try:
        # 1. Private snapshot of the candidate kernel inputs (header closure from the compiler).
        snapshot=scratch/'candidate';(snapshot/'src/physics').mkdir(parents=True);shutil.copytree(candidate/'include',snapshot/'include')
        for unit in UNITS+('native_tu15_contact_unwind_internal.hpp',):shutil.copyfile(candidate/'src/physics'/unit,snapshot/'src/physics'/unit)
        (snapshot/'driver.cpp').write_text(DRIVER);closure=set()
        for unit in ['driver.cpp']+[f'src/physics/{u}'for u in UNITS]:
            deps=subprocess.check_output(['g++','-std=c++20','-MM','-MT','x','-Iinclude',unit],cwd=snapshot,text=True).replace('\\\n',' ').split()[1:]
            closure.update(str(Path(d))for d in deps if not d.startswith('/'))
        closure.discard('driver.cpp');project=scratch/'project'
        for rel in sorted(closure):target=project/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(snapshot/rel,target)
        shutil.rmtree(snapshot);kernel_text=(project/KERNEL).read_text()
        input_hashes={rel:sha(project/rel)for rel in sorted(closure)}
        assert kernel_text.count(GSK_SIGNATURE)==1,'Candidate has no single native_tu15_create_gsk_contact_agent'
        begin=kernel_text.index(GSK_SIGNATURE);end=kernel_text.index('\n}\n',begin)+3;gsk_text=kernel_text[begin:end]
        report.update(candidate_inputs=input_hashes,candidate_gsk_function=gsk_text,candidate_gsk_function_sha256=sha_bytes(gsk_text.encode()))
        (report_dir/'candidate').mkdir(exist_ok=True)
        for rel in (KERNEL,KERNEL_HEADER):shutil.copyfile(project/rel,report_dir/'candidate'/Path(rel).name)
        flush()
        # 2. Unchanged XBE and actual02 BEFORE banks.
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
        banks,actual_fp,provenance=load_actual02()
        private={k:v for k,v in banks.items()if PRIVATE_SOURCE_BANKS[0]<=k<PRIVATE_SOURCE_BANKS[1]}
        captured={k:v for k,v in banks.items()if k not in private};provenance.update(excluded_private_source_banks=len(private),excluded_private_source_bytes=sum(map(len,private.values())))
        for k,v in captured.items():
            assert not(STACK<k+len(v)and k<STACK+0x10000)and not(UI<k+len(v)and k<UI+UI_BYTES)and not(0x70000000<=k<0x72000000),hex(k)
            assert k>=IMAGE_END or (k>=IMAGE_FIRST and k+len(v)<=IMAGE_END),hex(k)
        flat=bytearray(IMAGE_END-IMAGE_FIRST);xbe_raw=[]
        for s in image.sections:
            raw=image.section_bytes(s);assert IMAGE_FIRST<=s.va and s.va+max(len(raw),s.vsize)<=IMAGE_END
            flat[s.va-IMAGE_FIRST:s.va-IMAGE_FIRST+len(raw)]=raw;xbe_raw.append((s.va,len(raw)))
        differing={};nondata_differences=[]
        for k,v in captured.items():
            if k<IMAGE_END:
                for s in image.sections:
                    lo,hi=max(k,s.va),min(k+len(v),s.va+s.rsize)
                    if lo<hi:
                        diff=[lo+i for i,(x,y)in enumerate(zip(v[lo-k:hi-k],image.section_bytes(s)[lo-s.va:hi-s.va]))if x!=y]
                        if diff:differing[s.name]=differing.get(s.name,0)+len(diff)
                        if s.name!='.data':nondata_differences.extend(diff)
                flat[k-IMAGE_FIRST:k-IMAGE_FIRST+len(v)]=v
        assert '.text' not in differing,'Captured bytes differ from XBE .text'
        bodies={o:set(complete_body(code,o).code)for o in OWNERS}
        for o in OWNERS:
            for pc in bodies[o]:assert bytes(flat[pc-IMAGE_FIRST:pc-IMAGE_FIRST+code.at(pc).size])==code.at(pc).bytes,hex(pc)
        runs=merge_adjacent(captured);compare_regions=sorted([(a,len(r))for a,r in runs]+[ARGUMENTS_REGION])
        compare_cover=Intervals(compare_regions);xbe_cover=Intervals(xbe_raw)
        run_starts=[a for a,_ in runs]
        def word(p,n=4):
            i=bisect.bisect_right(run_starts,p)-1;a,r=runs[i];assert a<=p and p+n<=a+len(r),hex(p);return int.from_bytes(r[p-a:p-a+n],'little')
        def image_word(p):return int.from_bytes(flat[p-IMAGE_FIRST:p-IMAGE_FIRST+4],'little')
        assert word(DISPATCHER)==DISPATCHER_VTABLE and word(DISPATCHER+TABLE_OFFSET+4*((8<<5)+8))==ENTRY,'Captured TU1.5 dispatcher does not map (8,8) to 2F83B0'
        pair_inputs=[]
        for first,second in PAIRS:
            row={}
            for label,body in (('first',first),('second',second)):
                shape=word(body);vtable=word(shape);owner=image_word(vtable+0x14)
                assert vtable in CLASS8_SHAPE_VTABLES and owner==0xad390,(hex(body),hex(vtable),hex(owner))
                row[label]=dict(body=hex(body),shape=hex(shape),shape_vtable=hex(vtable),get_type=hex(owner),motion=hex(word(body+8)))
            pair_inputs.append(row)
        provenance.update(captured_image_bytes_differing_from_xbe=differing,dispatcher=dict(address=hex(DISPATCHER),vtable=hex(DISPATCHER_VTABLE),entry_8_8=hex(ENTRY)),
            pairs=pair_inputs,user_argument=hex(USER),actual_fp=[hex(x)for x in actual_fp],compare_regions=len(compare_regions),compare_region_bytes=sum(n for _,n in compare_regions))
        report['input_provenance']=provenance;flush()
        # 3. Physical original runs.
        sites,authority=site_metadata(image);observer,observer_text,observer_info=build_observer(scratch)
        (report_dir/'observe_gsk_tu15.cpp').write_text(observer_text);report.update(observer=observer_info,site_authority=authority)
        owned=scratch/'original';(owned/'high').mkdir(parents=True);image_file=owned/'image.bin';image_file.write_bytes(flat);high_files=[]
        for a_,r in runs:
            if a_>=IMAGE_END:path=owned/'high'/f'{a_:08x}.bin';path.write_bytes(r);high_files.append((a_,path))
        discovery_regions=[dict(address=IMAGE_FIRST,bytes=IMAGE_END-IMAGE_FIRST),dict(address=ARGUMENTS_REGION[0],bytes=ARGUMENTS_REGION[1])]+[dict(address=a_,bytes=len(r))for a_,r in runs if a_>=IMAGE_END]
        unwind=dict(fs_base=FS_BASE,frame_handler=FRAME_HANDLER,resume=RESUME,stack_margin=0x400)
        fixture=scratch/'fixture';(fixture/'banks').mkdir(parents=True)
        def persist(raw):
            target=fixture/'banks'/(sha_bytes(raw)+'.bin')
            if not target.exists():target.write_bytes(raw)
            return str(target.relative_to(fixture))
        cases=[];classification=dict(captured=0,xbe_only=[],uncovered=[]);executed=set()
        def run_case(name,elf,fault,expected_actions):
            base_meta=dict(sites=sites,unwind=unwind,**({'fault':fault}if fault else{}))
            found=observe(observer,elf,dict(base_meta,regions=discovery_regions,dump=[]),owned,name+'.discovery')
            for r in found['reads']:
                if compare_cover.covers(r['address'],r['bytes']):classification['captured']+=1
                elif xbe_cover.covers(r['address'],r['bytes']):classification['xbe_only'].append(dict(case=name,instruction=r['instruction'],address=r['address'],bytes=r['bytes']))
                else:classification['uncovered'].append(dict(case=name,instruction=r['instruction'],address=r['address'],bytes=r['bytes']))
            outside=[w for w in found['writes']if not compare_cover.covers(w['address'],len(w['written'])//2)]
            assert not outside,(name,'original writes outside captured banks',outside[:4])
            expected_reads=[r for r in found['reads']if compare_cover.covers(r['address'],r['bytes'])]
            accesses=[(r['address'],r['bytes'])for r in expected_reads]+[(w['address'],len(w['written'])//2)for w in found['writes']]
            dump=dump_extents(accesses+[ARGUMENTS_REGION],compare_regions)
            result=observe(observer,elf,dict(base_meta,regions=[dict(address=x,bytes=n)for x,n in compare_regions],dump=dump),owned,name)
            assert result['reads']==expected_reads and result['writes']==found['writes'] and result['allocator_calls']==found['allocator_calls'],name+' physical comparison run differs from discovery'
            actions=tuple(x['action']for f in result['unwind']for x in f['actions'])
            assert bool(result['declared_fault'])==bool(fault),name+' declared fault not reached'
            assert actions==tuple(expected_actions) and result['unwind_actions_run']==len(actions),(name,'original FuncInfo unwind differs from expectation',[hex(x)for x in actions])
            if not fault:assert result['final_esp']==SP+4,(name,'cdecl return ESP',hex(result['final_esp']))
            before={b['address']:b['before']for b in result.pop('initial_banks')};after=result.pop('banks')
            executed.update(result['executed_sites'])
            case=dict(name=name,entry=ENTRY,before_fp=list(actual_fp),declared_fault=bool(fault),**({'fault':fault}if fault else{}),
                banks=[dict(address=b['address'],before_file=persist(bytes.fromhex(before[b['address']])),after_file=persist(bytes.fromhex(b['after'])))for b in after],
                **{k:result[k]for k in('after_fp','return_word','final_esp','allocator_calls','reads','writes','unwind','original_instructions')})
            cases.append(case);return case
        for index,pair in enumerate(PAIRS):
            elf=link_original(owned/f'pair{index}',image_file,high_files,pair,actual_fp);tag=f'pair{index}_{pair[0]:08x}_{pair[1]:08x}'
            clean=run_case(tag+'_clean',elf,None,())
            for point,kind,select,expected in FAULT_POINTS:
                if kind=='read':
                    written=[(w['address'],len(w['written'])//2)for w in clean['writes']]
                    chosen=next(i for i,r in enumerate(clean['reads'])if select(r,bodies) and not any(x<r['address']+r['bytes']and r['address']<x+n for x,n in written))
                    r=clean['reads'][chosen];occurrence=sum(1 for q in clean['reads'][:chosen+1]if(q['address'],q['bytes'])==(r['address'],r['bytes']))
                    fault=dict(address=r['address'],bytes=r['bytes'],occurrence=occurrence,write=False,instruction=r['instruction'])
                else:
                    chosen=next(i for i,w in enumerate(clean['writes'])if select(w,bodies));w=clean['writes'][chosen];n=len(w['written'])//2
                    occurrence=sum(1 for q in clean['writes'][:chosen+1]if(q['address'],len(q['written'])//2)==(w['address'],n))
                    fault=dict(address=w['address'],bytes=n,occurrence=occurrence,write=True,instruction=w['instruction'])
                run_case(f'{tag}_fault_{point}',elf,fault,expected)
            shutil.rmtree(owned/f'pair{index}')
        touched_nondata=sorted({hex(x)for c in cases for r in c['reads']for x in nondata_differences if r['address']<=x<r['address']+r['bytes']})
        report['captured_nondata_image_differences']=dict(addresses=[hex(x)for x in sorted(nondata_differences)][:64],read_by_original=touched_nondata)
        assert not classification['uncovered'],('original reads outside XBE bytes and captured banks',classification['uncovered'][:8])
        xbe_only=sorted({(r['address'],r['bytes'],r['instruction'])for r in classification['xbe_only']})
        report['original']=dict(cases=len(cases),fault_cases=sum(c['declared_fault']for c in cases),physical_instructions=sum(c['original_instructions']for c in cases),
            ordered_reads=sum(len(c['reads'])for c in cases),ordered_stores=sum(len(c['writes'])for c in cases),allocator_calls=sum(len(c['allocator_calls'])for c in cases),
            reads_in_captured_banks=classification['captured'],xbe_constant_reads_not_captured=[dict(address=hex(a_),bytes=n,instruction=hex(i))for a_,n,i in xbe_only],
            uncovered_reads=0,unwind_actions_run=sum(sum(len(f['actions'])for f in c['unwind'])for c in cases),
            clean_summary=[dict(name=c['name'],return_word=hex(c['return_word']),final_esp=hex(c['final_esp']),after_fp=[hex(x)for x in c['after_fp']],stores=len(c['writes']),reads=len(c['reads']),
                allocator_calls=c['allocator_calls'],instructions=c['original_instructions'])for c in cases if not c['declared_fault']],
            fault_summary=[dict(name=c['name'],fault={k:(hex(v)if isinstance(v,int)and not isinstance(v,bool)else v)for k,v in c['fault'].items()},unwind_actions=[hex(x['action'])for f in c['unwind']for x in f['actions']],
                stores=len(c['writes']),allocator_calls=[hex(x['owner'])for x in c['allocator_calls']])for c in cases if c['declared_fault']],
            body_coverage={hex(o):dict(total=len(bodies[o]),executed=len(bodies[o]&executed))for o in OWNERS})
        flush()
        if a.discover_only:report.update(passed=False,discover_only=True,native_validation_not_run=True);return 0
        # 4. Native comparison under each compiler; then negative controls.
        fixture_json=fixture/'original.json';save(fixture_json,dict(image_sha256=NEW_PIN,oracle=ORACLE,cases=cases))
        summary_path=fixture/'summary.json';(project/'driver.cpp').write_text(DRIVER)
        units=' '.join(f'src/physics/{u}'for u in UNITS)
        cmake=('cmake_minimum_required(VERSION 3.20)\nproject(gsk_factory_oracle LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\n'
            f'add_executable(gsk driver.cpp {units})\ntarget_include_directories(gsk PRIVATE include src/physics)\ntarget_compile_features(gsk PRIVATE cxx_std_20)\n'
            'target_compile_options(gsk PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nif(GSK_WRONG_KERNEL)\n  target_compile_definitions(gsk PRIVATE GSK_WRONG_KERNEL=1)\nendif()\n'
            f'target_link_libraries(gsk PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME gsk_factory COMMAND gsk "{fixture_json}" "{summary_path}")\n')
        (project/'CMakeLists.txt').write_text(cmake);shutil.copyfile(project/'driver.cpp',report_dir/'driver.cpp');shutil.copyfile(project/'CMakeLists.txt',report_dir/'CMakeLists.txt')
        report['fixture']=dict(sha256=sha(fixture_json),cases=len(cases),bank_files=len(list((fixture/'banks').iterdir())),bank_bytes=sum(x.stat().st_size for x in (fixture/'banks').iterdir()))
        runs_dir=output/'runs';runs_dir.mkdir();report['runs']=[];report['commands']=[]
        def run(label,compiler,extra=()):
            summary_path.unlink(missing_ok=True)
            cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(runs_dir/label),'--target','gsk',
                 '--filter','^gsk_factory$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,*extra,'--jobs',str(a.jobs),'--test-jobs','1']
            report['commands'].append(cmd);r=subprocess.run(cmd,capture_output=True,text=True)
            summary=json.loads(summary_path.read_text())if summary_path.exists()else None
            row=dict(label=label,compiler=compiler,exit_code=r.returncode,comparison_ran=summary is not None)
            if summary:row.update(passed=summary['passed'],kernel=summary['kernel'],cases=summary['cases'],failed_cases=summary['failed_cases'],ordered_stores=summary['ordered_stores'],
                ordered_reads=summary['ordered_reads'],fault_cases=summary['fault_cases'],first_problems=[dict(case=x['name'],problem=x['problems'][0][:400])for x in summary['results']if x['problems']][:6],
                failing_case_names=[x['name']for x in summary['results']if x['problems']])
            else:
                log=runs_dir/label/'build.log';row['log_tail']=(log.read_text(errors='replace')[-3000:]if log.exists()else r.stderr[-3000:])
            report['runs'].append(row);flush();return row
        positives=[run('native-'+c.replace('+','x'),c)for c in a.compilers]
        gsk_mutants=[
            ('size_allocation_256','{allocator,owner,240u,0x1au}','{allocator,owner,256u,0x1au}'),
            ('size_word_256','agent.identity+4u,240u,2','agent.identity+4u,256u,2'),
            ('size_both_256',None,None),
            ('derived_vtable_41c370','storage.disarm();return result;','m.word(0x2e4f07u,agent.identity,0x41c370u);storage.disarm();return result;'),
            ('plus_f0_store','storage.disarm();return result;','m.word(0x2e4f0du,agent.identity+0xf0u,0u);storage.disarm();return result;'),
            ('size_store_site','m.word(0x2f83d8u,','m.word(0x2e4edbu,'),
            ('read_order_first_before_second','second=input(args.second_payload),first=input(args.first_payload)','first=input(args.first_payload),second=input(args.second_payload)'),
            ('read_order_user_last','const auto user_data=input(args.user_data),second=input(args.second_payload),first=input(args.first_payload);','const auto second=input(args.second_payload),first=input(args.first_payload),user_data=input(args.user_data);'),
            ('unwind_armed_after_ctor',None,None),
            ('unwind_armed_before_size_word',None,None),
            ('unwind_not_disarmed','storage.disarm();return result;','return result;'),
        ]
        def mutate(name,before,after):
            if name=='size_both_256':return mutate('',*gsk_mutants[0][1:]).replace('agent.identity+4u,240u,2','agent.identity+4u,256u,2')
            if name=='unwind_armed_after_ctor':
                t=gsk_text.replace('first=input(args.first_payload);storage.arm();','first=input(args.first_payload);');assert t!=gsk_text
                return t.replace('storage.disarm();return result;','storage.arm();storage.disarm();return result;')
            if name=='unwind_armed_before_size_word':
                t=gsk_text.replace('first=input(args.first_payload);storage.arm();','first=input(args.first_payload);');assert t!=gsk_text
                old='native_tu15_release_contact_agent(memory,agent,fp,s.allocation);});';assert t.count(old)==1;return t.replace(old,old+'storage.arm();')
            assert gsk_text.count(before)==1,name;return gsk_text.replace(before,after)
        negatives=[]
        try:
            for name,before,after in gsk_mutants:
                mutated=mutate(name,before,after);assert mutated!=gsk_text;(project/KERNEL).write_text(kernel_text.replace(gsk_text,mutated))
                row=run('negative-'+name,a.compilers[0]);row['mutation']=mutated;negatives.append(row)
        finally:(project/KERNEL).write_text(kernel_text)
        assert sha(project/KERNEL)==input_hashes[KERNEL]
        negatives.append(run('negative-wrong_kernel_2e4eb0_256B',a.compilers[0],('--cmake-arg=-DGSK_WRONG_KERNEL=ON',)))
        red=[n for n in negatives if n['exit_code']!=0 and n['comparison_ran'] and n['failed_cases']>0]
        unchanged={rel:(sha(candidate/rel)==h)for rel,h in input_hashes.items()if (candidate/rel).exists()}
        report.update(positive_runs_passed=all(r['exit_code']==0 and r.get('passed')and r['failed_cases']==0 and r['cases']==len(cases)for r in positives),
            negative_controls={n['label']:dict(red=n in red,failing_cases=len(n.get('failing_case_names',[])),first_problem=(n.get('first_problems')or[{}])[0].get('problem'))for n in negatives},
            all_negative_controls_red=len(red)==len(negatives),candidate_unchanged_during_run=all(unchanged.values()),candidate_changed_files=[k for k,v in unchanged.items()if not v])
        report['passed']=bool(report['positive_runs_passed']and report['all_negative_controls_red']and report['candidate_unchanged_during_run'])
        return 0 if report['passed']else 1
    except BaseException as e:
        report['error']=(str(e)or repr(e))[:4000];report['traceback']=traceback.format_exc()[-6000:];raise
    finally:
        report['elapsed_seconds']=round(time.time()-started,1)
        report['scratch_allocated_bytes_before_cleanup']=sum(x.stat().st_blocks*512 for x in scratch.rglob('*')if x.is_file()and not x.is_symlink())
        report['retained_receipts']=sorted({str(x.relative_to(report_dir))for x in report_dir.rglob('*')if x.is_file()}|{'summary.md'})
        flush();write_summary(report_dir,report)

def write_summary(report_dir,r):
    o=r.get('original',{});ip=r.get('input_provenance',{});L=[]
    verdict='PASS' if r.get('passed') else ('DISCOVERY ONLY (no native verdict)' if r.get('discover_only') else 'FAIL')
    L+=['# actual2F83B0 GSK factory: physical TU1.5 qualification','','Generated by the validator from report.json. Verdict: **'+verdict+'**.','']
    if r.get('error'):L+=['Error: `'+r['error'][:600].replace('`',"'")+'`','']
    L+=['## Inputs','',f"- Candidate: `{r.get('candidate_source')}`; native_tu15_create_gsk_contact_agent text SHA-256 `{r.get('candidate_gsk_function_sha256')}`;",
        f"  kernel `{KERNEL}` SHA-256 `{r.get('candidate_inputs',{}).get(KERNEL)}`; {len(r.get('candidate_inputs',{}))} privately copied closure files (hashes in report.json).",
        f"- Original: unchanged English TU1.5 XBE `{NEW_PIN}`; backend: {r.get('backend')}.",
        f"- actual02 BEFORE archive `{ip.get('archive_sha256')}`: {ip.get('regions')} regions, {ip.get('member_hashes_verified')} member hashes verified, {ip.get('excluded_private_source_banks')} private source banks excluded.",
        f"- Dispatcher {ip.get('dispatcher')}; actual FP {ip.get('actual_fp')}; observer patch `{r.get('observer',{}).get('patched_sha256')}` over `{r.get('observer',{}).get('base_sha256')}`.",'']
    if o:
        L+=['## Physical original','',f"{o.get('cases')} cases ({o.get('fault_cases')} declared-fault/unwind), {o.get('physical_instructions')} single-stepped instructions, {o.get('ordered_reads')} ordered reads, {o.get('ordered_stores')} ordered stores, {o.get('allocator_calls')} allocator calls, {o.get('unwind_actions_run')} original unwind funclets run. Reads outside XBE bytes or captured banks: {o.get('uncovered_reads')}; XBE-only constant reads: {len(o.get('xbe_constant_reads_not_captured',[]))}.",'',
            '| clean case | agent | ESP after ret | FP after | stores | reads | instructions |','|---|---|---|---|---|---|---|']
        L+=[f"| {c['name']} | {c['return_word']} | {c['final_esp']} | {' '.join(c['after_fp'])} | {c['stores']} | {c['reads']} | {c['instructions']} |" for c in o.get('clean_summary',[])]
        seen={}
        for f in o.get('fault_summary',[]):seen.setdefault(f['name'].split('_fault_')[1],f)
        L+=['','| fault point (pair 0 shown; same on all 4 pairs) | access | original unwind funclets | allocator calls |','|---|---|---|---|']
        L+=[f"| {k} | {'write' if f['fault']['write'] else 'read'} {f['fault']['address']}/{f['fault']['bytes']} @ {f['fault']['instruction']} | {' '.join(f['unwind_actions']) or 'none (trylevel -1)'} | {' '.join(f['allocator_calls'])} |" for k,f in seen.items()]
        L+=['','Body coverage (executed/total): '+', '.join(f"{k} {v['executed']}/{v['total']}" for k,v in o.get('body_coverage',{}).items()),'']
    if r.get('runs'):
        L+=['## Native comparison','','| run | exit | comparison | failed/cases | stores | reads | first problem |','|---|---|---|---|---|---|---|']
        L+=[f"| {x['label']} | {x['exit_code']} | {x['comparison_ran']} | {x.get('failed_cases')}/{x.get('cases')} | {x.get('ordered_stores')} | {x.get('ordered_reads')} | {((x.get('first_problems') or [{}])[0].get('problem') or '').replace('|','/')[:160]} |" for x in r['runs']]
        L+=['',f"Positive runs passed: {r.get('positive_runs_passed')}; every negative control red on a comparison: {r.get('all_negative_controls_red')}; candidate unchanged during run: {r.get('candidate_unchanged_during_run')} {r.get('candidate_changed_files') or ''}",'']
    L+=['## Storage','',f"Owned scratch `{r.get('owned_scratch')}` ({r.get('scratch_allocated_bytes_before_cleanup')} bytes before cleanup) is removed by tools/run_test.py. No ELF, bank, fixture or build is retained.",
        'Retained here: '+', '.join(f'`{x}`' for x in r.get('retained_receipts',[])),'']
    (report_dir/'summary.md').write_text('\n'.join(L)+'\n')

DRIVER=r'''#include "halo2/native_tu15_convex_agent_factory.hpp"
#include "halo2/native_tu15_contact_shape.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
// Physical-original comparison for actual2F83B0. Original outcomes come only
// from the owned ptrace fixture; this driver supplies BEFORE bytes, the real
// formal stack-argument bytes and native-only private scratch DATA.
namespace {
using namespace halo2;using J=nlohmann::json;
constexpr std::uint32_t UI=0x4000000u,UI_BYTES=0x10000u,SP=0x200f000u;
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::string hex(std::span<const std::byte>b){constexpr char digits[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;}
std::vector<std::byte> file(const std::filesystem::path&p){std::ifstream input(p,std::ios::binary);check(bool(input),"Missing owned original bank");std::vector<char>raw((std::istreambuf_iterator<char>(input)),{});std::vector<std::byte>out(raw.size());std::transform(raw.begin(),raw.end(),out.begin(),[](auto c){return std::byte(std::uint8_t(c));});return out;}
bool private_data(std::uint32_t p){return p>=UI&&p-UI<UI_BYTES;}
struct Fault{};
struct Memory final:SourcePhysicsContactMemory {
  std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();J fault;mutable unsigned fault_count{};bool log_reads{};
  auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked native operand "+std::to_string(p));--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Unbacked native extent "+std::to_string(p)+"+"+std::to_string(n));return i;}
  void load(std::uint32_t p,std::span<std::byte>out)const{if(!fault.is_null()&&!fault.value("write",false)&&p==fault.at("address")&&out.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads&&!private_data(p))reads.push_back(J::array({p,out.size()}));const auto i=at(p,out.size());std::copy_n(i->second.begin()+(p-i->first),out.size(),out.begin());}
  void read(std::uint32_t p,std::span<std::byte>out)const override{load(p,out);}
  // Retained ordinary storage specializes the vector operand as ONE ordered
  // 16-byte access (physics_contact_record.hpp contract; same convention as
  // tests/native_tu15_geometry_tests.cpp), matching the original MOVAPS.
  void read_vector(std::uint32_t p,std::span<std::byte,16>out)const override{load(p,out);}
  void write(std::uint32_t p,std::span<const std::byte>in)override{if(!fault.is_null()&&fault.value("write",false)&&p==fault.at("address")&&in.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};const auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+(p-i->first));}
};
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::string mismatch(const J&native,const J&original){std::size_t i{};while(i<native.size()&&i<original.size()&&native[i]==original[i])++i;return " at "+std::to_string(i)+" native="+(i<native.size()?native[i].dump():"end")+" original="+(i<original.size()?original[i].dump():"end");}
}
int main(int argc,char**argv)try{
  check(argc==3,"Supply the owned physical GSK fixture and summary path");const std::filesystem::path path(argv[1]);std::ifstream input(path);const auto fixture=J::parse(input);
  check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("oracle")=="unchanged-tu15-physical-gsk-factory-actual02-v1","Actual TU1.5 physical GSK original required");
#ifdef GSK_WRONG_KERNEL
  const std::string kernel="native_tu15_create_convex_contact_agent (2E4EB0 256B negative control)";
#else
  const std::string kernel="native_tu15_create_gsk_contact_agent";
#endif
  J results=J::array();std::size_t failed_cases{},stores{},read_count{},faults{},index{};
  for(const auto&c:fixture.at("cases")){
    const auto name=c.at("name").get<std::string>();J problems=J::array();
    for(bool read_pass:{false,true}){
      try{
        Memory memory;for(const auto&bank:c.at("banks"))check(memory.banks.emplace(bank.at("address").get<std::uint32_t>(),file(path.parent_path()/bank.at("before_file").get<std::string>())).second,"Duplicate actual bank");
        check(memory.banks.emplace(UI,std::vector<std::byte>(UI_BYTES,std::byte(index%2?0x5a:0xa5))).second,"Private scratch collides with an actual bank");
        {std::uint64_t end=0;for(const auto&[first,raw]:memory.banks){check(first>=end,"Overlapping fixture banks");end=std::uint64_t(first)+raw.size();}}
        memory.fault=c.value("fault",J{});memory.log_reads=read_pass;
        J actual_stores=J::array(),calls=J::array();SourcePhysicsContactRecordObserver observe;
        if(!read_pass)observe=[&](const SourcePhysicsContactRecordStore&event){if(private_data(event.address))return;actual_stores.push_back(J{{"instruction",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});};
        NativeTu15ArrayAllocatorMemory router(memory);NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0].get<std::uint16_t>(),c.at("before_fp")[1].get<std::uint16_t>(),c.at("before_fp")[2].get<std::uint32_t>()};
        NativeTu15ArrayAllocatorServices lower;NativeTu15ArrayGrowthServices growth;
        // Services never throw: a release may run inside a noexcept unwind guard.
        // A refusal is recorded and checked after the ordered call comparison.
        std::string service_problem;const auto require=[&](bool ok,const std::string&why){if(!ok&&service_problem.empty())service_problem=why;return ok;};
        lower.allocate_owner=[&](const NativeTu15ArrayAllocationRequest&r,auto&)->std::uint32_t{calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(!require(r.owner==0x233250u,"Unqualified pool allocation owner"))return 0u;NativeTu15PoolMemory pool(memory,r.receiver,observe);return native_tu15_pool_allocate(pool,r.bytes);};
        lower.release_owner=[&](const NativeTu15ArrayReleaseRequest&r,auto&){calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"data",r.data},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(!require(r.owner==0x233420u,"Unqualified pool release owner"))return;NativeTu15PoolMemory pool(memory,r.receiver,observe);require(native_tu15_pool_release(pool,r.data,r.bytes),"Pool release needs the imported free owner");};
        growth.allocate=[&](const NativeTu15ArrayAllocationRequest&r,NativeTu15PhysicsFrameFp&f)->std::uint32_t{calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(!require(r.owner==0x149da0u,"Unqualified allocator owner"))return 0u;try{return native_tu15_route_array_allocation(router,r.bytes,r.memory_class,f,lower);}catch(const std::exception&e){require(false,e.what());return 0u;}};
        growth.release=[&](const NativeTu15ArrayReleaseRequest&r,NativeTu15PhysicsFrameFp&f){calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"data",r.data},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(!require(r.owner==0x149e00u,"Unqualified release owner"))return;try{native_tu15_route_array_release(router,r.data,r.bytes,r.memory_class,f,lower);}catch(const std::exception&e){require(false,e.what());}};
        NativeTu15ConvexAgentServices convex;convex.allocation=growth;
        convex.shape_class=[&](auto,auto owner,auto&){check(owner.identity==0xad390u,"Unqualified GSK shape class owner "+std::to_string(owner.identity));return *native_tu15_activation_type(owner.identity);};
        convex.cache.shape_center=[&](auto shape,auto owner,auto out,auto&){native_tu15_contact_shape_center(memory,shape,owner,out,observe);};
        convex.cache.support_vertex=[&](auto shape,auto owner,auto direction,auto out,auto&state){const NativeTu15ContactShapeScratch child{UI+0x2500u,UI+0x2600u,nullptr};native_tu15_contact_shape_support(memory,shape,owner,direction,out,{UI+0x2300u,UI+0x2400u,&child},state,0,observe);};
        convex.cache.vertex_encoding=[&](auto,auto owner,auto&){check(owner.identity==0x1cb030u,"Unqualified vertex encoding owner");return native_tu15_contact_shape_vertex_encoding();};
        // The real cdecl formal argument slots are read through memory, so their order is compared.
        const auto argument=[&](unsigned offset){std::array<std::byte,4>b{};memory.read(SP+offset,b);std::uint32_t n{};for(unsigned i=0;i<4;++i)n|=std::to_integer<unsigned>(b[i])<<(8u*i);return n;};
        const NativeTu15ConvexAgentArguments args{[&]{return argument(4);},[&]{return argument(8);},[&]{return argument(16);}};
        const auto host_sw=status(),host_cw=control();const auto host_mx=_mm_getcsr();bool failed=false;std::uint32_t result{};
        try{
#ifdef GSK_WRONG_KERNEL
          result=native_tu15_create_convex_contact_agent(memory,args,{UI+0x2000u,{UI+0x2100u}},fp,convex,observe).identity;
#else
          result=native_tu15_create_gsk_contact_agent(memory,args,{UI+0x2000u,{UI+0x2100u}},fp,convex,observe).identity;
#endif
        }catch(const Fault&){failed=true;}
        check(failed==c.at("declared_fault").get<bool>(),"declared fault completion differs");
        check(host_sw==status()&&host_cw==control()&&host_mx==_mm_getcsr(),"native kernel leaked host FP state");
        const J native_fp=J::array({fp.control_word,fp.status,fp.mxcsr});check(native_fp==c.at("after_fp"),"x87/SSE state differs native="+native_fp.dump()+" original="+c.at("after_fp").dump());
        if(!failed)check(result==c.at("return_word"),"returned agent differs native="+std::to_string(result)+" original="+c.at("return_word").dump());
        check(calls==c.at("allocator_calls"),"allocator call order differs"+mismatch(calls,c.at("allocator_calls")));
        check(service_problem.empty(),"native service refused: "+service_problem);
        if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));check(memory.reads==expected,"ordered read differs"+mismatch(memory.reads,expected));read_count+=memory.reads.size();}
        else{check(actual_stores==c.at("writes"),"ordered store differs"+mismatch(actual_stores,c.at("writes")));stores+=actual_stores.size();}
        for(const auto&bank:c.at("banks"))check(memory.banks.at(bank.at("address").get<std::uint32_t>())==file(path.parent_path()/bank.at("after_file").get<std::string>()),"complete touched bank differs at "+bank.at("address").dump());
      }catch(const Fault&){problems.push_back(std::string(read_pass?"read pass: ":"store pass: ")+"fault escaped native cleanup");}
      catch(const std::exception&e){problems.push_back(std::string(read_pass?"read pass: ":"store pass: ")+e.what());}
    }
    faults+=c.at("declared_fault").get<bool>();failed_cases+=!problems.empty();results.push_back(J{{"name",name},{"problems",problems}});++index;
  }
  const J summary{{"passed",failed_cases==0},{"kernel",kernel},{"cases",fixture.at("cases").size()},{"failed_cases",failed_cases},{"ordered_stores",stores},{"ordered_reads",read_count},{"fault_cases",faults},{"results",results}};
  std::ofstream(argv[2])<<summary.dump()<<'\n';std::cout<<J{{"passed",failed_cases==0},{"kernel",kernel},{"failed_cases",failed_cases}}.dump()<<'\n';return failed_cases?1:0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
'''

if __name__=='__main__':raise SystemExit(main())
