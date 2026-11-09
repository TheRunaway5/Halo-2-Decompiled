"""Authenticate the complete184690 classifier and shared world value seam."""
import hashlib,re
BODY_PIN='2d60a15a07af88107316c4ee6484ae1784a617e760fad5e7a2cef8a71ed5d52e'
RESOLVER_PINS=('892cf508106b0c182d59e4dba45804e434a1833003cfb3c322f9a8875c54d7fb',)
DISPATCH_PINS=('c0e112fb90d7ea32e73e6b3b37d333b6a375456791c5f2eaf9a08906d613f6a9',)
PREFIX='\n    if(halo2::native_sphere_frustum_abi_detail::bridge(s))return;'
INCLUDE='#include "rendering/native_tu15_sphere_frustum_abi.hpp"\n'
PUBLIC_PIN='e6ca28979812881c081fe8c54dee6387203aa3b60458498507c8e7fe7ff67324'
PUBLIC_INCLUDE='#include "halo2/native_tu15_sphere_frustum_value.hpp"\n'
PUBLIC_BODY='''SourceSphereVisibility source_sphere_frustum_visibility(
    const SourceSphereFrustum& f,Vec3 center,float radius) {
    return native_tu15_sphere_frustum_value(f,center,radius);
}'''
def canonical_body(body):
 start=body.index('{')+1
 return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body

def replace_sphere_frustum(source):
 found=list(re.finditer(r'static void run_184690\(WorldState &s\) \{.*?\n\}',source,re.S))
 if not found:return source
 if len(found)!=1:raise ValueError('Expected one complete184690 classifier')
 current=found[0][0];original=canonical_body(current)
 if hashlib.sha256(original.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete184690 sphere/frustum body drift')
 resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
 if not resolver or hashlib.sha256(resolver[0].encode()).hexdigest()not in RESOLVER_PINS:raise ValueError('Complete sphere/frustum selected resolver drift')
 dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
 if not dispatch or hashlib.sha256(dispatch[0].encode()).hexdigest()not in DISPATCH_PINS:raise ValueError('Complete sphere/frustum dispatcher drift')
 if current==original:
  start=original.index('{')+1;changed=original[:start]+PREFIX+original[start:];source=source[:found[0].start()]+changed+source[found[0].end():]
 if source.count(INCLUDE)>1:raise ValueError('Duplicate native sphere/frustum include')
 if INCLUDE not in source:source=INCLUDE+source
 return source

def replace_world_visibility(source):
 from audit_native_execution_ownership import unique_function
 current=unique_function(source,'source_sphere_frustum_visibility')
 if current!=PUBLIC_BODY:
  if hashlib.sha256(current.encode()).hexdigest()!=PUBLIC_PIN:raise ValueError('Complete public sphere/frustum value body drift')
  source=source.replace(current,PUBLIC_BODY,1)
 if source.count(PUBLIC_INCLUDE)>1:raise ValueError('Duplicate public sphere/frustum value include')
 if PUBLIC_INCLUDE not in source:source=source.replace('#include "halo2/source_visibility.hpp"\n','#include "halo2/source_visibility.hpp"\n'+PUBLIC_INCLUDE,1)
 if PUBLIC_INCLUDE not in source:raise ValueError('Missing public visibility include anchor')
 return source
