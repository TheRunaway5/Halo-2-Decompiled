#!/usr/bin/env python3
"""Authenticate both installed-MCC and original-TU1.5 fraction signatures, read-only."""
import argparse,hashlib,json,os,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--mcc',type=Path,default=Path('/home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll'));a=p.parse_args();assert 'HALO2_TEST_SCRATCH'in os.environ
 raw=a.mcc.read_bytes();mcc_pin=hashlib.sha256(raw).hexdigest();assert mcc_pin=='de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946'
 pe=struct.unpack_from('<I',raw,0x3c)[0];count=struct.unpack_from('<H',raw,pe+6)[0];opt=pe+24;size=struct.unpack_from('<H',raw,pe+20)[0];assert struct.unpack_from('<H',raw,opt)[0]==0x20b;base=struct.unpack_from('<Q',raw,opt+24)[0]
 sections=[struct.unpack_from('<IIII',raw,opt+size+n*40+8)for n in range(count)]
 def read(va,width):
  rva=va-base
  for vs,at,rs,off in sections:
   if at<=rva and rva+width<=at+min(vs,rs):return raw[off+rva-at:off+rva-at+width]
  raise ValueError('MCC metadata outside physical section')
 descriptor=int.from_bytes(read(0x180e04850+329*8,8),'little');data=read(descriptor,64);name_at=int.from_bytes(data[8:16],'little');name=read(name_at,32).split(b'\0',1)[0].decode()
 assert descriptor==0x180b249c0 and name=='ai_living_fraction';assert int.from_bytes(data[:2],'little')==6;assert int.from_bytes(data[0x38:0x3a],'little')==1 and int.from_bytes(data[0x3a:0x3c],'little')==19;assert int.from_bytes(data[0x20:0x28],'little')==0x180786b90
 x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN;definition=int.from_bytes(x.read(0x480c00+327*4,4),'little');source=x.read(definition,16);assert definition==0x458494;assert struct.unpack('<HHI',source[:8])==(6,0,0x2af700);assert int.from_bytes(source[12:14],'little')==1 and int.from_bytes(source[14:16],'little')==19
 c=_Code(x,'.text');listing=[];pc=0x2af729
 while pc<0x2af761:
  i=c.at(pc);listing.append(dict(instruction=pc,bytes=i.bytes.hex(),operation=i.mnemonic,operands=i.op_str));pc+=i.size
 report=dict(passed=True,mcc_sha256=mcc_pin,mcc_descriptor=dict(opcode=329,address=descriptor,bytes=data.hex(),name=name,name_address=name_at,evaluator=0x180786b90,return_type=6,arguments=[19]),xbox_sha256=NEW_PIN,xbox_descriptor=dict(opcode=327,address=definition,bytes=source.hex(),evaluator=0x2af700,return_type=6,arguments=[19],flags=0),original_fraction_continuation=listing,semantic_contract='27DC40 mode0 full signed32living/population; pop>0 convertsliving thenpopulation withCVTSI2SS andDIVSS; otherwisepositive0. Directactor1/1 still reads actualunitvitality.',boundaries=['Ready/resumed212010+211DA0 execution, not first-dispatch argument expression evaluation','Returned stickyFP policy is tested; globalHSFP ownership/publication remains open'],production_executable_reads=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),retained_large_artifacts=[])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print('MCC329 REAL(AI) ->TU1.5 opcode327/2AF700 independently authenticated')
if __name__=='__main__':main()
