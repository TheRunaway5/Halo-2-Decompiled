#!/usr/bin/env python3
"""Unchanged complete startup/setup entries, stopping at their first real owner.

Both original images run with literal header/section bytes and explicit standalone
stack/CPU inputs. An optional independently compiled primary list owner uses an
explicit preserving native void ABI. No hardware, heap or timer outcome is
supplied. The capture retains the actual pending call frame and ordered stores.
"""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_PROT_EXEC
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

STACK, SP, STOP = 0x2000000, 0x200f000, 0x3000000
GP = [getattr(R,'UC_X86_REG_'+s) for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
def sha(raw): return hashlib.sha256(raw).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--with-shutdown',type=Path,
        help='Directory with independently compiled pinned primary list expressions; declared preserving native void ABI only')
    ap.add_argument('--with-initializers',type=Path,
        help='Pinned independently compiled primary107/113 expressions; stop before hardware322 load')
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    platform=None
    if a.with_shutdown:
        platform=json.loads((a.with_shutdown/'manifest.json').read_text())
        for p,pin in platform['primary_sources'].items():assert sha((ROOT/p).read_bytes())==pin
        for name,key in (('registry.c','generated_C_sha256'),('registry.bin','compiled_text_sha256'),
                         ('head.bin','compiled_head_sha256')):
            assert sha((a.with_shutdown/name).read_bytes())==platform[key]
        assert sha((ROOT/'tools/oracle_source_process_shutdown.py').read_bytes())==platform['tool_sha256']
        assert platform['head']==0x5000000 and platform['text']==0x9000000
        platform_text=(a.with_shutdown/'registry.bin').read_bytes()
    initializers=None
    if a.with_initializers:
        source=a.with_initializers
        assert sha((source/'manifest.json').read_bytes())=='63490871f75ed077575624a987ff768adc9bf70d9b9960016bf93fd1331e9d17'
        original=json.loads((source/'manifest.json').read_text())
        for p,pin in original['primary_pins'].items():assert sha(Path(p).read_bytes())==pin
        assert sha((source.parent/'oracle.py').read_bytes())==original['oracle_sha256']
        for name,key in (('initializers.cpp','source_sha256'),('initializers.bin','text_sha256'),
                         ('initializers.elf','elf_sha256'),('extracted-primary.txt','extracted_primary_sha256'),
                         ('compiled-instructions.asm','compiled_listing_sha256')):
            assert sha((source/name).read_bytes())==original['compiled'][key]
        # Relink the authenticated object in a separate arena. Byte equality
        # proves its relative instruction stream did not change under relocation.
        relocated=a.output/'initializers';relocated.mkdir()
        commands=[['ld','-m','elf_i386','-Ttext','0x09100000','-e',
            '_ZN4xbox15KeInitializeDpcEPNS_5_KDPCEPU7stdcallFvS1_PvS2_S2_ES2_',
            str(source/'initializers.o'),'-o',str(relocated/'initializers.elf')],
            ['objcopy','-O','binary','--only-section=.text',str(relocated/'initializers.elf'),str(relocated/'initializers.bin')]]
        logs=[]
        for cmd in commands:
            result=subprocess.run(cmd,text=True,capture_output=True)
            assert result.returncode==0,result.stderr
            logs.append(dict(command=[v.replace(str(a.output),'OUTPUT') for v in cmd],
                returncode=result.returncode,stdout=result.stdout,stderr=result.stderr))
        initializer_text=(relocated/'initializers.bin').read_bytes()
        assert initializer_text==(source/'initializers.bin').read_bytes()
        symbols=subprocess.check_output(['nm','-n','-C',str(relocated/'initializers.elf')],text=True)
        (relocated/'symbols.txt').write_text(symbols)
        entries={}
        for key,needle in [('107','xbox::KeInitializeDpc('),('113','xbox::KeInitializeTimerEx(')]:
            line=next(s for s in symbols.splitlines() if needle in s)
            entries[key]=int(line.split()[0],16)
            assert entries[key]==original['compiled']['entry'][key]+0x100000
        initializers=dict(source_manifest_sha256=sha((source/'manifest.json').read_bytes()),
            source_object_sha256=sha((source/'initializers.o').read_bytes()),
            source_oracle_sha256=original['oracle_sha256'],text=0x9100000,entry=entries,
            text_sha256=sha(initializer_text),relocated_elf_sha256=sha((relocated/'initializers.elf').read_bytes()),
            symbols_sha256=sha((relocated/'symbols.txt').read_bytes()),link_commands=logs)
    report=ROOT/'analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-ready/paired/report.json'
    assert sha(report.read_bytes())=='57d430a60d02fc41268779946603186fdbd19828af4872b8e394888b0dbe0521'
    rows=[r for r in json.loads(report.read_text())['owners'] if int(r['old_entry'],16) in (0x2d7f8e,0x2dc5ff,0x2dc5d2)]
    initial_stack=bytes((i*13+7)&255 for i in range(0x10000))
    cases=[];sources=[]
    for later,path,pin in ((False,ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',PIN),
                           (True,NEW_DEFAULT,NEW_PIN)):
        assert sha(path.read_bytes())==pin;image=Xbe(path)
        sources.append(dict(image='1.5' if later else '1.1',path=str(path),sha256=pin))
        canonical={}
        for body in rows:
            pair=[]
            for listing in body['listings']:
                p=report.parent/listing['file'];assert sha(p.read_bytes())==listing['sha256']
                pair.append([(int(s.split()[0],16),bytes.fromhex(s.split()[1])) for s in p.read_text().splitlines()])
            for (p,_),(q,_) in zip(*pair):canonical[q if later else p]=p
        for owner in rows:
            actual_entry=int(owner['u5_entry'] if later else owner['old_entry'],16)
            entry=int(owner['old_entry'],16)
            for flags in (2,0xcd7):
                for shift in (0,4,8):
                    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
                    raw=path.read_bytes();header_size=struct.unpack_from('<I',raw,0x108)[0]
                    g.mem_write(0x10000,raw[:header_size])
                    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
                    g.mem_map(STACK,0x10000);g.mem_write(STACK,initial_stack)
                    g.mem_map(STOP,0x1000);g.mem_map(0x80000000,0x1000,UC_PROT_EXEC)
                    if platform or initializers:g.mem_map(0x2200000,0x10000)
                    if platform:
                        g.mem_map(platform['head'],0x1000)
                        g.mem_write(platform['head'],(a.with_shutdown/'head.bin').read_bytes())
                        g.mem_map(platform['text'],0x1000);g.mem_write(platform['text'],platform_text)
                    if initializers:
                        g.mem_map(initializers['text'],0x1000);g.mem_write(initializers['text'],initializer_text)
                    g.mem_write(0,struct.pack('<I',0xffffffff))
                    sp=SP+shift;g.mem_write(sp,struct.pack('<I',STOP))
                    values=[0x11223344,0x22334455,0x33445566,0x44556677,
                            sp,0x66778899,0x778899aa,0x8899aabb]
                    for reg,value in zip(GP,values):g.reg_write(reg,value)
                    for i in range(8):g.reg_write(getattr(R,'UC_X86_REG_XMM'+str(i)),
                        sum((0x10000000+i*4+k)<<(k*32) for k in range(4)))
                    g.reg_write(R.UC_X86_REG_EFLAGS,flags);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
                    g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
                    g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
                    def abi():
                        return dict(gp=[g.reg_read(r) for r in GP],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
                            xmm=[[g.reg_read(getattr(R,'UC_X86_REG_XMM'+str(i)))>>(k*32)&0xffffffff
                                  for k in range(4)] for i in range(8)],
                            mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),x87_control_word=g.reg_read(R.UC_X86_REG_FPCW),
                            x87_status=g.reg_read(R.UC_X86_REG_FPSW),fs_base=0)
                    active=[0];trace=[];stores=[];boundary=[];platform_calls=[]
                    platform_active=[None];platform_instruction=[0]
                    def hook(uc,p,n,user):
                        if (platform or initializers) and p==STOP+0x100:
                            saved=platform_active[0]
                            assert saved is not None
                            # Explicit native ABI: the compiled compatibility expression's
                            # volatile platform stack/register state is not an Xbox claim.
                            for reg,value in zip(GP,saved['gp']):uc.reg_write(reg,value)
                            uc.reg_write(R.UC_X86_REG_EFLAGS,saved['flags'])
                            uc.reg_write(R.UC_X86_REG_ESP,saved['esp']+saved['cleanup'])
                            uc.reg_write(R.UC_X86_REG_EIP,saved['return'])
                            platform_active[0]=None
                            return
                        if platform and platform['text']<=p<platform['text']+len(platform_text):
                            assert platform_active[0] is not None
                            assert bytes(uc.mem_read(p,n))==platform_text[p-platform['text']:p-platform['text']+n]
                            platform_instruction[0]=p;return
                        if initializers and initializers['text']<=p<initializers['text']+len(initializer_text):
                            assert platform_active[0] is not None
                            assert bytes(uc.mem_read(p,n))==initializer_text[p-initializers['text']:p-initializers['text']+n]
                            platform_instruction[0]=p;return
                        if p>=0x80000000:
                            width={0x8000002f:12,0x8000006b:16,0x80000071:12,0x80000095:20}[p]
                            esp=uc.reg_read(R.UC_X86_REG_ESP)
                            if (platform and p==0x8000002f) or (initializers and p in (0x8000006b,0x80000071)):
                                frame=bytes(uc.mem_read(esp,width));words=struct.unpack('<'+'I'*(width//4),frame)
                                ret,record=words[:2]
                                if p==0x8000002f:
                                    assert active[0]==0x2d7fa5 and words[2]==1
                                    kind='registry';callee=platform['entry']
                                elif p==0x8000006b:
                                    assert active[0]==0x2dc611 and words[1:]==(
                                        0x56edc0 if later else 0x566ea0,0x2e009d if later else 0x2dc5ed,0)
                                    kind='dpc';callee=initializers['entry']['107']
                                else:
                                    assert active[0]==0x2dc61e and words[1:]==(0x56ede0 if later else 0x566ec0,0)
                                    kind='timer';callee=initializers['entry']['113']
                                platform_calls.append(dict(instruction=active[0],target=p,tail=False,
                                    frame_address=esp,frame=frame.hex(),stores=[]))
                                if initializers:platform_calls[-1]['kind']=kind
                                platform_active[0]={'gp':[uc.reg_read(r) for r in GP],'esp':esp,
                                    'flags':uc.reg_read(R.UC_X86_REG_EFLAGS),'return':ret,'cleanup':width,
                                    'kind':kind,'record':record}
                                uc.mem_write(0x220f000,struct.pack('<'+'I'*len(words),STOP+0x100,*words[1:]))
                                uc.reg_write(R.UC_X86_REG_ESP,0x220f000)
                                uc.reg_write(R.UC_X86_REG_EIP,callee);return
                            boundary.append(dict(instruction=active[0],target=p,tail=False,
                                frame_address=esp,frame=bytes(uc.mem_read(esp,width)).hex()))
                            uc.emu_stop();return
                        assert p in canonical and bytes(uc.mem_read(p,n))==image.read(p,n),hex(p)
                        if initializers and canonical[p]==0x2dc624:
                            cell=0x41b66c if later else 0x417bcc
                            encoded=struct.unpack('<I',bytes(uc.mem_read(cell,4)))[0];assert encoded==0x80000142
                            boundary.append(dict(kind='imported-data',instruction=canonical[p],actual_instruction=p,
                                cell=cell,ordinal=322,bytes=8,encoded_import_word=encoded,
                                instruction_raw_hex=bytes(uc.mem_read(p,n)).hex(),instruction_executed=False,
                                frame_address=uc.reg_read(R.UC_X86_REG_ESP),
                                frame=bytes(uc.mem_read(uc.reg_read(R.UC_X86_REG_ESP),48)).hex()))
                            uc.emu_stop();return
                        active[0]=canonical[p];trace.append(active[0])
                    def write(uc,access,p,n,value,user):
                        if platform_active[0] is not None:
                            if 0x2200000<=p<0x2210000:return
                            saved=platform_active[0]
                            if saved['kind']=='registry':
                                assert (platform['head']<=p<p+n<=platform['head']+8 or saved['record']<=p<p+n<=saved['record']+16)
                            else:assert saved['record']<=p<p+n<=saved['record']+(28 if saved['kind']=='dpc' else 40)
                            platform_calls[-1]['stores'].append(dict(instruction=platform_instruction[0],
                                address=p,before=bytes(uc.mem_read(p,n)).hex(),
                                written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
                            return
                        stores.append(dict(instruction=active[0],address=p,before=bytes(uc.mem_read(p,n)).hex(),
                            written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
                    g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
                    before=abi();g.emu_start(actual_entry,STOP,count=10000)
                    assert len(boundary)==1
                    assert platform_active[0] is None
                    banks=[dict(address=sp-0x100,hex=bytes(g.mem_read(sp-0x100,0x140)).hex()),
                           dict(address=0,hex=bytes(g.mem_read(0,4)).hex())]
                    for p,n in ((0x56edc0 if later else 0x566ea0,0x70),(0x56ee28 if later else 0x566f08,4),
                                (0x46e2e8 if later else 0x469228,16)):
                        banks.append(dict(address=p,hex=bytes(g.mem_read(p,n)).hex()))
                    if platform:banks.append(dict(address=platform['head'],hex=bytes(g.mem_read(platform['head'],8)).hex()))
                    cases.append(dict(image='1.5' if later else '1.1',entry=entry,actual_entry=actual_entry,
                        stack_pointer=sp,before=before,after=abi(),boundary=boundary[0],stores=stores,
                        trace=trace,final_banks=banks,platform_calls=platform_calls))
    fixture=dict(schema=1,tool_sha256=sha(Path(__file__).read_bytes()),source_images=sources,
        paired_report_sha256=sha(report.read_bytes()),stack_address=STACK,
        initial_stack_hex=initial_stack.hex(),cases=cases,instruction_patches=[],
        compiled_compatibility_registry=platform,
        supplied_hardware_heap_timer_outcomes=False,
        native_registry_ABI='void stdcall, 12-byte frame; other CPU state preserved' if platform else None,
        scope='Complete original entries; first required platform call after any declared native registry join; no startup completion or installed heap')
    if initializers:
        fixture['compiled_compatibility_initializers']=initializers
        fixture['native_initializer_ABI']='void stdcall107/113, complete16/12-byte frames; other CPU state preserved'
        fixture['scope']='Complete original entries, primary registry/107/113 joins; pre-load hardware322 boundary or unprovided timer149 call. No hardware payload, installed heap, scheduler or startup completion.'
    out=a.output/'fixture.json';out.write_text(json.dumps(fixture,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=len(cases),ordered_stores=sum(len(r['stores']) for r in cases),
        executed_source_instructions=len({p for r in cases for p in r['trace']}),fixture_sha256=sha(out.read_bytes()))))
if __name__=='__main__':main()
