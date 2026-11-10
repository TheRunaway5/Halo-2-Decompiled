"""Pin actual public16B9A0 DATA implementation for private shared-core parity."""
import hashlib,re
from audit_native_execution_ownership import unique_function
PUBLIC='src/physics/native_tu15_camera_dynamic_query_leaves.cpp'
PUBLIC_PIN='7a50b4a077d4fd51949c1b6b890953b6e1a386978e15e9d0ad482c65dac9998d'
HELPERS={'word':'e1768d85a18bad67e3caf9dae81d90a48a075086acea68041738851e2dba7f46','store':'8862e98426e949a6927f71902c83a320b6e653482926b28cc25d8217e35edd28','scalar':'93930836638a302d9e2a248c0fecb8afcd31f7c251fdc13408ff1c85c92c037a','bits':'1aaf04920a645a29c92ca1503d7ac35811bb2ca64262da3b54cb62e040d9a2de','mul':'18d4d1a301c5f62be01a02031b37731eeb01e01690acb6160b48c2d8c08e3836','add':'81267e61fe0e9952ef9845ea925504c51fa4d46ead2c8c14632ef33484cb0cbc','sub':'c4000a5fa48bdc7b1fefe1df55ff77eaf6092fbdc6bef1a22e9f061d52e7c167'}
ENV_PIN='eccd26bae6f5fc6f47cf6749ec2bf6f1645b7325fdc8168a3cfc8de13dbcf5bd'
SHARED_BODY='''void native_tu15_publish_camera_collision(SourcePhysicsContactMemory&m,std::uint32_t out,
    std::uint32_t transform,std::uint32_t collision,NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&o){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp);
    auto read=[&](std::uint32_t address,unsigned width=4u){return word(m,address,width);};
    auto write=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){store(m,o,pc,address,value,width);};
    native_camera_collision_publication_detail::Values view{read,write};
    native_camera_collision_publication_detail::Progress data;data.output=out;data.transform=transform;data.collision=collision;
    native_camera_collision_publication_detail::publish(view,data);
#else
    (void)m;(void)out;(void)transform;(void)collision;(void)fp;(void)o;throw std::runtime_error("Native camera collision backend unavailable");
#endif
}'''
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def prepare_public(root,scratch):
 source=(root/PUBLIC).read_text();current=unique_function(source,'native_tu15_publish_camera_collision')
 body=(root/'tests/native_tu15_camera_collision_publication_reference.inc').read_text().rstrip('\n');assert digest(body)==PUBLIC_PIN
 if current!=SHARED_BODY and digest(current)!=PUBLIC_PIN:raise ValueError('Public16B9A0 source drift')
 parts=['using F=__m128;']
 for name,pin in HELPERS.items():
  helper=unique_function(source,name);assert digest(helper)==pin;parts.append(helper)
 environment=re.search(r'class Environment\{.*?\};',source,re.S)[0];assert digest(environment)==ENV_PIN;parts.append(environment)
 shared='\n'.join(parts)
 (scratch/'publication_values.inc').write_text('namespace public_reference {\n'+shared+'\n'+body+'\n}\nnamespace public_shared {\n'+shared+'\n'+SHARED_BODY+'\n}\n')
 return dict(original_public_body=PUBLIC_PIN,helpers=HELPERS,environment=ENV_PIN,shared_public_body=digest(SHARED_BODY))

UNIT='src/physics/physics_live_support_tu15_generated.cpp'
def prepare_source(root,scratch,require_binding=False):
 import native_camera_collision_publication_replacements as binding
 from specialize_native_operands import specialize
 source=(root/UNIT).read_text();match,original=binding.authenticate(source);bound=binding.replace_camera_collision_publication(source)
 if require_binding and bound!=source:raise ValueError('Production16B9A0 publication binding missing')
 assert binding.replace_camera_collision_publication(bound)==bound
 prepared,_=specialize(bound);body=binding.BODY.search(prepared)[0]
 assert binding.PREFIX in body and 's.instruction_literal('in body
 if binding.PREPARED_OUTPUT_PIN is not None:assert digest(body)==binding.PREPARED_OUTPUT_PIN
 (scratch/'publication_bodies.inc').write_text('namespace bound {\n'+binding.BODY.search(bound)[0]+'\n}\nnamespace prepared {\n'+body+'\n}\nnamespace original {\n'+original+'\n}\n')
 resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
 dispatch=next(line for line in source.splitlines()if line.startswith('static void dispatch(WorldState &s,std::uint32_t target'))
 mutations=(source.replace(match[0],match[0].replace('0x28u','0x29u',1),1),source.replace(match[0],match[0].replace('_mm_mul_ss','_mm_add_ss',1),1),source.replace(match[0],match[0].replace('s.gp[4]+=0u','s.gp[4]+=4u',1),1),source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),source.replace(dispatch,dispatch.replace('run_16b9a0(s);','run_16b810(s);',1),1),bound.replace(binding.PREFIX,binding.PREFIX.replace('::bridge','::changed_bridge'),1))
 for changed in mutations:
  if changed in(source,bound):raise ValueError('Missing publication source mutation witness')
  try:binding.replace_camera_collision_publication(changed)
  except ValueError:pass
  else:raise ValueError('Complete16B9A0 source/body/routing mutation admitted')
 return dict(source_drift_rejections=len(mutations),original_body=digest(original),prepared_body=digest(body),unit=UNIT)
