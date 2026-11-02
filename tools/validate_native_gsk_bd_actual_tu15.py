#!/usr/bin/env python3
"""Physical TU1.5 actual2F83B0 GSK qualification for groups B (class7 capsule)
and D (class4 sphere), with the group-A actual02 pairs as a regression control.

Derived from validate_native_gsk_agent_factory_actual_tu15.py (group A). The
unchanged English TU1.5 XBE is linked at its real addresses with each domain's
captured BEFORE banks into a private i386 ELF and single-stepped under a
patched copy of tools/observe_native_array_growth_tu15.cpp (ptrace, not
Unicorn). Inputs are verified root cdBodies already present in those banks
(claude-gsk-remaining-domain.md section 4); no cdBody, control bank, after
state or outcome is supplied. Every original data read must hit a captured
bank, an immutable XBE .rdata constant (listed), or a declared .text switch
table (listed, code-internal, excluded from the native read order).

Additions over the group-A oracle:
  * ordered original owner-entry trace (role-keyed class/center/support/
    encoding calls with their return sites),
  * per-invocation original traces of every shape center/support owner, with
    each operand classified relative to that call's shape/direction/output
    (stack frame = local); this exposes reads/stores that only touch private
    stack DATA (sphere zero vector, capsule dot scalar),
  * two native drivers built under GCC and Clang:
      kernel  : native_tu15_create_gsk_contact_agent with role-keyed services
                whose class/encoding values are decoded from the XBE bytes,
                compared on allocator calls, service-call order and every
                per-invocation trace;
      binding : the candidate native_tu15_create_bound_gsk_contact_agent
                (its own gates, allocator routing and x87-depth gate),
                compared on activity counters and every private shape-kernel
                store value at original instruction sites.
    Both compare return, x87/SSE state, ordered stores and reads (including
    formal stack argument slots) and complete touched banks.
  * candidate mutants (binding gates, sphere/capsule kernels) that must go red
    on a comparison in their intended domain.

Run only under tools/run_test.py; ELFs, banks, fixture, projects and builds
stay under HALO2_TEST_SCRATCH. Small receipts go to --report-dir. --root
selects the checkout whose evidence/helpers are used; --candidate-source the
read-only native candidate tree.
"""
import argparse,bisect,concurrent.futures,hashlib,inspect,json,lzma,math,os,shutil,struct,subprocess,sys,tarfile,time,traceback
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

ORACLE='unchanged-tu15-physical-gsk-factory-bd-v1'
DEFAULT_REPORT=ROOT/'build/test-results/native-gsk-bd-actual-tu15'
ENTRY=0x2f83b0
DISPATCHER,DISPATCHER_VTABLE,TABLE_OFFSET=0x100680b0,0x41c7c4,0x18c
# Root cdBody pairs, exactly as claude-gsk-remaining-domain.md sections 4.1/4.2
# and gsk-physical-oracle (A). 'captured' arguments use the archive's own typed
# factory input context/manager words (shared by every factory of that world).
DOMAINS={
 'B':dict(title='group B: class7 capsule x class8 (7,8)/(8,7)',
    archive_dir='analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-merge-point-original-v1470-20261101/iteration-03-native-list-factory-actual-before-diagnostic-vehicle-camera/required-native-contact-agent-factory-before',
    pin='dbb30f303a93f8c9c4e575f08b54939b20f36393388ee61a4624525938daf614',manifest_pin=None,
    pairs=((0x1007de2c,0x1007bd6c),(0x1007bd6c,0x1007de2c),(0x1007de2c,0x1007d32c),(0x1007d32c,0x1007de2c)),arguments='captured',classes=(7,8)),
 'D':dict(title='group D: class4 sphere x class8 (4,8)/(8,4)',
    archive_dir='analysis/physics/native-list-contact-agent-20261101-required-before/destruction-scorpion',
    pin='8e0d01bd49c9466eda23f6024658d75ed563173cb7675f957272bb473edc88ee',manifest_pin='5238976cdcbfa6297c0184bc3788285e172f833bec76a29f49154c0c65a8ecbd',
    pairs=((0x10082eec,0x1007bb4c),(0x1007bb4c,0x10082eec),(0x1008428c,0x1007d22c),(0x1007d22c,0x1008428c)),arguments='captured',classes=(4,8)),
 'A':dict(title='group A regression control: class8 x class8 (actual02)',
    archive_dir='analysis/multiplayer/physics-live-tu15-20261012/mcc-native-flip-controller-original-v1465-20261101/iteration-02-native-pair-owned-release-input-vehicle-camera/required-native-pair-consumer-before',
    pin='05ce4baa6c8d8a71326a629ce93a25eca8259a5b52edd9f51fc62fd5b3cd90dd',manifest_pin=None,
    pairs=((0x1007bd6c,0x1007d32c),(0x1007d32c,0x1007bd6c),(0x1007bd6c,0x1007c758),(0x1007c758,0x1007bd6c)),arguments=(0,0x10082380),classes=(8,)),
}
# Shape vtables -> class, and the XBE slot owners each must hold (section 3).
SHAPE_VTABLES={0x461e58:8,0x461fe0:8,0x461518:7,0x4614d8:4}
ROLE_SLOTS=dict(class_=0x14,support=0x30,center=0x38,encoding=0x3c)
EXPECTED_SLOTS={8:dict(class_=0xad390,support=0x3077f0,center=0x3077c0,encoding=0x1cb030),
                7:dict(class_=0x31ede0,support=0x2e7d90,center=0x2e7e10,encoding=0x31d520),
                4:dict(class_=0x2e8cf0,support=0x2e8d20,center=0x2e8d70,encoding=0xad740)}
CLASS_OWNERS=(0xad390,0x31ede0,0x2e8cf0);ENCODING_OWNERS=(0x1cb030,0x31d520,0xad740)
CENTER_OWNERS=(0x3077c0,0x2e7e10,0x2e8d70);SUPPORT_OWNERS=(0x3077f0,0x2e7d90,0x2e8d20)
STACK,SP,STOP,RESUME=0x2000000,0x200f000,0x5000000,0x5000100
UI,UI_BYTES=0x4000000,0x10000  # native-only private DATA; never an original input
FS_BASE,FRAME_HANDLER=0x71000000,0x32c193
IMAGE_FIRST,IMAGE_END=0x10000,0x800000
PRIVATE_SOURCE_BANKS=(0x60000000,0x70000000)  # source control/TLS DATA is never an input
ARGUMENTS_REGION=(SP+4,16)
# 3077F0 switch on the movmskps lane mask: immutable .text index/jump tables.
CODE_TABLE_SITES={0x3078a2:(0x3079a4,15),0x3078a9:(0x307988,4*16)}
OWNERS=(0x2f83b0,0x2f8300,0x2e4550,0x309060,0x2e4690,0x2e47b0,0x3077c0,0x3077f0,0x1cb030,0xad390,
        0x149da0,0x233250,0x149e00,0x233420,0x37ea00,0x37ea20,0x2f0380,0x2f7a10,
        0x31ede0,0x2e7e10,0x2e7d90,0x31d520,0x2e8cf0,0x2e8d70,0x2e8d20,0xad740)
UNITS=('native_tu15_convex_agent_factory.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_inverse_geometry.cpp',
       'native_tu15_contact_cache.cpp','native_tu15_contact_shape.cpp','native_tu15_geometry.cpp','native_tu15_activation_services.cpp',
       'native_tu15_agent_cleanup_children.cpp','native_tu15_agent_cleanup.cpp','native_tu15_array_allocator.cpp','native_tu15_array_growth.cpp')
