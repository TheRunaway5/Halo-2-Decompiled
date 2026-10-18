#!/usr/bin/env python3
"""Execute unchanged TU1.5 bytes from a captured original live call.

Use actual physical RAM, page tables, segments and CPU inputs. RAM aliases share
one host mapping. No source services or game results are supplied. This is an
independent emulator comparison, not a native or physical Xbox certification.
Run via tools/run_test.py; keep only bounded traces and comparison evidence.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import sys
import traceback
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_READ,UC_PROT_ALL,UC_TLB_CPU
from unicorn import x86_const as R
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from original_xbox_pose_case import ROOT,cpu,physical_ram,PageTableMemory,sha
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

GP=[getattr(R,'UC_X86_REG_'+n)for n in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--case',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--kernel-prefix',type=Path,action='append',default=[],
        help='Explicit authenticated uninterrupted kernel diagnostic; no device/IRQ/scheduler results')
    p.add_argument('--cpu-page-tables',action='store_true',
        help='Use captured physical RAM and architectural CR0/CR3/CR4 paging, rather than mapped virtual aliases')
    p.add_argument('--stop-at',type=lambda value:int(value,0),
        help='Observe a separately declared original prefix boundary; the full-call comparison remains failing')
    p.add_argument('--stop-return-address',type=lambda value:int(value,0),
        help='Require the actual stack return address at that boundary; supply no guest result')
    args=p.parse_args();assert 'HALO2_TEST_SCRATCH'in os.environ
    assert args.stop_return_address is None or args.stop_at is not None
    out=args.output.absolute();out.mkdir(parents=True,exist_ok=False);case=args.case.absolute()
    (out/'attempt.json').write_text(json.dumps(dict(runner_scratch=os.environ['HALO2_TEST_SCRATCH'],
        argv=sys.argv,tool_sha256=sha(Path(__file__)),full_weapon_parity=False),indent=2)+'\n')
    call=json.loads((case/'call.json').read_text());before=cpu(case,'before');after=cpu(case,'after')
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    if args.stop_return_address is not None:
        caller=code.at(args.stop_return_address-5)
        assert caller.mnemonic=='call' and caller.size==5 and caller.operands[0].imm==args.stop_at
    text=next(s for s in image.sections if s.name=='.text')
    assert call['entry']==before['eip']and call['entry']in (0x168ded,0x168ca1,0x1030f0)and call['return_address']==after['eip']
    assert before['control_registers']==after['control_registers']
    trace=[];writes=[];pending=[];reads=set();pages={};errors=[];last=[call['entry']];exception=None
    estimates=[];pending_estimates=[];implicit_updates=[]
    kernel_sites={};kernel_report=None;kernel_code=Cs(CS_ARCH_X86,CS_MODE_32);kernel_code.detail=True
    for prefix_path in args.kernel_prefix:
        kernel_report=json.loads((prefix_path/'report.json').read_text())
        transfer=kernel_report.get('architectural_CPU_paging',False)and (kernel_report.get('stop')or{}).get('reason')=='outside authenticated mapped kernel executable pages'
        assert (kernel_report['kernel_return_reached']or transfer)and kernel_report['guest_CLI_STI_flags_only']
        if kernel_report.get('architectural_CPU_paging'):assert args.cpu_page_tables
        assert kernel_report['source_case_sha256']==sha(case/'call.json')
        assert not kernel_report['supplied_gameplay_or_kernel_returns']and not kernel_report['IRQ_PIC_or_scheduler_results_supplied']
        for row in json.loads((prefix_path/'instructions.json').read_text()):
            if row['address']in kernel_sites:assert kernel_sites[row['address']]==row
            kernel_sites[row['address']]=row
    with physical_ram(case,'before')as ram,physical_ram(case,'after')as expected_ram:
        memory=PageTableMemory(ram,before);expected=PageTableMemory(expected_ram,after)
        assert memory.read(text.va,text.rsize)==image.section_bytes(text)
        assert expected.read(text.va,text.rsize)==image.section_bytes(text)
        for pc,row in kernel_sites.items():
            raw=bytes.fromhex(row['bytes'])
            assert memory.read(pc,len(raw))==expected.read(pc,len(raw))==raw
        machine=Uc(UC_ARCH_X86,UC_MODE_32)
        pointer=ctypes.addressof(ctypes.c_char.from_buffer(ram))
        reconstruction=bytearray(ram[:])if args.cpu_page_tables else None
        if args.cpu_page_tables:
            machine.ctl_set_tlb_mode(UC_TLB_CPU)
            machine.mem_map_ptr(0,len(ram),UC_PROT_ALL,pointer)
            for name in ('CR4','CR3','CR2','CR0'):
                machine.reg_write(getattr(R,'UC_X86_REG_'+name),before['control_registers'][name])

        def map_page(address):
            virtual=address&~4095
            if virtual in pages:return
            physical,writable=memory.translate(virtual)
            assert physical%4096==0
            if not args.cpu_page_tables:machine.mem_map_ptr(virtual,4096,UC_PROT_ALL,pointer+physical)
            pages[virtual]=dict(physical=physical,writable=writable)

        def invalid(_machine,access,address,size,value,user):
            if args.cpu_page_tables:
                raise RuntimeError(f'CPU paging reached unavailable physical/device bytes at {last[0]:08x}: {address:08x}+{size:x}')
            try:
                for page in range(address&~4095,(address+size+4095)&~4095,4096):map_page(page)
                return True
            except Exception:
                raise RuntimeError(f'Actual original instruction {last[0]:08x} accessed unavailable virtual bytes {address:08x}+{size:x}')

        machine.hook_add(UC_HOOK_MEM_INVALID,invalid)
        gdt,limit=before['gdtr'];map_page(gdt)
        machine.reg_write(R.UC_X86_REG_GDTR,(0,gdt,limit,0))
        for name in ('cs','ds','es','ss','fs','gs'):
            machine.reg_write(getattr(R,'UC_X86_REG_'+name.upper()),before['selectors'][name])
        assert machine.reg_read(R.UC_X86_REG_FS_BASE)==before['fs_base']
        for register,value in zip(GP,before['gp']):machine.reg_write(register,value)
        for i,value in enumerate(before['xmm']):machine.reg_write(R.UC_X86_REG_XMM0+i,sum(v<<(j*32)for j,v in enumerate(value)))
        top=(before['x87_status']>>11)&7
        for i,value in enumerate(before['x87_register_bytes']):
            machine.reg_write(R.UC_X86_REG_FP0+((i+top)&7),struct.unpack('<QH',bytes.fromhex(value)))
        for name,key in (('EFLAGS','eflags'),('FPCW','x87_control'),('FPSW','x87_status'),('FPTAG','x87_tag'),('MXCSR','mxcsr')):
            machine.reg_write(getattr(R,'UC_X86_REG_'+name),before[key])

        def flush():
            for write in pending:
                address,size=write['address'],write['bytes']
                write['written']=(ram[address:address+size]if args.cpu_page_tables else memory.read(address,size)).hex();writes.append(write)
                if reconstruction is not None:
                    assert reconstruction[address:address+size].hex()==write['before']
                    reconstruction[address:address+size]=bytes.fromhex(write['written'])
            pending.clear()
            for estimate in pending_estimates:
                value=machine.reg_read(estimate['destination'])
                for lane,operand in enumerate(estimate['inputs']):
                    estimates.append(dict(instruction=estimate['instruction'],input=operand,output=value>>(lane*32)&0xffffffff))
            pending_estimates.clear()

        def instruction(_machine,pc,size,user):
            flush();last[0]=pc
            if args.stop_return_address is not None and pc==args.stop_at:
                stack=machine.reg_read(R.UC_X86_REG_ESP)
                if struct.unpack('<I',memory.read(stack,4))[0]==args.stop_return_address:
                    machine.emu_stop()
                    return
            if args.cpu_page_tables:map_page(pc)
            if text.va<=pc<text.va+text.rsize:operation=code.at(pc)
            else:
                assert pc in kernel_sites,f'Original crossed external code owner {pc:08x}'
                row=kernel_sites[pc];raw=bytes.fromhex(row['bytes'])
                assert len(raw)==size and raw==memory.read(pc,size)
                operation=next(kernel_code.disasm(raw,pc,count=1))
                assert operation.mnemonic==row['mnemonic']and operation.op_str==row['operands']
                assert operation.mnemonic not in ('in','out','rdtsc','rdtscp','cpuid','rdmsr','wrmsr','hlt','int','iret','iretd','sysenter','sysexit')
            assert bytes(operation.bytes)==memory.read(pc,size)
            trace.append(pc)
            if operation.mnemonic in ('rsqrtss','rsqrtps'):
                destination,source=operation.operands;lanes=4 if operation.mnemonic=='rsqrtps' else 1
                register=getattr(R,'UC_X86_REG_'+operation.reg_name(destination.reg).upper())
                if source.type==1:value=machine.reg_read(getattr(R,'UC_X86_REG_'+operation.reg_name(source.reg).upper()))
                else:
                    assert source.type==3
                    address=source.mem.disp
                    if source.mem.base:address+=machine.reg_read(getattr(R,'UC_X86_REG_'+operation.reg_name(source.mem.base).upper()))
                    if source.mem.index:address+=source.mem.scale*machine.reg_read(getattr(R,'UC_X86_REG_'+operation.reg_name(source.mem.index).upper()))
                    if source.mem.segment:
                        assert operation.reg_name(source.mem.segment)=='fs';address+=before['fs_base']
                    value=int.from_bytes(memory.read(address&0xffffffff,lanes*4),'little')
                pending_estimates.append(dict(instruction=pc,destination=register,inputs=[value>>(lane*32)&0xffffffff for lane in range(lanes)]))

        def access(_machine,kind,address,size,value,user):
            if kind==17:
                if args.cpu_page_tables:
                    assert 0<=address<address+size<=len(ram)
                    old=ram[address:address+size]
                    if old!=reconstruction[address:address+size]:
                        implicit_updates.append(dict(before_source_write_index=len(writes)+len(pending),
                            address=address,before=reconstruction[address:address+size].hex(),written=old.hex(),
                            observation='Implicit CPU RAM change observed before this source write'))
                        reconstruction[address:address+size]=old
                else:
                    for page in range(address&~4095,(address+size+4095)&~4095,4096):
                        _,writable=memory.translate(page)
                        assert writable or not before['control_registers']['CR0']&0x10000
                    old=memory.read(address,size)
                pending.append(dict(instruction=last[0],address=address,bytes=size,before=old.hex()))
            else:reads.add((address,size))

        machine.hook_add(UC_HOOK_CODE,instruction)
        machine.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access)
        try:machine.emu_start(call['entry'],call['return_address'] if args.stop_return_address is not None else args.stop_at or call['return_address'],count=2000000);flush()
        except Exception as error:exception=repr(error)
        actual=dict(eip=machine.reg_read(R.UC_X86_REG_EIP),gp=[machine.reg_read(r)for r in GP],
            xmm=[[machine.reg_read(R.UC_X86_REG_XMM0+i)>>(j*32)&0xffffffff for j in range(4)]for i in range(8)],
            eflags=machine.reg_read(R.UC_X86_REG_EFLAGS),x87_control=machine.reg_read(R.UC_X86_REG_FPCW),
            x87_status=machine.reg_read(R.UC_X86_REG_FPSW),x87_tag=machine.reg_read(R.UC_X86_REG_FPTAG),
            mxcsr=machine.reg_read(R.UC_X86_REG_MXCSR))
        actual_control={name:machine.reg_read(getattr(R,'UC_X86_REG_'+name))for name in ('CR0','CR2','CR3','CR4')}if args.cpu_page_tables else None
        if args.cpu_page_tables and actual_control!=after['control_registers']:
            errors.append(dict(kind='control_registers',actual=actual_control,expected=after['control_registers']))
        if exception:errors.append(dict(kind='execution',exception=exception,last_pc=last[0]))
        for key in ('eip','gp','xmm','x87_control','x87_status','x87_tag','mxcsr'):
            if actual[key]!=after[key]:errors.append(dict(kind=key,actual=actual[key],expected=after[key]))
        if actual['eflags']&0xcd5!=after['eflags']&0xcd5:errors.append(dict(kind='defined_flags',actual=actual['eflags'],expected=after['eflags']))
        models=[]
        for i,model in enumerate(call['models']):
            address=call['arguments'][3]+i*0xd0c
            descriptor=memory.read(address,12);nodes=memory.read(address+12,model['node_count']*52)
            expected_nodes=(case/model['nodes_file']).read_bytes()
            equal=nodes==expected_nodes
            models.append(dict(model=model['model'],node_count=model['node_count'],descriptor=list(struct.unpack('<3I',descriptor)),
                exact_nodes=equal,actual_sha256=hashlib.sha256(nodes).hexdigest(),expected_sha256=model['nodes_sha256']))
            if not equal:errors.append(dict(kind='model_nodes',model=model['model'],different_words=sum(a!=b for a,b in zip(struct.unpack('<'+'I'*(len(nodes)//4),nodes),struct.unpack('<'+'I'*(len(expected_nodes)//4),expected_nodes)))))
        touched_final=[]
        for address,size in sorted({(w['address'],w['bytes'])for w in writes}):
            if args.cpu_page_tables:got,want=ram[address:address+size],expected_ram[address:address+size]
            else:got,want=memory.read(address,size),expected.read(address,size)
            if got!=want:
                touched_final.append(dict(address=address,bytes=size,actual=got.hex(),expected=want.hex()))
        if touched_final:errors.append(dict(kind='final_written_bytes',different_spans=len(touched_final),examples=touched_final[:8]))
        state_bank=call.get('first_person_state');state_equal=None
        if state_bank:
            state_equal=memory.read(state_bank['address'],state_bank['bytes'])==(case/'after-first-person-state.bin').read_bytes()
            if not state_equal:errors.append(dict(kind='complete_first_person_state'))
        object_results=[]
        for obj in call.get('observed_objects',[]):
            raw=memory.read(obj['address'],obj['bytes']);equal=raw==(case/('after-'+obj['name']+'-body.bin')).read_bytes()
            object_results.append(dict(name=obj['name'],identity=obj['identity'],bytes=obj['bytes'],exact=equal))
            if not equal:errors.append(dict(kind='complete_object_body',name=obj['name']))
        physical_diff=[]
        for page in range(0,len(ram),4096):
            if ram[page:page+4096]!=expected_ram[page:page+4096]:
                different=[i for i,(a,b)in enumerate(zip(ram[page:page+4096],expected_ram[page:page+4096]))if a!=b]
                physical_diff.append(dict(physical_page=page,different_bytes=len(different),first=page+different[0]))
        if reconstruction is not None:
            for page in range(0,len(ram),4096):
                if reconstruction[page:page+4096]==ram[page:page+4096]:continue
                offset=page
                while offset<page+4096:
                    if reconstruction[offset]==ram[offset]:offset+=1;continue
                    start=offset
                    while offset<page+4096 and reconstruction[offset]!=ram[offset]:offset+=1
                    implicit_updates.append(dict(before_source_write_index=len(writes),address=start,
                        before=reconstruction[start:offset].hex(),written=ram[start:offset].hex(),
                        observation='Residual implicit CPU RAM change observed at this execution boundary'))
                    reconstruction[start:offset]=ram[start:offset]
            assert reconstruction==ram[:]
        report=dict(passed=not errors,scope=__doc__,entry=call['entry'],exit=actual['eip'],instruction_count=len(trace),
            unique_instructions=len(set(trace)),read_spans=len(reads),ordered_source_writes=len(writes),mapped_virtual_pages=len(pages),
            original_CPU_and_owned_outputs_equal=not errors,actual_CPU=actual,expected_CPU=after,models=models,errors=errors,
            complete_first_person_state_equal=state_equal,
            complete_object_bodies=object_results,
            differing_physical_pages=physical_diff,whole_physical_RAM_equal=not physical_diff,
            physical_RAM_comparison_note='Includes original page-table/device/asynchronous changes; these are recorded, not claimed as API-owned writes',
            source_instruction_patches=[],supplied_gameplay_returns=[],synthetic_memory_banks=[],
            authenticated_kernel_prefixes=[dict(path=str(path.absolute()),report_sha256=sha(path/'report.json'))for path in args.kernel_prefix],
            kernel_instructions=sum(pc in kernel_sites for pc in trace),
            uninterrupted_guest_execution=True,asynchronous_interrupts_delivered=False,
            IRQ_PIC_device_or_scheduler_results_supplied=False,
            architectural_CPU_paging=args.cpu_page_tables,
            write_and_read_trace_address_space='physical'if args.cpu_page_tables else 'virtual',
            final_control_registers=actual_control,
            declared_prefix_stop=args.stop_at,
            declared_prefix_stop_return_address=args.stop_return_address,
            declared_prefix_stop_reached=args.stop_at is not None and actual['eip']==args.stop_at and exception is None,
            implicit_CPU_RAM_updates=implicit_updates,
            complete_intermediate_RAM_reconstruction_sha256=hashlib.sha256(ram).hexdigest()if reconstruction is not None else None,
            case_files_sha256={str(p):sha(p)for p in case.iterdir()if p.is_file()},
            runner_scratch=os.environ['HALO2_TEST_SCRATCH'],native_translation_verified=False,physical_xbox_verified=False,full_weapon_parity=False)
    (out/'instructions.json').write_text(json.dumps(trace)+'\n')
    (out/'mapped-pages.json').write_text(json.dumps(pages,indent=2)+'\n')
    (out/'CPU-estimates.json').write_text(json.dumps(estimates,indent=2)+'\n')
    (out/'writes.json').write_text(json.dumps(writes,separators=(',',':'))+'\n')
    for source in (Path(__file__),Path(__file__).with_name('original_xbox_pose_case.py')):shutil.copyfile(source,out/source.name)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in('scope','actual_CPU','expected_CPU','case_files_sha256','differing_physical_pages')}) ,flush=True)
    assert report['passed'],str(out/'report.json')

if __name__=='__main__':main()
