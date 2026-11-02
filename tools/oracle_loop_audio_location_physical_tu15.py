#!/usr/bin/env python3
"""Re-execute all22 existing loop-location inputs on unchanged physical i386.

Preserve the existing emulator-trace fixture. Verify its exact nonstack writes
and hardware exits, and emit a separate physical-trace reference. Linux TLS is
configured by new caller-owned setup code, never by patching original code or
supplying a game/SDK result. Run through tools/run_test.py.
"""
import argparse,copy,ctypes,hashlib,itertools,json,os,signal,struct
from pathlib import Path
from original_audio_cpu import OriginalAudioCpu
from oracle_smg_firing_1_1 import _Code
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images

BASE,PRIVATE,PRIVATE_SIZE,STACK,STACK_SIZE,FS,STOP=0x1000000,0x2400000,0x40000,0x60000000,0x200000,0x60200000,0x60300000
REP={0x17ddb4,0x17ddfb}
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--input',type=Path,default=Path('tests/fixtures/loop_audio_locations_tu15_original.json'));p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--marker-matrix',action='store_true');a=p.parse_args();native_sites=REP|({0xbaeb0} if a.marker_matrix else set())
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);before=a.input.read_bytes();legacy=json.loads(before);_,image,pin=next(r for r in images() if r[0]=='TU1.5');assert legacy['original_xbe_sha256']==pin;assert len(legacy['cases'])==22;code=_Code(image,'.text');output=copy.deepcopy(legacy);rows=[];steps=0;corrected_access={};wide_stack=0;wide_nonstack=0
    with OriginalAudioCpu(image,scratch/'loop-locations-original.elf',639,8064,extra_declared_regions=[(STACK,STACK_SIZE+4096),(STOP,4096)],source_bank_bytes=0x1500000) as host:
        host.trace(6,host.debug_offset+7*8,0)
        # Actual32-bit set_thread_area, followed by loading the allocated FS
        # selector. This is independent caller environment setup at an owned
        # executable address outside every original XBE code range.
        descriptor=BASE+0x8000;setup=0x3000000
        stub=bytes.fromhex('b8f3000000bb')+struct.pack('<I',descriptor)+bytes.fromhex('cd808b0bc1e10383c9038ee1cc')
        host.write(descriptor,struct.pack('<IIII',0xffffffff,FS,0xfffff,0x51));host.write(setup,stub);host.prepare(setup,BASE+0xf000,{})
        host.trace(7);_,status=os.waitpid(host.pid,0);regs=host.registers();assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP and regs.rax==0,(status,regs.rax)
        assert regs.fs_base==FS,(hex(regs.fs),hex(regs.fs_base));host.initial.fs=regs.fs;host.initial.fs_base=regs.fs_base
        machine=EffectCpuState(host)
        for row in output['cases']:
            host.write(PRIVATE,bytes(PRIVATE_SIZE));host.write(STACK,bytes(STACK_SIZE));host.write(FS,bytes(4096))
            for address,value in row['source_pointer_overrides'].items():host.write(int(address,16),struct.pack('<I',value))
            for address,payload in row['private_bank']['nonzero_blocks']:host.write(address,bytes.fromhex(payload))
            caller=bytes.fromhex(row['caller_hex']);gp=list(struct.unpack_from('<8I',caller));assert len(caller)==168 and struct.unpack_from('<I',caller,164)[0]==FS
            host.write(gp[4],struct.pack('<'+'I'*len(row['stack_arguments']),*row['stack_arguments']))
            host.prepare(row['entry'],gp[4],dict(zip(ALIASES,gp)))
            machine.load(dict(cw=639,status=0,mxcsr=8064,stack=[],xmm=[caller[32+16*i:48+16*i].hex() for i in range(8)]))
            regs=host.registers();regs.eflags=struct.unpack_from('<I',caller,160)[0]|0x200;host.trace(13,0,ctypes.addressof(regs));trace=[];writes=[]
            for _ in range(20000):
                pc=regs.rip
                if pc==STOP:break
                ins=code.at(pc);assert host.read(pc,len(ins.bytes))==bytes(ins.bytes);trace.append(pc);pending=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&0xffffffff
                if ins.mnemonic=='push' or ins.mnemonic=='call':pending.append(((regs.rsp-4)&0xffffffff,4))
                if ins.mnemonic.startswith('rep'):
                    if regs.rcx&0xffffffff:
                        if 'movs' in ins.mnemonic or 'stos' in ins.mnemonic:pending.append((regs.rdi&0xffffffff,4))
                        else:raise ValueError('Undeclared repeat write '+ins.mnemonic)
                else:
                    for operand_index,op in enumerate(ins.operands):
                        if op.type!=3:continue
                        access=op.access
                        if ins.mnemonic=='movhps'and operand_index==0:
                            assert op.size==8 and bytes(ins.bytes).startswith(bytes.fromhex('0f17'))
                            corrected_access[pc]=dict(reported_access=op.access,actual_access=2,opcode=bytes(ins.bytes).hex());access=2
                        if not access&2:continue
                        at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp+(FS if ins.reg_name(op.mem.segment)=='fs' else 0))&0xffffffff;pending.append((at,op.size))
                saved=[(at,width,host.read(at,width)) for at,width in pending]
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(row['case'],hex(pc),status,hex(regs.rip))
                for at,width,prior in saved:
                    if pc in corrected_access:
                        if STACK<=at and at+width<=STACK+STACK_SIZE:wide_stack+=1
                        else:wide_nonstack+=1
                    if STACK<=at and at+width<=STACK+STACK_SIZE:continue
                    assert PRIVATE<=at and at+width<=PRIVATE+PRIVATE_SIZE,(row['case'],hex(pc),hex(at),width)
                    writes.append(struct.pack('<III',pc,at,width)+prior+host.read(at,width))
            else:raise ValueError('Original loop-location instruction limit')
            assert regs.rsp==gp[4]+4*len(row['stack_arguments'])
            state=machine.snapshot(regs);fp=state['fp'];assert not fp['stack'] and not fp['status']&0x3800
            exit_bytes=struct.pack('<8I',*state['gp'])+b''.join(bytes.fromhex(v) for v in fp['xmm'])+struct.pack('<IIIHHI',state['eflags'],regs.fs_base,0,fp['status'],fp['cw'],fp['mxcsr'])
            expected=bytes.fromhex(row['expected_hardware_exit_hex']);assert len(exit_bytes)==len(expected)==180
            actual_masked=bytearray(exit_bytes);expected_masked=bytearray(expected)
            for raw in [actual_masked,expected_masked]:struct.pack_into('<I',raw,160,struct.unpack_from('<I',raw,160)[0]&~(DEBUG_FLAGS|0x200))
            assert actual_masked==expected_masked,(row['case'],'hardware exit differs',exit_bytes.hex(),expected.hex())
            ordered=b''.join(writes);assert ordered.hex()==row['original_ordered_nonstack_writes_hex'],(row['case'],'ordered original nonstack writes differ')
            physical=b''.join(struct.pack('<I',pc) for pc in trace);old=bytes.fromhex(row['original_trace_hex']);changed=physical!=old
            old_pcs=list(struct.unpack('<'+'I'*(len(old)//4),old));groups=lambda pcs:[(pc,len(list(g))) for pc,g in itertools.groupby(pcs)]
            legacy_groups,physical_groups=groups(old_pcs),groups(trace);assert [pc for pc,n in legacy_groups]==[pc for pc,n in physical_groups],(row['case'],'physical branch/PC order differs')
            projected=[];removed=[];native_removed=[]
            for (pc,count),(actual_pc,actual_count) in zip(legacy_groups,physical_groups):
                if count!=actual_count:
                    assert code.at(pc).mnemonic.startswith('rep') and count==actual_count+1,(row['case'],hex(pc),count,actual_count)
                    removed.append(dict(pc=pc,legacy_count=count,physical_count=actual_count))
                use=actual_count if pc in native_sites else count
                if use!=count:
                    assert count==14 and actual_count==13;native_removed.append(pc)
                projected.extend([pc]*use)
            native_trace=b''.join(struct.pack('<I',pc) for pc in projected)
            if a.marker_matrix:assert native_trace==physical
            rows.append(dict(case=row['case'],instructions=len(trace),legacy_instructions=len(old_pcs),trace_changed=changed,extra_zero_count_REP_observations=removed,native_copy_removed_observations=native_removed,legacy_trace_hex=old.hex(),physical_trace_hex=physical.hex(),native_copy_trace_hex=native_trace.hex(),legacy_trace_sha256=sha(old),physical_trace_sha256=sha(physical),ordered_nonstack_writes_sha256=sha(ordered),hardware_exit_sha256=sha(exit_bytes),original_writes_equal=True,original_full_hardware_exit_equal_except_IF_TF_RF=True))
    expected={'attached_world','encoded_world_without_view'}|({'marker_missing_model','marker_missing_name','parented_marker_fallback'} if a.marker_matrix else set());assert {r['case'] for r in rows if r['native_copy_removed_observations']}==expected
    reference=dict(schema='HALO2_LOOP_AUDIO_MARKER_MATRIX_PHYSICAL_TRACE_V1' if a.marker_matrix else 'HALO2_LOOP_AUDIO_NATIVE_COPY_PHYSICAL_TRACE_V1',original_xbe_sha256=pin,provenance='Physical Linux i386 single-step PC trace of every existing declared input; original nonstack writes and hardware exits verified. All22 complete case traces are physical.' if a.marker_matrix else 'Physical native-copy observations with explicitly retained lower legacy observation contracts.',physical_trace_backend='Linux i386 PTRACE_SINGLESTEP',legacy_fixture_sha256=sha(before),native_copy_rep_sites=sorted(native_sites),cases=rows,source_instruction_patches=[],supplied_game_services=[],physical_complete_case_traces=a.marker_matrix,all_lower_paths_or_lifecycles_proven=False,physical_xbox_verified=False)
    data=(json.dumps(reference,indent=2)+'\n').encode();a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_bytes(data);assert a.input.read_bytes()==before
    report=dict(passed=True,cases=22,hardware_instructions=steps,original_sha256=pin,legacy_fixture=str(a.input),legacy_fixture_sha256=sha(before),legacy_fixture_unchanged=True,fixture=str(a.fixture),fixture_bytes=len(data),fixture_sha256=sha(data),capstone_access_corrections=corrected_access,movhps_stack_store_events=wide_stack,movhps_nonstack_store_events=wide_nonstack,rows=[{k:v for k,v in r.items() if not k.endswith('_hex')} for r in rows],generator_sha256=sha(Path(__file__).read_bytes()),caller_TLS_setup_address=setup,caller_TLS_setup_sha256=sha(stub),actual_Linux_set_thread_area=True,scratch_owner=str(scratch),instruction_patches=[],supplied_game_services=[],physical_xbox_proof=False,full_user_goal_complete=False,retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=True,cases=22,hardware_instructions=steps,physical_trace_differences=sum(r['trace_changed'] for r in rows),native_adjusted_cases=len(expected),physical_complete_case_traces=a.marker_matrix,fixture_bytes=len(data),fixture_sha256=sha(data),legacy_fixture_unchanged=True)))
if __name__=='__main__':main()