KERNEL='src/physics/native_tu15_convex_agent_factory.cpp'
SHAPE='src/physics/native_tu15_contact_shape.cpp'
BINDING='include/halo2/native_tu15_convex_agent_binding.hpp'
RETAINED_CANDIDATE=(KERNEL,'include/halo2/native_tu15_convex_agent_factory.hpp',SHAPE,'include/halo2/native_tu15_contact_shape.hpp',BINDING)
GSK_SIGNATURE='NativeTu15ContactAgent native_tu15_create_gsk_contact_agent(SourcePhysicsContactMemory&memory,'
BINDING_SIGNATURE='inline NativeTu15ContactAgent native_tu15_create_bound_gsk_contact_agent('
CTOR_STATE0,FACTORY_STATE0=0x37ea00,0x37ea20
F,C=FACTORY_STATE0,CTOR_STATE0
def _in(owner):return lambda r,b,x:r['instruction'] in b[owner]
def _at(pc):return lambda r,b,x:r['instruction']==pc
def _slot(which,slot):return lambda r,b,x:r['instruction'] in b[0x309060] and r['address']==x[which+'_vtable']+slot
# name, access, selector, pick, original unwind actions (FuncInfo 46BBC0/46BBE4), domains (None=all).
FAULT_POINTS=(
    ('pool_entry_read','read',_in(0x233250),'first',(),None),
    ('size_word_write','write',_at(0x2f83d8),'first',(),None),
    ('ctor_user_store_write','write',_at(0x2f832a),'first',(F,),None),
    ('first_shape_read','read',_at(0x2f8341),'first',(F,),None),
    ('inverse_entry_read','read',_in(0x2e4550),'first',(C,F),None),
    ('shape_class_read','read',_at(0x2f8360),'first',(C,F),None),
    ('first_center_slot_read','read',_slot('first',0x38),'first',(C,F),None),
    ('center_entry_read','read',_in(0x3077c0),'first',(C,F),None),
    ('second_support_slot_read','read',_slot('second',0x30),'last',(C,F),None),
    ('support_entry_read','read',_in(0x3077f0),'first',(C,F),None),
    ('second_encoding_slot_read','read',_slot('second',0x3c),'last',(C,F),None),
    ('capsule_center_read','read',_at(0x2e7e14),'first',(C,F),('B',)),
    ('capsule_support_second_read','read',_at(0x2e7d99),'first',(C,F),('B',)),
    ('capsule_support_first_read','read',_at(0x2e7d9d),'first',(C,F),('B',)),
    ('capsule_constant_read','read',_at(0x2e7dd2),'first',(C,F),('B',)),
    ('empty_count_write','write',_at(0x2f838c),'first',(C,F),None),
    ('final_store_write','write',_at(0x2f8394),'first',(C,F),None),
)
# Recorded and compared, but not part of the verdict: a fault on a read of the
# mapped, immutable XBE image cannot occur in the TU1.5 process.
NON_GATING_FAULT_POINTS={'capsule_constant_read':'declared fault on the immutable XBE .rdata zero constant 0x46A0E8 (unrealizable on TU1.5); FP-at-fault is reported, not gated'}
# Capstone reports x87 store memory operands (e.g. 30792B fstp m32) as READ only.
X87_MEMORY_STORES={'fst','fstp','fist','fistp','fisttp','fbstp','fnstcw','fstcw','fnstsw','fstsw','fnstenv','fstenv','fnsave','fsave'}
# (name, driver mode, file, before, after, domains that must go red)
B_ANCHOR='if(owner.identity==0xab4b0u)return native_tu15_contact_child_class_24();'
E_ANCHOR='if(owner.identity==0x1cb030u)return native_tu15_contact_shape_vertex_encoding();'
MUTANTS=(
 ('gate_class_31ede0_refused','binding',BINDING,' || owner.identity==0x31ede0u','',('B',)),
 ('gate_class_2e8cf0_refused','binding',BINDING,' || owner.identity==0x2e8cf0u','',('D',)),
 ('gate_class_31ede0_as_5','binding',BINDING,B_ANCHOR,'if(owner.identity==0x31ede0u)return 5u;'+B_ANCHOR,('B',)),
 ('gate_class_2e8cf0_as_5','binding',BINDING,B_ANCHOR,'if(owner.identity==0x2e8cf0u)return 5u;'+B_ANCHOR,('D',)),
 ('gate_encoding_31d520_sentinel','binding',BINDING,E_ANCHOR,E_ANCHOR.replace('0x1cb030u)','0x1cb030u||owner.identity==0x31d520u)'),('B',)),
 ('gate_encoding_ad740_sentinel','binding',BINDING,E_ANCHOR,E_ANCHOR.replace('0x1cb030u)','0x1cb030u||owner.identity==0xad740u)'),('D',)),
 ('gate_encoding_role_swap_1_2','binding',BINDING,'return *encoding;','return 3u-*encoding;',('B','D')),
 ('gate_encoding_31d520_refused','binding',BINDING,'owner.identity==0x31d520u || ','',('B',)),
 ('gate_encoding_ad740_refused','binding',BINDING,' || owner.identity==0xad740u','',('D',)),
 ('sphere_center_w_one','binding',SHAPE,'const std::array<std::byte,16> zero{};m.bytes(0x2e8d7cu,',
    'std::array<std::byte,16> zero{};zero[14]=std::byte(0x80);zero[15]=std::byte(0x3f);m.bytes(0x2e8d7cu,',('D',)),
 ('sphere_center_reads_shape','binding',SHAPE,'if(output&15u)throw std::runtime_error("Native TU1.5 sphere center requires aligned vector output");',
    '(void)m.vector_bytes(shape.identity+0x10u);if(output&15u)throw std::runtime_error("Native TU1.5 sphere center requires aligned vector output");',('D',)),
 ('sphere_support_tag_0x10','binding',SHAPE,'const std::array<std::byte,16> zero{};m.bytes(0x2e8d2cu,',
    'std::array<std::byte,16> zero{};zero[12]=std::byte(0x10);m.bytes(0x2e8d2cu,',('D',)),
 ('sphere_support_w_half','binding',SHAPE,'const std::array<std::byte,16> zero{};m.bytes(0x2e8d2cu,',
    'std::array<std::byte,16> zero{};zero[15]=std::byte(0x3f);m.bytes(0x2e8d2cu,',('D',)),
 ('sphere_support_reads_shape','binding',SHAPE,'if(output&15u)throw std::runtime_error("Native TU1.5 sphere support requires aligned vector output");',
    '(void)m.word(shape.identity);if(output&15u)throw std::runtime_error("Native TU1.5 sphere support requires aligned vector output");',('D',)),
 ('sphere_support_reads_direction','kernel',SHAPE,'if(output&15u)throw std::runtime_error("Native TU1.5 sphere support requires aligned vector output");',
    '(void)m.vector_bytes(direction);if(output&15u)throw std::runtime_error("Native TU1.5 sphere support requires aligned vector output");',('D',)),
 ('capsule_center_reads_first_endpoint','binding',SHAPE,'if(owner.identity==0x2e7e10u){const auto value=m.vector_bytes(shape.identity+0x20u);',
    'if(owner.identity==0x2e7e10u){const auto value=m.vector_bytes(shape.identity+0x10u);',('B',)),
 ('capsule_support_inverted','binding',SHAPE,'const bool use_second=(status&0x0500u)!=0x0100u;','const bool use_second=(status&0x0500u)==0x0100u;',('B',)),
 ('capsule_support_skips_xbe_zero','binding',SHAPE,'m.word(0x2e7dcau,scratch.scalar,low(dot));const auto value=m.word(scratch.scalar),zero=m.word(0x46a0e8u);',
    'm.word(0x2e7dcau,scratch.scalar,low(dot));const auto value=m.word(scratch.scalar),zero=0u;',('B',)),
 ('capsule_support_tag_0x20','binding',SHAPE,'const auto feature=use_second?0x10u:0;','const auto feature=use_second?0x20u:0u;',('B',)),
 ('capsule_support_read_order','binding',SHAPE,'const auto second=m.vector(shape.identity+0x20u),first=m.vector(shape.identity+0x10u),input=m.vector(direction);',
    'const auto first=m.vector(shape.identity+0x10u),second=m.vector(shape.identity+0x20u),input=m.vector(direction);',('B',)),
 ('capsule_dot_store_site','binding',SHAPE,'m.word(0x2e7dcau,scratch.scalar,low(dot));','m.word(0x2e7dceu,scratch.scalar,low(dot));',('B',)),
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
    def regions(self):return [(a,e-a)for a,e in self.ranges]

def merge_adjacent(banks):
    """Exactly contiguous captured banks become one run (no gap is ever filled)."""
    runs=[]
    for a,raw in sorted(banks.items()):
        if runs and runs[-1][0]+len(runs[-1][1])==a:runs[-1][1].extend(raw)
        else:
            assert not runs or runs[-1][0]+len(runs[-1][1])<a,hex(a)
            runs.append([a,bytearray(raw)])
    return runs

def load_domain(d):
    adir=ROOT/d['archive_dir'];archive=adir/'actual-native-before.tar.zst'
    assert sha(archive)==d['pin'],'archive pin differs '+str(archive)
    capture_raw=(adir/'capture-native-before.json').read_bytes();capture=json.loads(capture_raw)
    if d['manifest_pin']:assert sha_bytes(capture_raw)==d['manifest_pin'],'capture manifest pin differs'
    hashes_file=adir/'all-file-hashes.json.xz';hashes=json.loads(lzma.decompress(hashes_file.read_bytes()))if hashes_file.exists()else None
    wanted={r['before_file']:r for r in capture['regions']};banks={};verified=0
    stream=subprocess.Popen(['zstd','-dc',str(archive)],stdout=subprocess.PIPE)
    with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
        for member in tar:
            key=Path(member.name).name
            if key in wanted:
                raw=tar.extractfile(member).read();r=wanted[key];assert len(raw)==r['size'],key
                h=hashes.get(key)if hashes else None
                if h is not None:assert h['bytes']==len(raw) and h['sha256']==sha_bytes(raw),key;verified+=1
                assert r['address'] not in banks,key;banks[r['address']]=raw
    assert stream.wait()==0 and len(banks)==len(capture['regions'])
    fp=capture['input']['floating_point'];depth=capture['input'].get('preserved_x87_depth',0)
    if d['arguments']=='captured':args=capture['input']['arguments'];context,user=args[2],args[3]
    else:context,user=d['arguments']
    provenance=dict(archive=d['archive_dir']+'/actual-native-before.tar.zst',archive_sha256=d['pin'],capture_manifest_sha256=sha_bytes(capture_raw),
        manifest_pin_checked=bool(d['manifest_pin']),member_hash_file=hashes_file.exists(),regions=len(banks),bank_bytes=sum(map(len,banks.values())),
        member_hashes_verified=verified,boundary=capture.get('boundary'),schema=capture.get('schema'),captured_after_consumed=capture.get('captured_after_consumed',capture.get('expected_after_consumed')),
        floating_point=fp,preserved_x87_depth=depth,context_argument=hex(context),user_argument=hex(user),argument_source=('archive typed factory input arguments[2],[3]' if d['arguments']=='captured' else 'group-A qualified oracle declaration'),
        typed_input_owner=hex(capture['input']['owner'])if 'owner' in capture['input'] and capture['input']['owner'] else None)
    return banks,(fp['x87_control_word'],fp['x87_status'],fp['mxcsr']),depth,(context,user),provenance

# Observer: the group-A patch set, plus ordered owner-entry trace and per-call
# classified traces of shape center/support owners.
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
  auto start_action=[&]{auto r=regs;r.rip=actions[next_action].first;r.rbp=actions[next_action].second;r.rsp=unwind_stack-4u;poke32(unwind_stack-4u,resume);request(PTRACE_SETREGS,child.pid,nullptr,&r);++next_action;};
  std::set<std::uint32_t> trace_entries;for(const auto&x:m.value("trace_entries",J::array()))trace_entries.insert(x.get<std::uint32_t>());
  std::map<std::uint32_t,std::string> invocation_kinds;for(const auto&x:m.value("invocations",J::array()))invocation_kinds.emplace(x.at(0).get<std::uint32_t>(),x.at(1).get<std::string>());
  const std::uint32_t stack_first=m.value("stack_first",0u),stack_bytes=m.value("stack_bytes",0u);
  struct Invocation{std::uint32_t owner,esp,shape,direction,output;bool support;std::size_t index;};std::vector<Invocation> active;J owner_trace=J::array(),invocation_log=J::array();
  auto classify=[&](const Invocation&v,std::uint32_t p,std::uint32_t n)->J{
    if(p>=v.shape&&std::uint64_t(p-v.shape)+n<=0x40u)return J{{"role","shape"},{"offset",p-v.shape}};
    if(v.support&&p>=v.direction&&std::uint64_t(p-v.direction)+n<=16u)return J{{"role","direction"},{"offset",p-v.direction}};
    if(p>=v.output&&std::uint64_t(p-v.output)+n<=16u)return J{{"role","output"},{"offset",p-v.output}};
    if(p>=stack_first&&std::uint64_t(p-stack_first)+n<=stack_bytes)return J{{"role","local"}};
    return J{{"role","absolute"},{"address",p}};};'''
    head='''executed.insert(pc);const auto registers=gp(regs);J pending=J::array();
    while(!active.empty()&&registers[4]>active.back().esp){invocation_log[active.back().index]["completed"]=true;active.pop_back();}
    if(trace_entries.count(pc))owner_trace.push_back(J{{"owner",pc},{"esp",registers[4]},{"return",word32(registers[4])},{"ecx",registers[1]},{"unwinding",unwinding}});
    if(const auto kind=invocation_kinds.find(pc);kind!=invocation_kinds.end()&&!unwinding){const bool support=kind->second=="support";
      const Invocation v{pc,registers[4],registers[1],support?word32(registers[4]+4u):0u,word32(registers[4]+(support?8u:4u)),support,invocation_log.size()};
      invocation_log.push_back(J{{"owner",pc},{"kind",kind->second},{"shape",v.shape},{"direction",v.direction},{"output",v.output},{"reads",J::array()},{"writes",J::array()},{"completed",false}});active.push_back(v);}'''
    trace_operands='''    std::vector<std::pair<std::size_t,J>> pending_invocation;
    if(!active.empty()&&!unwinding)for(const auto&a:site->second.at("operands")){
      const auto p=operand(a,registers),n=a.at("bytes").get<std::uint32_t>();const auto&v=active.back();auto c=classify(v,p,n);
      if(a.at("read").get<bool>()){J r=c;r["bytes"]=n;r["instruction"]=pc;invocation_log[v.index]["reads"].push_back(r);}
      if(a.at("write").get<bool>()){J w=c;w["instruction"]=pc;pending_invocation.emplace_back(v.index,J{{"entry",w},{"address",p},{"bytes",n}});}
    }
    if(!repeated_empty)for(const auto&a:site->second.at("operands")){'''
    after_step='''    for(auto&a:pending){a["written"]=hex(memory(child.pid,a.at("address"),a.at("bytes")));a.erase("bytes");writes.push_back(a);}
    for(auto&[index,w]:pending_invocation){J entry=w.at("entry");entry["written"]=hex(memory(child.pid,w.at("address"),w.at("bytes")));invocation_log[index]["writes"].push_back(entry);}'''
    patches=[
        ('auto retained=[&](std::uint32_t p,std::uint32_t n){for(const auto&r:m.at("regions")){const auto first=r.at("address").get<std::uint32_t>();if(p>=first&&std::uint64_t(p-first)+n<=r.at("bytes").get<std::uint32_t>())return true;}return false;};',
         'std::map<std::uint32_t,std::uint32_t> retained_regions;for(const auto&r:m.at("regions"))check(retained_regions.emplace(r.at("address").get<std::uint32_t>(),r.at("bytes").get<std::uint32_t>()).second,"Duplicate retained region");\n  {std::uint64_t end=0;for(const auto&[first,bytes]:retained_regions){check(first>=end,"Overlapping retained regions");end=std::uint64_t(first)+bytes;}}\n  auto retained=[&](std::uint32_t p,std::uint32_t n){auto i=retained_regions.upper_bound(p);if(i==retained_regions.begin())return false;--i;return std::uint64_t(p-i->first)+n<=i->second;};'),
        ('J before=J::array();for(const auto&r:m.at("regions"))before.push_back(','const J dump_regions=m.value("dump",m.at("regions"));J before=J::array();for(const auto&r:dump_regions)before.push_back('),
        ('std::set<unsigned> applied;std::set<std::uint32_t> executed;','std::set<unsigned> applied;std::set<std::uint32_t> executed;\n  '+unwind),
        ('const auto pc=std::uint32_t(regs.rip);if(pc==stop)break;','const auto pc=std::uint32_t(regs.rip);if(pc==stop)break;\n    if(unwinding&&pc==resume){if(next_action>=actions.size())break;start_action();continue;}'),
        ('executed.insert(pc);const auto registers=gp(regs);J pending=J::array();',head),
        ('    if(declared_fault)break;','    if(declared_fault&&!unwinding){for(const auto&v:active)invocation_log[v.index]["aborted"]=true;active.clear();if(!m.contains("unwind"))break;unwinding=true;collect_actions(registers[4]);if(actions.empty())break;start_action();continue;}'),
        ('    if(!repeated_empty)for(const auto&a:site->second.at("operands")){',trace_operands),
        ('    for(auto&a:pending){a["written"]=hex(memory(child.pid,a.at("address"),a.at("bytes")));a.erase("bytes");writes.push_back(a);}',after_step),
        ('J banks=J::array();for(const auto&r:m.at("regions"))banks.push_back(','J banks=J::array();for(const auto&r:dump_regions)banks.push_back('),
        ('check(site!=sites.end(),"Original array escaped authenticated complete bodies");','check(site!=sites.end(),("Original array escaped authenticated complete bodies pc="+std::to_string(pc)).c_str());'),
        ('{"declared_fault",declared_fault},','{"declared_fault",declared_fault},{"unwind",unwind_log},{"unwind_actions_run",next_action},{"final_esp",std::uint32_t(regs.rsp)},{"owner_trace",owner_trace},{"invocations",invocation_log},'),
    ]
    for old,new in patches:
        assert text.count(old)==1,old[:60];text=text.replace(old,new)
    owned=scratch/'observer';owned.mkdir();source=owned/'observe_gsk_bd_tu15.cpp';source.write_text(text)
    built=subprocess.run(['c++','-std=c++20','-O2',str(source),'-o',str(owned/'observer')],capture_output=True,text=True)
    assert built.returncode==0,('observer build failed',built.stderr[-4000:])
    return owned/'observer',text,dict(base='tools/observe_native_array_growth_tu15.cpp',base_sha256=base_sha,patched_sha256=sha_bytes(text.encode()),patches=len(patches))

def site_metadata(image):
    source=inspect.getsource(array_base.metadata);needle="assert not op.mem.segment or ins.reg_name(op.mem.segment)=='es'"
    assert source.count(needle)==1
    source=source.replace(needle,"if op.mem.segment and ins.reg_name(op.mem.segment)=='fs':continue\n                    "+needle)
    namespace=dict(array_base.__dict__,OWNERS=OWNERS);exec(source,namespace)
    m,authority=namespace['metadata'](image,{});return m['sites'],authority

def link_original(owned,image_file,high_files,stack_words,fp):
    owned.mkdir()
    stack=bytearray(0x10000);struct.pack_into('<5I',stack,SP-STACK,*stack_words);(owned/'stack.bin').write_bytes(stack)
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

def immediate(code,owner):
    raw=code.at(owner).bytes+(code.at(owner+code.at(owner).size).bytes)
    if len(raw)==6 and raw[0]==0xb8 and raw[5]==0xc3:return int.from_bytes(raw[1:5],'little'),raw.hex()
    if raw==bytes.fromhex('83c8ffc3'):return 0xffffffff,raw.hex()
    raise AssertionError(('owner is not an immediate mov eax/or eax,-1; ret',hex(owner),raw.hex()))

def normalize_invocations(invocations):
    out=[]
    for v in invocations:
        reads=[{k:x for k,x in r.items()if k!='instruction'}for r in v['reads']if r['role']!='local' and r['instruction'] not in CODE_TABLE_SITES]
        writes=[{k:x for k,x in w.items()}for w in v['writes']]
        out.append(dict(owner=v['owner'],kind=v['kind'],completed=bool(v.get('completed'))and not v.get('aborted'),reads=reads,writes=writes))
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--root',type=Path,default=ROOT,help='checkout whose evidence/helpers are used (parsed before import)')
    p.add_argument('--output',type=Path,required=True,help='owned output directory inside HALO2_TEST_SCRATCH')
    p.add_argument('--report-dir',type=Path,default=DEFAULT_REPORT,help='small retained receipts')
    p.add_argument('--candidate-source',type=Path,required=True,help='read-only native candidate tree')
    p.add_argument('--domains',nargs='+',default=['B','D','A'],choices=sorted(DOMAINS))
    p.add_argument('--compilers',nargs='+',default=['g++','clang++'])
    p.add_argument('--jobs',type=int,default=4);p.add_argument('--parallel-builds',type=int,default=3)
    p.add_argument('--skip-negatives',action='store_true')
    p.add_argument('--discover-only',action='store_true',help='original discovery/classification only; no native verdict')
    a=p.parse_args();started=time.time();assert a.root.resolve()==ROOT
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();output=a.output.resolve();assert output.is_relative_to(scratch),'outputs must stay in owned scratch'
    output.mkdir(parents=True,exist_ok=False);report_dir=a.report_dir.resolve();report_dir.mkdir(parents=True,exist_ok=True);candidate=a.candidate_source.resolve()
    assert not report_dir.is_relative_to(scratch) and not report_dir.is_relative_to(candidate)
    report=dict(passed=False,validator=str(Path(__file__).resolve()),validator_sha256=sha(__file__),argv=sys.argv,root=str(ROOT),owned_scratch=str(scratch),
        candidate_source=str(candidate),domains=a.domains,backend='physical i386 ELF single-stepped by ptrace (not Unicorn)',supplied_original_outcomes=False,
        synthetic_inputs=False,retained_large_artifacts=[],
        helper_sha256={f:sha(ROOT/f)for f in('tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tools/generate_physics_live_support_tu15.py',
            'tools/oracle_smg_firing_1_1.py','tools/research_mp_sandbox_update_functions.py','tools/run_tests.py','tools/run_test.py')})
    def flush():save(report_dir/'report.json',report)
    shutil.copyfile(__file__,report_dir/Path(__file__).name)
    try:
        # 1. Private snapshot of the candidate (compiler-derived header closure, both driver modes).
        snapshot=scratch/'candidate';(snapshot/'src/physics').mkdir(parents=True);shutil.copytree(candidate/'include',snapshot/'include')
        for unit in UNITS+('native_tu15_contact_unwind_internal.hpp',):shutil.copyfile(candidate/'src/physics'/unit,snapshot/'src/physics'/unit)
        (snapshot/'driver.cpp').write_text(DRIVER);closure=set()
        for unit,defs in [('driver.cpp',()),('driver.cpp',('-DGSK_BINDING=1',))]+[(f'src/physics/{u}',())for u in UNITS]:
            deps=subprocess.check_output(['g++','-std=c++20','-MM','-MT','x','-Iinclude',*defs,unit],cwd=snapshot,text=True).replace('\\\n',' ').split()[1:]
            closure.update(str(Path(d))for d in deps if not d.startswith('/'))
        closure.discard('driver.cpp');project=scratch/'project'
        for rel in sorted(closure):target=project/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(snapshot/rel,target)
        shutil.rmtree(snapshot);assert BINDING in closure and SHAPE in closure
        input_hashes={rel:sha(project/rel)for rel in sorted(closure)}
        kernel_text=(project/KERNEL).read_text();binding_text=(project/BINDING).read_text()
        assert kernel_text.count(GSK_SIGNATURE)==1,'Candidate has no single native_tu15_create_gsk_contact_agent'
        assert binding_text.count(BINDING_SIGNATURE)==1,'Candidate has no single native_tu15_create_bound_gsk_contact_agent'
        begin=kernel_text.index(GSK_SIGNATURE);gsk_text=kernel_text[begin:kernel_text.index('\n}\n',begin)+3]
        begin=binding_text.index(BINDING_SIGNATURE);bound_text=binding_text[begin:binding_text.index('\n}\n',begin)+3]
        report.update(candidate_inputs=input_hashes,candidate_gsk_function_sha256=sha_bytes(gsk_text.encode()),candidate_binding_function=bound_text,candidate_binding_function_sha256=sha_bytes(bound_text.encode()))
        (report_dir/'candidate').mkdir(exist_ok=True)
        for rel in RETAINED_CANDIDATE:shutil.copyfile(project/rel,report_dir/'candidate'/Path(rel).name)
        flush()
        # 2. Unchanged XBE: owner bytes, vtable slots and decoded immediate roles.
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
        flat_xbe=bytearray(IMAGE_END-IMAGE_FIRST);xbe_raw=[];sections=[]
        for s in image.sections:
            raw=image.section_bytes(s);assert IMAGE_FIRST<=s.va and s.va+max(len(raw),s.vsize)<=IMAGE_END
            flat_xbe[s.va-IMAGE_FIRST:s.va-IMAGE_FIRST+len(raw)]=raw;xbe_raw.append((s.va,len(raw)));sections.append((s.va,s.va+len(raw),s.name))
        def section_of(p,n):return next((name for lo,hi,name in sections if lo<=p and p+n<=hi),None)
        def xbe_word(p):return int.from_bytes(flat_xbe[p-IMAGE_FIRST:p-IMAGE_FIRST+4],'little')
        slots={}
        for vt,kind in SHAPE_VTABLES.items():
            assert section_of(vt,0x40)=='.rdata'
            got={role:xbe_word(vt+off)for role,off in ROLE_SLOTS.items()};assert got==EXPECTED_SLOTS[kind],(hex(vt),{k:hex(v)for k,v in got.items()})
            slots[hex(vt)]=dict(shape_class=kind,**{k.rstrip('_'):hex(v)for k,v in got.items()})
        roles=dict(class_={},encoding={})
        for role,owners in (('class_',CLASS_OWNERS),('encoding',ENCODING_OWNERS)):
            for o in owners:value,raw=immediate(code,o);roles[role][o]=value;report.setdefault('xbe_immediate_owners',{})[hex(o)]=dict(role=role.rstrip('_'),value=hex(value),bytes=raw)
        assert {SHAPE_VTABLES[vt]for vt in SHAPE_VTABLES}=={roles['class_'][EXPECTED_SLOTS[k]['class_']]for k in (8,7,4)}
        assert set(roles['class_']).isdisjoint(roles['encoding'])
        report['xbe_shape_vtables']=slots
        bodies={o:set(complete_body(code,o).code)for o in OWNERS}
        for pc,(table,n) in CODE_TABLE_SITES.items():assert pc in bodies[0x3077f0] and section_of(table,n)=='.text'
        sites,authority=site_metadata(image);corrected=[]
        for s in sites:
            ins=code.at(s['instruction'])
            if ins.mnemonic in X87_MEMORY_STORES and s['operands']:
                for op in s['operands']:op['read'],op['write']=False,True
                corrected.append(f"{s['instruction']:x} {ins.mnemonic} {ins.op_str}")
        report['x87_store_access_corrections']=corrected
        observer,observer_text,observer_info=build_observer(scratch)
        (report_dir/'observe_gsk_bd_tu15.cpp').write_text(observer_text);report.update(observer=observer_info,site_authority=authority)
        shape_site_pcs=sorted(set().union(*(bodies[o]for o in CENTER_OWNERS+SUPPORT_OWNERS)))
        role_of={**{o:'class'for o in CLASS_OWNERS},**{o:'encoding'for o in ENCODING_OWNERS},**{o:'center'for o in CENTER_OWNERS},**{o:'support'for o in SUPPORT_OWNERS}}
        trace_entries=sorted(set(role_of)|{0x2f83b0,0x2f8300,0x2e4550,0x309060,0x2e4690,0x2e47b0,0x149da0,0x233250,0x149e00,0x233420,0x37ea00,0x37ea20})
        invocation_spec=[[o,'center']for o in CENTER_OWNERS]+[[o,'support']for o in SUPPORT_OWNERS]
        unwind=dict(fs_base=FS_BASE,frame_handler=FRAME_HANDLER,resume=RESUME,stack_margin=0x400)
        base_meta_common=dict(sites=sites,unwind=unwind,trace_entries=trace_entries,invocations=invocation_spec,stack_first=STACK,stack_bytes=0x10000)
        fixture=scratch/'fixture';(fixture/'banks').mkdir(parents=True)
        def persist(raw):
            target=fixture/'banks'/(sha_bytes(raw)+'.bin')
            if not target.exists():target.write_bytes(raw)
            return str(target.relative_to(fixture))
        cases=[];executed=set();report['domains_detail']={}
        # 3. Physical original runs per domain.
        for dname in a.domains:
            d=DOMAINS[dname];banks,actual_fp,depth,(context,user),provenance=load_domain(d)
            private={k:v for k,v in banks.items()if PRIVATE_SOURCE_BANKS[0]<=k<PRIVATE_SOURCE_BANKS[1]}
            captured={k:v for k,v in banks.items()if k not in private};del banks
            provenance.update(excluded_private_source_banks=len(private),excluded_private_source_bytes=sum(map(len,private.values())));del private
            for k,v in captured.items():
                assert not(STACK<k+len(v)and k<STACK+0x10000)and not(UI<k+len(v)and k<UI+UI_BYTES)and not(0x70000000<=k<0x72000000)and not(FS_BASE<=k<FS_BASE+0x1000),hex(k)
                assert k>=IMAGE_END or (k>=IMAGE_FIRST and k+len(v)<=IMAGE_END),hex(k)
            flat=bytearray(flat_xbe);differing={};nondata_differences=[]
            for k,v in captured.items():
                if k<IMAGE_END:
                    for s in image.sections:
                        lo,hi=max(k,s.va),min(k+len(v),s.va+s.rsize)
                        if lo<hi:
                            diff=[lo+i for i,(x,y)in enumerate(zip(v[lo-k:hi-k],image.section_bytes(s)[lo-s.va:hi-s.va]))if x!=y]
                            if diff:differing[s.name]=differing.get(s.name,0)+len(diff)
                            if s.name!='.data':nondata_differences.extend(diff)
                    flat[k-IMAGE_FIRST:k-IMAGE_FIRST+len(v)]=v
            assert '.text' not in differing,('captured image bytes differ from XBE code',differing)
            provenance['captured_nondata_image_differences']=[dict(address=hex(x),section=section_of(x,1),xbe=flat_xbe[x-IMAGE_FIRST],captured=flat[x-IMAGE_FIRST])for x in sorted(nondata_differences)][:64]
            for o in OWNERS:
                for pc in bodies[o]:assert bytes(flat[pc-IMAGE_FIRST:pc-IMAGE_FIRST+code.at(pc).size])==code.at(pc).bytes,hex(pc)
            runs=merge_adjacent(captured);del captured
            captured_regions=[(a_,len(r))for a_,r in runs];captured_cover=Intervals(captured_regions);xbe_cover=Intervals(xbe_raw)
            run_starts=[a_ for a_,_ in runs]
            def word(p,n=4):
                i=bisect.bisect_right(run_starts,p)-1;a_,r=runs[i];assert a_<=p and p+n<=a_+len(r),hex(p);return int.from_bytes(r[p-a_:p-a_+n],'little')
            def image_word(p):return int.from_bytes(flat[p-IMAGE_FIRST:p-IMAGE_FIRST+4],'little')
            assert word(DISPATCHER)==DISPATCHER_VTABLE,'Captured TU1.5 dispatcher vtable differs'
            pair_inputs=[];contexts=[]
            for first,second in d['pairs']:
                row={};classes=[]
                for label,body in (('first',first),('second',second)):
                    shape,key,motion,parent=(word(body+4*i)for i in range(4));vtable=word(shape)
                    assert vtable in SHAPE_VTABLES and image_word(vtable+0x14)==EXPECTED_SLOTS[SHAPE_VTABLES[vtable]]['class_'],(hex(body),hex(vtable))
                    assert parent==0,('not a root cdBody',hex(body),hex(parent))
                    t=struct.unpack('<16f',b''.join(word(motion+0x20+4*i).to_bytes(4,'little')for i in range(16)))
                    rows=[t[0:3],t[4:7],t[8:11]];err=max(abs(sum(x*y for x,y in zip(rows[i],rows[j]))-(1.0 if i==j else 0.0))for i in range(3)for j in range(3))
                    # B/D inputs must be verified root cdBodies; the A regression pairs are the
                    # already-qualified group-A oracle inputs and are recorded, not re-gated.
                    if dname!='A':assert err<=1e-5,('motion transform is not orthonormal',hex(body),err)
                    payload={}
                    vec=lambda q:list(struct.unpack('<4f',b''.join(word(q+4*i).to_bytes(4,'little')for i in range(4))))
                    if SHAPE_VTABLES[vtable]==7:payload=dict(endpoint_a=vec(shape+0x10),endpoint_b=vec(shape+0x20),scalar_0c=struct.unpack('<f',word(shape+0xc).to_bytes(4,'little'))[0])
                    if SHAPE_VTABLES[vtable]==4:payload=dict(radius=struct.unpack('<f',word(shape+0xc).to_bytes(4,'little'))[0])
                    classes.append(SHAPE_VTABLES[vtable])
                    row[label]=dict(body=hex(body),shape=hex(shape),shape_vtable=hex(vtable),shape_class=SHAPE_VTABLES[vtable],get_type=hex(image_word(vtable+0x14)),key=hex(key),
                        motion=hex(motion),parent=hex(parent),transform_orthonormal_error=err,position=[round(x,4)for x in t[12:15]],
                        words_before_body=[hex(word(body-4*k))if captured_cover.covers(body-4*k,4)else None for k in (2,1)],**payload)
                entry=word(DISPATCHER+TABLE_OFFSET+4*((classes[0]<<5)+classes[1]))
                assert entry==ENTRY,('dispatcher pair does not map to 2F83B0',classes,hex(entry))
                row['dispatcher_entry']=hex(entry);row['classes']=classes;pair_inputs.append(row)
                contexts.append(dict(first_vtable=int(row['first']['shape_vtable'],16),second_vtable=int(row['second']['shape_vtable'],16),classes=tuple(classes)))
            provenance.update(captured_image_bytes_differing_from_xbe=differing,dispatcher=dict(address=hex(DISPATCHER),vtable=hex(DISPATCHER_VTABLE)),pairs=pair_inputs,
                actual_fp=[hex(x)for x in actual_fp],captured_runs=len(runs),captured_run_bytes=sum(n for _,n in captured_regions))
            detail=report['domains_detail'][dname]=dict(title=d['title'],input_provenance=provenance);flush()
            owned=scratch/'original'/dname;(owned/'high').mkdir(parents=True);image_file=owned/'image.bin';image_file.write_bytes(flat);high_files=[]
            for a_,r in runs:
                if a_>=IMAGE_END:path=owned/'high'/f'{a_:08x}.bin';path.write_bytes(r);high_files.append((a_,path))
            discovery_regions=[dict(address=IMAGE_FIRST,bytes=IMAGE_END-IMAGE_FIRST),dict(address=ARGUMENTS_REGION[0],bytes=ARGUMENTS_REGION[1])]+[dict(address=a_,bytes=len(r))for a_,r in runs if a_>=IMAGE_END]
            del runs
            constants={}   # immutable XBE .rdata extents read by the original but absent from the capture
            classification=dict(captured=0,xbe_rdata=[],code_table=[],uncovered=[])
            def compare_regions():return Intervals(captured_regions+[ARGUMENTS_REGION]+list(constants.items())).regions()
            domain_cases=[]
            def run_case(name,elf,fault,expected_actions,stack_words,ctx):
                found=observe(observer,elf,dict(base_meta_common,regions=discovery_regions,dump=[],**({'fault':fault}if fault else{})),owned,name+'.discovery')
                for r in found['reads']:
                    ad,n=r['address'],r['bytes']
                    if r['instruction'] in CODE_TABLE_SITES:
                        lo,size=CODE_TABLE_SITES[r['instruction']];assert lo<=ad and ad+n<=lo+size and xbe_cover.covers(ad,n),(name,'switch table read outside its .text table',r)
                        classification['code_table'].append(dict(case=name,instruction=r['instruction'],address=ad,bytes=n));continue
                    if captured_cover.covers(ad,n) or Intervals([ARGUMENTS_REGION]).covers(ad,n):classification['captured']+=1
                    elif xbe_cover.covers(ad,n) and section_of(ad,n)=='.rdata':
                        constants[ad]=max(constants.get(ad,0),n);classification['xbe_rdata'].append(dict(case=name,instruction=r['instruction'],address=ad,bytes=n))
                    else:classification['uncovered'].append(dict(case=name,instruction=r['instruction'],address=ad,bytes=n,section=section_of(ad,n)))
                regions=compare_regions();cover=Intervals(regions)
                outside=[w for w in found['writes']if not captured_cover.covers(w['address'],len(w['written'])//2)]
                assert not outside,(name,'original writes outside captured banks',outside[:4])
                expected_reads=[r for r in found['reads']if r['instruction'] not in CODE_TABLE_SITES and cover.covers(r['address'],r['bytes'])]
                accesses=[(r['address'],r['bytes'])for r in expected_reads]+[(w['address'],len(w['written'])//2)for w in found['writes']]
                dump=dump_extents(accesses+[ARGUMENTS_REGION],regions)
                result=observe(observer,elf,dict(base_meta_common,regions=[dict(address=x,bytes=n)for x,n in regions],dump=dump,**({'fault':fault}if fault else{})),owned,name)
                result['reads']=[r for r in result['reads']if r['instruction'] not in CODE_TABLE_SITES]
                assert result['reads']==expected_reads and result['writes']==found['writes'] and result['allocator_calls']==found['allocator_calls'],name+' physical comparison run differs from discovery'
                assert result['owner_trace']==found['owner_trace'] and result['invocations']==found['invocations'],name+' physical trace differs from discovery'
                actions=tuple(x['action']for f in result['unwind']for x in f['actions'])
                assert bool(result['declared_fault'])==bool(fault),name+' declared fault not reached'
                assert actions==tuple(expected_actions) and result['unwind_actions_run']==len(actions),(name,'original FuncInfo unwind differs from expectation',[hex(x)for x in actions])
                if not fault:assert result['final_esp']==SP+4,(name,'cdecl return ESP',hex(result['final_esp']))
                service_calls=[dict(role=role_of[t['owner']],owner=t['owner'])for t in result['owner_trace']if t['owner'] in role_of and not t['unwinding']]
                invocations=normalize_invocations(result['invocations'])
                if not fault:
                    assert [s['role']for s in service_calls]==['class','center','center','support','support','encoding','encoding'],(name,service_calls)
                    expect_cls=[SHAPE_VTABLES[ctx['second_vtable']]];assert [roles['class_'][s['owner']]for s in service_calls if s['role']=='class']==expect_cls
                    assert all(v['completed']for v in invocations)
                before={b['address']:b['before']for b in result.pop('initial_banks')};after=result.pop('banks')
                executed.update(result['executed_sites'])
                point=name.split('_fault_')[1]if fault else None
                case=dict(name=name,domain=dname,entry=ENTRY,before_fp=list(actual_fp),x87_depth=depth,declared_fault=bool(fault),**({'fault':fault}if fault else{}),
                    gating=point not in NON_GATING_FAULT_POINTS,
                    banks=[dict(address=b['address'],before_file=persist(bytes.fromhex(before[b['address']])),after_file=persist(bytes.fromhex(b['after'])))for b in after],
                    service_calls=service_calls,invocations=invocations,private_shape_writes=[dict(instruction=w['instruction'],written=w['written'])for v in invocations for w in v['writes']],
                    owner_trace=[dict(owner=t['owner'],ret=t['return'],unwinding=t['unwinding'])for t in result['owner_trace']],
                    **{k:result[k]for k in('after_fp','return_word','final_esp','allocator_calls','reads','writes','unwind','original_instructions')})
                domain_cases.append(case);return case
            for index,(pair,ctx) in enumerate(zip(d['pairs'],contexts)):
                stack_words=(STOP,pair[0],pair[1],context,user)
                elf=link_original(owned/f'pair{index}',image_file,high_files,stack_words,actual_fp);tag=f'{dname}{index}_{pair[0]:08x}_{pair[1]:08x}'
                clean=run_case(tag+'_clean',elf,None,(),stack_words,ctx)
                for point,kind,select,pick,expected,domains in FAULT_POINTS:
                    if domains and dname not in domains:continue
                    rows=clean['reads']if kind=='read'else clean['writes']
                    written=[(w['address'],len(w['written'])//2)for w in clean['writes']]
                    matches=[i for i,r in enumerate(rows)if select(r,bodies,ctx) and (kind=='write' or not any(x<r['address']+r['bytes']and r['address']<x+n for x,n in written))]
                    assert matches,(tag,point,'fault point not present in the clean original')
                    chosen=matches[0]if pick=='first'else matches[-1];r=rows[chosen];n=r['bytes']if kind=='read'else len(r['written'])//2
                    occurrence=sum(1 for q in rows[:chosen+1]if(q['address'],(q['bytes']if kind=='read'else len(q['written'])//2))==(r['address'],n))
                    fault=dict(address=r['address'],bytes=n,occurrence=occurrence,write=kind=='write',instruction=r['instruction'])
                    run_case(f'{tag}_fault_{point}',elf,fault,expected,stack_words,ctx)
                shutil.rmtree(owned/f'pair{index}')
            shutil.rmtree(owned)
            touched_nondata=sorted({hex(x)for c in domain_cases for r in c['reads']for x in nondata_differences if r['address']<=x<r['address']+r['bytes']})
            assert not classification['uncovered'],(dname,'original reads outside XBE constants and captured banks',classification['uncovered'][:8])
            uniq=lambda rows:sorted({(r['address'],r['bytes'],r['instruction'])for r in rows})
            clean=[c for c in domain_cases if not c['declared_fault']]
            detail['original']=dict(cases=len(domain_cases),fault_cases=sum(c['declared_fault']for c in domain_cases),physical_instructions=sum(c['original_instructions']for c in domain_cases),
                ordered_reads=sum(len(c['reads'])for c in domain_cases),ordered_stores=sum(len(c['writes'])for c in domain_cases),allocator_calls=sum(len(c['allocator_calls'])for c in domain_cases),
                reads_in_captured_banks=classification['captured'],uncovered_reads=0,
                xbe_rdata_constant_reads=[dict(address=hex(a_),bytes=n,instruction=hex(i),count=sum(1 for r in classification['xbe_rdata']if(r['address'],r['bytes'],r['instruction'])==(a_,n,i)))for a_,n,i in uniq(classification['xbe_rdata'])],
                xbe_code_table_reads=[dict(address=hex(a_),bytes=n,instruction=hex(i))for a_,n,i in uniq(classification['code_table'])],
                captured_nondata_image_differences_read=touched_nondata,
                unwind_actions_run=sum(sum(len(f['actions'])for f in c['unwind'])for c in domain_cases),
                clean_summary=[dict(name=c['name'],return_word=hex(c['return_word']),final_esp=hex(c['final_esp']),after_fp=[hex(x)for x in c['after_fp']],stores=len(c['writes']),reads=len(c['reads']),
                    allocator_calls=[hex(x['owner'])for x in c['allocator_calls']],instructions=c['original_instructions'],service_calls=[f"{s['role']}:{s['owner']:x}"for s in c['service_calls']],
                    shape_kernel_private_writes=[f"{w['instruction']:x}={w['written']}"for w in c['private_shape_writes']if w['instruction'] in (0x2e8d7c,0x2e8d2c,0x2e7e18,0x2e7dca,0x2e7de9,0x2e7dec,0x2e7dff,0x2e7e02)],
                    cache_stores=[f"{w['instruction']:x}@+{w['address']-c['return_word']:x}={w['written']}"for w in c['writes']if 0<=w['address']-c['return_word']<0xf0 and w['instruction'] in range(0x309060,0x309150)])for c in clean],
                fault_summary=[dict(name=c['name'],fault={k:(hex(v)if isinstance(v,int)and not isinstance(v,bool)else v)for k,v in c['fault'].items()},unwind_actions=[hex(x['action'])for f in c['unwind']for x in f['actions']],
                    stores=len(c['writes']),allocator_calls=[hex(x['owner'])for x in c['allocator_calls']],service_calls=[f"{s['role']}:{s['owner']:x}"for s in c['service_calls']])for c in domain_cases if c['declared_fault']])
            cases.extend(domain_cases);flush()
        report['body_coverage']={hex(o):dict(total=len(bodies[o]),executed=len(bodies[o]&executed))for o in OWNERS}
        flush()
        if a.discover_only:report.update(passed=False,discover_only=True,native_validation_not_run=True);return 0
        # 4. Native comparison: kernel and binding drivers under each compiler; then mutants.
        fixture_json=fixture/'original.json'
        save(fixture_json,dict(image_sha256=NEW_PIN,oracle=ORACLE,cases=cases,shape_site_pcs=shape_site_pcs,
            roles={'class':{str(k):v for k,v in roles['class_'].items()},'encoding':{str(k):v for k,v in roles['encoding'].items()}}))
        units=' '.join(f'src/physics/{u}'for u in UNITS)
        cmake=('cmake_minimum_required(VERSION 3.20)\nproject(gsk_bd_oracle LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\n'
            f'add_executable(gsk driver.cpp {units})\ntarget_include_directories(gsk PRIVATE include src/physics)\ntarget_compile_features(gsk PRIVATE cxx_std_20)\n'
            'target_compile_options(gsk PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nif(GSK_BINDING)\n  target_compile_definitions(gsk PRIVATE GSK_BINDING=1)\nendif()\n'
            'target_link_libraries(gsk PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME gsk_factory COMMAND gsk "${GSK_FIXTURE}" "${GSK_SUMMARY}")\n')
        (project/'driver.cpp').write_text(DRIVER);(project/'CMakeLists.txt').write_text(cmake)
        shutil.copyfile(project/'driver.cpp',report_dir/'driver.cpp');shutil.copyfile(project/'CMakeLists.txt',report_dir/'CMakeLists.txt')
        report['fixture']=dict(sha256=sha(fixture_json),cases=len(cases),bank_files=len(list((fixture/'banks').iterdir())),bank_bytes=sum(x.stat().st_size for x in (fixture/'banks').iterdir()))
        runs_dir=output/'runs';runs_dir.mkdir();projects=scratch/'projects';projects.mkdir();report['runs']=[];report['commands']=[]
        def run(label,compiler,mode,mutation=None):
            source=projects/label;shutil.copytree(project,source)
            if mutation:
                rel,before,after=mutation;text=(source/rel).read_text();assert text.count(before)==1,(label,'mutation anchor not unique');(source/rel).write_text(text.replace(before,after))
            summary_path=output/f'{label}.summary.json'
            cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),'--output',str(runs_dir/label),'--target','gsk',
                 '--filter','^gsk_factory$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DGSK_FIXTURE='+str(fixture_json),'--cmake-arg=-DGSK_SUMMARY='+str(summary_path),
                 *(['--cmake-arg=-DGSK_BINDING=ON']if mode=='binding'else[]),'--jobs',str(a.jobs),'--test-jobs','1']
            r=subprocess.run(cmd,capture_output=True,text=True)
            summary=json.loads(summary_path.read_text())if summary_path.exists()else None
            row=dict(label=label,compiler=compiler,mode=mode,exit_code=r.returncode,comparison_ran=summary is not None,command=cmd)
            if mutation:row['mutation']=dict(file=mutation[0],before=mutation[1],after=mutation[2])
            if summary:row.update(passed=summary['passed'],kernel=summary['kernel'],cases=summary['cases'],failed_cases=summary['failed_cases'],ordered_stores=summary['ordered_stores'],
                ordered_reads=summary['ordered_reads'],fault_cases=summary['fault_cases'],invocations_compared=summary.get('invocations_compared'),private_shape_writes_compared=summary.get('private_shape_writes_compared'),
                service_calls_compared=summary.get('service_calls_compared'),activity_checks=summary.get('activity_checks'),
                first_problems=[dict(case=x['name'],problem=x['problems'][0][:400])for x in summary['results']if x['problems']][:6],
                failing_case_names=[x['name']for x in summary['results']if x['problems']])
            if summary and not mutation:row['failing_problems']={x['name']:[p[:600]for p in x['problems']]for x in summary['results']if x['problems']}
            if summary and mutation:row['_case_problems']={x['name']:x['problems'][0][:400]for x in summary['results']if x['problems']}
            else:
                log=runs_dir/label/'build.log';row['log_tail']=(log.read_text(errors='replace')[-3000:]if log.exists()else r.stderr[-3000:])
            shutil.rmtree(source,ignore_errors=True);return row
        def run_all(jobs):
            with concurrent.futures.ThreadPoolExecutor(max_workers=max(1,a.parallel_builds))as pool:
                rows=list(pool.map(lambda j:run(*j),jobs))
            report['runs'].extend(rows);flush();return rows
        positives=run_all([(f'positive-{mode}-'+c.replace('+','x'),c,mode)for mode in ('kernel','binding')for c in a.compilers])
        gating={c['name']for c in cases if c['gating']};domain_of={c['name']:c['domain']for c in cases}
        for r in positives:
            failing=set(r.get('failing_case_names',[]));r['gating_failures']=sorted(failing&gating);r['non_gating_failures']=sorted(failing-gating)
            r['gating_passed']=bool(r['comparison_ran']and r['cases']==len(cases)and not r['gating_failures'])
        report['positive_runs_passed']=all(r['gating_passed']for r in positives)
        report['positive_runs_fully_green']=all(r['exit_code']==0 and r.get('passed')and r['failed_cases']==0 and r['cases']==len(cases)for r in positives)
        report['non_gating_cases']={n:NON_GATING_FAULT_POINTS[n.split('_fault_')[1]]for n in sorted(set(domain_of)-gating)}
        negatives=[]
        if not a.skip_negatives:
            jobs=[]
            for name,mode,rel,before,after,domains in MUTANTS:
                if not set(domains)&set(a.domains):continue
                jobs.append((f'negative-{name}',a.compilers[0],mode,(rel,before,after)))
            negatives=run_all(jobs)
            expected={f'negative-{n}':set(ds)for n,_,_,_,_,ds in MUTANTS}
            baseline={r['mode']:set(r.get('failing_case_names',[]))for r in positives if r['compiler']==a.compilers[0]}
            for row in negatives:
                new=(set(row.get('failing_case_names',[]))&gating)-baseline.get(row['mode'],set())
                failing={domain_of[n]for n in new};row['new_gating_failures']=len(new);row['failing_domains']=sorted(failing)
                row['red']=bool(row['exit_code']!=0 and row['comparison_ran'] and new and (expected[row['label']]&set(a.domains))<=failing)
                problems=row.pop('_case_problems',{});first=next((n for n in (c['name']for c in cases)if n in new),None)
                row['first_new_gating_failure']=dict(case=first,problem=problems.get(first))if first else None
            flush()
        unchanged={rel:(sha(candidate/rel)==h)for rel,h in input_hashes.items()if (candidate/rel).exists()}
        report.update(negative_controls={n['label']:dict(red=n['red'],mode=n['mode'],new_gating_failures=n['new_gating_failures'],failing_domains=n['failing_domains'],first_new_gating_failure=n['first_new_gating_failure'])for n in negatives},
            all_negative_controls_red=bool(negatives)and all(n['red']for n in negatives),negatives_run=not a.skip_negatives,
            candidate_unchanged_during_run=all(unchanged.values()),candidate_changed_files=[k for k,v in unchanged.items()if not v])
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
    L=[];verdict='PASS' if r.get('passed') else ('DISCOVERY ONLY (no native verdict)' if r.get('discover_only') else 'FAIL')
    L+=['# actual2F83B0 GSK groups B/D: physical TU1.5 qualification','','Generated by the validator from report.json. Verdict: **'+verdict+'**.','']
    if r.get('error'):L+=['Error: `'+r['error'][:600].replace('`',"'")+'`','']
    L+=['## Inputs','',f"- Candidate (read only): `{r.get('candidate_source')}`; GSK factory text `{r.get('candidate_gsk_function_sha256')}`; GSK binding text `{r.get('candidate_binding_function_sha256')}`; {len(r.get('candidate_inputs',{}))} closure files hashed in report.json.",
        f"- Original: unchanged English TU1.5 XBE `{NEW_PIN}`; {r.get('backend')}. Observer patch `{r.get('observer',{}).get('patched_sha256')}` over `{r.get('observer',{}).get('base_sha256')}`.",
        '- XBE immediate owners (decoded, role keyed): '+', '.join(f"{k} {v['role']}={v['value']}" for k,v in r.get('xbe_immediate_owners',{}).items()),'']
    for dname,dd in r.get('domains_detail',{}).items():
        ip=dd.get('input_provenance',{});o=dd.get('original',{})
        L+=[f"## Domain {dname}: {dd.get('title')}",'',f"- Archive `{ip.get('archive')}` `{ip.get('archive_sha256')}`: {ip.get('regions')} regions, member hashes verified {ip.get('member_hashes_verified')} (hash file present: {ip.get('member_hash_file')}), manifest pin checked {ip.get('manifest_pin_checked')}; {ip.get('excluded_private_source_banks')} private source banks excluded.",
            f"- FP {ip.get('actual_fp')}, x87 depth {ip.get('preserved_x87_depth')}; context {ip.get('context_argument')}, user {ip.get('user_argument')} ({ip.get('argument_source')}).",'',
            '| pair | first body/shape (class) | second body/shape (class) | dispatcher |','|---|---|---|---|']
        L+=[f"| {i} | {p['first']['body']}/{p['first']['shape']} ({p['first']['shape_class']}) | {p['second']['body']}/{p['second']['shape']} ({p['second']['shape_class']}) | {p['dispatcher_entry']} |" for i,p in enumerate(ip.get('pairs',[]))]
        if o:
            L+=['',f"Physical original: {o.get('cases')} cases ({o.get('fault_cases')} declared-fault/unwind), {o.get('physical_instructions')} single-stepped instructions, {o.get('ordered_reads')} ordered reads, {o.get('ordered_stores')} ordered stores, {o.get('allocator_calls')} allocator calls, {o.get('unwind_actions_run')} original unwind funclets. Uncovered reads: {o.get('uncovered_reads')}.",
                f"XBE .rdata constant reads (immutable, not in the capture): {', '.join(x['address']+'/'+str(x['bytes'])+'@'+x['instruction']+' x'+str(x['count']) for x in o.get('xbe_rdata_constant_reads',[])) or 'none'}. XBE .text switch-table reads (code-internal, excluded from native read order): {', '.join(x['address']+'@'+x['instruction'] for x in o.get('xbe_code_table_reads',[])) or 'none'}.",'',
                '| clean case | agent | ESP | FP after | stores | reads | service calls | cache stores (309060) | shape-kernel private stores |','|---|---|---|---|---|---|---|---|---|']
            L+=[f"| {c['name']} | {c['return_word']} | {c['final_esp']} | {' '.join(c['after_fp'])} | {c['stores']} | {c['reads']} | {' '.join(c['service_calls'])} | {' '.join(c['cache_stores'])} | {' '.join(c['shape_kernel_private_writes'])} |" for c in o.get('clean_summary',[])]
            seen={}
            for f in o.get('fault_summary',[]):seen.setdefault(f['name'].split('_fault_')[1],f)
            L+=['','| fault point (first pair shown) | access | original unwind funclets | allocator calls |','|---|---|---|---|']
            L+=[f"| {k} | {'write' if f['fault']['write'] else 'read'} {f['fault']['address']}/{f['fault']['bytes']} @ {f['fault']['instruction']} | {' '.join(f['unwind_actions']) or 'none (trylevel -1)'} | {' '.join(f['allocator_calls'])} |" for k,f in seen.items()]
        L+=['']
    if r.get('body_coverage'):L+=['Body coverage (executed/total): '+', '.join(f"{k} {v['executed']}/{v['total']}" for k,v in r['body_coverage'].items()),'']
    if r.get('runs'):
        L+=['## Native comparison','','Positive runs (every case; counts are compared items over both passes of all cases):','',
            '| run | exit | failed/cases | gating failures | ordered stores | ordered reads | service calls | shape invocations | private shape stores | binding activity checks |','|---|---|---|---|---|---|---|---|---|---|']
        L+=[f"| {x['label']} | {x['exit_code']} | {x.get('failed_cases')}/{x.get('cases')} | {len(x.get('gating_failures',[]))} | {x.get('ordered_stores')} | {x.get('ordered_reads')} | {x.get('service_calls_compared')} | {x.get('invocations_compared')} | {x.get('private_shape_writes_compared')} | {x.get('activity_checks')} |" for x in r['runs']if x['label'].startswith('positive-')]
        L+=['','Negative controls (candidate mutants, '+(r['runs'][0]['compiler']if r['runs']else'')+'): red = comparison ran and new gating failures in every intended domain.','',
            '| mutant | mode | red | new gating failures | domains | first new gating failure |','|---|---|---|---|---|---|']
        L+=[f"| {x['label'][9:]} | {x['mode']} | {x.get('red')} | {x.get('new_gating_failures')} | {' '.join(x.get('failing_domains',[]))} | {(((x.get('first_new_gating_failure')or{}).get('case')or'')+': '+((x.get('first_new_gating_failure')or{}).get('problem')or x.get('log_tail','')[-160:]or'')).replace('|','/').replace(chr(10),' ')[:230]} |" for x in r['runs']if x['label'].startswith('negative-')]
        L+=['',f"Positive runs passed on every gating case: {r.get('positive_runs_passed')} (fully green including non-gating: {r.get('positive_runs_fully_green')}); every negative control red on new gating failures in its intended domain: {r.get('all_negative_controls_red')}; candidate unchanged during run: {r.get('candidate_unchanged_during_run')} {r.get('candidate_changed_files') or ''}",'']
        for x in r['runs']:
            if x['label'].startswith('positive-')and x.get('failing_problems'):
                L+=[f"- {x['label']} failing cases ({'gating' if x.get('gating_failures') else 'non-gating only'}):"]
                L+=[f"  - `{n}`: {' / '.join(p.replace('|','/') for p in ps)[:500]}" for n,ps in x['failing_problems'].items()]
        if r.get('non_gating_cases'):L+=['','Non-gating cases: '+'; '.join(f'`{k}`' for k in r['non_gating_cases'])+' — '+next(iter(r['non_gating_cases'].values())),'']
    if r.get('x87_store_access_corrections') is not None:L+=[f"Observer metadata correction (capstone reports x87 memory stores as reads): {', '.join(r['x87_store_access_corrections']) or 'none'}.",'']
    dots=[]
    for dname,dd in r.get('domains_detail',{}).items():
        for c in dd.get('original',{}).get('clean_summary',[]):
            for w in c.get('shape_kernel_private_writes',[]):
                if w.startswith('2e7dca='):dots.append(f"{c['name'].split('_')[0]} {struct.unpack('<f',bytes.fromhex(w.split('=')[1]))[0]:+.4f}")
    a_inputs=[p for p in r.get('domains_detail',{}).get('A',{}).get('input_provenance',{}).get('pairs',[])]
    odd={x['body']:x for p in a_inputs for x in (p['first'],p['second'])if x['transform_orthonormal_error']>1e-5}
    ng=[(x['label'],n,(x.get('failing_problems',{}).get(n)or[''])[0])for x in r.get('runs',[])if x['label'].startswith('positive-')for n in x.get('non_gating_failures',[])]
    L+=['## Findings and exact limits','',
        '- Gating domain: the 4 B pairs and 4 D pairs above (plus the 4 A regression pairs), each clean and with every declared fault/unwind point listed; nothing else.',
        (f"- Non-gating, reported divergence ({len(ng)} positive case runs red, all non-gating; e.g. `{ng[0][1]}`: {ng[0][2][:160]}). A declared fault on the capsule support read of the XBE .rdata zero `0x46A0E8` (`2E7DD2 fcomp`) leaves the original with `fld [esp+0xC]` already executed (FSW TOP=7: `0x3920`/`0x3820`), while the candidate reads the constant before its x87 load (FSW unchanged: `0x120`/`0x20`). Control word and MXCSR match. That read cannot fault on TU1.5 (mapped immutable image), so it is not gated. To match it anyway, `2E7D90` would need fld(dot) -> read(0x46A0E8) -> fcomp order. The 2E5DB0 sweep branch uses the same pre-read pattern (not in this domain)."
            if ng else '- Non-gating cases: all passed.'),
        f"- Capsule `2E7D90` dot values on the real pairs: {', '.join(dots) or 'n/a'}. Both endpoint branches run, but the tie (dot == +/-0) and unordered (NaN) branches do not run on any real input.",
        '- Class values: `2F8300` branches only on `getType(second)==5`, so class 7 vs 6 (or 4 vs 6) is not observable. The mutants use 7->5 and 4->5 and gate removal instead. getType runs only for the second shape, so the (7,8) and (4,8) cases never call `31EDE0`/`2E8CF0`.',
        '- Sphere kernels `2E8D70`/`2E8D20` touch only the private stack output. Their zero values, store sites and absence of shape or direction reads are compared through per-invocation traces (kernel driver) and private store values (binding driver). Fault injection is address keyed, so no declared fault sits on those private stores; the adjacent slot-read and class-8 fault points cover unwind in that phase.',
        '- The binding driver builds its own allocator services, so allocator call arguments are compared only in kernel mode. Binding mode compares the ordered pool stores, return value, FP, banks, activity counters and private shape stores.',
        "- In the candidate binding, the encoding gate returns `native_tu15_activation_type()` (a getType table) for `31D520` and `AD740`. The values (2 and 1) match the XBE immediates, and the explicit owner list gates them, so behaviour is correct here. The role-confusion hazard from the handoff is still in the source. The class gate also admits `31EDF0` (class 6), which is not qualified here: class 6 and 24 appear only as children, with no captured root pairs.",
        '- The capstone metadata used by the shared observer helper reports x87 memory stores as reads. This validator corrects `30792B fstp m32` (the only x87 store in these 26 owner bodies; it targets the private stack, so the group-A global logs were unaffected). Any other oracle that uses `validate_native_array_growth_tu15.metadata` with x87 stores to captured memory would mis-log them.',
        '- 3077F0 switch-table (.text) reads: '+('none occurred; every lane mask was 0 or 15, so partial-mask lane selection is not exercised.' if not any(dd.get('original',{}).get('xbe_code_table_reads') for dd in r.get('domains_detail',{}).values()) else 'present and excluded from the native read order (code-internal).'),
        "- Captured `.rdata` `0x41B774..0x41B777` differs from the XBE in all three archives (runtime-patched). No case reads it.",
        ('- The A regression inputs are the existing group-A oracle pairs, recorded but not re-gated as roots: '+'; '.join(f"`{b}` words-before {x['words_before_body']}, motion+0x20 orthonormal error {x['transform_orthonormal_error']:.3f}, position {x['position']}" for b,x in odd.items())+'. They do not meet the remaining-domain root criteria; the A code-path comparison on captured bytes is unaffected.') if odd else '- A regression inputs: all orthonormal.',
        '- B and D archives are not Zanzibar captures (map unidentified); the kernels are content independent. The D archive has no member hash file (archive and manifest pins only). The B/D context and user formals come from each archive\'s typed factory input arguments [2] and [3]. The context formal is never read, and user is only stored at agent+8.',
        '- Original FuncInfo unwind funclets run physically. Only their dispatch order is modelled, as in the group-A oracle.',
        '- Not covered: group C (no retail data), group E, class-6/24 pairs, the world-integration admission site, the live terminal route, and the `2F8B60`/`2F8630` post-creation slots.','']
    L+=['## Storage','',f"Owned scratch `{r.get('owned_scratch')}` ({r.get('scratch_allocated_bytes_before_cleanup')} bytes before cleanup) is removed by tools/run_test.py. No ELF, bank, fixture, project or build is retained.",
        'Retained here: '+', '.join(f'`{x}`' for x in r.get('retained_receipts',[])),'']
    (report_dir/'summary.md').write_text('\n'.join(L)+'\n')

DRIVER=r'''#include "halo2/native_tu15_convex_agent_factory.hpp"
#include "halo2/native_tu15_contact_shape.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#ifdef GSK_BINDING
#include "halo2/native_tu15_convex_agent_binding.hpp"
#endif
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
// Physical-original comparison for actual2F83B0 groups B/D (+A). Original
// outcomes come only from the owned ptrace fixture; this driver supplies
// BEFORE bytes, the real formal stack-argument bytes and native-only private
// scratch DATA. Kernel mode: role-keyed services decoded from XBE bytes.
// Binding mode: the candidate's own GSK binding, gates and allocator routing.
namespace {
using namespace halo2;using J=nlohmann::json;
constexpr std::uint32_t UI=0x4000000u,UI_BYTES=0x10000u,SP=0x200f000u;
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::string hex(std::span<const std::byte>b){constexpr char digits[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;}
std::vector<std::byte> file(const std::filesystem::path&p){std::ifstream input(p,std::ios::binary);check(bool(input),"Missing owned original bank");std::vector<char>raw((std::istreambuf_iterator<char>(input)),{});std::vector<std::byte>out(raw.size());std::transform(raw.begin(),raw.end(),out.begin(),[](auto c){return std::byte(std::uint8_t(c));});return out;}
bool private_data(std::uint32_t p){return p>=UI&&p-UI<UI_BYTES;}
struct Fault{};
struct Invocation{std::string kind;std::uint32_t owner{},shape{},direction{},output{};bool support{},completed{};J reads=J::array(),writes=J::array();};
J classify(const Invocation&v,std::uint32_t p,std::size_t n){
  if(p>=v.shape&&std::uint64_t(p-v.shape)+n<=0x40u)return J{{"role","shape"},{"offset",p-v.shape}};
  if(v.support&&p>=v.direction&&std::uint64_t(p-v.direction)+n<=16u)return J{{"role","direction"},{"offset",p-v.direction}};
  if(p>=v.output&&std::uint64_t(p-v.output)+n<=16u)return J{{"role","output"},{"offset",p-v.output}};
  if(private_data(p))return J{{"role","local"}};
  return J{{"role","absolute"},{"address",p}};
}
struct Memory final:SourcePhysicsContactMemory {
  std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();J fault;mutable unsigned fault_count{};bool log_reads{};
  std::vector<Invocation>*invocations{};int active=-1;
  auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked native operand "+std::to_string(p));--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Unbacked native extent "+std::to_string(p)+"+"+std::to_string(n));return i;}
  void load(std::uint32_t p,std::span<std::byte>out)const{if(!fault.is_null()&&!fault.value("write",false)&&p==fault.at("address")&&out.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};
    if(log_reads&&!private_data(p))reads.push_back(J::array({p,out.size()}));
    if(log_reads&&invocations&&active>=0){auto c=classify((*invocations)[active],p,out.size());if(c.at("role")!="local"){c["bytes"]=out.size();(*invocations)[active].reads.push_back(c);}}
    const auto i=at(p,out.size());std::copy_n(i->second.begin()+(p-i->first),out.size(),out.begin());}
  void read(std::uint32_t p,std::span<std::byte>out)const override{load(p,out);}
  // Retained ordinary storage specializes the vector operand as ONE ordered
  // 16-byte access (physics_contact_record.hpp contract), matching MOVAPS.
  void read_vector(std::uint32_t p,std::span<std::byte,16>out)const override{load(p,out);}
  void write(std::uint32_t p,std::span<const std::byte>in)override{if(!fault.is_null()&&fault.value("write",false)&&p==fault.at("address")&&in.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};const auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+(p-i->first));}
};
struct Active{Memory&m;std::vector<Invocation>&v;bool done{};~Active(){if(done)v[m.active].completed=true;m.active=-1;}};
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::string mismatch(const J&native,const J&original){std::size_t i{};while(i<native.size()&&i<original.size()&&native[i]==original[i])++i;return " at "+std::to_string(i)+" native="+(i<native.size()?native[i].dump():"end")+" original="+(i<original.size()?original[i].dump():"end");}
J invocation_json(const std::vector<Invocation>&v){J out=J::array();for(const auto&x:v)out.push_back(J{{"owner",x.owner},{"kind",x.kind},{"completed",x.completed},{"reads",x.reads},{"writes",x.writes}});return out;}
}
int main(int argc,char**argv)try{
  check(argc==3,"Supply the owned physical GSK fixture and summary path");const std::filesystem::path path(argv[1]);std::ifstream input(path);const auto fixture=J::parse(input);
  check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("oracle")=="unchanged-tu15-physical-gsk-factory-bd-v1","Actual TU1.5 physical GSK original required");
