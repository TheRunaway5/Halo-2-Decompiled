#!/usr/bin/env python3
"""Execute original netgame-tag fixups through a real access adapter.

The sole external service applies the pinned compatibility conversion and
actual Unicorn memory permissions. No original MM/PTE/hardware parity is
claimed. Raw cache/scenario binding is explicit; map activation is not supplied.
"""
import argparse,ctypes,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_combat_postprocess_updates import ROOT,OLD,NEW_DEFAULT,NEW_PIN,PIN,CACHE_PIN,STACK,STOP,sha,digest,h
from oracle_smg_firing_1_1 import Cache,Xbe,_Code,_trace

COMPAT=ROOT/'analysis/multiplayer/integration/spawn-source/local-map-protection'
COMPAT_SOURCE_PIN='7658c40ade81ad9513652926c561fd2a9f0b4526cc87764742f266501afd48a4'
COMPAT_LIBRARY_PIN='61b354c312ced6fa882eeb1371715eafc82328f5e780017c1688e29509741e23'
CACHE_PATHS=(ROOT/'assets/disc/maps/beavercreek.map',ROOT/'assets/disc/maps/lockout.map',
 NEW_DEFAULT.parents[1]/'c/4d53006400000003/dune.map')
IMAGES=(('disc_1_1',OLD,PIN,0x11df00,0x4e7920,0x2d7a1a),('owned_1_5',NEW_DEFAULT,NEW_PIN,0x11f060,0x4ecc20,0x2db4ca))
REGS=(UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)

def adapter():
    assert sha(COMPAT/'compiled-compatibility-access.cpp')==COMPAT_SOURCE_PIN
    assert sha(COMPAT/'compiled-compatibility-access.so')==COMPAT_LIBRARY_PIN
    lib=ctypes.CDLL(str(COMPAT/'compiled-compatibility-access.so'))
    cb=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_uint32,ctypes.c_size_t,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32))
    lib.contract.argtypes=[ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)];lib.contract.restype=ctypes.c_int
    lib.apply.argtypes=[ctypes.c_uint32,ctypes.c_size_t,ctypes.c_uint32,cb];lib.apply.restype=ctypes.c_int
    return lib,cb

