#!/usr/bin/env python3
"""Separate explicit ABI-input branch controls for both combat routines.

Only named raw metadata control operands are changed in disposable memory.
These are deliberately altered kernel-input fixtures, not authored game state
or source initialization. Original assets and all preceding freezes stay intact.
No unknown callback/service/return or successful gameplay state is supplied.
"""
import argparse,bisect,json,struct
from pathlib import Path
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import *
from oracle_mp_combat_postprocess_updates import ROOT,IMAGES,TAGS,STACK,STOP,sha,digest,h,CACHE_PIN,schema_fields
from oracle_smg_firing_1_1 import Cache,Xbe,_Code,_trace

def definitions(c):
    matg=next(t for t in c.tags.values()if t['group']=='matg');m=c.tags[c.u(matg['offset']+0x16c)];runtime=c.u(m['offset']+0xc)-c.magic
    weapons=c.u(runtime+0x6c)-c.magic;vehicles=c.u(runtime+0x74)-c.magic;grenades=c.u(matg['offset']+0x104)-c.magic
    def weapon(slot):return c.tags[c.u(weapons+slot*8+4)]['offset']
    def barrel(slot):return c.u(weapon(slot)+0x2d4)-c.magic
    def projectile(slot):return c.tags[c.u(barrel(slot)+0x90)]['offset']
    variants=[('authored_lockout',[])]
    def add(name,offset,value,size=4):variants.append((name,[{'file_offset':offset,'size':size,'value':value}]))
    for val in(0,2,3,4,0xffff):add('scenario_type_'+str(val),c.scenario+0x10,val,2)
    for slot in(0,1,3,5,7,12):add(f'weapon_slot_{slot}_NONE',weapons+slot*8+4,0xffffffff)
    add('vehicle_slot_7_NONE',vehicles+7*8+4,0xffffffff)
    for count in(0,0xffffffff,0x80000000,0x10000,2):add('BR_barrel_signed32_'+h(count),weapon(3)+0x2d0,count)
    for slot in(1,5,12):add(f'weapon_slot_{slot}_melee_damage_NONE',weapon(slot)+0x1c0,0xffffffff)
    for slot in(0,1,12):add(f'weapon_slot_{slot}_barrel_projectile_NONE',barrel(slot)+0x90,0xffffffff)
    for slot in(0,1):add(f'weapon_slot_{slot}_projectile_impact_damage_NONE',projectile(slot)+0x14c,0xffffffff)
    add('weapon_slot_12_projectile_detonation_damage_NONE',projectile(12)+0x104,0xffffffff)
    for i in(0,1):
        add(f'grenade_{i}_projectile_NONE',grenades+i*0x2c+0x28,0xffffffff)
        p=c.tags[c.u(grenades+i*0x2c+0x28)]['offset'];add(f'grenade_{i}_detonation_damage_NONE',p+0x104,0xffffffff)
    for count in(0,1,3,0xffffffff):add('grenade_count_'+h(count),matg['offset']+0x100,count)
    for slot in(0,1,7,12):add(f'weapon_slot_{slot}_barrel_count_zero_pointer_retained',weapon(slot)+0x2d0,0)
    add('runtime_count_zero_pointer_retained',m['offset']+8,0)
    add('weapons_count_zero_pointer_retained',runtime+0x68,0)
    add('vehicles_count_zero_pointer_retained',runtime+0x70,0)
    add('matg_multiplayer_reference_NONE',matg['offset']+0x16c,0xffffffff)
    return variants,matg