#ifdef GSK_BINDING
  const std::string kernel="native_tu15_create_bound_gsk_contact_agent (candidate binding)";
#else
  const std::string kernel="native_tu15_create_gsk_contact_agent (role-keyed services)";
#endif
  std::map<std::uint32_t,std::uint32_t> class_roles,encoding_roles;
  for(const auto&[k,v]:fixture.at("roles").at("class").items())class_roles.emplace(std::uint32_t(std::stoul(k)),v.get<std::uint32_t>());
  for(const auto&[k,v]:fixture.at("roles").at("encoding").items())encoding_roles.emplace(std::uint32_t(std::stoul(k)),v.get<std::uint32_t>());
  std::set<std::uint32_t> shape_sites;for(const auto&x:fixture.at("shape_site_pcs"))shape_sites.insert(x.get<std::uint32_t>());
  J results=J::array();std::size_t failed_cases{},stores{},read_count{},faults{},index{},invocations_compared{},private_compared{},services_compared{},activity_checks{};
  for(const auto&c:fixture.at("cases")){
    const auto name=c.at("name").get<std::string>();J problems=J::array();std::vector<Invocation> pass_invocations[2];
    for(bool read_pass:{false,true}){
      try{
        Memory memory;for(const auto&bank:c.at("banks"))check(memory.banks.emplace(bank.at("address").get<std::uint32_t>(),file(path.parent_path()/bank.at("before_file").get<std::string>())).second,"Duplicate actual bank");
        check(memory.banks.emplace(UI,std::vector<std::byte>(UI_BYTES,std::byte(index%2?0x5a:0xa5))).second,"Private scratch collides with an actual bank");
        {std::uint64_t end=0;for(const auto&[first,raw]:memory.banks){check(first>=end,"Overlapping fixture banks");end=std::uint64_t(first)+raw.size();}}
        memory.fault=c.value("fault",J{});memory.log_reads=read_pass;auto&invocations=pass_invocations[read_pass];memory.invocations=&invocations;
        J actual_stores=J::array(),calls=J::array(),services=J::array(),private_writes=J::array();SourcePhysicsContactRecordObserver observe;
        if(!read_pass)observe=[&](const SourcePhysicsContactRecordStore&event){
          if(memory.active>=0){auto w=classify(invocations[memory.active],event.address,event.written.size());w["instruction"]=event.instruction;w["written"]=hex(event.written);invocations[memory.active].writes.push_back(w);}
          if(private_data(event.address)){if(shape_sites.count(event.instruction))private_writes.push_back(J{{"instruction",event.instruction},{"written",hex(event.written)}});return;}
          actual_stores.push_back(J{{"instruction",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});};
        NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0].get<std::uint16_t>(),c.at("before_fp")[1].get<std::uint16_t>(),c.at("before_fp")[2].get<std::uint32_t>()};
        // The real cdecl formal argument slots are read through memory, so their order is compared.
        const auto argument=[&](unsigned offset){std::array<std::byte,4>b{};memory.read(SP+offset,b);std::uint32_t n{};for(unsigned i=0;i<4;++i)n|=std::to_integer<unsigned>(b[i])<<(8u*i);return n;};
        const NativeTu15ConvexAgentArguments args{[&]{return argument(4);},[&]{return argument(8);},[&]{return argument(16);}};
        std::string service_problem;const auto require=[&](bool ok,const std::string&why){if(!ok&&service_problem.empty())service_problem=why;return ok;};
#ifdef GSK_BINDING
        NativeTu15ConvexAgentBindingActivity activity{};
#else
        NativeTu15ArrayAllocatorMemory router(memory);NativeTu15ArrayAllocatorServices lower;NativeTu15ArrayGrowthServices growth;
        // Services never throw: a release may run inside a noexcept unwind guard.
        lower.allocate_owner=[&](const NativeTu15ArrayAllocationRequest&r,auto&)->std::uint32_t{calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(!require(r.owner==0x233250u,"Unqualified pool allocation owner"))return 0u;NativeTu15PoolMemory pool(memory,r.receiver,observe);return native_tu15_pool_allocate(pool,r.bytes);};
        lower.release_owner=[&](const NativeTu15ArrayReleaseRequest&r,auto&){calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"data",r.data},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(!require(r.owner==0x233420u,"Unqualified pool release owner"))return;NativeTu15PoolMemory pool(memory,r.receiver,observe);require(native_tu15_pool_release(pool,r.data,r.bytes),"Pool release needs the imported free owner");};
        growth.allocate=[&](const NativeTu15ArrayAllocationRequest&r,NativeTu15PhysicsFrameFp&f)->std::uint32_t{calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(!require(r.owner==0x149da0u,"Unqualified allocator owner"))return 0u;try{return native_tu15_route_array_allocation(router,r.bytes,r.memory_class,f,lower);}catch(const std::exception&e){require(false,e.what());return 0u;}};
        growth.release=[&](const NativeTu15ArrayReleaseRequest&r,NativeTu15PhysicsFrameFp&f){calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"data",r.data},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(!require(r.owner==0x149e00u,"Unqualified release owner"))return;try{native_tu15_route_array_release(router,r.data,r.bytes,r.memory_class,f,lower);}catch(const std::exception&e){require(false,e.what());}};
        NativeTu15ConvexAgentServices convex;convex.allocation=growth;
        convex.shape_class=[&](auto,auto owner,auto&){services.push_back(J{{"role","class"},{"owner",owner.identity}});const auto i=class_roles.find(owner.identity);check(i!=class_roles.end(),"Unqualified GSK shape class owner "+std::to_string(owner.identity));return i->second;};
        convex.cache.shape_center=[&](auto shape,auto owner,auto out,auto&){services.push_back(J{{"role","center"},{"owner",owner.identity}});
          invocations.push_back(Invocation{"center",owner.identity,shape.identity,0u,out,false});memory.active=int(invocations.size()-1);Active scope{memory,invocations};
          native_tu15_contact_shape_center(memory,shape,owner,out,observe);scope.done=true;};
        convex.cache.support_vertex=[&](auto shape,auto owner,auto direction,auto out,auto&state){services.push_back(J{{"role","support"},{"owner",owner.identity}});
          invocations.push_back(Invocation{"support",owner.identity,shape.identity,direction,out,true});memory.active=int(invocations.size()-1);Active scope{memory,invocations};
          const NativeTu15ContactShapeScratch child{UI+0x2500u,UI+0x2600u,nullptr};native_tu15_contact_shape_support(memory,shape,owner,direction,out,{UI+0x2300u,UI+0x2400u,&child},state,0,observe);scope.done=true;};
        convex.cache.vertex_encoding=[&](auto,auto owner,auto&){services.push_back(J{{"role","encoding"},{"owner",owner.identity}});const auto i=encoding_roles.find(owner.identity);check(i!=encoding_roles.end(),"Unqualified vertex encoding owner "+std::to_string(owner.identity));return i->second;};
