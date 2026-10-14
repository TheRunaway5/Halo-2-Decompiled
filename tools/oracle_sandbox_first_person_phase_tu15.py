#!/usr/bin/env python3
"""Replay real composed first-person phases with unchanged Xbox TU1.5 bytes."""
import argparse
import bisect
import hashlib
import json
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE, UC_MEM_READ
from unicorn import x86_const as R

GP=[getattr(R,'UC_X86_REG_'+x) for x in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
GDT=0x71000000

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def segment(base,access):
    return struct.pack('<II',0xffff | ((base & 0xffff)<<16),
        ((base>>16)&255) | (access<<8) | 0xf0000 | (0xc<<20) | (base & 0xff000000))

def compare(directory,tick,hardware_inputs=None):
    before_path=directory/f'{tick}-before.json';after_path=directory/f'{tick}-after.json'
    before=json.loads(before_path.read_text());after=json.loads(after_path.read_text())
    assert before['original_sha256']==after['original_sha256']==NEW_PIN
    assert before['entry']==after['entry'] and before['entry'] in (0x168ca1,0x168ded,0x223b20,0x108a70,0xbc200,0x179df0)
    assert before['regions']==after['regions'], 'Source FP phase changed storage ownership; replay needs the actual allocation/I/O owner'
    assert all(sample['kind']==2 for sample in after['cpu_inputs']),'Only explicit host reciprocal-sqrt estimates are supported here'
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    original_before=before_path.with_suffix('.bin').read_bytes();native_after=after_path.with_suffix('.bin').read_bytes()
    regions=before['regions'];starts=[r['address'] for r in regions]
    assert starts==sorted(starts)
    def admitted(address,size):
        i=bisect.bisect_right(starts,address)-1
        return i>=0 and address+size<=starts[i]+regions[i]['size']
    machine=Uc(UC_ARCH_X86,UC_MODE_32)
    pages=set()
    for region in regions:
        address,size=region['address'],region['size']
        pages.update(range(address & ~4095,(address+size+4095)&~4095,4096))
    assert GDT not in pages
    # Only rounded mapping is a Unicorn requirement. Every source read/write
    # still checks the exact committed extents, including subpage holes.
    runs=[]
    for page in sorted(pages):
        if runs and page==runs[-1][1]:runs[-1][1]+=4096
        else:runs.append([page,page+4096])
    for address,end in runs:machine.mem_map(address,end-address)
    for region in regions:
        address,offset,size=region['address'],region['offset'],region['size']
        machine.mem_write(address,original_before[offset:offset+size])
    for section in image.sections:
        if section.name=='.text':
            expected=image.section_bytes(section)
            assert bytes(machine.mem_read(section.va,len(expected)))==expected,'Captured original code is modified'
            text_start,text_end=section.va,section.va+len(expected)
    machine.mem_map(GDT,4096)
    machine.mem_write(GDT+8,segment(0,0x9b)+segment(0,0x93)+segment(before['abi']['fs_base'],0x93))
    machine.reg_write(R.UC_X86_REG_GDTR,(0,GDT,4095,0))
    for register,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):
        machine.reg_write(register,value)
    assert machine.reg_read(R.UC_X86_REG_FS_BASE)==before['abi']['fs_base']
    initial=before['abi'];expected=after['abi']
    assert not initial['x87_depth'],'Live x87 values require their actual retained representation'
    for register,value in zip(GP,initial['gp']):machine.reg_write(register,value)
    for n,lanes in enumerate(initial['xmm']):machine.reg_write(R.UC_X86_REG_XMM0+n,sum(x<<(32*k) for k,x in enumerate(lanes)))
    machine.reg_write(R.UC_X86_REG_EFLAGS,initial['eflags'])
    machine.reg_write(R.UC_X86_REG_FPCW,initial['x87_control'])
    machine.reg_write(R.UC_X86_REG_FPSW,initial['x87_status'])
    machine.reg_write(R.UC_X86_REG_FPTAG,0xffff)
    machine.reg_write(R.UC_X86_REG_MXCSR,initial['mxcsr'])
    stop=struct.unpack('<I',machine.mem_read(initial['gp'][4],4))[0]
    trace=[];touched=set();written=set();estimate_cursor=0
    def instruction(_,pc,size,user):
        nonlocal estimate_cursor
        assert text_start<=pc<text_end,f'Original reached external code owner {pc:08x}; no supplied return is allowed'
        trace.append(pc)
        operation=code.at(pc)
        if operation.mnemonic in ('rsqrtss','rsqrtps'):
            lanes=4 if operation.mnemonic=='rsqrtps' else 1
            destination,source=operation.operands
            if source.type==1:
                value=machine.reg_read(getattr(R,'UC_X86_REG_'+operation.reg_name(source.reg).upper()))
            else:
                assert source.type==3 and source.size==lanes*4
                location=source.mem.disp
                if source.mem.base:location+=machine.reg_read(getattr(R,'UC_X86_REG_'+operation.reg_name(source.mem.base).upper()))
                if source.mem.index:location+=source.mem.scale*machine.reg_read(getattr(R,'UC_X86_REG_'+operation.reg_name(source.mem.index).upper()))
                if source.mem.segment:
                    assert operation.reg_name(source.mem.segment)=='fs';location+=initial['fs_base']
                location&=0xffffffff;memory(None,UC_MEM_READ,location,lanes*4,0,None)
                value=int.from_bytes(machine.mem_read(location,lanes*4),'little')
            register=getattr(R,'UC_X86_REG_'+operation.reg_name(destination.reg).upper())
            result=machine.reg_read(register)
            for lane in range(lanes):
                assert estimate_cursor<len(after['cpu_inputs']),'Original requested an unrecorded CPU estimate'
                sample=after['cpu_inputs'][estimate_cursor];estimate_cursor+=1
                assert sample['input']==(value>>(lane*32)&0xffffffff),'Original reciprocal-sqrt input differs from the declared native CPU sample'
                result=(result&~(0xffffffff<<(lane*32)))|(sample['value']<<(lane*32))
            machine.reg_write(register,result);machine.reg_write(R.UC_X86_REG_EIP,pc+size)
    def memory(_,access,address,size,value,user):
        assert admitted(address,size),f'Original accessed uncommitted bytes {address:08x}+{size:x}'
        touched.update(range(address&~4095,(address+size+4095)&~4095,4096))
        if access==UC_MEM_WRITE:written.update(range(address&~4095,(address+size+4095)&~4095,4096))
    machine.hook_add(UC_HOOK_CODE,instruction)
    machine.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,memory)
    machine.emu_start(before['entry'],stop,count=2000000)
    assert machine.reg_read(R.UC_X86_REG_EIP)==stop,'Original FP phase did not return'
    assert estimate_cursor==len(after['cpu_inputs']),f'Original consumed {estimate_cursor} of {len(after["cpu_inputs"])} declared CPU estimates'
    assert before['entry'] in trace,'Replay never reached its actual first-person owner'
    if before['entry']==0x168ca1:assert 0x169c46 in trace,'Replay bypassed the original registered manager'
    errors=[]
    gp=[machine.reg_read(r) for r in GP]
    if gp!=expected['gp']:errors.append(dict(kind='gp',original=gp,native=expected['gp']))
    xmm=[[machine.reg_read(R.UC_X86_REG_XMM0+n)>>(32*k)&0xffffffff for k in range(4)] for n in range(8)]
    if xmm!=expected['xmm']:errors.append(dict(kind='xmm',original=xmm,native=expected['xmm']))
    flags=machine.reg_read(R.UC_X86_REG_EFLAGS)
    if (flags&0xcd5)!=(expected['eflags']&0xcd5):errors.append(dict(kind='defined_flags',original=flags,native=expected['eflags']))
    for key,register in (('x87_control',R.UC_X86_REG_FPCW),('x87_status',R.UC_X86_REG_FPSW),('mxcsr',R.UC_X86_REG_MXCSR)):
        value=machine.reg_read(register)
        if value!=expected[key]:errors.append(dict(kind=key,original=value,native=expected[key]))
    assert machine.reg_read(R.UC_X86_REG_FPTAG)==0xffff,'Original retains live x87 values not represented by this case'
    changed=[]
    for region in regions:
        address,offset,size=region['address'],region['offset'],region['size']
        actual=bytes(machine.mem_read(address,size));native=native_after[offset:offset+size]
        if actual!=native:
            differences=[i for i,(a,b) in enumerate(zip(actual,native)) if a!=b]
            errors.append(dict(kind='memory',address=address,count=len(differences),first=address+differences[0],
                original=actual[differences[0]:differences[0]+32].hex(),native=native[differences[0]:differences[0]+32].hex()))
        if actual!=original_before[offset:offset+size]:changed.append(address)
    native_trace=after['instructions']
    if trace!=native_trace:
        mismatch=next((i for i,(a,b) in enumerate(zip(trace,native_trace)) if a!=b),min(len(trace),len(native_trace)))
        errors.append(dict(kind='instruction_trace',index=mismatch,original_count=len(trace),native_count=len(native_trace),
            original=[f'{p:08x}' for p in trace[max(0,mismatch-3):mismatch+8]],native=[f'{p:08x}' for p in native_trace[max(0,mismatch-3):mismatch+8]]))
    if hardware_inputs is not None:
        banks=[]
        for region in regions:
            for page in sorted(touched):
                left=max(page,region['address']);right=min(page+4096,region['address']+region['size'])
                if left<right:
                    offset=region['offset']+left-region['address']
                    banks.append(dict(address=left,hex=original_before[offset:offset+right-left].hex()))
        hardware_inputs.append(dict(id=tick,entry=before['entry'],exit_entry=stop,return_address=stop,
            entry_abi=dict(gp=initial['gp'],eflags=initial['eflags'],mxcsr=initial['mxcsr'],
                x87_control_word=initial['x87_control'],x87_status=initial['x87_status'],x87_tag=65535,
                x87_values=[],xmm=[f'{sum(v<<(32*k) for k,v in enumerate(lanes)):032x}' for lanes in initial['xmm']],
                fs_base=initial['fs_base']),initial_banks=banks,exit={},original_instructions=trace,declared_cpu_inputs=after['cpu_inputs']))
    return dict(tick=tick,status='pass' if not errors else 'fail',original_instructions=len(trace),
        exact_memory_bytes=len(original_before),source_regions=len(regions),changed_regions=changed,
        read_write_pages=len(touched),errors=errors,declared_cpu_estimates=estimate_cursor,
        executed_owners={f'{p:08x}':p in trace for p in (0x168ca1,0x169c46,0x16a4dd,0x168ded,0x168d76,0x16b240,0x16b4e8,0x1e1390,0x169c21,0x169c33,0x169ba2,0x18be50,0x18cdd0,0x223b20,0x224740,0x224ec0)},
        director_callback=struct.unpack('<I',machine.mem_read(0x4f5514+0xc,4))[0],
        inputs_sha256={p.name:sha(p) for p in (before_path,after_path,before_path.with_suffix('.bin'),after_path.with_suffix('.bin'))})

