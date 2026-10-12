#!/usr/bin/env python3
"""Complete driver caller and its CPU helper correspondence, never admission."""
import argparse,json
from pathlib import Path
import research_animation_selection_u5 as authority
OWNERS=(0x2205c0,0x220bf0,0x12d7b0,0x1656a0)
BOUNDARIES=(0x192d30,0x2228a0,0x2215f0,0x222b80,0x192e50,0x2235a0,0x2213a0,0x2b4470)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();authority.OWNERS=OWNERS;authority.BOUNDARIES=BOUNDARIES;authority.run(a.output)
 r=json.loads((a.output/'report.json').read_text());row=next(q for q in r['owners']if q['old']=='002205c0');images=[authority.Xbe(p)for p in(authority.OLD,authority.NEW)];constants={}
 for name,address in(('one_bits','00465030'),('basis_forward','0046fc48'),('basis_up','0046fc50')):
  operand=next(q for q in row['memory']if q['old_va']==address)
  if name=='one_bits':constants[name]=dict(old_va=operand['old_va'],u5_va=operand['new_va'],raw4_hex=operand['old_hex']);assert operand['old_hex']==operand['new_hex']
  else:
   pointers=[int.from_bytes(x.read(int(operand['old_va'if i==0 else'new_va'],16),4),'little')for i,x in enumerate(images)];values=[x.read(p,12).hex()for x,p in zip(images,pointers)];assert values[0]==values[1]
   constants[name]=dict(old_pointer_va=operand['old_va'],u5_pointer_va=operand['new_va'],old_value_va=f'{pointers[0]:08x}',u5_value_va=f'{pointers[1]:08x}',raw12_hex=values[0])
 lines=[(a.output/'instructions'/f'002205c0-{tag}.asm').read_text().splitlines()for tag in('old','u5')]
 at=next(i for i,q in enumerate(lines[0])if q.startswith('002209f9 '));plugin=[int(q[at].split()[-1],16)for q in lines]
 env_at=next(i for i,q in enumerate(lines[0])if q.startswith('00220a2b '));env=[int(q[env_at].split()[-1],16)for q in lines]
 packet=dict(schema='source-sound-driver-startup-providers-v1',constants=constants,source_selected_plugin_identity=dict(old=f'{plugin[0]:08x}',u5=f'{plugin[1]:08x}',native_retained_mutable_plugin_owner_required=True),runtime_environment_packet=dict(old=f'{env[0]:08x}',u5=f'{env[1]:08x}',copied_into_constants=False),image_sha256=r['image_sha256'],bounds=['Raw immutable basis vectors and one word only; source saved settings, device/caps/plugin/environment/stream ownership remain required.','Helper bodies are statically paired; mandatory source CPU helpers must execute their owning state, never callback default success. Whole source/platform initialization is not admitted.'])
 authority.save(a.output/'providers.json',authority.encoded(packet));authority.save(a.output/'invocation.json',authority.encoded(dict(tool_sha256=authority.sha(__file__),authority_tool_sha256=authority.sha(authority.__file__),source_instruction_patches=[],platform_success_returns=[])))
 print(json.dumps(dict(providers_sha256=authority.sha(a.output/'providers.json'))))
if __name__=='__main__':main()
