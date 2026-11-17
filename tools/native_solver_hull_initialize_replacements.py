"""Exact actual313560 raw/prepared source overlays, including source spill policy."""
import hashlib,re
OWNER_MARKER='native_solver_hull_initialize_owner_detail'
RESUME_MARKER='native_hull_initialize_resume_'
BODY=re.compile(r'static void run_313560\(WorldState &s\) \{.*?\n\}',re.S)
HELPER=re.compile(r'std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?\n\}',re.S)
STATE=re.compile(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',re.S)
STATE_PINS=('086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575')
HELPER_PIN='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1'
INCLUDE='#include "physics/runtime/native_tu15_solver_hull_initialize_source_owner.hpp"\n'
RAW_CONTEXTS=(('474f03b91b22890d68e62c38e2397369437b7e93d699150091907b02a8ba82d7', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'), ('56075468bf5123e211ba5bac6ed106d71191dbd4a4e72b307a7ffe5f8debaccb', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'))
PREPARED_CONTEXTS=(('e5816fcaa3baf4378b58833b9fdf7ff025cb51c75f7dddc16c630e952058c02d', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'), ('eda02e2ab26aa9d6771477be503163a704cce80b7d9538bbe0b9d4ab1418364d', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'))
FRONTIERS=((3224928, 3224929), (3224937, 3224944), (3224944, 3224945), (3224947, 3224951), (3224954, 3224959), (3224988, 3224992), (3224992, 3224996), (3224996, 3225002), (3225009, 3225017), (3225017, 3225025), (3225025, 3225033), (3225033, 3225041), (3225045, 3225046), (3225050, 3225051), (3225053, 3225058), (3225062, 3225063), (3225065, 3225070), (3225070, 3225073), (3225073, 3225076), (3225105, 3225109), (3225109, 3225113), (3225113, 3225119), (3225126, 3225129), (3225129, 3225132), (3225160, 3225165), (3225169, 3225175), (3225175, 3225180), (3225184, 3225188), (3225188, 3225192), (3225192, 3225197), (3225197, 3225200), (3225200, 3225203), (3225203, 3225208), (3225208, 3225212), (3225212, 3225219), (3225219, 3225222), (3225222, 3225225), (3225225, 3225226), (3225228, 3225229), (3225232, 3225235), (3225235, 3225238), (3225238, 3225239), (3225241, 3225242))
LABELS={after:'native_hull_initialize_resume_'+format(after,'x')+': 'for _,after in FRONTIERS}
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def prefix(prepared,fresh):
    children='[&](auto&v,std::uint32_t target,std::uint32_t site){if(target==0x313390u){run_313390(v);return;}if(target==0x313290u){run_313290(v);return;}throw std::logic_error("Different hull-initialize native child");}'
    spill='[](auto&v){return native_single_spill(v.fp.st(0),v.fp.status);}'if fresh else'[](auto&v){return halo2::physics_step_detail::u(halo2::physics_step_detail::spill(v.fp.st(0)));}'
    return '\n    switch(halo2::native_solver_hull_initialize_owner_detail::bridge<'+('true'if prepared else'false')+'>(s,'+children+','+spill+')){case 0u:break;case 1u:return;'+''.join('case 0x'+format(pc,'x')+'u:goto '+label[:-2]+';'for pc,label in LABELS.items())+'default:throw std::logic_error("Unsupported native hull-initialize continuation");}'
PREFIXES={(prepared,fresh):prefix(prepared,fresh)for prepared in(False,True)for fresh in(False,True)}
def instruction(pc,prepared):
    return 's.instruction_literal(0x'+format(pc,'x')+'u);'if prepared else'if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
def overlay(body,prepared,fresh):
    for pc,label in LABELS.items():
        needle=instruction(pc,prepared)
        if body.count(needle)!=1:raise ValueError('Hull-initialize authored continuation instruction drift')
        body=body.replace(needle,label+needle,1)
    opening=body.index('{')+1
    return body[:opening]+PREFIXES[prepared,fresh]+body[opening:]
def split_body(body,intermediate=False):
    found=[key for key,value in PREFIXES.items()if value in body]
    if not found:
        if 'native_hull_initialize_resume_'in body or'native_solver_hull_initialize_owner_detail'in body:raise ValueError('Partial hull-initialize overlay')
        return body,None
    if len(found)!=1:raise ValueError('Ambiguous hull-initialize source profile')
    key=found[0];prepared,fresh=key;value=PREFIXES[key];opening=body.index('{')+1
    if not body.startswith(value,opening)or body.count(value)!=1:raise ValueError('Hull-initialize prefix moved from exact opening')
    result=body[:opening]+body[opening+len(value):]
    source_format=True if intermediate else prepared
    for pc,label in LABELS.items():
        needle=instruction(pc,source_format)
        if result.count(label+needle)!=1:raise ValueError('Hull-initialize label moved from authored instruction')
        result=result.replace(label,'',1)
    expected=(PREPARED_CONTEXTS if source_format else RAW_CONTEXTS)[int(fresh)][0]
    if digest(result)!=expected:raise ValueError('Complete hull-initialize body/source-format/spill profile drift')
    if overlay(result,source_format,fresh).replace(PREFIXES[source_format,fresh],value,1)!=body:raise ValueError('Hull-initialize exact overlay reconstruction drift')
    return result,key
def canonical_body(body):return split_body(body)[0]
def context(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing/ambiguous actual hull-initialize owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete hull-initialize context')
    return matches[0],resolver[0],dispatcher[0]
def helper(source,fresh):
    if fresh:
        values=HELPER.findall(source)
        if len(values)!=1 or digest(values[0])!=HELPER_PIN:raise ValueError('Complete native-single-spill source helper drift')
def validate(source,prepared_only=None,intermediate=False):
    match,resolver,dispatcher=context(source);body,key=split_body(match[0],intermediate)
    prepared=key[0]if key else False
    if prepared_only is not None and prepared!=prepared_only:raise ValueError('Wrong hull-initialize source format')
    contexts=PREPARED_CONTEXTS if intermediate or prepared else RAW_CONTEXTS
    triple=tuple(digest(value)for value in(body,resolver,dispatcher))
    if triple not in contexts:raise ValueError('Complete hull-initialize body/resolver/dispatcher drift')
    fresh=contexts.index(triple)==1
    if key is not None and key[1]!=fresh:raise ValueError('Wrong hull-initialize source spill policy')
    states=STATE.findall(source)
    if len(states)!=1 or digest(states[0])!=STATE_PINS[int(fresh)]:raise ValueError('Complete hull-initialize LiveState wrapper drift')
    helper(source,fresh)
    if source.count(INCLUDE)!=int(key is not None):raise ValueError('Incomplete hull-initialize include ownership')
    if any(source.count(value)!=int(key==other)for other,value in PREFIXES.items()):raise ValueError('Foreign hull-initialize prefix')
    if source.count(OWNER_MARKER)!=int(key is not None) or source.count(RESUME_MARKER)!=2*len(LABELS)*int(key is not None):raise ValueError('Foreign hull-initialize owner or continuation marker')
    return match,body,key,fresh

def authenticate(source):
    if not BODY.search(source):
        if INCLUDE in source or OWNER_MARKER in source or RESUME_MARKER in source:raise ValueError('Missing actual hull-initialize owner')
        return None
    return validate(source,False)
def replace_solver_hull_initialize(source):
    found=authenticate(source)
    if found is None:return source
    match,body,key,fresh=found
    if key is not None:return source
    result=source[:match.start()]+overlay(body,False,fresh)+source[match.end():]
    if result.count('namespace halo2 {')!=1:raise ValueError('Hull-initialize global preamble anchor drift')
    at=result.index('namespace halo2 {')
    return result[:at]+INCLUDE+result[at:]
def remove_solver_hull_initialize(source):
    found=authenticate(source)
    if found is None:return source
    match,body,_,_=found
    return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def rebind_prepared(source):
    match,body,key,fresh=validate(source,intermediate=True)
    if key is None:raise ValueError('Missing hull-initialize prepared output prefix')
    result=overlay(body,True,fresh)
    return source[:match.start()]+result+source[match.end():]
def installed_entries(source):
    _,_,key,_=validate(source)
    if key is None:raise ValueError('Hull-initialize default not installed')
    return ['313560']
