"""Complete-body/context authentication for the four affine-inverse owners.

The two actual-named bodies own only TU1.5. The canonical Motion and observed
World bodies own both separately proved images; observation mode is explicit.
No dispatcher or operand expression is normalized away during admission.
"""
import hashlib,importlib.util,re
from pathlib import Path

INCLUDE='#include "physics/runtime/native_tu15_affine_inverse.hpp"\n'
BODY=re.compile(r'static void run_(144090|142d00)\((WorldState|ObservedWorldState|MotionState) &s\) \{.*?\n\}',re.S)
CONTEXTS={
 ('69a1ac07734132b1faa22fc1905209d8dafd12de15e2258f02bae67f38e0ee64','5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657','1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b'):('actual','ai'),
 ('69a1ac07734132b1faa22fc1905209d8dafd12de15e2258f02bae67f38e0ee64','6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9','f754c70cbc7f2e502ea1e757ef67688c89a38f38b665e7e2ba66f711571dc84c'):('actual','map-world'),
 ('cd2162d2861fc0cb302081120dcc26518f64d137394aff7268aaae47b569d299','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c5ea0c5e4c241d9a0b448bea12cb9fc24cc8a208b0d861a57d22dd9dc0fc143c'):('world','world'),
 ('fe78ee954089255c5897328b8a348e9a799b9559bb640d2a8e71598537537047','771300930484d824648b4efc363e270e680eae6c45ec6ff73b04312731cab022','256b7dcb6f209e0f424b18208ae7f67b530185f594c7805a947a3afc45736624'):('canonical','motion'),
 # Actual fresh generators: the complete body equals the authenticated
 # canonical body after verifying every consumed selected pair is identity.
 # Both entire fresh resolvers and entire dispatchers remain independently
 # pinned; no arbitrary map/case/body normalization is accepted here.
 ('962838d201696158328484bb2dafc52fd853d5cba86bd9d39551067830d40256','b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8','974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a'):('actual','ai-fresh'),
 ('962838d201696158328484bb2dafc52fd853d5cba86bd9d39551067830d40256','5dda1bc027ca22bb0dfb680fea8c6ac399b3de4002aea0733e29373ed7f7b08d','e32f2927198be567ac60d36c8f00767d8941832313a643fd964f3297e651f5f4'):('actual','map-world-fresh'),
}
BODY_PINS={context[0]for context in CONTEXTS}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def prefix(mode):return '\n    if(halo2::native_affine_inverse_detail::bridge(s,halo2::native_affine_inverse_detail::Observation::'+mode+'))return;'
PREFIXES=tuple(prefix(mode)for mode in ('actual','canonical','world'))
ACTUAL_PREFIX,CANONICAL_PREFIX,WORLD_PREFIX=PREFIXES
def canonical_body(body):
 opening=body.index('{')+1
 for candidate in PREFIXES:
  if body[opening:].startswith(candidate):return body[:opening]+body[opening+len(candidate):]
 return body
def authenticate(source):
 found=list(BODY.finditer(source))
 if len(found)!=1:raise ValueError('Expected one complete affine-inverse owner')
 match=found[0];body=canonical_body(match[0]);state=match[2]
 if digest(body)not in BODY_PINS:raise ValueError('Complete affine-inverse body drift')
 resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
 dispatcher=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
 if not resolver or not dispatcher:raise ValueError('Complete affine-inverse resolver/dispatcher missing')
 context=(digest(body),digest(resolver[0]),digest(dispatcher[0]))
 if context not in CONTEXTS:raise ValueError('Complete affine-inverse owning context drift')
 mode,owner=CONTEXTS[context]
 if match[0]!=body and not match[0][match[0].index('{')+1:].startswith(prefix(mode)):
  raise ValueError('Affine-inverse observation mode drift')
 if mode=='world':
  helper=re.search(r'std::uint32_t world_observation_pc\(.*?\n\}',source,re.S)
  expected='std::uint32_t world_observation_pc(std::uint32_t disc,std::uint32_t tu15,\n    const ObservedWorldState &state) {\n    return state.selected_observation_pcs && state.image==SourcePhysicsContactImage::TitleUpdate15?tu15:disc;\n}'
  if not helper or helper[0]!=expected:raise ValueError('Affine-inverse observation selector drift')
  state=re.search(r'struct ObservedWorldState : physics_world_runtime_detail::WorldState \{.*?\n\};',source,re.S)
  expected_state='struct ObservedWorldState : physics_world_runtime_detail::WorldState {\n    using physics_world_runtime_detail::WorldState::WorldState;\n    bool selected_observation_pcs{};\n};'
  if not state or state[0]!=expected_state:raise ValueError('Affine-inverse observed State drift')
 return match,body,mode,owner
def replace_affine_inverse(source):
 if not BODY.search(source):return source
 match,body,mode,_=authenticate(source)
 if match[0]==body:
  opening=body.index('{')+1;bound=body[:opening]+prefix(mode)+body[opening:]
  source=source[:match.start()]+bound+source[match.end():]
 if source.count(INCLUDE)>1:raise ValueError('Duplicate affine-inverse native include')
 return source if INCLUDE in source else INCLUDE+source
def remove_affine_inverse(source):
 if not BODY.search(source):return source
 match,body,_,_=authenticate(source)
 source=source[:match.start()]+body+source[match.end():]
 if source.count(INCLUDE)>1:raise ValueError('Duplicate affine-inverse native include')
 return source.replace(INCLUDE,'',1)

PUBLIC_INCLUDE='#include "halo2/native_tu15_affine_inverse_value.hpp"\n'
def replace_public(source):
 spec=importlib.util.spec_from_file_location('affine_inverse_public_inputs',Path(__file__).with_name('native_affine_inverse_inputs.py'))
 helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
 body=helper.unique_function(source,'inverse_frame')
 if body!=helper.SHARED_BODY and digest(body)!=helper.PUBLIC_PIN:raise ValueError('Complete public affine inverse drift')
 source=source.replace(body,helper.SHARED_BODY,1)
 if source.count(PUBLIC_INCLUDE)>1:raise ValueError('Duplicate public affine inverse include')
 return source if PUBLIC_INCLUDE in source else PUBLIC_INCLUDE+source
