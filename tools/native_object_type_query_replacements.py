"""Exact private TypeQuery parent overlay; inherited children keep their owners."""
import hashlib,re
FRONTIERS=(0x10de46,0x10de49,0x10de4d,0x10de51,0x10de5f,0x10de66,0x10de6d,
    0x10de70,0x10de74,0x10de76,0x10de82,0x10de89,0x10de91,0x10de92,0x10de93,
    0x10de94,0x10de95,0x10de97,0x10de9f,0x10dea0,0x10deae,0x10deb0,0x10deb1,0x10deb2)
INCLUDE='#include "gameplay/objects/native_tu15_object_type_query_source_owner.hpp"\n'
HEADER='static void run_10de40(WorldState &s) {'
BRIDGE='halo2::native_object_type_query_owner_detail::bridge'
BODY_PINS={False:'12cde1f032681088a6ab567a5f360e66e28bb509ae903b7c620886f8a1ef190d',
    True:'b4a50df4fffaca4257c8bcc0a792c5c4e630232199c04dfea17d745ab38a01fa'}
SIGN='cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b'
RESOLVER='1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4'
# Entire correlated dispatcher, State, public entry and ownership profiles.
# Prepared dispatchers are different real complete owners, not a cross product.
CONTEXTS={
    (False,'5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0','086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46','912e109873b4775f13979a32e325106d74712a3e4a0bf43d5485d911cd01f35a','f53eb05fed15f9f03fb04925d9a466a46992c9c79539232e8c54f76a96b43fab'),
    (False,'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575','7e227c0aa7f7b21a540fa749c52d95a75432e51a9513543be7bddc25fa58ca09','5a8d30ef7f8b2b214d9e32e5e4cdb91d43c2556853b5ad03692086703cec016c'),
    (True,'37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052','086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46','912e109873b4775f13979a32e325106d74712a3e4a0bf43d5485d911cd01f35a','f53eb05fed15f9f03fb04925d9a466a46992c9c79539232e8c54f76a96b43fab'),
    (True,'8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575','7e227c0aa7f7b21a540fa749c52d95a75432e51a9513543be7bddc25fa58ca09','5a8d30ef7f8b2b214d9e32e5e4cdb91d43c2556853b5ad03692086703cec016c'),
}
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def one(source,pattern,label):
    values=re.findall(pattern,source,re.M|re.S)
    if len(values)!=1:raise ValueError('Missing/ambiguous complete TypeQuery '+label)
    return values[0]
def parts(source):
    return dict(body=one(source,r'^static void run_10de40\(WorldState &s\) \{.*?^\}','body'),
        resolver=one(source,r'^std::uint32_t selected\([^\n]+','resolver'),
        dispatcher=one(source,r'^static void dispatch\([^\n]+','dispatcher'),
        state=one(source,r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;','State'),
        sign_extend=one(source,r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}','sign extension'),
        public_entry=one(source,r'^void source_run_physics_live_support_tu15\(.*?^\}','public entry'),
        owns_entry=one(source,r'^bool source_physics_live_support_tu15_owns_entry\([^\n]+','public ownership'))
def callback_format(body):
    raw=re.findall(r'if\(s.services.instruction\)\{s.services.instruction\(0x[0-9a-f]+u\);\}',body)
    literal=re.findall(r's.instruction_literal\(0x[0-9a-f]+u\);',body)
    if(len(raw),len(literal))==(43,0):return False
    if(len(raw),len(literal))==(0,43):return True
    raise ValueError('TypeQuery must contain exactly43 uniform callback sites')
def prefix(literal):
    return '\n    {const auto native_child=[&](auto&state,std::uint32_t target,std::uint32_t site,bool tail){dispatch(state,target,site,tail);};switch('+BRIDGE+'<'+('true'if literal else'false')+'>(s,native_child)){case 0u:break;case 0xffffffffu:return;'+''.join(f'case 0x{pc:x}u:goto NativeObjectTypeQuery{pc:x};'for pc in FRONTIERS)+'default:throw std::runtime_error("Invalid native TypeQuery continuation");}}'
def entry_prefix(body,check_format=True):
    literal=callback_format(body);found=[value for value in(prefix(False),prefix(True))if value in body]
    if not found:
        if BRIDGE in body:raise ValueError('Unknown TypeQuery bridge')
        return None
    if len(found)!=1 or body.count(found[0])!=1 or not body.startswith(HEADER+found[0]):
        raise ValueError('TypeQuery prefix is not at its exact owner entry')
    if check_format and found[0]!=prefix(literal):raise ValueError('Wrong TypeQuery callback format tag')
    return found[0]
def canonical_body(body):
    current=entry_prefix(body)
    if current:
        body=body.replace(current,'',1)
        for pc in FRONTIERS:
            label=f'NativeObjectTypeQuery{pc:x}: '
            callback=f's.instruction_literal(0x{pc:x}u);'if callback_format(body)else f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}'
            if body.count(label)!=1 or body.count(label+callback)!=1:raise ValueError('TypeQuery continuation is not at the exact untouched instruction')
            body=body.replace(label,'',1)
    if 'NativeObjectTypeQuery'in body:raise ValueError('Partial TypeQuery continuation overlay')
    return body
