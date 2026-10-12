#!/usr/bin/env python3
"""Run the owned CRT bodies on the host's real 32-bit x87/SSE backend.

Linux ELF is only a test harness. Original paired instruction bytes are copied
unchanged. An unported owner receives a jump to the capture trampoline before
any of its body executes; the jump changes no guest registers, flags or stack
and supplies no callee result. Hardware output is an independent final-state
oracle, not a source of production behavior or an instruction trace.
"""
import argparse,copy,json,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_vehicle_control_20261011 import sha
from oracle_smg_firing_1_1 import _Code
from capstone import CS_AC_WRITE

ASM=r'''
.code32
.section .bridge,"awx"
.globl _start
_start:
 mov $harness_stack_end,%esp
loop:
 mov $input,%ecx
 mov $288,%edx
 call read_exact
 mov $phase,%ecx
 mov $92,%edx
 call read_exact
 mov input+284,%ebp
 mov $banks,%edi
bank_read:
 test %ebp,%ebp
 jz setup
 mov %edi,%ecx
 mov $8,%edx
 call read_exact
 mov (%edi),%ecx
 mov 4(%edi),%edx
 call read_exact
 add $8,%edi
 dec %ebp
 jmp bank_read
setup:
 mov $0x100f000,done_entry
 frstor input+48
 ldmxcsr input+44
 movdqu input+156,%xmm0
 movdqu input+172,%xmm1
 movdqu input+188,%xmm2
 movdqu input+204,%xmm3
 movdqu input+220,%xmm4
 movdqu input+236,%xmm5
 movdqu input+252,%xmm6
 movdqu input+268,%xmm7
 push input+40
 popfl
 mov input+8,%eax
 mov input+12,%ecx
 mov input+16,%edx
 mov input+20,%ebx
 mov input+28,%ebp
 mov input+32,%esi
 mov input+36,%edi
 mov input+24,%esp
 int3
 jmp *input+4
.globl capture
capture:
 mov %eax,output
 mov %ecx,output+4
 mov %edx,output+8
 mov %ebx,output+12
 mov %esp,output+16
 mov %ebp,output+20
 mov %esi,output+24
 mov %edi,output+28
 mov $harness_stack_end,%esp
 pushfl
 pop output+32
 stmxcsr output+36
 fnsave output+40
 movdqu %xmm0,output+148
 movdqu %xmm1,output+164
 movdqu %xmm2,output+180
 movdqu %xmm3,output+196
 movdqu %xmm4,output+212
 movdqu %xmm5,output+228
 movdqu %xmm6,output+244
 movdqu %xmm7,output+260
 mov $harness_stack_end,%esp
 cmpl $0x100f000,done_entry
 jne emit
 cmpl $0,phase
 je emit
 testb $255,output
 je emit
 mov phase,%eax
 mov %eax,input+4
 movl $0,phase
 mov $output,%esi
 mov $input+8,%edi
 mov $8,%ecx
 rep movsl
 mov output+32,%eax
 mov %eax,input+40
 mov output+36,%eax
 mov %eax,input+44
 mov $output+40,%esi
 mov $input+48,%edi
 mov $59,%ecx
 rep movsl
 mov phase+4,%edx
 mov $phase+8,%esi
 mov $input+8,%edi
 mov $8,%ecx
phase_gp:
 shr $1,%edx
 jnc phase_gp_next
 mov (%esi),%eax
 mov %eax,(%edi)
phase_gp_next:
 add $4,%esi
 add $4,%edi
 loop phase_gp
 mov phase+40,%ebp
 mov $phase+44,%esi
phase_stack:
 test %ebp,%ebp
 jz setup
 mov (%esi),%edi
 add input+24,%edi
 mov 4(%esi),%eax
 mov %eax,(%edi)
 add $8,%esi
 dec %ebp
 jmp phase_stack
emit:
 mov $output,%ecx
 mov $280,%edx
 call write_exact
 mov input+284,%ebp
 mov $banks,%edi
bank_write:
 test %ebp,%ebp
 jz loop
 mov (%edi),%ecx
 mov 4(%edi),%edx
 call write_exact
 add $8,%edi
 dec %ebp
 jmp bank_write
read_exact:
 test %edx,%edx
 jz io_return
 mov $3,%eax
 xor %ebx,%ebx
 int $0x80
 test %eax,%eax
 jle exit
 add %eax,%ecx
 sub %eax,%edx
 jmp read_exact
write_exact:
 test %edx,%edx
 jz io_return
 mov $4,%eax
 mov $1,%ebx
 int $0x80
 test %eax,%eax
 jle failure
 add %eax,%ecx
 sub %eax,%edx
 jmp write_exact
io_return:
 ret
failure:
 mov $1,%ebx
 jmp terminate
exit:
 xor %ebx,%ebx
terminate:
 mov $1,%eax
 int $0x80
.balign 16
input: .space 288
phase: .space 92
output: .space 276
done_entry: .long 0
banks: .space 65536
harness_stack: .space 65536
harness_stack_end:
'''