#endif
        const auto host_sw=status(),host_cw=control();const auto host_mx=_mm_getcsr();bool failed=false;std::uint32_t result{};
        try{
#ifdef GSK_BINDING
          result=native_tu15_create_bound_gsk_contact_agent(memory,args,{UI+0x2000u,UI+0x2100u,UI+0x2300u,UI+0x2400u,UI+0x2500u,UI+0x2600u},fp,c.at("x87_depth").get<unsigned>(),observe,&activity).identity;
#else
          result=native_tu15_create_gsk_contact_agent(memory,args,{UI+0x2000u,{UI+0x2100u}},fp,convex,observe).identity;
#endif
        }catch(const Fault&){failed=true;}
        check(failed==c.at("declared_fault").get<bool>(),"declared fault completion differs");
        check(host_sw==status()&&host_cw==control()&&host_mx==_mm_getcsr(),"native kernel leaked host FP state");
        const J native_fp=J::array({fp.control_word,fp.status,fp.mxcsr});check(native_fp==c.at("after_fp"),"x87/SSE state differs native="+native_fp.dump()+" original="+c.at("after_fp").dump());
        if(!failed)check(result==c.at("return_word"),"returned agent differs native="+std::to_string(result)+" original="+c.at("return_word").dump());
#ifdef GSK_BINDING
        {std::size_t pool_allocations{},pool_releases{};for(const auto&x:c.at("allocator_calls")){pool_allocations+=x.at("owner")==0x233250u;pool_releases+=x.at("owner")==0x233420u;}
         std::map<std::string,std::uint64_t> roles;for(const auto&s:c.at("service_calls"))++roles[s.at("role").get<std::string>()];
         const J got{{"attempts",activity.attempts},{"completed",activity.completed},{"refusals",activity.refusals},{"pool_releases",activity.pool_releases}};
         const J want{{"attempts",1},{"completed",failed?0:1},{"refusals",failed?1:0},{"pool_releases",pool_releases}};
         check(got==want,"binding activity differs native="+got.dump()+" original="+want.dump());
         if(!failed){const J g{{"pool_allocations",activity.pool_allocations},{"class_queries",activity.class_queries},{"centers",activity.centers},{"supports",activity.supports},{"encodings",activity.encodings}};
           const J w{{"pool_allocations",pool_allocations},{"class_queries",roles["class"]},{"centers",roles["center"]},{"supports",roles["support"]},{"encodings",roles["encoding"]}};
           check(g==w,"binding service counts differ native="+g.dump()+" original="+w.dump());}
         if(!read_pass)++activity_checks;}
        if(!read_pass){check(private_writes==c.at("private_shape_writes"),"private shape-kernel store differs"+mismatch(private_writes,c.at("private_shape_writes")));private_compared+=private_writes.size();}
