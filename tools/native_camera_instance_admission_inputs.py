"""Owned source-copy preparation for the complete16B810 predicate."""
import re
from pathlib import Path
import native_camera_instance_admission_replacements as binding
from specialize_native_operands import specialize
UNITS=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/gameplay/units/death_cycle_runtime_tu15_generated.cpp')
PUBLIC='src/physics/native_tu15_camera_instanced_query.cpp'

def prepare(root,scratch,require_binding=False):
 pieces=[];rejections=0;receipts=[]
 for name,path in zip(('primary','ai','death'),UNITS,strict=True):
  source=(root/path).read_text();match,original=binding.authenticate(source);bound=binding.replace_camera_instance_admission(source)
  if require_binding and bound!=source:raise ValueError('Production16B810 binding missing '+name)
  assert binding.replace_camera_instance_admission(bound)==bound
  prepared,_=specialize(bound);body=binding.BODY.search(prepared)[0]
  assert binding.PREFIX in body and 's.instruction_literal('in body and binding.digest(body)==binding.PREPARED_OUTPUT_PIN
  # The prepared form is output of the actual owning specializer, never an
  # independently admitted input to the source installer.
  resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
  for suffix,code in (('bound',binding.BODY.search(bound)[0]),('prepared',body),('retained',original)):
   pieces.append('namespace '+name+'_'+suffix+' {\n'+resolver+'\n'+code+'\n}\n')
  dispatch=next(line for line in source.splitlines()if line.startswith('static void dispatch(WorldState &s,std::uint32_t target'))
  mutations=(source.replace(match[0],match[0].replace('0x98u','0x99u',1),1),source.replace(match[0],match[0].replace('s.gp[4]+=4u','s.gp[4]+=8u',1),1),source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),source.replace(dispatch,dispatch.replace('run_16b810(s);','run_16c000(s);',1),1),source.replace(dispatch,dispatch.replace('s.required(site,target,tail);','s.required(site,target+1u,tail);',1),1),bound.replace(binding.PREFIX,binding.PREFIX.replace('::bridge','::changed_bridge'),1))
  for changed in mutations:
   if changed in(source,bound):raise ValueError('Missing source mutation witness '+name)
   try:binding.replace_camera_instance_admission(changed)
   except ValueError:rejections+=1
   else:raise ValueError('16B810 binder admitted changed owner '+name)
  receipts.append(dict(unit=path,original_body=binding.digest(original),resolver=binding.digest(resolver),dispatcher=binding.digest(dispatch),prepared_body=binding.digest(body)))
 public=(root/PUBLIC).read_text();shared=binding.replace_public(public)
 if require_binding and shared!=public:raise ValueError('Public16B810 DATA binding missing')
 assert binding.replace_public(shared)==shared
 current=binding.public_body(public)
 try:binding.replace_public(public.replace(current,current.replace('std::uint32_t mask','std::uint32_t changed_mask',1),1))
 except ValueError:rejections+=1
 else:raise ValueError('Public16B810 binder admitted changed signature')
 memory=re.search(r'class Memory \{.*?\n\};',public,re.S)[0]
 assert binding.digest(binding.ORIGINAL_PUBLIC_BODY)==binding.PUBLIC_PIN
 pieces.append('namespace public_reference {\n'+memory+'\n'+binding.ORIGINAL_PUBLIC_BODY+'\n}\n')
 pieces.append('namespace public_shared {\n'+memory+'\n'+binding.PUBLIC_BODY+'\n}\n')
 (scratch/'camera_admission_bodies.inc').write_text(''.join(pieces))
 return dict(source_drift_rejections=rejections,owners=receipts,public_reference_pin=binding.PUBLIC_PIN,public_memory_pin=binding.digest(memory))
