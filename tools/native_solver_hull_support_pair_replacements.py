"""Private full actual313390 context pins; virtual dispatch remains the owning source dispatcher."""
import hashlib
import re
OWNER_MARKER='native_solver_hull_support_pair_owner_detail'
RESUME_MARKER='native_support_pair_resume_'
BODY=re.compile(r'static void run_313390\(WorldState &s\) \{.*?\n\}',re.S)
PREFIX='\n    switch(halo2::native_solver_hull_support_pair_owner_detail::bridge(s,[&](auto&v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);})){case 0u:break;case 1u:return;case 0x313391u:goto native_support_pair_resume_313391;case 0x31339au:goto native_support_pair_resume_31339a;case 0x31339du:goto native_support_pair_resume_31339d;case 0x31339eu:goto native_support_pair_resume_31339e;case 0x31339fu:goto native_support_pair_resume_31339f;case 0x3133a2u:goto native_support_pair_resume_3133a2;case 0x3133a7u:goto native_support_pair_resume_3133a7;case 0x3133a9u:goto native_support_pair_resume_3133a9;case 0x3133aau:goto native_support_pair_resume_3133aa;case 0x3133abu:goto native_support_pair_resume_3133ab;case 0x3133aeu:goto native_support_pair_resume_3133ae;case 0x3133b2u:goto native_support_pair_resume_3133b2;case 0x3133b9u:goto native_support_pair_resume_3133b9;case 0x3133bcu:goto native_support_pair_resume_3133bc;case 0x3133bfu:goto native_support_pair_resume_3133bf;case 0x3133c3u:goto native_support_pair_resume_3133c3;case 0x3133c6u:goto native_support_pair_resume_3133c6;case 0x313419u:goto native_support_pair_resume_313419;case 0x313422u:goto native_support_pair_resume_313422;case 0x313424u:goto native_support_pair_resume_313424;case 0x313425u:goto native_support_pair_resume_313425;case 0x313428u:goto native_support_pair_resume_313428;case 0x31342bu:goto native_support_pair_resume_31342b;case 0x31342fu:goto native_support_pair_resume_31342f;case 0x313433u:goto native_support_pair_resume_313433;case 0x313441u:goto native_support_pair_resume_313441;case 0x313458u:goto native_support_pair_resume_313458;case 0x313465u:goto native_support_pair_resume_313465;case 0x313466u:goto native_support_pair_resume_313466;case 0x313467u:goto native_support_pair_resume_313467;case 0x313468u:goto native_support_pair_resume_313468;case 0x31346bu:goto native_support_pair_resume_31346b;default:throw std::logic_error("Unsupported native support-pair continuation");}'
INCLUDE='#include "physics/runtime/native_tu15_solver_hull_support_pair_source_owner.hpp"\n'
BODY_PIN='84b0e536ea240e5c8aeeaf372cc0db14667185c3bd622418fc78544e59fa70ef'
RESOLVER_PIN='1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4'
DISPATCHERS=('5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0','aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849')
PREPARED_CONTEXTS=(
    ('16c29c574425e5f9ba24635fc3b229408b7a51faad50e68499566bc58e72abfc',RESOLVER_PIN,'37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'),
    ('16c29c574425e5f9ba24635fc3b229408b7a51faad50e68499566bc58e72abfc',RESOLVER_PIN,'8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'),
)
INSTALLED_BODY_PINS=('2dd5eff18cc25250a8e381abc57fda8c19da2aa659f4fad9387ce57c89cdcec4','16c29c574425e5f9ba24635fc3b229408b7a51faad50e68499566bc58e72abfc')
FRONTIERS=((3224464, 3224465), (3224473, 3224474), (3224474, 3224477), (3224477, 3224478), (3224478, 3224479), (3224479, 3224482), (3224484, 3224487), (3224487, 3224489), (3224489, 3224490), (3224490, 3224491), (3224491, 3224494), (3224494, 3224498), (3224498, 3224505), (3224505, 3224508), (3224508, 3224511), (3224511, 3224515), (3224515, 3224518), (3224600, 3224601), (3224605, 3224610), (3224610, 3224612), (3224612, 3224613), (3224613, 3224616), (3224616, 3224619), (3224619, 3224623), (3224623, 3224627), (3224637, 3224641), (3224661, 3224664), (3224673, 3224677), (3224677, 3224678), (3224678, 3224679), (3224679, 3224680), (3224682, 3224683))
LABELS={3224465: 'native_support_pair_resume_313391: ', 3224474: 'native_support_pair_resume_31339a: ', 3224477: 'native_support_pair_resume_31339d: ', 3224478: 'native_support_pair_resume_31339e: ', 3224479: 'native_support_pair_resume_31339f: ', 3224482: 'native_support_pair_resume_3133a2: ', 3224487: 'native_support_pair_resume_3133a7: ', 3224489: 'native_support_pair_resume_3133a9: ', 3224490: 'native_support_pair_resume_3133aa: ', 3224491: 'native_support_pair_resume_3133ab: ', 3224494: 'native_support_pair_resume_3133ae: ', 3224498: 'native_support_pair_resume_3133b2: ', 3224505: 'native_support_pair_resume_3133b9: ', 3224508: 'native_support_pair_resume_3133bc: ', 3224511: 'native_support_pair_resume_3133bf: ', 3224515: 'native_support_pair_resume_3133c3: ', 3224518: 'native_support_pair_resume_3133c6: ', 3224601: 'native_support_pair_resume_313419: ', 3224610: 'native_support_pair_resume_313422: ', 3224612: 'native_support_pair_resume_313424: ', 3224613: 'native_support_pair_resume_313425: ', 3224616: 'native_support_pair_resume_313428: ', 3224619: 'native_support_pair_resume_31342b: ', 3224623: 'native_support_pair_resume_31342f: ', 3224627: 'native_support_pair_resume_313433: ', 3224641: 'native_support_pair_resume_313441: ', 3224664: 'native_support_pair_resume_313458: ', 3224677: 'native_support_pair_resume_313465: ', 3224678: 'native_support_pair_resume_313466: ', 3224679: 'native_support_pair_resume_313467: ', 3224680: 'native_support_pair_resume_313468: ', 3224683: 'native_support_pair_resume_31346b: '}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    at=body.index('{')+1
    if not body[at:].startswith(PREFIX):return body
    # Count-only stripping would authenticate a relocated goto target. Pin the
    # complete authored installed body before removing any continuation label.
    if digest(body)not in INSTALLED_BODY_PINS:raise ValueError('Complete support-pair installed body or continuation placement drift')
    result=body[:at]+body[at+len(PREFIX):]
    for label in LABELS.values():
        if result.count(label)!=1:raise ValueError('Incomplete support-pair continuation labels')
        result=result.replace(label,'',1)
    return result
