"""Authenticate the distinct fixed-1.1 animation inverse and its real owner."""
import hashlib,importlib.util,re
from pathlib import Path
BODY_PIN='5177581f9047014da84e3128b702bd2a70d9549b8dcaa3643415590882a6df1d'
STATE_PIN='f4a9132b99c547907a01ff822ce15a7145522e1c69f75cb4b3f1504f03572c08'
PUBLIC_PIN='10c9f9409a059e655cb80a2d8b621789df7c92bc69f0468f3fcd2bf42c1385b6'
UNIT='src/gameplay/animation/unit_animation_selection_runtime.cpp'
INCLUDE='#include "physics/runtime/native_tu15_affine_inverse.hpp"\n'
PREFIX='\n    if(halo2::native_affine_inverse_detail::bridge(s,halo2::native_affine_inverse_detail::Observation::canonical))return;'
BODY=re.compile(r'\[\[gnu::noinline\]\] static void run_142d00\(State &s\) \{.*?\n\}',re.S)
STATE=re.compile(r'struct State : physics_contact_runtime_detail::State \{.*?\n\};',re.S)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
 opening=body.index('{')+1
 return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
 matches=list(BODY.finditer(source));states=list(STATE.finditer(source))
 if len(matches)!=1 or len(states)!=1:raise ValueError('Missing complete fixed-1.1 inverse/State owner')
 match=matches[0];body=canonical_body(match[0]);state=states[0][0]
 if digest(body)!=BODY_PIN or digest(state)!=STATE_PIN:raise ValueError('Complete fixed-1.1 inverse/State drift')
 spec=importlib.util.spec_from_file_location('animation_inverse_owner_audit',Path(__file__).with_name('audit_native_execution_ownership.py'))
 audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
 public=audit.unique_function(source,'source_run_unit_animation_selection_1_1')
 if digest(public)!=PUBLIC_PIN or public.count('case 0x142d00u:run_142d00(s);break;')!=1:raise ValueError('Complete fixed-1.1 inverse public entry drift')
 return match,body,state,public
def replace_affine_inverse_animation(source):
 if not BODY.search(source):return source
 match,body,_,_=authenticate(source)
 if match[0]==body:
  opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:];source=source[:match.start()]+bound+source[match.end():]
 if source.count(INCLUDE)>1:raise ValueError('Duplicate fixed-1.1 inverse include')
 return source if INCLUDE in source else INCLUDE+source