#else
        check(calls==c.at("allocator_calls"),"allocator call order differs"+mismatch(calls,c.at("allocator_calls")));
        check(service_problem.empty(),"native service refused: "+service_problem);
        check(services==c.at("service_calls"),"service call order differs"+mismatch(services,c.at("service_calls")));if(!read_pass)services_compared+=services.size();
#endif
        if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));check(memory.reads==expected,"ordered read differs"+mismatch(memory.reads,expected));read_count+=memory.reads.size();}
        else{check(actual_stores==c.at("writes"),"ordered store differs"+mismatch(actual_stores,c.at("writes")));stores+=actual_stores.size();}
        for(const auto&bank:c.at("banks"))check(memory.banks.at(bank.at("address").get<std::uint32_t>())==file(path.parent_path()/bank.at("after_file").get<std::string>()),"complete touched bank differs at "+bank.at("address").dump());
      }catch(const Fault&){problems.push_back(std::string(read_pass?"read pass: ":"store pass: ")+"fault escaped native cleanup");}
      catch(const std::exception&e){problems.push_back(std::string(read_pass?"read pass: ":"store pass: ")+e.what());}
    }
#ifndef GSK_BINDING
    // Per-invocation traces: reads from the read pass, stores from the store pass.
    try{
      const auto&r=pass_invocations[1];const auto&s=pass_invocations[0];check(r.size()==s.size(),"invocation count differs between passes");
      std::vector<Invocation> merged=s;for(std::size_t i=0;i<merged.size();++i){check(r[i].owner==s[i].owner&&r[i].kind==s[i].kind&&r[i].completed==s[i].completed,"invocation identity differs between passes");merged[i].reads=r[i].reads;}
      const auto native=invocation_json(merged);check(native==c.at("invocations"),"shape invocation trace differs"+mismatch(native,c.at("invocations")));invocations_compared+=merged.size();
    }catch(const std::exception&e){problems.push_back(std::string("invocation trace: ")+e.what());}
#endif
    faults+=c.at("declared_fault").get<bool>();failed_cases+=!problems.empty();results.push_back(J{{"name",name},{"problems",problems}});++index;
  }
  const J summary{{"passed",failed_cases==0},{"kernel",kernel},{"cases",fixture.at("cases").size()},{"failed_cases",failed_cases},{"ordered_stores",stores},{"ordered_reads",read_count},{"fault_cases",faults},
    {"invocations_compared",invocations_compared},{"private_shape_writes_compared",private_compared},{"service_calls_compared",services_compared},{"activity_checks",activity_checks},{"results",results}};
  std::ofstream(argv[2])<<summary.dump()<<'\n';std::cout<<J{{"passed",failed_cases==0},{"kernel",kernel},{"failed_cases",failed_cases}}.dump()<<'\n';return failed_cases?1:0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
'''

if __name__=='__main__':raise SystemExit(main())
