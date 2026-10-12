#!/usr/bin/env python3
"""Fresh English TU1.5 startup prefixes with independently compiled record owners.

No older executable, captured world or output result is an execution input.
Primary compatibility expressions establish declared native record semantics,
not Xbox kernel clobbers, hardware/settings data or timer scheduling.
"""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_source_process_shutdown import body,compile_primary as compile_registry

TEXT,STACK,SP,STOP,PRIVATE,PSP=0x9100000,0x2000000,0x200f000,0x3000000,0x2200000,0x220f000
GP=[getattr(R,'UC_X86_REG_'+s) for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
E=ROOT/'analysis/multiplayer/sandbox-source/tu15-startup-records-20261011'
GENERATED=ROOT/'analysis/damage-shields-goal-20261011/process-heap-startup-generated/manifest.json'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def run(argv):return subprocess.run(argv,check=True,capture_output=True,text=True).stdout
def compile_records(out):
    out.mkdir(parents=True,exist_ok=False)
    base=ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world'
    helper=ROOT/'analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-startup-platform/primary/EmuKrnl.cpp'
    pins={base/'Cxbx-Reloaded-EmuKrnlKe.cpp':'3cfc332ef4f9bd2e38214af13c8a91a778bf77aba6884ef3a257b28f51b1e18c',
        base/'Cxbx-Reloaded-types.h':'7223ea0a9fb7e31e804008d5b0b1b3167908ab9c16d43fc2c8a67375c0f58f88',
        helper:'e7ac7dc9687602b784e7abdd7fa71b06980eb34d233f91a1370f20d5a07bc5ce'}
    for p,pin in pins.items():assert sha(p)==pin
    ke,types,helpers=[p.read_text() for p in pins]
    def decl(tag,alias):
        begin=types.index('typedef '+tag);end=types.index(';',types.index('\n'+alias,begin))+1
        return types[begin:end]
    declarations='\n'.join((decl('union _ULARGE_INTEGER','ULARGE_INTEGER'),
        decl('struct _LIST_ENTRY','LIST_ENTRY'),decl('struct _DISPATCHER_HEADER','DISPATCHER_HEADER'),
        decl('enum _TIMER_TYPE','TIMER_TYPE'),decl('struct _KTIMER','KTIMER'),
        decl('struct _KDPC\n','KDPC'),decl('enum _KOBJECTS','KOBJECTS')))
    initializers=[]
    for name in ('KeInitializeDpc','KeInitializeTimerEx'):
        value=body(ke,'xbox::'+name+'\n')
        value,n=re.subn(r'LOG_FUNC_BEGIN.*?LOG_FUNC_END;','',value,flags=re.S);assert n==1
        value=value.replace('assert(Timer);','')
        initializers.append(value)
    list_body=body(helpers,'void InitializeListHead(').replace('xbox::','')
    source='''typedef unsigned char uchar_xt,boolean_xt; typedef short cshort_xt;
typedef int long_xt; typedef unsigned int dword_xt,ULONG;
typedef unsigned long long ulonglong_xt; typedef void void_xt; typedef void *PVOID;
typedef void (*PKDEFERRED_ROUTINE)(void);
#define FALSE 0
#define NULL ((void*)0)
'''+declarations+'''
_Static_assert(sizeof(KDPC)==28,"KDPC32");
_Static_assert(sizeof(KTIMER)==40,"KTIMER32");
void InitializeListHead(PLIST_ENTRY pListHead)'''+list_body+'''
__attribute__((stdcall,noinline,used)) void primary_dpc(KDPC *Dpc,PKDEFERRED_ROUTINE DeferredRoutine,PVOID DeferredContext)'''+initializers[0]+'''
__attribute__((stdcall,noinline,used)) void primary_timer(PKTIMER Timer,TIMER_TYPE Type)'''+initializers[1]+'\n'
    c=out/'records.c';c.write_text(source)
    run(['cc','-m32','-O0','-mno-sse','-mno-mmx','-msoft-float','-ffreestanding','-fno-pie','-fno-pic',
        '-fno-stack-protector','-fno-asynchronous-unwind-tables','-c',str(c),'-o',str(out/'records.o')])
    run(['ld','-m','elf_i386','--section-start=.text='+hex(TEXT),'-e','primary_dpc',str(out/'records.o'),'-o',str(out/'records.elf')])
    run(['objcopy','-O','binary','--only-section=.text',str(out/'records.elf'),str(out/'records.bin')])
    symbols=run(['nm','-n',str(out/'records.elf')]).splitlines()
    entries={name:int(next(s.split()[0] for s in symbols if s.endswith(' '+name)),16) for name in ('primary_dpc','primary_timer')}
    manifest=dict(primary_sources={str(p.relative_to(ROOT)):pin for p,pin in pins.items()},
        generated_c_sha256=sha(c),compiled_text_sha256=sha(out/'records.bin'),entries=entries,text=TEXT,
        optimization='O0 i386 without SSE/MMX',excluded=['logging','non-null assertion'],
        extracted_record_bodies=initializers,extracted_list_body=list_body)
    (out/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    return manifest
def abi(g):
    return dict(gp=[g.reg_read(r) for r in GP],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
        xmm=[[g.reg_read(getattr(R,'UC_X86_REG_XMM'+str(i)))>>(k*32)&0xffffffff for k in range(4)] for i in range(8)],
        mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),x87_control_word=g.reg_read(R.UC_X86_REG_FPCW),
        x87_status=g.reg_read(R.UC_X86_REG_FPSW),fs_base=0)
def load(g,state):
    for r,v in zip(GP,state['gp']):g.reg_write(r,v)
    g.reg_write(R.UC_X86_REG_EFLAGS,state['eflags'])
    for i,v in enumerate(state['xmm']):g.reg_write(getattr(R,'UC_X86_REG_XMM'+str(i)),sum(x<<(k*32) for k,x in enumerate(v)))
    for name,key in (('FPCW','x87_control_word'),('FPSW','x87_status'),('MXCSR','mxcsr')):g.reg_write(getattr(R,'UC_X86_REG_'+name),state[key])
    g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
def cpu(sp,flags,top):
    return dict(gp=[0x11223344,0x22334455,0x33445566,0x44556677,sp,0x66778899,0x778899aa,0x8899aabb],
        eflags=flags,xmm=[[0x10000000+i*4+k for k in range(4)] for i in range(8)],
        x87_control_word=0x27f,mxcsr=0x1f80,x87_status=top<<11,fs_base=0)
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    a.output.mkdir(parents=True,exist_ok=False);tool_pin=sha(__file__)
    primary=compile_records(a.output/'primary');registry=compile_registry(a.output/'registry')
    assert sha(NEW_DEFAULT)==NEW_PIN
    generated=json.loads(GENERATED.read_text());assert generated['later_original_sha256']==NEW_PIN
    correspondence=GENERATED.with_name('normalized-correspondence.json')
    assert sha(correspondence)==generated['correspondence_sha256']
    mapping={int(p,16):q for p,q in json.loads(correspondence.read_text())['instruction_map'].items()}
    inverse={q:p for p,q in mapping.items()}
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    bodies={};byte_rows={}
    starts={0x2dba3e,0x2e00af,0x2e0082}
    for p in sorted(starts):
        trace=_trace(code,p,starts)
        bodies[f'{p:08x}']=sorted(trace.code)
        for q in sorted(trace.code):
            i=code.at(q);assert q in inverse and image.read(q,len(i.bytes))==i.bytes
            byte_rows[f'{q:08x}']=dict(raw=i.bytes.hex(),mnemonic=i.mnemonic,operand=i.op_str,canonical=inverse[q])
    assert len(byte_rows)==169
    contracts=[(0x2e00c1,0x41b70c,0x8000006b,0x2e00c7,16),
        (0x2e00ce,0x41b708,0x80000071,0x2e00d4,12)]
    for site,cell,target,ret,width in contracts:
        i=code.at(site);assert i.mnemonic=='call' and i.size==6 and i.operands[0].mem.disp==cell
        assert int.from_bytes(image.read(cell,4),'little')==target and site+i.size==ret
    assert image.read(0x41b66c,4).hex()=='42010080'
    assert code.at(0x2e00e7).mnemonic=='test' and code.at(0x2e00e7).operands[0].mem.base
    raw_records=(a.output/'primary/records.bin').read_bytes();raw_registry=(a.output/'registry/registry.bin').read_bytes()
    stack=bytes((i*13+7)&255 for i in range(0x10000));cases=[]
    def machine():
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        raw=NEW_DEFAULT.read_bytes();header_size=struct.unpack_from('<I',raw,0x108)[0]
        g.mem_write(0x10000,raw[:header_size])
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        for p in (STACK,PRIVATE):g.mem_map(p,0x10000)
        g.mem_write(STACK,stack);g.mem_map(STOP,0x1000);g.mem_map(0x80000000,0x1000)
        g.mem_map(TEXT,0x1000);g.mem_write(TEXT,raw_records)
        g.mem_map(registry['text'],0x1000);g.mem_write(registry['text'],raw_registry)
        g.mem_map(registry['head'],0x1000);g.mem_write(registry['head'],(a.output/'registry/head.bin').read_bytes())
        g.mem_write(0,struct.pack('<I',0xffffffff));return g
    for entry in (0x2dba3e,0x2e00af):
      for flags in (2,0xcd7):
       for shift in (0,4,8):
        for top in (0,3):
            g=machine();sp=SP+shift;g.mem_write(sp,struct.pack('<I',STOP));load(g,cpu(sp,flags,top))
            before=abi(g);active=0;current=None;trace=[];stores=[];calls=[];boundary=None
            def hook(uc,p,n,user):
                nonlocal active,current,boundary
                if current is not None:
                    if p==STOP+0x100:
                        load(uc,current['before_abi']);uc.reg_write(R.UC_X86_REG_ESP,current['frame_address']+current['width'])
                        uc.reg_write(R.UC_X86_REG_EIP,current['return_address']);current=None;return
                    base,raw=(registry['text'],raw_registry) if current['target']==0x8000002f else (TEXT,raw_records)
                    assert base<=p<p+n<=base+len(raw) and bytes(uc.mem_read(p,n))==raw[p-base:p-base+n]
                    return
                if p>=0x80000000:
                    width={0x8000002f:12,0x8000006b:16,0x80000071:12}[p]
                    esp=uc.reg_read(R.UC_X86_REG_ESP);frame=bytes(uc.mem_read(esp,width));words=struct.unpack('<'+'I'*(width//4),frame)
                    expected={0x8000002f:(0x2dba55,[0x2dba5b,0x46e2e8,1]),
                        0x8000006b:(0x2e00c1,[0x2e00c7,0x56edc0,0x2e009d,0]),
                        0x80000071:(0x2e00ce,[0x2e00d4,0x56ede0,0])}[p]
                    assert active==expected[0] and list(words)==expected[1]
                    current=dict(instruction=active,canonical_instruction=inverse[active],target=p,
                        frame_address=esp,frame=frame.hex(),return_address=words[0],width=width,before_abi=abi(uc),trace_index=len(trace),stores=[])
                    calls.append(current)
                    uc.mem_write(PSP,struct.pack('<'+'I'*(width//4),STOP+0x100,*words[1:]));uc.reg_write(R.UC_X86_REG_ESP,PSP)
                    target=registry['entry'] if p==0x8000002f else primary['entries']['primary_dpc' if p==0x8000006b else 'primary_timer']
                    uc.reg_write(R.UC_X86_REG_EIP,target);return
                assert p in inverse and bytes(uc.mem_read(p,n))==image.read(p,n),hex(p)
                active=p;trace.append(p)
                if p==0x2e00e7:
                    assert uc.reg_read(R.UC_X86_REG_EAX)==0x80000142
                    boundary=dict(kind='unresolved_import_data',instruction=p,canonical_instruction=inverse[p],
                        address=0x80000142,size=1,import_cell=0x41b66c,ordinal=322)
                    uc.emu_stop()
            def write(uc,access,p,n,value,user):
                change=dict(instruction=active,trace_index=len(trace),address=p,before=bytes(uc.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex())
                if current is not None:
                    if PRIVATE<=p<p+n<=PRIVATE+0x10000:return
                    assert (registry['head']<=p<p+n<=registry['head']+8 or 0x46e2e8<=p<p+n<=0x46e2f8 or 0x56edc0<=p<p+n<=0x56ee08)
                    current['stores'].append(change)
                else:change['canonical_instruction']=inverse[active];stores.append(change)
            g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
            g.emu_start(entry,STOP,count=10000);assert boundary and current is None
            assert bytes(g.mem_read(image.section('.text').va,image.section('.text').rsize))==image.section_bytes(image.section('.text'))
            banks=[(sp-0x100,0x140),(0,4),(0x56edc0,0x70),(0x56ee28,4),(0x46e2e8,16),(registry['head'],8)]
            cases.append(dict(image='1.5',entry=entry,canonical_entry=inverse[entry],stack_pointer=sp,before=before,after=abi(g),boundary=boundary,
                instructions=trace,stores=stores,platform_calls=calls,final_banks=[dict(address=p,hex=bytes(g.mem_read(p,n)).hex()) for p,n in banks]))
    components=[]
    for dpc in (True,False):
      for seed in (0,1,17,127,128,255):
       for shift in (0,4,8):
        g=machine();p=0x56edc0 if dpc else 0x56ede0;n=28 if dpc else 40
        initial=bytes((seed+19*i)&255 for i in range(n));g.mem_write(p,initial)
        frame=[0x2e00c7,p,0x2e009d,0] if dpc else [0x2e00d4,p,0]
        writes=[]
        def write(uc,access,q,width,value,user):
            if p<=q<q+width<=p+n:writes.append(dict(address=q,before=bytes(uc.mem_read(q,width)).hex(),written=(value&((1<<(width*8))-1)).to_bytes(width,'little').hex()))
        g.hook_add(UC_HOOK_MEM_WRITE,write);g.mem_write(PSP,struct.pack('<'+'I'*len(frame),STOP,*frame[1:]));g.reg_write(R.UC_X86_REG_ESP,PSP)
        g.emu_start(primary['entries']['primary_dpc' if dpc else 'primary_timer'],STOP,count=1000)
        assert g.reg_read(R.UC_X86_REG_EIP)==STOP
        components.append(dict(dpc=dpc,frame_address=SP+shift,frame=struct.pack('<'+'I'*len(frame),*frame).hex(),
            instruction=0x2e00c1 if dpc else 0x2e00ce,canonical_instruction=inverse[0x2e00c1 if dpc else 0x2e00ce],
            target=0x8000006b if dpc else 0x80000071,address=p,initial_hex=initial.hex(),final_hex=bytes(g.mem_read(p,n)).hex(),stores=writes))
    result=dict(schema=1,parity_target='English TU1.5 only',tool_sha256=tool_pin,original_path=str(NEW_DEFAULT),original_sha256=NEW_PIN,
        generated_parent_manifest_sha256=sha(GENERATED),source_bodies=bodies,source_instructions=byte_rows,
        primary_record_owner=primary,primary_registry_owner=registry,stack_address=STACK,initial_stack_hex=stack.hex(),cases=cases,component_cases=components,
        source_instruction_patches=[],older_image_executed=False,captured_world_input_used=False,native_expected_data_used=False,
        native_platform_abi='void stdcall, exact actual TU1.5 frame cleanup, other live CPU state preserved',
        hardware_data_supplied=False,settings_result_supplied=False,timer_armed=False,process_heap_installed=False,full_startup_complete=False,live_physics_parity=False)
    assert sha(__file__)==tool_pin
    out=a.output/'fixture.json';out.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),component_cases=len(components),source_instructions=sum(len(c['instructions']) for c in cases),
        source_writes=sum(len(c['stores']) for c in cases),platform_writes=sum(len(v['stores']) for c in cases for v in c['platform_calls']),fixture_sha256=sha(out))))
if __name__=='__main__':main()