def execute(path,config,platform,out):
    label,xpath,xpin,entry,scenario_slot,wrapper=config
    assert sha(xpath)==xpin;x=Xbe(xpath);code=_Code(x,'.text');c=Cache(path)
    try:
        lib,cb_type=platform;g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
        for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
        page=c.magic&~0xfff;g.mem_map(page,((len(c.data)+(c.magic-page)+0xfff)//0x1000)*0x1000);g.mem_write(c.magic,bytes(c.data))
        g.mem_map(0x2000000,0x200000);g.mem_map(STOP,0x1000)
        g.mem_write(scenario_slot,struct.pack('<I',c.scenario+c.magic));g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(UC_X86_REG_ESP,STACK)
        g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        net=c.block(c.scenario+0x118,0x20);start=net[0];size=len(net)*0x20
        level=c.u(c.scenario+0x39c)-c.magic;mp=c.u(level+0x14)-c.magic;mapid=c.i(mp)
        original=bytes(c.data[start:start+size]);stores=[];executed=[];services=[]
        def u(p):return int.from_bytes(g.mem_read(p,4),'little')
        def hook(_g,p,_size,_u):
            if p!=0x800000b6:executed.append(p);return
            sp=g.reg_read(UC_X86_REG_ESP);base,n,flags=[u(sp+4+i*4)for i in range(3)];ret=u(sp)
            assert base==start+c.magic and n==size and flags in(2,4)
            rounded_start=base&~0xfff;rounded_end=(base+n+0xfff)&~0xfff
            before=bytes(g.mem_read(rounded_start,rounded_end-rounded_start));regs=[g.reg_read(r)for r in REGS]
            regions_before=[{'start':h(lo),'end':h(hi),'permissions':perms}for lo,hi,perms in g.mem_regions()if lo<rounded_end and hi>=rounded_start]
            conversion=(ctypes.c_uint32*4)();assert lib.contract(flags,conversion)==1
            applied=[];errors=[]
            def transport(addr,count,windows,old):
                try:
                    assert(addr,count,windows)==(base,n,conversion[3]);permissions={0x40:7,0x20:5}[windows]
                    prior={r['permissions']for r in regions_before};assert len(prior)==1;old[0]={7:0x40,5:0x20}[next(iter(prior))]
                    g.mem_protect(rounded_start,rounded_end-rounded_start,permissions)
                    regions_after=[{'start':h(lo),'end':h(hi),'permissions':perms}for lo,hi,perms in g.mem_regions()if lo<rounded_end and hi>=rounded_start]
                    assert all(r['permissions']==permissions for r in regions_after)
                    applied.append({'host_windows_access':h(windows),'actual_unicorn_permissions':permissions,'previous_access':h(old[0]),'rounded_start':h(rounded_start),'rounded_size':rounded_end-rounded_start,'regions_after':regions_after});return 1
                except Exception as e:errors.append(str(e));return 0
            callback=cb_type(transport);result=lib.apply(base,n,flags,callback)
            assert result==1 and len(applied)==1 and not errors,(result,errors)
            after=bytes(g.mem_read(rounded_start,rounded_end-rounded_start));assert after==before
            services.append({'kernel_target':h(p),'ordinal':182,'original_return_site':h(ret),'arguments':[h(base),h(n),h(flags)],
                             'compatibility_conversion_words':[h(v)for v in conversion],'applied_access':applied[0],'regions_before':regions_before,
                             'rounded_contents_before_sha256':digest(before),'rounded_contents_after_sha256':digest(after),
                             'gpr_values_before':[h(v)for v in regs],'void_no_result_supplied':True,'actual_platform_operation_completed':True})
            g.reg_write(UC_X86_REG_ESP,sp+16);g.reg_write(UC_X86_REG_EIP,ret)
            assert [g.reg_read(r)for r in REGS]==regs;services[-1]['gpr_values_after']=[h(g.reg_read(r))for r in REGS]
        def write(_g,access,p,n,v,_u):
            if not c.magic<=p<c.magic+len(c.data):return
            off=p-c.magic;assert start<=off and off+n<=start+size
            row=(off-start)//0x20;member=(off-start)%0x20;before=bytes(g.mem_read(p,n));after=(v&((1<<(n*8))-1)).to_bytes(n,'little')
            stores.append({'instruction':h(g.reg_read(UC_X86_REG_EIP)),'address':h(p),'file_offset':h(off),'size':n,'netgame_flag_index':row,'member_offset':h(member),
                           'before_hex':before.hex(),'after_hex':after.hex(),'before_bits':before[::-1].hex(),'after_bits':after[::-1].hex(),'before_float':struct.unpack('<f',before)[0],'after_float':struct.unpack('<f',after)[0]})
        g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write);g.emu_start(entry,STOP,count=10000)
        assert g.reg_read(UC_X86_REG_EIP)==STOP and g.reg_read(UC_X86_REG_ESP)==STACK+4
        processed=bytes(g.mem_read(start+c.magic,size));fn=_trace(code,entry,{entry,wrapper});wf=_trace(code,wrapper,{entry,wrapper})
        assert set(executed)<=set(fn.code)|set(wf.code)
        out.mkdir(parents=True,exist_ok=True)
        for p,f in((entry,fn),(wrapper,wf)):(out/(h(p)+'.asm')).write_text(''.join(f'{a:08x} {code.at(a).bytes.hex()} {code.at(a).mnemonic} {code.at(a).op_str}\n'for a in sorted(f.code)))
        for p in executed:
            i=code.at(p);assert i.bytes==x.read(p,len(i.bytes))==bytes(g.mem_read(p,len(i.bytes)))
        (out/'netgame-flags-raw.bin').write_bytes(original);(out/'netgame-flags-processed.bin').write_bytes(processed)
        return{'image_label':label,'xbe_sha256':xpin,'entry':h(entry),'cache_path':str(path),'cache_sha256':sha(path),'cache_magic':h(c.magic),
               'declared_scenario_pointer_binding':{'slot':h(scenario_slot),'value':h(c.scenario+c.magic)},'source_scenario_type':c.h(c.scenario+0x10),
               'source_map_id':mapid,'source_netgame_count':len(net),'source_netgame_file_offset':h(start),'raw_netgame_hex':original.hex(),'processed_netgame_hex':processed.hex(),
               'source_netgame_records':[{'index':n,'file_offset':h(q),'original_va':h(q+c.magic),'raw_hex':bytes(c.data[q:q+0x20]).hex(),'processed_hex':bytes(g.mem_read(q+c.magic,0x20)).hex()}for n,q in enumerate(net)],
               'ordered_netgame_stores':stores,'actual_changed_stores':sum(v['before_hex']!=v['after_hex']for v in stores),'access_services':services,
               'executed_instruction_addresses':[h(p)for p in executed],'complete_cache_output_sha256':digest(bytes(g.mem_read(c.magic,len(c.data)))),
               'stack_cleanup':4,'original_gameplay_or_activation_results_supplied':[]}
    finally:c.data.close();c.stream.close()

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);args=ap.parse_args();platform=adapter()
    rows=[]
    for path in CACHE_PATHS:
        assert path.exists(),path
        for q in IMAGES:rows.append(execute(path,q,platform,args.output/(path.stem+'-'+q[0])))
    assert [r['source_map_id']for r in rows]==[100,100,50,50,1200,1200]
    assert [r['actual_changed_stores']for r in rows]==[2,2,0,0,0,2]
    r={'tool_sha256':sha(__file__),'compatibility_source_sha256':COMPAT_SOURCE_PIN,'compatibility_library_sha256':COMPAT_LIBRARY_PIN,
       'compatibility_owner_scope':'Frozen unchanged Cxbx access conversion+UpdateMemoryPermissions compiled bodies; actual Unicorn access provider on pages touched by original request. Page coverage derived from address/size; no complete Xbox MM/PTE/cache/hardware execution.',
       'primary_source_reference':'https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/tree/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel',
       'scope':'Six complete unchanged original netgame metadata postprocessor executions on authored Beaver Creek, Lockout and Relic. Raw metadata/scenario pointer is an explicit input; preceding tag/group processing and complete map activation are not executed.',
       'cases':rows,'native_gameplay_update_applied':False}
    args.output.mkdir(parents=True,exist_ok=True);(args.output/'report.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({'cases':len(rows),'changed_stores':[v['actual_changed_stores']for v in rows],'platform_requests':[len(v['access_services'])for v in rows],'report_sha256':sha(args.output/'report.json')}))
if __name__=='__main__':main()
