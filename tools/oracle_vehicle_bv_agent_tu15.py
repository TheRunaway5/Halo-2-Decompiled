#!/usr/bin/env python3
"""Run untouched TU1.5 from an actual scheduler input into the reached BV agent.

This is an offline oracle. It supplies no subcall return, collision, contact or
admission result. Raw source banks and derived fixtures stay in test scratch.
"""
import argparse
import bisect
import hashlib
import json
import mmap
import os
from pathlib import Path
import struct
import time

from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_vehicle_runtime_20261011 import capture

GP=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
    R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]


def sha(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report-output',type=Path,required=True)
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    if not args.output.resolve().is_relative_to(owner):parser.error('Raw oracle output must stay in owned scratch')
    args.output.mkdir(parents=True,exist_ok=False)
    assert sha(NEW_DEFAULT)==NEW_PIN
    metadata=args.capture/'capture-entry.json';d=json.loads(metadata.read_text())
    assert d['entry']==0x14fec0 and d['reference']=='English TU1.5 only'
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    # The actual world reaches an indirect call. Its target must come from
    # original execution over captured input, never an injected direct callee.
    call=code.at(0x315a4c)
    assert call.mnemonic=='call' and 0x315a4c+call.size==0x315a53
    text=next(section for section in image.sections if section.name=='.text')
    # Native data retains narrow literal/table slices inside the text address
    # range. Authenticate every overlap before the offline CPU fills in code.
    # Production continues to execute compiled kernels, not these bytes.
    data_regions=[];text_pins={}
    for row in d['regions']:
        if row['address']+row['size']<=text.va or row['address']>=text.va+text.rsize:
            data_regions.append(row);continue
        assert text.va<=row['address'] and row['address']+row['size']<=text.va+text.rsize
        path=args.capture/row['before_file'];raw=path.read_bytes()
        assert raw==image.read(row['address'],row['size']),'Captured source literal differs from original text'
        text_pins[path.name]=sha(path)
    regions=sorted([*data_regions,dict(address=text.va,size=text.rsize,original_code=True)],key=lambda row:row['address'])
    starts=[row['address'] for row in regions]
    g=Uc(UC_ARCH_X86,UC_MODE_32);ranges=[];files=[]
    for row in regions:
        lo=row['address']&~4095;hi=(row['address']+row['size']+4095)&~4095
        if ranges and lo<=ranges[-1][1]:ranges[-1]=(ranges[-1][0],max(hi,ranges[-1][1]))
        else:ranges.append((lo,hi))
    for lo,hi in ranges:g.mem_map(lo,hi-lo)
    pins=dict(text_pins)
    for row in regions:
        if row.get('original_code'):
            g.mem_write(text.va,image.section_bytes(text));continue
        path=args.capture/row['before_file'];stream=path.open('rb')
        raw=mmap.mmap(stream.fileno(),0,access=mmap.ACCESS_READ);files.append((stream,raw))
        assert len(raw)==row['size'];pins[path.name]=sha(path);g.mem_write(row['address'],raw[:])
    assert bytes(g.mem_read(d['entry'],16))==image.read(d['entry'],16)
    descriptor=0x7ff00000
    assert all(not(lo<=descriptor<hi) for lo,hi in ranges);g.mem_map(descriptor,4096)
    def desc(base,access):
        return struct.pack('<HHBBBB',65535,base&65535,(base>>16)&255,access,0xcf,(base>>24)&255)
    initial=d['before']
    g.mem_write(descriptor,b'\0'*8+desc(0,0x9b)+desc(0,0x93)+desc(initial['fs_base'],0x93))
    g.reg_write(R.UC_X86_REG_GDTR,(0,descriptor,31,0))
    for reg,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),
                      (R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24),(R.UC_X86_REG_GS,16)]:g.reg_write(reg,value)
    for reg,value in zip(GP,initial['gp']):g.reg_write(reg,value)
    for n,value in enumerate(initial['xmm']):g.reg_write(getattr(R,f'UC_X86_REG_XMM{n}'),int.from_bytes(struct.pack('<4I',*value),'little'))
    for reg,key in [(R.UC_X86_REG_EFLAGS,'eflags'),(R.UC_X86_REG_FPCW,'x87_control_word'),
                    (R.UC_X86_REG_MXCSR,'mxcsr'),(R.UC_X86_REG_FPSW,'x87_status')]:g.reg_write(reg,initial[key])
    top=(initial['x87_status']>>11)&7;tag=0xffff
    for n,value in enumerate(initial['x87_values']):
        raw=bytes.fromhex(value);index=(top+n)&7;tag&=~(3<<(2*index))
        g.reg_write(getattr(R,f'UC_X86_REG_FP{index}'),(int.from_bytes(raw[:8],'little'),int.from_bytes(raw[8:],'little')))
    g.reg_write(R.UC_X86_REG_FPTAG,tag)
    root_instructions=0;entered=False;returned=False;entry_abi=None;return_pc=None
    trace=[];writes=[];pages={};pending=[];estimates=[];timestamps=[];cpu_pending=[]
    started=time.monotonic()
    text=next(section for section in image.sections if section.name=='.text')
    def region_at(address,size):
        at=bisect.bisect_right(starts,address)-1
        assert at>=0 and address+size<=regions[at]['address']+regions[at]['size'],f'Original access outside actual committed input {address:08x}+{size}'
        return regions[at]
    def touch(address,size):
        row=region_at(address,size)
        if not entered:return
        for page in range(address&~4095,((address+size-1)&~4095)+1,4096):
            lo=max(page,row['address']);hi=min(page+4096,row['address']+row['size'])
            if lo not in pages:pages[lo]=bytes(g.mem_read(lo,hi-lo))
    def flush(cpu=False):
        while pending:
            row=pending.pop(0);row['written']=bytes(g.mem_read(row['address'],row.pop('size'))).hex();writes.append(row)
        while cpu and cpu_pending:
            kind,row,reg=cpu_pending.pop(0)
            row['output']=(g.reg_read(R.UC_X86_REG_EDX)<<32)|g.reg_read(R.UC_X86_REG_EAX) if kind=='timestamp' else g.reg_read(reg)&0xffffffff
            (timestamps if kind=='timestamp' else estimates).append(row)
    def on_code(_g,address,size,_):
        nonlocal root_instructions,entered,returned,entry_abi,return_pc
        flush(True)
        assert time.monotonic()-started<180,'Finite original oracle deadline'
        if entered and address==return_pc:returned=True;g.emu_stop();return
        assert text.va<=address<text.va+text.vsize,f'Original execution escaped text {address:08x}'
        assert bytes(g.mem_read(address,size))==image.read(address,size),f'Modified original instruction {address:08x}'
        root_instructions+=1
        if address==0x2f18d0 and not entered:
            entered=True;entry_abi=capture(g);return_pc=int.from_bytes(g.mem_read(entry_abi['gp'][4],4),'little')
        if not entered:return
        trace.append(address);instruction=code.at(address)
        if instruction.mnemonic=='rdtsc':cpu_pending.append(('timestamp',dict(instruction=address),None))
        if instruction.mnemonic=='rsqrtss':
            source=instruction.operands[1]
            if source.type==1:
                reg=getattr(R,'UC_X86_REG_'+instruction.reg_name(source.reg).upper());word=g.reg_read(reg)&0xffffffff
            else:
                assert source.type==3
                point=source.mem.disp
                if source.mem.base:point+=g.reg_read(getattr(R,'UC_X86_REG_'+instruction.reg_name(source.mem.base).upper()))
                if source.mem.index:point+=source.mem.scale*g.reg_read(getattr(R,'UC_X86_REG_'+instruction.reg_name(source.mem.index).upper()))
                if source.mem.segment:assert instruction.reg_name(source.mem.segment)=='fs';point+=initial['fs_base']
                word=int.from_bytes(g.mem_read(point&0xffffffff,4),'little')
            destination=getattr(R,'UC_X86_REG_'+instruction.reg_name(instruction.operands[0].reg).upper())
            cpu_pending.append(('estimate',dict(instruction=address,input=word),destination))
    def on_read(_g,_access,address,size,_value,_):flush();touch(address,size)
    def on_write(_g,_access,address,size,_value,_):
        assert not region_at(address,size).get('original_code'),'Original text is immutable'
        flush();touch(address,size)
        if entered:pending.append(dict(instruction=g.reg_read(R.UC_X86_REG_EIP),address=address,size=size,before=bytes(g.mem_read(address,size)).hex()))
    g.hook_add(UC_HOOK_CODE,on_code);g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_MEM_WRITE,on_write)
    error=None
    try:g.emu_start(d['entry'],0,count=5_000_000)
    except Exception as exc:error=str(exc)
    flush(True);fixture_path=None
    if entered:
        case=dict(id=0,entry=0x2f18d0,entry_abi=entry_abi,complete=returned,
                  exit_entry=None if returned else g.reg_read(R.UC_X86_REG_EIP),exit=capture(g),
                  initial_banks=[dict(address=at,hex=raw.hex()) for at,raw in sorted(pages.items())],
                  final_banks=[dict(address=at,hex=bytes(g.mem_read(at,len(raw))).hex()) for at,raw in sorted(pages.items())],
                  instructions=trace,writes=writes,estimates=estimates,timestamps=timestamps,
                  x87_defined_status_mask=0x7f00,
                  declared_input='Actual source scheduler input; untouched original14FEC0 produces this BV-agent entry. No native expected output or subcall return.')
        fixture_path=args.output/'original.json';fixture_path.write_text(json.dumps(dict(schema=1,image='1.5',cases=[case]),separators=(',',':'))+'\n')
    unchanged=all(sha(args.capture/name)==pin for name,pin in pins.items())
    report=dict(original_entry_reached=entered,original_kernel_returned=returned,error=error,
                root_instructions=root_instructions,kernel_instructions=len(trace),ordered_stores=len(writes),
                touched_input_bytes=sum(map(len,pages.values())),exit_pc=hex(g.reg_read(R.UC_X86_REG_EIP)),
                source_banks_unchanged=unchanged,original_sha256=NEW_PIN,capture_entry_sha256=sha(metadata),
                raw_input_sha256=pins,tool_sha256=sha(Path(__file__)),fixture=str(fixture_path) if fixture_path else None,
                fixture_sha256=sha(fixture_path) if fixture_path else None,original_instruction_patches=[],
                cpu_input_boundary='RDTSC and RSQRTSS outputs are recorded from the oracle CPU; no physical Xbox CPU estimate claim',
                native_expected_output_used=False,in_game_or_full_parity=False,raw_output=str(args.output))
    report['offline_original_text_loaded_separately']=True
    report['authenticated_native_text_literal_regions']=len(text_pins)
    report['reached_call']={'address':0x315a4c,'raw_hex':call.bytes.hex(),'operands':call.op_str,
                            'target_supplied':False}
    args.report_output.parent.mkdir(parents=True,exist_ok=True);args.report_output.write_text(json.dumps(report,indent=2)+'\n')
    for stream,raw in files:raw.close();stream.close()
    print(json.dumps({k:report[k] for k in ('original_entry_reached','original_kernel_returned','error','kernel_instructions','exit_pc')}),flush=True)
    return 0 if entered and returned and unchanged else 1


if __name__=='__main__':raise SystemExit(main())
