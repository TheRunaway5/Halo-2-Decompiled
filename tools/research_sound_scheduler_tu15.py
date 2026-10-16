#!/usr/bin/env python3
"""Pin original scheduler expressions and official NVIDIA named relations.

Research only: no scheduler register/native owner is accepted by these tools.
The original TU1.5 file and official prebuilt library remain offline references.
"""
import argparse, hashlib, json, re, struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

PRIMARY=Path('analysis/asset-setup/mcc-native-20261015/native-sound-configuration/primary-recovery/nvidia')
ELF_PIN='5ef228b2a9e8bcf5e67a66d970710c198a93f0577e8488243a55062621d74f55'
ENTRIES=[0x38e75b,0x38e8c9,0x388a1d,0x388a8b,0x385b4f]
SYMBOLS=['CNvAudio::ReadIGSCNT()','CNvAudio::ReadXGSCNT()','CNvAudio::ReadSampleClock()',
 'CNvAudio::IncrementSampleClock(unsigned long)','CNvAudio::StartApu()',
 'CNvAudio::ResourceManagerInit(NVAUDIO_CONFIG*)','CHalMCP1::SetPowerState(NV_POWER_STATE)',
 'CHalMCP1::AllocateObject(unsigned int, void*)','CHalMCP1::FreeObject(unsigned int, void*)',
 'CVPMethod::SetOutputLatency(unsigned int)','CVPMethod::SetOutputFifoOffset(unsigned int)',
 'CEPMethod::SetOutputLatency(unsigned int)','CEPMethod::SetOutputFifoOffset(unsigned int)',
 'RealMCPXVoiceProcessor::Synchronize(unsigned long)','RealMCPXVoiceProcessor::SetTime(unsigned long)',
 'RealMCPXVoiceProcessor::SetMode(unsigned long)']

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True);image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'DSOUND');records=[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES));lines=[];instructions={}
        for pc in sorted(body.code):
            op=code.at(pc);assert bytes(op.bytes)==image.read(pc,op.size);instructions[f'{pc:08x}']=bytes(op.bytes).hex();lines.append(f'{pc:08X}: {op.mnemonic} {op.op_str}')
        out=args.output/f'tu15-{entry:08x}.asm';out.write_text('\n'.join(lines)+'\n');records.append(dict(entry=entry,sites=len(body.code),file=str(out),instruction_bytes=instructions))
    elf=PRIMARY/'0292/nvsound/main/mcpmain.o';raw=elf.read_bytes();assert hashlib.sha256(raw).hexdigest()==ELF_PIN
    assert raw[:6]==b'\x7fELF\x01\x01';shoff=struct.unpack_from('<I',raw,32)[0];shsize,count,strings=struct.unpack_from('<3H',raw,46)
    sections=[struct.unpack_from('<10I',raw,shoff+i*shsize) for i in range(count)];names=raw[sections[strings][4]:sections[strings][4]+sections[strings][5]]
    def secname(i):
        p=sections[i][0];return names[p:names.index(0,p)].decode()
    def section_data(i):
        s=sections[i];return raw[s[4]:s[4]+s[5]]
    roindex=next(i for i in range(count) if secname(i)=='.rodata');ro=section_data(roindex)
    relindex=next(i for i in range(count) if secname(i)=='.rel.rodata');rel=section_data(relindex);symindex=sections[relindex][6]
    symbols=section_data(symindex);strs=section_data(sections[symindex][6]);rows=[]
    for offset in [0x14,0x18,0x1c]:
        where=0x2d428+offset;found=[struct.unpack_from('<2I',rel,p) for p in range(0,len(rel),8) if struct.unpack_from('<I',rel,p)[0]==where];assert len(found)==1
        _,info=found[0];assert info&255==1;name,value,size,kind,other,section=struct.unpack_from('<3I2BH',symbols,(info>>8)*16)
        assert secname(section)=='.text';addend=struct.unpack_from('<I',ro,where)[0];address=value+addend
        rows.append(dict(interface=0x2d428,offset=offset,relocation='R_386_32 .text',raw_addend=addend,resolved_text=address))
    assert [r['resolved_text'] for r in rows]==[0x90c3a,0x90c4e,0x90c62]
    disassembly=(PRIMARY/'mcpmain-disassembly.txt').read_text();heads=list(re.finditer(r'^([0-9a-f]+) <(.+)>:\n',disassembly,re.M));vendor=[]
    for i,h in enumerate(heads):
        if h[2] not in SYMBOLS:continue
        data=disassembly[h.start():heads[i+1].start() if i+1<len(heads) else len(disassembly)];out=args.output/f'nvidia-{h[1]}.asm';out.write_text(data)
        vendor.append(dict(symbol=h[2],elf_text=int(h[1],16),source_line=disassembly.count('\n',0,h.start())+1,file=str(out),sha256=hashlib.sha256(out.read_bytes()).hexdigest()))
    assert len(vendor)==len(SYMBOLS)
    manifest=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,nvidia_elf_sha256=ELF_PIN,
        nvidia_package_url='https://download.nvidia.com/XFree86/nforce/1.0-0292/NFORCE-Linux-x86-1.0-0292-pkg1.run',
        nvidia_package_sha256='218c67fbbab6db044269ae99bef658df8eb9c64ae9e303d157874fca63aed2ba',
        nvidia_disassembly_sha256=hashlib.sha256((PRIMARY/'mcpmain-disassembly.txt').read_bytes()).hexdigest(),original=records,vendor=vendor,real_vp_interface_relocations=rows,
        accepted_native_configuration=[],counter_units_or_live_producer_verified=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(f'Pinned {len(records)} original bodies/{len(vendor)} named vendor bodies; real VP clock interface relocations match')
if __name__=='__main__':main()
