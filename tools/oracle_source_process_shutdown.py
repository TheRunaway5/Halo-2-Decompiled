#!/usr/bin/env python3
"""Compile pinned compatibility intrusive-list expressions; no IRQ/callback claim."""
import argparse
import hashlib
import itertools
import json
import re
import struct
import subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

ROOT=Path(__file__).resolve().parents[1]
HEAD,RECORDS,TEXT,STACK,SP,STOP=0x5000000,0x5001000,0x9000000,0x2200000,0x220f000,0x3000000
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def body(source,marker):
    begin=source.index('{',source.index(marker));depth=1;p=begin+1
    while depth:
        depth+=(source[p]=='{')-(source[p]=='}');p+=1
    return source[begin:p]
def compile_primary(output):
    output.mkdir(parents=True,exist_ok=False)
    directory=ROOT/'analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-startup-platform/primary'
    pins={ROOT/'analysis/multiplayer/map-pack/smg-audio-host-contract/reference/EmuKrnlHal.cpp':
          '2c540d4df5a1fca33e91ee8031c3eef201e380174c142c0327d25d17e446f446',
          directory/'EmuKrnl.cpp':'e7ac7dc9687602b784e7abdd7fa71b06980eb34d233f91a1370f20d5a07bc5ce',
          directory/'hal.h':'1d646be76f5252ef22494436b8e6bc8390999c3d6e5daaef158d5f7d06e92ca6',
          ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h':
          '988e679c3e042bb9f0c3836a77d3aee24117e83683da9cf839ba2a84c7fe32fb',
          ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world/Cxbx-Reloaded-types.h':
          '7223ea0a9fb7e31e804008d5b0b1b3167908ab9c16d43fc2c8a67375c0f58f88'}
    for p,pin in pins.items():assert sha(p)==pin
    hal,helpers,types,nxdk,base_types=[p.read_text() for p in pins]
    registration=body(hal,'xbox::HalRegisterShutdownNotification\n')
    registration,n=re.subn(r'LOG_FUNC_BEGIN.*?LOG_FUNC_END;','',registration,flags=re.S);assert n==1
    for line in ('KIRQL OldIrql;','OldIrql = KeRaiseIrqlToDpcLevel();','KfLowerIrql(OldIrql);'):
        assert registration.count(line)==1;registration=registration.replace(line,'')
    insert=body(helpers,'void InsertTailList(').replace('xbox::','')
    remove=body(helpers,'xbox::boolean_xt RemoveEntryList(').replace('xbox::','').replace('nullptr','0')
    decl=types[types.index('typedef void_xt (*PHAL_SHUTDOWN_NOTIFICATION)'):types.index('XBSYSAPI EXPORTNUM(47)')]
    list_decl=re.search(r'typedef struct _LIST_ENTRY\s*\{.*?\}\s*LIST_ENTRY,\s*\*PLIST_ENTRY;',base_types,re.S).group()
    containing=next(s for s in nxdk.splitlines() if s.startswith('#define CONTAINING_RECORD('))
    source='''typedef void void_xt; typedef int long_xt; typedef char *PCHAR; typedef unsigned int ULONG_PTR;
#define IN
#define FALSE 0
'''+list_decl+'\n'+decl+'\n'+containing+'''
__attribute__((section(".registry"),used)) LIST_ENTRY ShutdownRoutineList={&ShutdownRoutineList,&ShutdownRoutineList};
void InsertTailList(PLIST_ENTRY pListHead,PLIST_ENTRY pEntry)'''+insert+'''
unsigned char RemoveEntryList(PLIST_ENTRY pEntry)'''+remove+'''
__attribute__((stdcall,noinline,used)) void source_registry(PHAL_SHUTDOWN_REGISTRATION ShutdownRegistration,unsigned char Register)'''+registration+'\n'
    c=output/'registry.c';c.write_text(source)
    def run(args):subprocess.run(args,check=True,capture_output=True,text=True)
    run(['cc','-m32','-O2','-ffreestanding','-fno-pie','-fno-pic','-fno-stack-protector',
         '-fno-asynchronous-unwind-tables','-c',str(c),'-o',str(output/'registry.o')])
    run(['ld','-m','elf_i386','--section-start=.text=0x9000000','--section-start=.registry=0x5000000',
         '-e','source_registry',str(output/'registry.o'),'-o',str(output/'registry.elf')])
    for name,section in (('registry.bin','.text'),('head.bin','.registry')):
        run(['objcopy','-O','binary','--only-section='+section,str(output/'registry.elf'),str(output/name)])
    symbols=subprocess.check_output(['nm','-n',str(output/'registry.elf')],text=True)
    entry=int(next(s.split()[0] for s in symbols.splitlines() if s.endswith(' source_registry')),16)
    assert (output/'head.bin').read_bytes()==struct.pack('<II',HEAD,HEAD)
    manifest=dict(scope='Pinned compatibility intrusive-list expressions only; IRQ raise/lower/logging excluded; no callback execution or Xbox kernel claim',
        primary_sources={str(p.relative_to(ROOT)):pin for p,pin in pins.items()},tool_sha256=sha(Path(__file__)),
        generated_C_sha256=sha(c),compiled_text_sha256=sha(output/'registry.bin'),
        compiled_head_sha256=sha(output/'head.bin'),entry=entry,head=HEAD,text=TEXT,
        extracted_registration_body=registration,excluded=['logging','IRQ raise/lower and pending interrupt dispatch'])
    (output/'manifest.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
    return manifest
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();manifest=compile_primary(a.output/'compiled')
    rows=[];text=(a.output/'compiled/registry.bin').read_bytes()
    for priorities in ((-0x80000000,0,0x7fffffff),(0,0,0),(-1,-1,1),(1,-1,1)):
        for order in itertools.permutations(range(3)):
            g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(HEAD,0x3000);g.mem_map(TEXT,0x1000)
            g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000);g.mem_write(TEXT,text)
            g.mem_write(HEAD,(a.output/'compiled/head.bin').read_bytes())
            initial=bytearray((i*19+3)&255 for i in range(0x1000))
            for i,p in enumerate(priorities):struct.pack_into('<IiII',initial,i*16,0x2dc68c,p,0xdeadbeef,0xbad00bad)
            g.mem_write(RECORDS,bytes(initial));stores=[];active=[0]
            def code(uc,p,n,user):
                assert TEXT<=p<TEXT+len(text) and bytes(uc.mem_read(p,n))==text[p-TEXT:p-TEXT+n]
                active[0]=p
            def write(uc,access,p,n,value,user):
                if not HEAD<=p<HEAD+0x3000:return
                stores.append(dict(instruction=active[0],address=p,before=bytes(uc.mem_read(p,n)).hex(),
                    written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
            g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,write)
            steps=[]
            operations=[(3,False)]+[(i,True) for i in order]+[(order[1],False),(order[1],False),
                (order[1],True)]+[(i,False) for i in order]+[(0,True),(0,True)]
            for index,add in operations:
                stores.clear();p=RECORDS+index*16
                g.mem_write(SP,struct.pack('<III',STOP,p,add));g.reg_write(R.UC_X86_REG_ESP,SP)
                g.emu_start(manifest['entry'],STOP,count=10000)
                assert g.reg_read(R.UC_X86_REG_EIP)==STOP
                entries=[];seen=set();q=struct.unpack('<I',g.mem_read(HEAD,4))[0]
                while q!=HEAD and q not in seen:
                    seen.add(q);routine,priority=struct.unpack('<Ii',g.mem_read(q-8,8))
                    entries.append(dict(address=q-8,routine=routine,priority=priority));q=struct.unpack('<I',g.mem_read(q,4))[0]
                steps.append(dict(record=p,add=add,stores=list(stores),head=bytes(g.mem_read(HEAD,8)).hex(),
                    records=bytes(g.mem_read(RECORDS,0x1000)).hex(),order=entries,cycle=q!=HEAD))
            rows.append(dict(priorities=priorities,input_records=bytes(initial).hex(),steps=steps))
    fixture=dict(compiled=manifest,head=HEAD,records=RECORDS,cases=rows)
    out=a.output/'fixture.json';out.write_text(json.dumps(fixture,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=len(rows),steps=sum(len(r['steps']) for r in rows),
        stores=sum(len(s['stores']) for r in rows for s in r['steps']),fixture_sha256=sha(out))))
if __name__=='__main__':main()
