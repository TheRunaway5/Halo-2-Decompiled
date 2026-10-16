#!/usr/bin/env python3
"""Compare one actual opening guard cloth birth with untouched TU1.5 code.

The input ABI and every source memory region come from the live119060 entry.
Original allocation, marker query, geometry and release callees execute without
supplied results. Native final state is used only after original execution.
"""
import argparse,bisect,hashlib,json,mmap,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_UNMAPPED
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

GP=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
    R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path);a=p.parse_args()
    assert not a.output.exists();a.output.mkdir(parents=True)
    assert hashlib.file_digest(NEW_DEFAULT.open('rb'),'sha256').hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);d=json.loads((a.capture/'capture.json').read_text())
    assert d['entry']==0x119060 and d['reference']=='English TU1.5 only'
    regions=sorted(d['regions'],key=lambda x:x['address']);starts=[x['address'] for x in regions]
    g=Uc(UC_ARCH_X86,UC_MODE_32);files=[];loaded=set()
    for region in regions:
        stream=(a.capture/region['before_file']).open('rb');raw=mmap.mmap(stream.fileno(),0,access=mmap.ACCESS_READ)
        files.append((stream,raw));assert len(raw)==region['size']
    # Captured cache backing remains on disk. Each CPU page fault installs
    # exactly the overlapping captured bytes, including all original code.
    # No game read or write may access uncaptured bytes (checked below).
    def load_page(page):
        if page in loaded:return
        data=bytearray(4096)
        for region,(_,raw) in zip(regions,files):
            lo=max(page,region['address']);hi=min(page+4096,region['address']+region['size'])
            if lo<hi:data[lo-page:hi-page]=raw[lo-region['address']:hi-region['address']]
        g.mem_map(page,4096);g.mem_write(page,bytes(data));loaded.add(page)
    def on_unmapped(_g,_access,address,size,_value,_):
        for page in range(address&~4095,((address+size-1)&~4095)+1,4096):load_page(page)
        return True
    g.hook_add(UC_HOOK_MEM_UNMAPPED,on_unmapped)
    load_page(d['entry']&~4095)
    assert bytes(g.mem_read(d['entry'],16))==image.read(d['entry'],16)
    # A CPU-only descriptor page supplies the observed FS base. It is outside
    # all game input regions and must never be accessed by game instructions.
    descriptor=0x7ff00000
    assert all(not(r['address']<=descriptor<r['address']+r['size']) for r in regions);g.mem_map(descriptor,4096)
    def desc(base,access):
        limit=0xfffff
        return struct.pack('<HHBBBB',limit&65535,base&65535,(base>>16)&255,access,
                           ((limit>>16)&15)|0xc0,(base>>24)&255)
    g.mem_write(descriptor,b'\0'*8+desc(0,0x9b)+desc(0,0x93)+desc(d['before']['fs_base'],0x93))
    g.reg_write(R.UC_X86_REG_GDTR,(0,descriptor,31,0))
    for reg,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),
                      (R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24),(R.UC_X86_REG_GS,16)]:g.reg_write(reg,value)
    initial=d['before']
    for reg,value in zip(GP,initial['gp']):g.reg_write(reg,value)
    for n,value in enumerate(initial['xmm']):g.reg_write(getattr(R,f'UC_X86_REG_XMM{n}'),int.from_bytes(struct.pack('<4I',*value),'little'))
    g.reg_write(R.UC_X86_REG_EFLAGS,initial['eflags']);g.reg_write(R.UC_X86_REG_FPCW,initial['x87_control_word'])
    g.reg_write(R.UC_X86_REG_MXCSR,initial['mxcsr']);g.reg_write(R.UC_X86_REG_FPSW,initial['x87_status'])
    top=(initial['x87_status']>>11)&7;tag=0xffff
    for n,value in enumerate(initial['x87_values']):
        raw=bytes.fromhex(value);index=(top+n)&7;tag&=~(3<<(2*index))
        g.reg_write(getattr(R,f'UC_X86_REG_FP{index}'),(int.from_bytes(raw[:8],'little'),int.from_bytes(raw[8:],'little')))
    g.reg_write(R.UC_X86_REG_FPTAG,tag)
    instructions=0;writes=[];original_pages=set();returned=False
    text=next(s for s in image.sections if s.name=='.text')
    def on_code(_g,address,size,_):
        nonlocal instructions,returned
        if address==d['return']:returned=True;g.emu_stop();return
        assert text.va<=address<text.va+text.vsize,f'Original execution escaped text at {address:08x}'
        assert bytes(g.mem_read(address,size))==image.read(address,size),f'Modified instruction at {address:08x}'
        instructions+=1
    def accessible(address,size):
        index=bisect.bisect_right(starts,address)-1
        return index>=0 and address+size<=regions[index]['address']+regions[index]['size']
    def on_read(_g,_access,address,size,_value,_):
        assert accessible(address,size),f'Original read outside captured source backing {address:08x}+{size}'
    def on_write(_g,_access,address,size,value,_):
        assert accessible(address,size),f'Original write outside captured source backing {address:08x}+{size}'
        writes.append(dict(instruction=g.reg_read(R.UC_X86_REG_EIP),address=address,size=size))
        original_pages.update(range(address&~4095,((address+size-1)&~4095)+1,4096))
    g.hook_add(UC_HOOK_CODE,on_code);g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_MEM_WRITE,on_write)
    error=None
    try:g.emu_start(d['entry'],0,count=5_000_000)
    except Exception as exc:error=str(exc)
    differences=[];native_pages=set()
    if returned:
        actual=dict(gp=[g.reg_read(reg) for reg in GP],xmm=[list(struct.unpack('<4I',g.reg_read(getattr(R,f'UC_X86_REG_XMM{n}')).to_bytes(16,'little'))) for n in range(8)],
                    eflags=g.reg_read(R.UC_X86_REG_EFLAGS),mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),x87_control_word=g.reg_read(R.UC_X86_REG_FPCW),x87_status=g.reg_read(R.UC_X86_REG_FPSW))
        fp_top=(actual['x87_status']>>11)&7;fp_tag=g.reg_read(R.UC_X86_REG_FPTAG)
        actual['x87_values']=[]
        for n in range(8):
            index=(fp_top+n)&7
            if (fp_tag>>(2*index))&3==3:break
            mantissa,exponent=g.reg_read(getattr(R,f'UC_X86_REG_FP{index}'))
            actual['x87_values'].append((mantissa.to_bytes(8,'little')+exponent.to_bytes(2,'little')).hex())
        for key in actual:
            if actual[key]!=d['after'][key]:differences.append(dict(field=key,original=actual[key],native=d['after'][key]))
        # Scan every native-changed byte and every original-written page.
        # Output banks never participate in original execution.
        for region,(_,before) in zip(regions,files):
            final_stream=(a.capture/region['after_file']).open('rb') if 'after_file' in region else None
            final=mmap.mmap(final_stream.fileno(),0,access=mmap.ACCESS_READ) if final_stream else before
            for offset in range(0,region['size'],4096):
                size=min(4096,region['size']-offset);address=region['address']+offset
                changed=before[offset:offset+size]!=final[offset:offset+size]
                if changed:native_pages.add(address&~4095)
                touched=any(page in original_pages for page in range(address&~4095,((address+size-1)&~4095)+1,4096))
                if changed or touched:
                    for page in range(address&~4095,((address+size-1)&~4095)+1,4096):load_page(page)
                if (changed or touched) and bytes(g.mem_read(address,size))!=final[offset:offset+size]:
                    differences.append(dict(address=address,size=size,field='memory'))
            if final_stream:final.close();final_stream.close()
    report=dict(reference='English TU1.5 only',capture=str(a.capture),returned=returned,error=error,
                original_instructions=instructions,original_stores=len(writes),native_changed_pages=len(native_pages),
                original_written_pages=len(original_pages),loaded_cpu_pages=len(loaded),differences=differences,passed=returned and not differences and error is None,
                boundary='One actual opening guard cloth constructor: untouched TU1.5 instructions, captured ABI and final touched/native-changed bytes. No substituted service result; not cloth animation, pixels or complete opening acceptance.')
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');(a.output/'original-stores.json').write_text(json.dumps(writes,indent=2)+'\n')
    for stream,raw in files:raw.close();stream.close()
    print(json.dumps(report));return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
