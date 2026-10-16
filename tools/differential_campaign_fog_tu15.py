#!/usr/bin/env python3
"""Compare camera fog samples against complete unchanged English TU1.5 code.

The original image, static fog initializer, map/BSP/tag backing, declared eye,
forward vector and cluster selector are inputs. No code, callee or fog result is
replaced. This is CPU math/metadata evidence, not Xbox framebuffer proof.
"""
import argparse,hashlib,json,math,resource,struct,subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_MXCSR,UC_X86_REG_FPCW
from audit_rendering import Cache
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):
    with p.open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map',type=Path,required=True);parser.add_argument('--native',type=Path,required=True)
    parser.add_argument('--bsp',type=int,default=0)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--limit',type=int)
    parser.add_argument('--portal-samples',action='store_true',help='Also sample authored portal centers and declared normal offsets from each adjacent cluster')
    a=parser.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);cache=Cache(a.map)
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    state,eye,direction,stack,stop,tagtable=0x2100000,0x2101000,0x2101100,0x21fff00,0x3000000,0x2300000
    g.mem_map(state,0x100000);g.mem_map(tagtable,0x100000);g.mem_map(stop,4096)
    base=cache.magic&~4095;size=(cache.magic+len(cache.data)-base+4095)&~4095
    g.mem_map(base,size);g.mem_write(cache.magic,bytes(cache.data))
    _,scnr=cache.tag(cache.scenario,'scnr');bsp=cache.block(scnr+0x210,68)[a.bsp]
    offset,extent,address=struct.unpack_from('<3I',cache.data,bsp)
    g.mem_write(address,bytes(cache.data[offset:offset+extent]))
    for p,value in [(0x4ecc20,cache.magic+scnr),(0x4ecc18,address+16),(0x4f0414,tagtable)]:g.mem_write(p,struct.pack('<I',value))
    for tag in cache.tags.values():g.mem_write(tagtable+(tag['datum']&65535)*16+8,struct.pack('<I',tag['address']))
    failures=[]
    def invalid(guest,kind,at,size,value,user):
        failures.append({'instruction':guest.reg_read(UC_X86_REG_EIP),'address':at,'bytes':size});return False
    g.hook_add(UC_HOOK_MEM_INVALID,invalid)
    def invoke(entry,args=(),registers=()):
        g.mem_write(stack,struct.pack('<'+'I'*(len(args)+1),stop,*args));g.reg_write(UC_X86_REG_ESP,stack)
        g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
        for reg,value in registers:g.reg_write(reg,value)
        g.emu_start(entry,stop,count=2000000)
        assert g.reg_read(UC_X86_REG_EIP)==stop,hex(g.reg_read(UC_X86_REG_EIP))
    invoke(0x3811b0);assert bytes(g.mem_read(0x556e10,4))==struct.pack('<f',1.)
    clusters=cache.block(offset+16+156,176,address-offset);samples=[];cases=[]
    for index,cluster in enumerate(clusters):
        bounds=struct.unpack_from('<6f',cache.data,cluster+0x54)
        for z in [0.,-4.,4.]:
            point=[(bounds[i]+bounds[i+1])*.5 for i in [0,2,4]];point[2]+=z
            cases.append((index,point,{'kind':'cluster-centroid','z_offset':z}))
    if a.portal_samples:
        collision=cache.block(offset+16+20,64,address-offset)[0]
        planes=cache.block(collision+8,16,address-offset)
        for ordinal,portal in enumerate(cache.block(offset+16+0x5c,36,address-offset)):
            back,front,plane=struct.unpack_from('<hhI',cache.data,portal)
            center=struct.unpack_from('<3f',cache.data,portal+8)
            normal=struct.unpack_from('<3f',cache.data,planes[plane])
            for index in [back,front]:
                if index<0:continue
                for distance in [0.,-.25,.25,-1.,1.,-5.,5.,-9.9999,9.9999]:
                    cases.append((index,[c+n*distance for c,n in zip(center,normal)],
                        {'kind':'portal-normal-offset','portal':ordinal,'distance':distance}))
    try:
        for index,point,origin in cases:
            point=list(struct.unpack('<3f',struct.pack('<3f',*point)))
            g.mem_write(eye,struct.pack('<3f',*point));g.mem_write(direction,struct.pack('<3f',1.,0.,0.))
            control=state+0x2000;g.mem_write(control,bytes(20))
            invoke(0x14de90,registers=[(UC_X86_REG_ECX,index),(UC_X86_REG_EDX,control),(UC_X86_REG_EBX,control+4),(UC_X86_REG_EDI,control+8)])
            sky_model=g.reg_read(UC_X86_REG_EAX)&255
            invoke(0x1310f0,(eye,direction,sky_model,1),[(UC_X86_REG_EAX,state),(UC_X86_REG_ECX,index)])
            raw=bytes(g.mem_read(state,288));f=lambda at,n:list(struct.unpack_from('<'+'f'*n,raw,at))
            values=f(0x1c,6)+f(0x34,6)+f(0xa0,6)+f(0x70,10)+f(0x108,3)+f(0x118,1)+[raw[0x114]]+f(0xe0,4)+f(0x5c,3)+f(0x68,1)
            assert len(values)==41
            samples.append({'eye':point,'cluster':index,'origin':origin,
                'atmospheric_palette':struct.unpack_from('<b',cache.data,clusters[index]+0x6f)[0],
                'classification':struct.unpack_from('<I',raw)[0],'state_values':values,'raw_hex':raw.hex(),'sky_model_control':sky_model})
            if a.limit and len(samples)>=a.limit:break
    except Exception as exc:
        (a.output/'original-failure.json').write_text(json.dumps({'error':str(exc),'invalid_memory':failures},indent=2)+'\n');raise
    (a.output/'original-samples.json').write_text(json.dumps({'reference':'English TU1.5 only','original_sha256':NEW_PIN,'samples':samples},indent=2)+'\n')
    inputs=a.output/'native-inputs.txt';inputs.write_text(''.join(' '.join([a.map.name,*map(str,s['eye']),str(s['cluster'])])+'\n'for s in samples))
    def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    native=subprocess.run([str(a.native.resolve()),str(a.map.resolve().parent),str(inputs.resolve()),str(a.map.resolve().parent),str(a.bsp)],capture_output=True,text=True,preexec_fn=no_core)
    (a.output/'native-stdout.txt').write_text(native.stdout);(a.output/'native-stderr.txt').write_text(native.stderr)
    differences=[];rows=native.stdout.splitlines()
    if native.returncode==0:
        assert len(rows)==len(samples)
        for sample,line in zip(samples,rows):
            fields=line.split();actual_class=int(fields[3]);values=list(map(float,fields[4:45]))
            assert len(values)==len(sample['state_values'])
            if actual_class!=sample['classification']:differences.append({'cluster':sample['cluster'],'field':'classification','original':sample['classification'],'native':actual_class})
            for field,(wanted,actual)in enumerate(zip(sample['state_values'],values)):
                if struct.pack('<f',wanted)!=struct.pack('<f',actual):differences.append({'cluster':sample['cluster'],'eye':sample['eye'],'field':field,'original':wanted,'native':actual})
    report={'passed':native.returncode==0 and not differences,'reference':'English TU1.5 only','original_sha256':NEW_PIN,'map_sha256':sha(a.map),'native_sha256':sha(a.native),'bsp':a.bsp,'samples':len(samples),'native_exit':native.returncode,'differences':differences,'aggregation_stubs':[],'boundary':__doc__,'comparison':'All 41 published state values compared as exact binary32 including signed zero; exact classification', 'compared_binary32_fields':len(samples)*41, 'declared_portal_samples':a.portal_samples, 'atmospheric_palette_indices':sorted({s['atmospheric_palette'] for s in samples})}
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='differences'}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
