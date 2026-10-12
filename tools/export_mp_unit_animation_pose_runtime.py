#!/usr/bin/env python3
"""Export complete original packed pose calls and independent CPU estimates.

The unchanged original bodies execute again against their captured initial
bank and full authenticated image. Every source store, including stack cells,
is checked against the frozen ordinary-death trace before exporting literal
RCPSS/RSQRTSS/RSQRTPS input-output pairs. The inherited124F30 delivery of actual cache bytes remains an explicit
resource boundary. No pose or arithmetic result is supplied.
This is component evidence, not live Match or Xbox framebuffer evidence.
"""
import argparse,bisect,json,struct
from pathlib import Path
from export_mp_ragdoll_owners import spans,sha
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN,CACHE_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn import x86_const as R
CAPTURE_ENTRY="000bdf80"
CAPTURE_ORACLE="oracle_mp_corpse_current_nodes_1_1.py"
ARGUMENT_COUNTS={0xbea50:3,0xbe860:6,0x10d1e0:3,0x1ca950:4,0x280030:8,0x1d0f80:7,0x1d1a00:0,0xddbc0:2}
ARGUMENT_COUNTS.update({0xe1800:2,0xe4f40:0,0xdf7c0:0,0x142e80:0,0x143f60:0,0x1d1290:1})
ARGUMENT_COUNTS.update({0xc04f0:3,0xe8050:2})
ROOTS={0x280030,0x1ca950,0x1d0f80,0x1d1a00,0xbea50,0xbe860,0x10d1e0}
CONFIGURATION_TOOL=None
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    j=json.loads(a.report.read_text())
    assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN
    assert j['capture_entry']==CAPTURE_ENTRY and j['failure']is None and j['frozen_prior_byte_identical']
    assert not j['instruction_patches'] and not j['new_supplied_guest_fields_or_results']
    assert j['tool_sha256']==sha(Path(__file__).with_name(CAPTURE_ORACLE))
    for q in j['instructions']:assert sha(a.report.parent/q['file'])==q['sha256']
    image_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(image_path)==PIN
    image=Xbe(image_path);code=_Code(image,'.text')
    events=sorted([(q['order'],'R',q)for q in j['rebuild_reads']]+[(q['order'],'W',q)for q in j['rebuild_stores']]);orders=[q[0]for q in events]
    rows=[];audit=[]
    for c in j['rebuild_calls']:
        entry=int(c['entry'],16)
        if entry not in ROOTS:continue
        ev=events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]
        initial={};current={};writes=[]
        for _,kind,q in ev:
            at=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']if kind=='R'else q['before_hex'])
            for i,v in enumerate(raw):
                if at+i not in current:initial[at+i]=current[at+i]=v
                assert current[at+i]==v
            if kind=='W':
                writes.append(q)
                for i,v in enumerate(bytes.fromhex(q['written_hex'])):current[at+i]=v
        gp=[int(c['registers'][k]if k!='esp'else c['entry_esp'],16)for k in ('eax','ecx','edx','ebx','esp','ebp','esi','edi')]
        count=ARGUMENT_COUNTS[entry]
        for i in range(count):
            for k,value in enumerate(struct.pack('<I',int(c['stack16'][i],16))):
                p=gp[4]+4+4*i+k
                if p in initial:assert initial[p]==value,(c['id'],'API input bank',hex(p))
                else:initial[p]=value
        # A captured FS read/write gives its literal thread-bank address.
        # Only absolute FS operands are admitted here; no thread state or
        # exception result is invented. The routing descriptors below are
        # emulator infrastructure, not asserted Xbox selector values.
        fs_bases=set()
        for _,_,q in ev:
            ins=code.at(int(q['instruction'],16))
            for operand in ins.operands:
                if operand.type==3 and operand.mem.segment:
                    segment=ins.reg_name(operand.mem.segment)
                    if segment!='fs':
                        assert segment in {'cs','ds','es','ss'}
                        continue
                    assert not operand.mem.base and not operand.mem.index
                    fs_bases.add((int(q['address'],16)-operand.mem.disp)&0xffffffff)
        assert len(fs_bases)<=1,(c['id'],'original FS routing')
        fs_base=next(iter(fs_bases),0)
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        pages={p&~4095 for p in initial if p>=0x810000}
        for p in sorted(pages):g.mem_map(p,4096)
        for p,raw in spans(initial):g.mem_write(p,raw)
        if fs_bases:
            routing=0x800000
            assert not any(routing<=p<routing+4096 for p in initial)
            def descriptor(base,access):
                limit=0xfffff
                return struct.pack('<II',(limit&0xffff)|((base&0xffff)<<16),
                    ((base>>16)&255)|(access<<8)|(limit&0xf0000)|(0xc<<20)|(base&0xff000000))
            g.mem_write(routing+8,descriptor(0,0x9b)+descriptor(0,0x93)+descriptor(fs_base,0x93))
            g.reg_write(R.UC_X86_REG_GDTR,(0,routing,0xfff,0))
            for register,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),
                (R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):
                g.reg_write(register,value)
            assert g.reg_read(R.UC_X86_REG_FS_BASE)==fs_base
        for name,v in zip(('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI'),gp):g.reg_write(getattr(R,'UC_X86_REG_'+name),v)
        for i in range(8):g.reg_write(getattr(R,'UC_X86_REG_XMM'+str(i)),int(c['xmm'][str(i)],16))
        for reg,v in ((R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,int(c['fpcw'],16)),(R.UC_X86_REG_MXCSR,int(c['mxcsr'],16)),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_FPSW,0)):g.reg_write(reg,v)
        resources=[q for q in j['rebuild_calls']if c['order']<q['order']<c['return_order']and q['entry']=='00124f30'];ri=[0]
        stop=struct.unpack('<I',g.mem_read(gp[4],4))[0];pending=[];actual=[];estimates=[];estimate_pending=[];site=[0]
        def flush():
            for p,size,before,pc in pending:actual.append((pc,p,before,bytes(g.mem_read(p,size))))
            pending.clear()
        def hook(g,p,size,user):
            flush()
            for kind,inputs,dst,count in estimate_pending:
                output=g.reg_read(dst)
                estimates.extend((kind,x,(output>>(i*32))&0xffffffff)for i,x in enumerate(inputs[:count]))
            estimate_pending.clear();site[0]=p
            if p==0x124f30:
                q=resources[ri[0]];ri[0]+=1;sp=g.reg_read(R.UC_X86_REG_ESP)
                assert sp==int(q['entry_esp'],16)and g.reg_read(R.UC_X86_REG_EAX)==int(q['registers']['eax'],16)
                g.reg_write(R.UC_X86_REG_EAX,int(q['returned_eax'],16));g.reg_write(R.UC_X86_REG_ESP,sp+8)
                g.reg_write(R.UC_X86_REG_EIP,struct.unpack('<I',g.mem_read(sp,4))[0]);return
            ins=code.at(p)
            if ins.mnemonic in {'rcpss','rsqrtss','rsqrtps'}:
                assert ins.bytes==image.read(p,len(ins.bytes))
                dst=getattr(R,'UC_X86_REG_'+ins.reg_name(ins.operands[0].reg).upper())
                source=ins.operands[1]
                if source.type==1:bits=g.reg_read(getattr(R,'UC_X86_REG_'+ins.reg_name(source.reg).upper()))
                else:
                    assert source.type==3 and not source.mem.segment
                    address=source.mem.disp
                    if source.mem.base:address+=g.reg_read(getattr(R,'UC_X86_REG_'+ins.reg_name(source.mem.base).upper()))
                    if source.mem.index:address+=g.reg_read(getattr(R,'UC_X86_REG_'+ins.reg_name(source.mem.index).upper()))*source.mem.scale
                    bits=int.from_bytes(g.mem_read(address&0xffffffff,source.size),'little')
                count=4 if ins.mnemonic=='rsqrtps'else 1
                estimate_pending.append(('R'if ins.mnemonic=='rcpss'else'S',[(bits>>(i*32))&0xffffffff for i in range(count)],dst,count))
        def store(g,access,p,size,value,user):pending.append((p,size,bytes(g.mem_read(p,size)),site[0]))
        g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,store)
        def invalid(g,access,p,size,value,user):
            print((c['id'],c['entry'],f"{g.reg_read(R.UC_X86_REG_EIP):08x}",f"{p:08x}",size),flush=True);return False
        g.hook_add(UC_HOOK_MEM_INVALID,invalid)
        g.emu_start(entry,stop,count=2000000);flush()
        assert g.reg_read(R.UC_X86_REG_EIP)==stop and not estimate_pending,(c['id'],'original return')
        wanted=[(int(q['instruction'],16),int(q['address'],16),bytes.fromhex(q['before_hex']),bytes.fromhex(q['written_hex']))for q in writes]
        assert len(actual)==len(wanted),(c['id'],len(actual),len(wanted))
        for i,(x,y)in enumerate(zip(actual,wanted)):assert x==y,(c['id'],i,x,y)
        assert ri[0]==len(resources)
        assert g.reg_read(R.UC_X86_REG_EAX)==int(c['returned_eax'],16)
        assert g.reg_read(R.UC_X86_REG_ESP)==gp[4]+c['return_esp_delta']
        rows.append(' '.join(['C',str(c['id']),c['entry'],c['fpcw'],c['mxcsr'],c['returned_eax'],str(c['return_esp_delta']),*[f'{v:08x}'for v in gp]]))
        rows.append('Q '+' '.join(c['stack16']))
        if fs_bases:rows.append(f'F {fs_base:08x}')
        for i in range(8):
            x=int(c['xmm'][str(i)],16);rows.append('X '+str(i)+' '+' '.join(f'{(x>>(32*n))&0xffffffff:08x}'for n in range(4)))
        for q in resources:rows.append('P '+q['entry_esp']+' '+q['registers']['eax']+' '+q['stack16'][0]+' '+q['returned_eax'])
        for kind,x,y in estimates:rows.append(f'E {kind} {x:08x} {y:08x}')
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        output={}
        for q in writes:
            rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
            for i,v in enumerate(bytes.fromhex(q['written_hex'])):output[int(q['address'],16)+i]=v
        for p,raw in spans(output):rows.append(f'A {p:08x} {raw.hex()}')
        audit.append(dict(id=c['id'],entry=c['entry'],stores=len(writes),bytes=len(output),cpu_estimates=len(estimates),unchanged_original_stores_identical=True,resource_byte_delivery_boundaries=len(resources),original_fs_base=f'{fs_base:08x}'if fs_bases else None,captured_pose_results=[]))
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(rows)+'\n')
    a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),configuration_tool_sha256=sha(CONFIGURATION_TOOL)if CONFIGURATION_TOOL else None,capture_entry=CAPTURE_ENTRY,capture_oracle=CAPTURE_ORACLE,roots=[f'{entry:08x}'for entry in sorted(ROOTS)],fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_expected_data_used=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),estimates=sum(c['cpu_estimates']for c in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
