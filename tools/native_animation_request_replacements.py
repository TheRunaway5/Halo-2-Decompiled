"""Bind the authenticated TU1.5 animation request and its original children."""
import hashlib
import re

BODY = re.compile(r'^static void run_113d30\(WorldState &s\) \{.*?^\}(?=\n\n)', re.M | re.S)
CANONICAL = '09619117aa0cd7d05004e7fc5cc71fa1e19e1f93c3eeeca87f2d1211c687bf5e'
PREPARED = '25e13564a848194df00d7997abada28561d53bba29de25b6454ef541b77150a5'
HEADER = 'gameplay/animation/native_tu15_animation_request.hpp'
INCLUDE = f'#include "{HEADER}"\n'
DETAIL = 'halo2::native_animation_request'
DISPATCH = 'case 0x113d30u:run_113d30(s);break;'
PUSH = re.compile(r'\bs\.push\((0x[0-9a-f]+u,)([^;\n]+)\);')
DIRECT = re.compile(r's\.push\(0x([0-9a-f]+)u,0x([0-9a-f]+)u\);run_([0-9a-f]+)\(s\);')
ROUTED = re.compile(r'\{const auto target=0x([0-9a-f]+)u;s\.push\(0x([0-9a-f]+)u,0x([0-9a-f]+)u\);'
                    r'dispatch\(s,target,0x\2u,false\);\}')
MODIFY = re.compile(r'\bw\(s\.memory,s\.observe,(0x[0-9a-f]+u),(\(0x[0-9a-f]+u\+s\.gp\[\d\]\)),(\d),'
                    r's\.flags\.logic\(\(r\(s\.memory,\2,\3\)\)([|&])\((.*?)\),\3\)\);')


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def routes(body):
    direct = [(int(pc,16),int(resume,16),int(target,16),False) for pc,resume,target in DIRECT.findall(body)]
    routed = [(int(pc,16),int(resume,16),int(target,16),True) for target,pc,resume in ROUTED.findall(body)]
    entries = sorted(direct+routed)
    if (len(direct),len(routed)) != (36,11) or any(pc+5 != resume for pc,resume,_,_ in entries):
        raise ValueError('Prepared 113D30 call sites changed')
    return entries


def retained(body):
    image = 'if(s.image==SourcePhysicsContactImage::TitleUpdate15)'
    body, pushes = PUSH.subn(
        lambda m: image+f'halo2::native_two_bone_solve::push_word(s,{m[1]}{m[2]});else '+m[0],
        body,
    )

    def atomic_write(match):
        pc,address,width,operator,value=match.groups()
        mask,bits=('0xffffffffu',value) if operator=='|' else (value,'0u')
        return image+f'{DETAIL}::update_field(s,{pc},{address},{width},{mask},{bits});else '+match[0]

    body, updates = MODIFY.subn(atomic_write,body)
    if (pushes,updates)!=(107,7):
        raise ValueError(f'Prepared 113D30 memory commits changed: {(pushes,updates)}')
    return body


def hook(body):
    direct = sorted({target for _,_,target,required in routes(body) if not required})
    cases = ''.join(f'        case 0x{target:x}u:run_{target:x}(v);return;\n' for target in direct)
    return ('\n    if('+DETAIL+'::bridge(s,[](auto &v,std::uint32_t target,std::uint32_t site){\n'
            '        switch(target){\n'+cases+
            '        default:dispatch(v,target,site,false);return;\n'
            '        }\n    }))return;\n')


def install(canonical, prepared):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return prepared,[]
    raw = list(BODY.finditer(canonical))
    known = list(BODY.finditer(prepared))
    if len(raw)!=1 or digest(raw[0][0])!=CANONICAL or len(known)!=1 or digest(known[0][0])!=PREPARED:
        raise ValueError('Authenticated 113D30 body changed')
    if canonical.count(DISPATCH)!=1 or prepared.count(DISPATCH)!=1 or INCLUDE in prepared:
        raise ValueError('Authenticated 113D30 dispatcher changed')
    match = known[0]
    original_routes=routes(raw[0][0])
    if routes(match[0])!=original_routes:
        raise ValueError('Authenticated 113D30 child routing changed')
    for target in {target for _,_,target,required in original_routes if not required}:
        if canonical.count(f'static void run_{target:x}(WorldState &s) {{')!=1:
            raise ValueError(f'Authenticated 113D30 child {target:x} disappeared')
    body=retained(match[0])
    opening=body.index('{')+1
    return INCLUDE+prepared[:match.start()]+body[:opening]+hook(match[0])+body[opening:]+prepared[match.end():],['113d30']
