#!/usr/bin/env python3
"""Replay one captured original36B50 call over its actual inputs, without services.

The observed original return is an independent RAM/register export. Execution
stops honestly at uncaptured/kernel/MMIO services. No readiness, elapsed time,
admission, random stream, shader program or SDK callee is supplied in its place.
"""
from pathlib import Path
import collections
import hashlib
import json
import re
import sys

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from audit_source_weather_particle_simulation_original import load, reg, xmm, snapshot_record, GPRS
from differential_source_object_admission_snapshot import PIN, words
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def main():
    base = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1'
    output = ROOT / 'analysis/multiplayer-shading/weather-full-original'
    output.mkdir(parents=True, exist_ok=True)
    entry = load(base / 'lockout-weather-full-entry')
    actual = load(base / 'lockout-weather-full-return')
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    registers = entry.report['registers_before']
    stack = reg(registers, 'ESP')
    return_pc = words(entry.read(stack, 4))[0]
    assert reg(registers, 'EIP') == 0x36b50
    assert reg(actual.report['registers_before'], 'EIP') == return_pc == 0x2c16e
    assert reg(actual.report['registers_before'], 'ESP') == stack + 4
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    pages, written, seen = {}, set(), {}
    counters, calls = collections.Counter(), collections.Counter()
    boundaries, order, admission = [], [], []
    watched = {0x36b50:'weather',0x398d0:'intensity',0x4b3d0:'wind',
        0x37260:'plate simulation',0x38450:'particle simulation',0x37a60:'plate draw',
        0x391f0:'particle draw',0x3a5c0:'plate pixel program',0x39780:'particle pixel program',
        0x12de70:'resource gate',0x143c0:'bitmap request',0x137550:'image binding',
        0x12fa0:'camera constants'}
    def page(address):
        at = address & ~4095
        if at not in pages:
            raw = entry.read(at, 4096)
            u.mem_map(at, 4096)
            u.mem_write(at, raw)
            pages[at] = raw
    def read(address, size):
        for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
            page(at)
        return bytes(u.mem_read(address, size))
    def missing(g, access, address, size, value, _):
        try:
            read(address, size)
            return True
        except ValueError as error:
            boundaries.append({'kind':'Uncaptured original memory/MMIO; not supplied',
                'PC':f'{g.reg_read(UC_X86_REG_EIP):08X}','address':f'{address:08X}','detail':str(error)})
            g.emu_stop()
            return False
    def code(g, pc, size, _):
        if pc == return_pc:
            g.emu_stop()
            return
        try:
            source = image.read(pc, size)
        except (ValueError, KeyError):
            boundaries.append({'kind':'Actual external/kernel entry; not replaced','PC':f'{pc:08X}'})
            g.emu_stop()
            return
        assert read(pc, size) == source, f'Changed original instruction {pc:08X}'
        seen[pc] = size
        counters['instructions'] += 1
        if pc in watched:
            calls[f'{pc:08X}'] += 1
            order.append({'PC':f'{pc:08X}','role':watched[pc],
                'EAX':f'{g.reg_read(UC_X86_REG_EAX):08X}','EBX':f'{g.reg_read(UC_X86_REG_EBX):08X}',
                'ECX':f'{g.reg_read(UC_X86_REG_ECX):08X}','ESI':f'{g.reg_read(UC_X86_REG_ESI):08X}',
                'XMM0':f'{g.reg_read(UC_X86_REG_XMM0):032X}',
                'stack_words':[f'{v:08X}' for v in words(read(g.reg_read(UC_X86_REG_ESP),16))]})
        if pc == 0x36dd9:
            sp = g.reg_read(UC_X86_REG_ESP)
            admission.append({'flags':list(read(sp+0x30,16)),
                'distance_words':[f'{v:08X}' for v in words(read(sp+0x40,64))]})
    def writing(g, access, address, size, value, _):
        assert not any(pc < address+size and address < pc+n for pc,n in seen.items())
        read(address, size)
        written.update(range(address,address+size))
        counters['writes'] += 1
    u.hook_add(UC_HOOK_MEM_UNMAPPED, missing)
    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, writing)
    ga,gl=(int(v,16) for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',registers).groups())
    read(ga,gl+1)
    u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
    for name in ['CS','SS','DS','ES','FS','GS']:
        u.reg_write(globals()['UC_X86_REG_'+name],int(re.search(r'\b'+name+r'\s*=([0-9a-fA-F]+)',registers)[1],16))
    for name in GPRS:
        u.reg_write(globals()['UC_X86_REG_'+name],reg(registers,name))
    u.reg_write(UC_X86_REG_EFLAGS,reg(registers,'EFL'))
    for i in range(8):
        u.reg_write(globals()['UC_X86_REG_XMM'+str(i)],xmm(registers,i))
    assert reg(registers,'FTW') == 0
    for i in range(8):
        value=re.search(r'FPR%d=([0-9a-fA-F]{16})\s+([0-9a-fA-F]{4})'%i,registers)
        u.reg_write(globals()['UC_X86_REG_FP'+str(i)],(int(value[1],16),int(value[2],16)))
    for name,number in [('FCW',UC_X86_REG_FPCW),('FSW',UC_X86_REG_FPSW),('MXCSR',UC_X86_REG_MXCSR)]:
        u.reg_write(number,reg(registers,name))
    u.reg_write(UC_X86_REG_FPTAG,0xffff)
    try:
        u.emu_start(0x36b50,return_pc,count=4_000_000)
    except UcError:
        if not boundaries:
            raise
    complete=not boundaries and u.reg_read(UC_X86_REG_EIP)==return_pc
    budget_exhausted=not complete and not boundaries
    durable=sorted(a for a in written if not stack-0x2000<=a<stack+4)
    differences=[a for a in durable if read(a,1)!=actual.read(a,1)] if complete else None
    # Compare only named source producer banks at the real boundary. Equality
    # with the independently observed call return does not bypass that boundary
    # or certify the SDK command tail, register cleanup or GPU consumption.
    producer_banks={}
    simulation=next((row for row in order if row['role']=='particle simulation'),None)
    if simulation:
        system=int(simulation['EBX'],16)
        owner=int(simulation['stack_words'][1],16)
        group=words(read(system+0x68,4))[0]
        header=words(read(group,16))
        count,render,state=header[0],header[1],header[3]
        assert 0<count<=1_000_000
        auxiliary=words(read(0x4e7408,4))[0]
        for name,address,size in [('system',system,140),('owner_weather',owner,176),
            ('render',render,count*20),('state',state,count*32),
            ('auxiliary_RNG',auxiliary+4,4),('elapsed_clock',0x509418,8),
            ('intensity',0x4670fc,8)]:
            before,observed,expected=entry.read(address,size),read(address,size),actual.read(address,size)
            files={}
            for suffix,data in [('entry',before),('replay-at-boundary',observed),('actual-return',expected)]:
                path=output/(name+'-'+suffix+'.bin');path.write_bytes(data)
                files[suffix]={'file':str(path.relative_to(ROOT)),'bytes':size,'sha256':sha(data)}
            producer_banks[name]={'address':f'{address:08X}','bytes':size,'files':files,
                'differing_bytes_at_boundary_vs_actual_return':sum(a!=b for a,b in zip(observed,expected))}
    cpu={}
    if complete:
        actual_registers=actual.report['registers_before']
        for name in GPRS:
            a,b=u.reg_read(globals()['UC_X86_REG_'+name]),reg(actual_registers,name)
            cpu[name]={'replay':f'{a:08X}','actual':f'{b:08X}','equal':a==b}
        for i in range(8):
            a,b=u.reg_read(globals()['UC_X86_REG_XMM'+str(i)]),xmm(actual_registers,i)
            cpu['XMM'+str(i)]={'replay':f'{a:032X}','actual':f'{b:032X}','equal':a==b}
        for name,number in [('EFL',UC_X86_REG_EFLAGS),('FCW',UC_X86_REG_FPCW),('FSW',UC_X86_REG_FPSW),('MXCSR',UC_X86_REG_MXCSR)]:
            a,b=u.reg_read(number),reg(actual_registers,name)
            cpu[name]={'replay':f'{a:08X}','actual':f'{b:08X}','equal':a==b}
    report={'scope':__doc__,'source_executable_sha256':PIN,'tool_sha256':sha(Path(__file__).read_bytes()),
        'imported_replay_utility_sha256':sha((ROOT/'tools/audit_source_weather_particle_simulation_original.py').read_bytes()),
        'entry':snapshot_record(entry),'actual_return':snapshot_record(actual),
        'complete_whole36B50':complete,'diagnostic_instruction_budget_exhausted':budget_exhausted,
        'original_external_boundaries':boundaries,'unique_original_instructions':len(seen),
        'executed_instructions':counters['instructions'],'original_write_operations':counters['writes'],
        'mapped_pages':len(pages),'nonstack_written_bytes':len(durable),
        'nonstack_differing_written_bytes':len(differences) if complete else None,
        'first_differences':[f'{a:08X}' for a in differences[:32]] if complete else None,
        'CPU':cpu,'calls':dict(calls),'source_call_order':order,'admission':admission,
        'named_producer_banks_at_boundary':producer_banks,
        'source_instruction_patches':[],'services_replaced':[],'readiness_or_GPU_results_supplied':False,
        'guest_IO':False,'native_runtime_activation':False,'physical_Xbox_output_claimed':False,
        'comparison_limit':('Complete-call nonstack written bytes and listed registers are compared; not entire RAM histories or GPU consumption.'
            if complete else 'Only the seven complete named producer-bank prefixes at the unsupplied boundary are compared with the actual return. Nonstack-written-byte and return-register comparisons are not performed; no SDK tail or GPU consumption claim.')}
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ['complete_whole36B50','diagnostic_instruction_budget_exhausted',
        'original_external_boundaries','unique_original_instructions','executed_instructions',
        'nonstack_written_bytes','nonstack_differing_written_bytes','calls']},indent=2))
    print(json.dumps({'producer_bank_differences':{name:row['differing_bytes_at_boundary_vs_actual_return']
        for name,row in producer_banks.items()}},indent=2))
    # This bounded proof must retain its actual unsupplied GPU boundary. A
    # different stop or a producer disagreement is a failed reproduction,
    # not a successful whole-call comparison.
    assert not complete and not budget_exhausted
    assert len(boundaries)==1 and boundaries[0]['PC']=='003FA886' and boundaries[0]['address']=='FD800044'
    assert len(producer_banks)==7
    assert all(row['differing_bytes_at_boundary_vs_actual_return']==0 for row in producer_banks.values())

if __name__=='__main__':
    main()
