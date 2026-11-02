#!/usr/bin/env python3
"""Build an untouched original i386 BEFORE loader for owned physical probes.

The caller must run within tools/run_test.py and put output under its scratch.
No captured AFTER bytes or result fields are read. The process stops before
executing original code, with actual input GP/FP/SIMD/FS already installed.
"""
import json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from replay_live_phantom_hardware_tu15 import ASM,reference_text_gaps
from validate_native_contact_agent_dispatch_tu15 import sha,save

def build_before_reference(capture,output):
 capture=Path(capture).resolve();output=Path(output).resolve();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert output.is_relative_to(scratch);output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN
 d=json.loads((capture/'capture.json').read_text());assert d['reference']=='English TU1.5 only';before=d['before'];assert len(before['gp'])==8 and len(before['xmm'])==8 and len(before['x87_values'])<=8
 environment=bytearray(108);tag=0xffff;top=(before['x87_status']>>11)&7
 for n,text in enumerate(before['x87_values']):
  raw=bytes.fromhex(text);assert len(raw)==10;mantissa=int.from_bytes(raw[:8],'little');exponent=int.from_bytes(raw[8:],'little')&0x7fff;kind=1 if exponent==0 and mantissa==0 else 2 if exponent in(0,0x7fff)else 0;index=(top+n)&7;tag=(tag&~(3<<(index*2)))|(kind<<(index*2));environment[28+n*10:38+n*10]=raw
 for offset,value in((0,before['x87_control_word']),(4,before['x87_status']),(8,tag)):struct.pack_into('<H',environment,offset,value)
 actual_input=struct.pack('<10I',*before['gp'],before['eflags'],before['mxcsr'])+environment+b''.join(struct.pack('<4I',*x)for x in before['xmm']);assert len(actual_input)==276;(output/'input.bin').write_bytes(actual_input)
 image=Xbe(NEW_DEFAULT);regions=sorted(d['regions'],key=lambda r:r['address']);text,gaps,authentication=reference_text_gaps(image,capture,regions,output);assembly=[];sections=[]
 for n,row in enumerate(sorted([*regions,*gaps],key=lambda r:r['address'])):
  path=row.get('reference_file',capture/row.get('before_file',''));path=Path(path).resolve();assert path.is_relative_to(capture)or path.is_relative_to(output);assert not(row['address']<=0x70000000<row['address']+row['size']);assert path.stat().st_size==row['size']
  assembly.append(f'.section .bank{n},"awx"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{row["address"]:x}:{{*(.bank{n})}}')
 bridge=ASM.replace('EMIT_BANKS','').replace('ENTRY',str(d['entry'])).replace('INPUT_FILE',str(output/'input.bin')).replace('FS_BASE',str(before['fs_base']));(output/'reference.s').write_text(bridge+'\n'+'\n'.join(assembly));(output/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n.bridge 0x70000000:{*(.bridge)}\n}\n')
 commands=[['as','--32',str(output/'reference.s'),'-o',str(output/'reference.o')],['ld','-melf_i386','-T',str(output/'link.ld'),str(output/'reference.o'),'-o',str(output/'reference')]]
 for command in commands:subprocess.run(command,check=True,capture_output=True,text=True)
 receipt=dict(elf=str(output/'reference'),capture_metadata_sha256=sha(capture/'capture.json'),original_sha256=NEW_PIN,code_authentication=authentication,assembly_commands=commands,captured_after_consumed=False,instruction_patches=[],actual_gp_fp_simd_fs_installed_before_stop=True,source_sha256=sha(Path(__file__)));save(output/'loader-report.json',receipt);return receipt