def execute(c,config,name,edits,out):
    label,path,pin,entry,caller,slots,_=config;assert sha(path)==pin
    x=Xbe(path);code=_Code(x,'.text');g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
    raw=bytearray(c.data);inputs=[]
    for z in edits:
        off,n=z['file_offset'],z['size'];before=bytes(raw[off:off+n]);after=z['value'].to_bytes(n,'little');raw[off:off+n]=after
        inputs.append({'file_offset':h(off),'original_va':h(off+c.magic),'size':n,'authored_before_hex':before.hex(),'explicit_control_after_hex':after.hex()})
    page=c.magic&~0xfff;g.mem_map(page,((len(raw)+(c.magic-page)+0xfff)//0x1000)*0x1000);g.mem_write(c.magic,bytes(raw))
    g.mem_map(TAGS,0x100000);g.mem_map(0x2000000,0x200000);g.mem_map(STOP,0x1000)
    index=c.u(16);table=c.u(index+8)-(c.u(index)-(index+32));count=c.u(index+24);g.mem_write(TAGS,bytes(c.data[table:table+count*16]))
    matg=next(t for t in c.tags.values()if t['group']=='matg')
    for p,v in zip(slots,(TAGS,matg['offset']+c.magic,c.scenario+c.magic)):g.mem_write(p,struct.pack('<I',v))
    g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
    tags=sorted((t for t in c.tags.values()if t['offset']is not None),key=lambda t:t['offset']);offsets=[t['offset']for t in tags]
    writes=[];executed=[];invalid=[]
    def code_hook(_g,p,_s,_u):executed.append(p)
    def store_hook(_g,_access,p,n,v,_u):
        if not c.magic<=p<c.magic+len(raw):return
        off=p-c.magic;t=tags[bisect.bisect_right(offsets,off)-1];assert t['offset']<=off and off+n<=t['offset']+t['size']
        before=bytes(g.mem_read(p,n));after=(v&((1<<(n*8))-1)).to_bytes(n,'little')
        writes.append({'instruction':h(g.reg_read(UC_X86_REG_EIP)),'file_offset':h(off),'address':h(p),'size':n,'tag_datum':t['datum'],'tag_name':t['name'],'tag_group':t['group'],'tag_relative_offset':h(off-t['offset']),'before_hex':before.hex(),'after_hex':after.hex()})
    def invalid_hook(_g,access,p,n,v,_u):
        invalid.append({'access_kind':access,'instruction':h(g.reg_read(UC_X86_REG_EIP)),'address':h(p),'size':n,'registers':{name:h(g.reg_read(r))for name,r in(('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI),('esp',UC_X86_REG_ESP))}});return False
    g.hook_add(UC_HOOK_CODE,code_hook);g.hook_add(UC_HOOK_MEM_WRITE,store_hook);g.hook_add(UC_HOOK_MEM_INVALID,invalid_hook)
    error=None
    try:g.emu_start(entry,STOP,count=5000)
    except UcError as e:error=str(e)
    returned=g.reg_read(UC_X86_REG_EIP)==STOP
    assert returned or len(invalid)==1,(name,error)
    if returned:assert g.reg_read(UC_X86_REG_ESP)==STACK+4 and not invalid
    for p in executed:
        i=code.at(p);assert i.bytes==x.read(p,len(i.bytes))==bytes(g.mem_read(p,len(i.bytes)))
    output=bytes(g.mem_read(c.magic,len(raw)));patched=bytearray(raw)
    for w in writes:
        off=int(w['file_offset'],16);n=w['size'];assert bytes(patched[off:off+n]).hex()==w['before_hex'];patched[off:off+n]=bytes.fromhex(w['after_hex'])
    assert patched==output
    fn=_trace(code,entry,{entry});out.mkdir(parents=True,exist_ok=True)
    (out/(h(entry)+'.asm')).write_text(''.join(f'{p:08x} {code.at(p).bytes.hex()} {code.at(p).mnemonic} {code.at(p).op_str}\n'for p in sorted(fn.code)))
    return{'case':name,'image_label':label,'entry':h(entry),'xbe_sha256':pin,'explicit_control_operand_changes':inputs,
           'fixture_input_cache_sha256':digest(raw),'returned':returned,'stack_cleanup':g.reg_read(UC_X86_REG_ESP)-STACK,
           'final_eip':h(g.reg_read(UC_X86_REG_EIP)),'unprovided_invalid_access_stop':invalid,'unicorn_error':error,
           'executed_instruction_addresses':[h(p)for p in executed],'ordered_metadata_stores':writes,'metadata_store_count':len(writes),
           'actual_changed_store_count':sum(w['before_hex']!=w['after_hex']for w in writes),'complete_cache_output_sha256':digest(output),
           'original_callbacks_services_gameplay_results_supplied':[]}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();c=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(c.path)==CACHE_PIN
    try:
        variants,_=definitions(c);cases=[]
        for name,changes in variants:
            pair=[execute(c,q,name,changes,a.output/q[0])for q in IMAGES]
            keys=('returned','stack_cleanup','metadata_store_count','actual_changed_store_count','complete_cache_output_sha256','fixture_input_cache_sha256')
            assert all(pair[0][k]==pair[1][k]for k in keys)
            ws=[[{k:v for k,v in z.items()if k!='instruction'}for z in p['ordered_metadata_stores']]for p in pair];assert ws[0]==ws[1]
            cases.extend(pair)
        r={'tool_sha256':sha(__file__),'original_cache_sha256':CACHE_PIN,'original_fixture_dependency_sha256':sha(ROOT/'tools/oracle_mp_combat_postprocess_updates.py'),
           'scope':'Deliberately modified disposable raw metadata ABI-input controls. Original assets untouched; control edits are literal supplied test inputs, not authored values, game initialization, or an inferred engine producer. Complete untouched original postprocessors run without service/callback/result stubs. Invalid pointer/count cases retain exact original access stops.',
           'all_paired_original_write_traces_and_cache_outputs_equal':True,'variants':len(variants),'cases':cases,'native_gameplay_update_applied':False}
        a.output.mkdir(parents=True,exist_ok=True);(a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
        print(json.dumps({'variants':len(variants),'original_runs':len(cases),'returned':sum(q['returned']for q in cases),'actual_access_stops':sum(not q['returned']for q in cases),'report_sha256':sha(a.output/'report.json')}))
    finally:c.data.close();c.stream.close()
if __name__=='__main__':main()
