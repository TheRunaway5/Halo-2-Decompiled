"""Exact complete VehicleEnvironment raw lookup owner overlay."""
import hashlib,re
INCLUDE='#include "physics/vehicles/native_tu15_vehicle_object_lookup_owner.hpp"\n'
HEADER='void run_bcdf0(MathState &s) {'
PREFIX='\n if(halo2::native_vehicle_object_lookup_owner_detail::bridge(s))return;'
SOURCE_PIN='d68c9ad2f8e26a2e6ef70c3b40c19ae80b970ccacdb23f4199f75b852b276eae'
BODY_PIN='0b00de7a5443ca306ccdc00399f92a82e90f9ce380177cf2468010b578fc4355'
PATTERN=re.compile(r'^void run_bcdf0\(MathState &s\) \{.*?^\}',re.M|re.S)
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def canonical_body(body):
    if PREFIX in body:
        if body.count(PREFIX)!=1 or not body.startswith(HEADER+PREFIX):raise ValueError('Vehicle lookup prefix is not at the exact entry')
        body=body.replace(PREFIX,'',1)
    if 'native_vehicle_object_lookup_owner_detail'in body or digest(body)!=BODY_PIN:
        raise ValueError('Complete VehicleEnvironment raw lookup changed')
    return body
def authenticate(source):
    if 'void source_run_physics_vehicle_environment_state_runtime('not in source:
        if INCLUDE in source or 'native_vehicle_object_lookup_owner_detail'in source:raise ValueError('Vehicle lookup owner missing')
        return None
    values=PATTERN.findall(source)
    if len(values)!=1:raise ValueError('Missing/ambiguous complete VehicleEnvironment lookup')
    body=canonical_body(values[0]);owned=int(PREFIX in values[0])
    if source.count(INCLUDE)!=owned or source.count('halo2::native_vehicle_object_lookup_owner_detail::bridge')!=owned:
        raise ValueError('Partial, external or duplicate VehicleEnvironment native overlay')
    original=source.replace(values[0],body,1).replace(INCLUDE,'',1)
    if digest(original)!=SOURCE_PIN:raise ValueError('Complete VehicleEnvironment source/public/context changed')
    return values[0],body
def replace_vehicle_object_lookup(source):
    found=authenticate(source)
    if found is None:return source
    original,body=found
    if PREFIX in original:return source
    result=INCLUDE+source.replace(original,HEADER+PREFIX+body[len(HEADER):],1)
    authenticate(result);return result
def remove_vehicle_object_lookup(source):
    found=authenticate(source)
    if found is None:return source
    original,body=found;return source.replace(original,body,1).replace(INCLUDE,'',1)
def installed_entries(source):
    found=authenticate(source)
    if found is None or PREFIX not in found[0]:raise ValueError('VehicleEnvironment native lookup not installed')
    return ['bcdf0']
