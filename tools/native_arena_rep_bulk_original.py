"""Hardware-breakpoint envelope for the unchanged complete arena REP span.

No guest instruction/register/memory mutation follows declared input setup.
The genuine following CALL retires after the execution breakpoint, so escaped
EFLAGS are compared after hardware clears its diagnostic RF image naturally.
"""
import copy,hashlib,json,subprocess
from pathlib import Path
from validate_native_tu15_prt_helpers import physical_original

def physical_bulk_rep(image,rows,scratch,regions):
    smoke=copy.deepcopy(rows)
    for row in smoke:
        row['gp'][1]=1;row['max_steps']=20;row['stop_before_entries']=[0x12667a];row.pop('bulk_repeat_to',None)
    _,preparation=physical_original(image,smoke,scratch,entries=(0x126640,),regions=regions,
        trace_calls=[],trace_extended=True,preserve_x87_prefix=True)
    directory=scratch/'physical-original';observer=directory/'observer.cpp';source=observer.read_text()
    anchor='executed.insert(pc);'
    if source.count(anchor)!=1:raise ValueError('Physical observer instruction boundary drifted')
    fast=r'''
   if(c.contains("bulk_repeat_to")&&pc==0x126678u){
    const auto target=c.at("bulk_repeat_to").get<std::uint32_t>();
    check(target==0x12667au&&std::uint32_t(regs.rcx)==0xff800u&&std::uint32_t(regs.rax)==0,"Unexpected complete arena REP inputs");
    check(!(regs.eflags&0x10000u),"Incoming bulk REP has diagnostic RF");
    request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(struct user,u_debugreg[0])),reinterpret_cast<void*>(std::uintptr_t(target)));
    request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(struct user,u_debugreg[7])),reinterpret_cast<void*>(std::uintptr_t(1)));
    request(PTRACE_CONT,child.pid);wait();
    check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Complete original REP did not reach hardware breakpoint");
    request(PTRACE_GETREGS,child.pid,nullptr,&regs);
    check(std::uint32_t(regs.rip)==target&&std::uint32_t(regs.rcx)==0,"Hardware breakpoint did not follow complete original REP");
    request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(struct user,u_debugreg[7])),nullptr);
    request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(struct user,u_debugreg[0])),nullptr);
    continue;
   }
'''
    source='#include <cstddef>\n'+source.replace(anchor,anchor+fast,1);observer.write_text(source)
    command=['c++','-std=c++20','-O2',str(observer),'-o',str(directory/'bulk-observer')]
    built=subprocess.run(command,capture_output=True,text=True)
    if built.returncode:raise ValueError('Bulk original observer compile failed: '+built.stderr[-5000:])
    inputs=directory/'bulk-inputs.json';output=directory/'bulk-observed.json';inputs.write_text(json.dumps(rows,separators=(',',':')))
    command=[str(directory/'bulk-observer'),str(directory/'reference'),str(directory/'metadata.json'),str(inputs),str(output)]
    done=subprocess.run(command,capture_output=True,text=True,timeout=90)
    if done.returncode:raise ValueError('Bulk original failed: '+(done.stdout+done.stderr)[-6000:])
    data=json.loads(output.read_text())
    if data['instruction_patches']or data['supplied_original_outcomes']:raise ValueError('Bulk original supplied an instruction/outcome')
    for row in data['cases']:
        if not row['completed']or row['stopped_at']!=0x21b990 or row['boundary_eflags']&0x10000:raise ValueError('Missing genuine post-CALL child entry/escaped flag authority')
        if row['executed_sites']!=[0x126678,0x12667a]:raise ValueError('Bulk original retired unexpected sites')
    return data,dict(backend='physical-unchanged-i386-hardware-execution-breakpoint',cases=len(rows),
        original_image_sha256=hashlib.sha256(image.data).hexdigest(),original_text_sha256=hashlib.sha256((directory/'text.bin').read_bytes()).hexdigest(),
        observer_sha256=hashlib.sha256(source.encode()).hexdigest(),metadata_sha256=hashlib.sha256((directory/'metadata.json').read_bytes()).hexdigest(),
        preparation_one_word_probe=preparation,bulk_word_count=0xff800,bulk_bytes=0x3fe000,
        per_word_trace_qualified_here=False,following_call_retires=True,saved_debug_rf_not_used_as_execution_flags=True,
        guest_instruction_patches=[],supplied_original_outcomes=False)