def execute(fixture,paired,path,directory):
    assert sha(path)==(NEW_PIN if fixture['image']=='1.5' else PIN)
    latest=fixture['image']=='1.5'
    selected={int(p,16):q for name in ('address_map','instruction_map','entries') for p,q in paired[name].items()}
    image=Xbe(path);flat=bytearray(0x7f0000)
    for section in image.sections:
        raw=image.section_bytes(section);flat[section.va-0x10000:section.va-0x10000+len(raw)]=raw
    cases=fixture['cases']
    directory.mkdir(parents=True,exist_ok=False)
    (directory/'bridge.s').write_text(ASM)
    (directory/'link.ld').write_text('ENTRY(_start)\nSECTIONS { . = 0x10000; .image : { *(.image) } . = 0x900000; .bridge : { *(.bridge) } . = 0x1000000; .guest (NOLOAD) : { . += 0x120000; } }\n')
    subprocess.run(['as','--32',str(directory/'bridge.s'),'-o',str(directory/'bridge.o')],check=True,capture_output=True)
    subprocess.run(['ld','-melf_i386','-T',str(directory/'link.ld'),str(directory/'bridge.o'),'-o',str(directory/'probe')],check=True,capture_output=True)
    symbols=subprocess.check_output(['nm',str(directory/'probe')],text=True)
    symbols={name:int(p,16) for p,kind,name in (line.split() for line in symbols.splitlines())}
    code=_Code(image,'.text')
    protected={byte for p,q in paired['instruction_map'].items() for byte in range(q if latest else int(p,16),(q if latest else int(p,16))+code.at(q if latest else int(p,16)).size)}
    # The only replaced bytes belong to unported owner entries, not paired code.
    traps=[]
    for target in sorted({c['exit_entry'] for c in cases if not c['complete']}):
        assert not any(target<=p<target+15 for p in protected)
        stub=b'\xc7\x05'+struct.pack('<II',symbols['done_entry'],target)
        stub+=b'\xe9'+struct.pack('<i',symbols['capture']-(target+len(stub)+5))
        original=bytes(flat[target-0x10000:target-0x10000+len(stub)])
        flat[target-0x10000:target-0x10000+len(stub)]=stub
        traps.append(dict(target=target,original_hex=original.hex(),capture_only_stub_hex=stub.hex()))
    (directory/'image.bin').write_bytes(flat)
    for p,q in paired['instruction_map'].items():
        actual=q if latest else int(p,16);i=code.at(actual)
        assert flat[actual-0x10000:actual-0x10000+i.size]==i.bytes
    (directory/'image.s').write_text('.section .image,"awx"\n.incbin "'+str(directory/'image.bin')+'"\n')
    # STOP is harness-owned call return storage, not original executable code.
    (directory/'stop.s').write_text('.section .stop,"awx"\njmp capture\n')
    with (directory/'link.ld').open('a') as f:f.write('SECTIONS { .stop 0x100f000 : { *(.stop) } }\n')
    # Reserve around STOP instead of overlapping a NOBITS output section.
    linker=(directory/'link.ld').read_text().replace('.guest (NOLOAD) : { . += 0x120000; }','.guest (NOLOAD) : { . += 0xf000; } . = 0x1010000; .guest_tail (NOLOAD) : { . += 0x110000; }')
    linker+='SECTIONS { .authored 0x80000000 (NOLOAD) : { . += 0x4000000; } }\n'
    (directory/'link.ld').write_text(linker)
    for name in ('image','stop'):
        subprocess.run(['as','--32',str(directory/(name+'.s')),'-o',str(directory/(name+'.o'))],check=True,capture_output=True)
    subprocess.run(['ld','-melf_i386','-T',str(directory/'link.ld'),*[str(directory/(name+'.o')) for name in ('bridge','image','stop')],'-o',str(directory/'original-hardware')],check=True,capture_output=True)
    data=bytearray()
    for c in cases:
        a=c['entry_abi'];env=bytearray(108)
        for offset,value in ((0,a['x87_control_word']),(4,a['x87_status']),(8,a['x87_tag'])):struct.pack_into('<H',env,offset,value)
        for n,value in enumerate(a['x87_values']):env[28+n*10:38+n*10]=bytes.fromhex(value)
        entry=selected[c['entry']] if latest else c['entry']
        data+=struct.pack('<10I',c['id'],entry,*a['gp'])+struct.pack('<II',a['eflags'],a['mxcsr'])+env
        data+=b''.join(int(x,16).to_bytes(16,'little') for x in a['xmm'])+struct.pack('<I',len(c['initial_banks']))
        phase=bytearray(92);call=c.get('subsequent_call')
        if call:
            struct.pack_into('<I',phase,0,selected[call['entry']] if latest else call['entry'])
            names=('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI');mask=0
            for name,value in call['registers'].items():
                n=names.index(name);mask|=1<<n;struct.pack_into('<I',phase,8+n*4,value)
            struct.pack_into('<I',phase,4,mask)
            struct.pack_into('<I',phase,40,len(call['stack']))
            for n,(p,value) in enumerate(sorted(call['stack'].items())):struct.pack_into('<II',phase,44+n*8,int(p),value)
        data+=phase
        assert len(c['initial_banks'])<8192
        for bank in c['initial_banks']:
            raw=bytes.fromhex(bank['hex']);data+=struct.pack('<II',bank['address'],len(raw))+raw
    (directory/'input.bin').write_bytes(data)
    gp={'eax':0,'ecx':1,'edx':2,'ebx':3,'esp':4,'ebp':5,'esi':6,'edi':7}
    instructions=[]
    for p,q in paired['instruction_map'].items():
        actual=q if latest else int(p,16);i=code.at(actual);writes=[]
        if i.mnemonic in ('push','call'):writes.append(dict(base=4,index=-1,scale=1,displacement=0xfffffffc,width=4))
        else:
            for index,operand in enumerate(i.operands):
                explicit_store=i.mnemonic in ('fst','fstp','fistp','fnstcw','fnstsw','stmxcsr') or (index==0 and i.mnemonic in ('mov','movss','movaps','movhps','movsd','rep movsd','rep movsb','rep stosd','rep stosb'))
                if operand.type!=3 or not (operand.access&CS_AC_WRITE or explicit_store):continue
                assert not operand.mem.segment or i.reg_name(operand.mem.segment)=='es',i.op_str
                writes.append(dict(base=gp[i.reg_name(operand.mem.base)] if operand.mem.base else -1,
                    index=gp[i.reg_name(operand.mem.index)] if operand.mem.index else -1,
                    scale=operand.mem.scale,displacement=operand.mem.disp&0xffffffff,width=operand.size))
        instructions.append(dict(actual=actual,canonical=int(p,16),repeated=i.mnemonic.startswith('rep '),writes=writes))
    setups={str(c['id']):[dict(address=c['entry_abi']['gp'][4]+int(p),written=struct.pack('<I',value).hex()) for p,value in sorted(c['subsequent_call']['stack'].items())] for c in cases if c.get('subsequent_call')}
    (directory/'metadata.json').write_text(json.dumps(dict(instructions=instructions,input_address=symbols['input'],setups=setups)))
    tracer=ROOT/'tools/vehicle_motion_math_hardware_trace_20261011.cpp'
    subprocess.run(['c++','-std=c++20','-O2',str(tracer),'-o',str(directory/'tracer')],check=True,capture_output=True)
    subprocess.run([str(directory/'tracer'),str(directory/'original-hardware'),str(directory/'input.bin'),str(directory/'output.bin'),str(directory/'metadata.json'),str(directory/'trace.json')],check=True,timeout=600)
    output=(directory/'output.bin').read_bytes();trace=json.loads((directory/'trace.json').read_text());traces={c['id']:c for c in trace['cases']}
    offset=0;results=[]
    for c in cases:
        assert offset+280<=len(output),(c['id'],offset,len(output))
        record=output[offset:offset+280];offset+=280
        e=copy.deepcopy(c['exit']);e['gp']=list(struct.unpack_from('<8I',record));e['eflags'],e['mxcsr']=struct.unpack_from('<II',record,32)
        env=record[40:148];e['x87_control_word']=struct.unpack_from('<H',env)[0];e['x87_status']=struct.unpack_from('<H',env,4)[0];e['x87_tag']=struct.unpack_from('<H',env,8)[0]
        e['x87_values']=[env[28+n*10:38+n*10].hex() for n in range(len(c['exit']['x87_values']))]
        e['xmm']=[f'{int.from_bytes(record[148+n*16:164+n*16],"little"):032x}' for n in range(8)]
        stop=struct.unpack_from('<I',record,276)[0];assert stop==c['exit_entry'],(c['id'],stop,c['exit_entry'])
        banks=[]
        for bank in c['initial_banks']:
            size=len(bytes.fromhex(bank['hex']));banks.append(dict(address=bank['address'],hex=output[offset:offset+size].hex()));offset+=size
        results.append(dict(id=c['id'],exit=e,final_banks=banks,exit_entry=stop,instructions=traces[c['id']]['instructions'],writes=traces[c['id']]['writes']))
    assert offset==len(output),(offset,len(output))
    return dict(schema=1,image=fixture['image'],tool_sha256=sha(__file__),original_sha256=sha(path),
        cases=results,unported_owner_capture_stubs=traps,paired_instruction_bytes_changed=False,
        original_code_used_in_production=False,host_platform='Linux x86_64 with kernel i386 compatibility',
        trace_observer_sha256=sha(tracer),single_stepped_original_instructions=trace['single_stepped_original_instructions'],
        cpu_model=next(line.split(':',1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines() if line.startswith('model name')),
        original_hardware_binary_sha256=sha(directory/'original-hardware'))

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--correspondence',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=False);paired=json.loads(a.correspondence.read_text())
    report={}
    for label,path in (('u5',NEW_DEFAULT),):
        source=a.original/(label+'.json');fixture=json.loads(source.read_text());result=execute(fixture,paired,path,a.output/(label+'-harness'))
        result.update(input_fixture_sha256=sha(source),correspondence_sha256=sha(a.correspondence))
        target=a.output/(label+'.json');target.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
        hardware={c['id']:c for c in result['cases']}
        combined=copy.deepcopy(fixture)
        for case in combined['cases']:
            row=hardware[case['id']]
            for key in ('exit','final_banks','instructions','writes'):case[key]=row[key]
            case['writes']=[dict(event,kind='W') for event in case['writes']]
        combined.update(oracle_backend='original_i386_hardware_single_step',hardware_fixture_sha256=sha(target),hardware_tool_sha256=sha(__file__),
            unported_owner_capture_stubs=result['unported_owner_capture_stubs'],paired_instruction_bytes_changed=False,
            trace_observer_sha256=result['trace_observer_sha256'],single_stepped_original_instructions=result['single_stepped_original_instructions'])
        native=a.output/(label+'-native.json');native.write_text(json.dumps(combined,sort_keys=True,separators=(',',':'))+'\n')
        report[label]=dict(cases=len(result['cases']),sha256=sha(target))
    (a.output/'report.json').write_text(json.dumps(dict(tool_sha256=sha(__file__),fixtures=report),indent=2,sort_keys=True)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