def hardware_compare(directory,cases,output):
    from oracle_sandbox_first_person_hardware_tu15 import execute
    instructions={p for case in cases for p in case['original_instructions']}
    paired=dict(address_map={},instruction_map={f'{p:08x}':p for p in instructions},entries={f'{c["entry"]:08x}':c['entry'] for c in cases})
    result=execute(dict(image='1.5',cases=cases),paired,output.resolve())
    comparisons=[]
    for case in result['cases']:
        tick=case['id'];after_path=directory/f'{tick}-after.json';after=json.loads(after_path.read_text())
        expected=after['abi'];actual=case['exit'];native=after_path.with_suffix('.bin').read_bytes();errors=[]
        initial=(directory/f'{tick}-before.bin').read_bytes()
        for key in ('gp','mxcsr','x87_status'):
            if actual[key]!=expected[key]:errors.append(dict(kind=key,original=actual[key],native=expected[key]))
        if actual['x87_control_word']!=expected['x87_control']:
            errors.append(dict(kind='x87_control',original=actual['x87_control_word'],native=expected['x87_control']))
        if (actual['eflags']&0xcd5)!=(expected['eflags']&0xcd5):
            errors.append(dict(kind='defined_flags',original=actual['eflags'],native=expected['eflags']))
        assert actual['x87_tag']==65535 and not actual['x87_values'],'Hardware has unrepresented live x87 values'
        xmm=[[int(actual['xmm'][n],16)>>(32*k)&0xffffffff for k in range(4)] for n in range(8)]
        if xmm!=expected['xmm']:errors.append(dict(kind='xmm',original=xmm,native=expected['xmm']))
        for bank in case['final_banks']:
            value=bytes.fromhex(bank['hex'])
            region=next(r for r in after['regions'] if r['address']<=bank['address'] and
                bank['address']+len(value)<=r['address']+r['size'])
            offset=region['offset']+bank['address']-region['address']
            if value!=native[offset:offset+len(value)]:errors.append(dict(kind='memory',address=bank['address']))
        covered=sorted((bank['address'],bank['address']+len(bank['hex'])//2) for bank in case['initial_banks'])
        for region in after['regions']:
            cursor=region['address'];end=cursor+region['size']
            for left,right in covered:
                if right<=cursor or left>=end:continue
                left=max(left,cursor);right=min(right,end)
                offset=region['offset']+cursor-region['address'];length=left-cursor
                if native[offset:offset+length]!=initial[offset:offset+length]:
                    errors.append(dict(kind='unobserved_native_write',address=cursor))
                cursor=right
            offset=region['offset']+cursor-region['address'];length=end-cursor
            if native[offset:offset+length]!=initial[offset:offset+length]:
                errors.append(dict(kind='unobserved_native_write',address=cursor))
        if case['instructions']!=after['instructions']:errors.append(dict(kind='instruction_trace'))
        observed=[]
        for sample in case['estimates']:
            if isinstance(sample['input'],list):observed.extend(zip(sample['input'],sample['output']))
            else:observed.append((sample['input'],sample['output']))
        declared=[(sample['input'],sample['value']) for sample in after['cpu_inputs']]
        if observed!=declared:errors.append(dict(kind='cpu_estimates',original_count=len(observed),native_count=len(declared)))
        comparisons.append(dict(tick=tick,status='pass' if not errors else 'fail',errors=errors,
            original_instructions=len(case['instructions']),mxcsr=actual['mxcsr'],writes=len(case['writes']),
            exact_memory_bytes=sum(len(b['hex'])//2 for b in case['final_banks']),
            all_committed_bytes_accounted=len(native),cpu_estimate_lanes=len(declared)))
    target=output/'original-results.json';target.write_text(json.dumps(result,separators=(',',':'))+'\n')
    return dict(oracle_backend=result['oracle_backend'],cases=comparisons,
        original_instruction_patches=result['original_instruction_patches'],
        native_expected_data_used=result['native_expected_data_used'],
        retained_input_reads='reject_unretained',results_sha256=sha(target),
        hardware_tool_sha256=result['hardware_tool_sha256'],trace_observer_sha256=result['trace_observer_sha256'],
        bridge_input_tool_sha256=result['bridge_input_tool_sha256'],xbox_hardware_proof=False)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--hardware-directory',type=Path)
    args=parser.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN
    ticks=sorted(int(p.stem.split('-')[0]) for p in args.capture.glob('*-before.json'))
    assert ticks,'No real source FP phases captured'
    hardware_inputs=[] if args.hardware_directory else None
    cases=[compare(args.capture,tick,hardware_inputs) for tick in ticks]
    hardware=hardware_compare(args.capture,hardware_inputs,args.hardware_directory) if hardware_inputs else None
    report=dict(original_sha256=NEW_PIN,scope=__doc__,substituted_gameplay_returns=0,
        xbox_pixels_or_pcm_verified=False,script_sha256=sha(Path(__file__)),unicorn_cases=cases,hardware=hardware)
    # Unicorn diagnostics remain complete. Hardware acceptance requires exact
    # registers, path, CPU estimates, touched bytes and unchanged remaining banks.
    if hardware:
        passed=all(c['status']=='pass' for c in hardware['cases'])
        report['acceptance_backend']='independent_original_i386_hardware'
        report['unicorn_discrepancies_retained']=any(c['errors'] for c in cases)
    else:passed=all(c['status']=='pass' for c in cases)
    report['status']='pass' if passed else 'fail'
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    for case in cases:print(json.dumps(case))
    if hardware:print(json.dumps(hardware))
    return int(not passed)
if __name__=='__main__':raise SystemExit(main())