def context(source):
    bodies=list(BODY.finditer(source))
    if len(bodies)!=1:raise ValueError('Missing or ambiguous actual solver-hull support-pair owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete solver-hull support-pair context')
    return bodies[0],resolver[0],dispatcher[0]
def require_owned_markers(source,installed):
    if source.count(OWNER_MARKER)!=int(installed) or source.count(RESUME_MARKER)!=2*len(LABELS)*int(installed):raise ValueError('Foreign solver-hull support-pair owner or continuation marker')
def authenticate(source):
    if not BODY.search(source):
        if INCLUDE in source or OWNER_MARKER in source or RESUME_MARKER in source:raise ValueError('Missing actual solver-hull support-pair owner')
        return None
    match,resolver,dispatcher=context(source);body=canonical_body(match[0])
    if digest(body)!=BODY_PIN or digest(resolver)!=RESOLVER_PIN or digest(dispatcher)not in DISPATCHERS:
        raise ValueError('Complete solver-hull support-pair body/resolver/dispatcher drift')
    if source.count(INCLUDE)>1 or(PREFIX in match[0])!=(INCLUDE in source):raise ValueError('Incomplete solver-hull support-pair overlay')
    require_owned_markers(source,PREFIX in match[0])
    if 'selected('in body:raise ValueError('Actual solver-hull support-pair literal identity drift')
    return match,body
def replace_solver_hull_support_pair(source):
    found=authenticate(source)
    if not found:return source
    match,body=found;installed=body
    for pc,label in LABELS.items():
        needle='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        if installed.count(needle)!=1:raise ValueError('Support-pair continuation site drift')
        installed=installed.replace(needle,label+needle,1)
    at=installed.index('{')+1;value=source[:match.start()]+installed[:at]+PREFIX+installed[at:]+source[match.end():]
    return value if INCLUDE in value else INCLUDE+value
def remove_solver_hull_support_pair(source):
    found=authenticate(source)
    if not found:return source
    match,body=found;return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    match,resolver,dispatcher=context(source)
    if source.count(INCLUDE)!=1 or match[0].count(PREFIX)!=1:raise ValueError('Missing actual solver-hull support-pair overlay')
    require_owned_markers(source,True)
    if tuple(digest(value)for value in(match[0],resolver,dispatcher))not in PREPARED_CONTEXTS:authenticate(source)
    return['313390']