def authenticate(source):
    if 'void source_run_physics_live_support_tu15('not in source:
        if INCLUDE in source or BRIDGE in source:raise ValueError('Missing complete TypeQuery owner')
        return None
    values=parts(source);body=canonical_body(values['body']);literal=callback_format(body)
    if digest(body)!=BODY_PINS[literal]:raise ValueError('Complete TypeQuery parent changed')
    if digest(values['resolver'])!=RESOLVER or digest(values['sign_extend'])!=SIGN:
        raise ValueError('Complete TypeQuery resolver/sign extension changed')
    profile=(literal,*(digest(values[key])for key in('dispatcher','state','public_entry','owns_entry')))
    if profile not in CONTEXTS:raise ValueError('Complete correlated TypeQuery source context changed')
    owned=int(entry_prefix(values['body'])is not None)
    if source.count(INCLUDE)!=owned or source.count(BRIDGE)!=owned:
        raise ValueError('Partial, external or duplicate TypeQuery owner overlay')
    if sum(source.count(f'NativeObjectTypeQuery{pc:x}: ')for pc in FRONTIERS)!=owned*len(FRONTIERS):
        raise ValueError('External TypeQuery continuation label')
    return values,body
def replace_object_type_query(source):
    found=authenticate(source)
    if found is None:return source
    values,body=found
    if entry_prefix(values['body']):return source
    literal=callback_format(body);proposed=body
    for pc in FRONTIERS:
        callback=f's.instruction_literal(0x{pc:x}u);'if literal else f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}'
        if proposed.count(callback)!=1:raise ValueError('Missing exact TypeQuery continuation instruction')
        proposed=proposed.replace(callback,f'NativeObjectTypeQuery{pc:x}: '+callback,1)
    proposed=HEADER+prefix(literal)+proposed[len(HEADER):]
    result=INCLUDE+source.replace(values['body'],proposed,1);authenticate(result);return result
def remove_object_type_query(source):
    found=authenticate(source)
    if found is None:return source
    values,body=found;return source.replace(values['body'],body,1).replace(INCLUDE,'',1)
def installed_entries(source):
    found=authenticate(source)
    if found is None or not entry_prefix(found[0]['body']):raise ValueError('Complete TypeQuery native default missing')
    return ['10de40']
def retag_specialized(source,output):
    found=authenticate(source)
    if found is None or not entry_prefix(found[0]['body']):return output
    values=parts(output);old=entry_prefix(found[0]['body']);current=entry_prefix(values['body'],False)
    if current!=old:raise ValueError('Specializer moved or altered the authenticated TypeQuery prefix')
    corrected=output.replace(values['body'],values['body'].replace(current,prefix(callback_format(values['body'])),1),1)
    installed_entries(corrected);return corrected
