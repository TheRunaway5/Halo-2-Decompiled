"""Exact actual313790 raw/prepared source overlays, including source spill policy."""
import hashlib,re
BODY=re.compile(r'static void run_313790\(WorldState &s\) \{.*?\n\}',re.S)
ESTIMATE=re.compile(r'std::uint32_t source_estimate\(WorldState &s,std::uint32_t input\) \{.*?\n\}',re.S)
ESTIMATE_PIN='bb50fdcd1d22250d871032ff227a462ef42c30981de20b9f23137bb1c60efd89'
HELPER=re.compile(r'std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?\n\}',re.S)
STATE=re.compile(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',re.S)
STATE_PINS=('086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575')
HELPER_PIN='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1'
INCLUDE='#include "physics/runtime/native_tu15_solver_hull_basis_source_owner.hpp"\n'
INCLUDE_ANCHOR='#include "halo2/source_first_person_action_tu15.hpp"\n'
OWNER_MARKER='native_solver_hull_basis_owner_detail'
RESUME_MARKER='native_hull_basis_resume_'
RAW_CONTEXTS=(('929233cfbc4586134b4d9b0602b4b3738cfa21228f6ac73afc265dedf268f59b', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'), ('adf5489d917fbee14c3451428865c93c49553026f59faf009e2a358e5865994b', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'))
PREPARED_CONTEXTS=(('c7b2f71ad9eed5477f830ce2506e3e4e638bacf1467657a0c78a260155a6fd0e', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'), ('91c802bfc52a87ad7106609b33ee969a25e2284444e5491f5060f60115e3fdb7', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'))
FRONTIERS=((3225488, 3225489), (3225500, 3225501), (3225503, 3225506), (3225506, 3225510), (3225510, 3225513), (3225516, 3225521), (3225521, 3225525), (3225529, 3225533), (3225535, 3225536), (3225536, 3225540), (3225540, 3225544), (3225556, 3225560), (3225573, 3225577), (3225582, 3225586), (3225604, 3225608), (3225608, 3225612), (3225628, 3225636), (3225636, 3225640), (3225640, 3225644), (3225644, 3225649), (3225702, 3225705), (3225712, 3225717), (3225717, 3225721), (3225721, 3225727), (3225727, 3225732), (3225740, 3225748), (3225748, 3225754), (3225761, 3225769), (3225769, 3225775), (3225813, 3225818), (3225832, 3225837), (3225837, 3225841), (3225841, 3225847), (3225847, 3225852), (3225868, 3225869), (3225880, 3225881), (3225892, 3225893), (3225895, 3225900), (3225900, 3225905), (3225905, 3225908), (3225915, 3225916), (3225916, 3225917), (3225921, 3225922), (3225924, 3225929), (3225929, 3225933), (3225933, 3225940), (3225951, 3225952), (3225954, 3225959), (3225959, 3225960), (3225960, 3225961), (3225963, 3225964), (3225972, 3225973), (3225975, 3225980), (3225980, 3225981), (3225981, 3225982), (3225984, 3225985))
LABELS={after:'native_hull_basis_resume_'+format(after,'x')+': 'for _,after in FRONTIERS}
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def prefix(prepared,fresh):
    spill='[](auto&v){return native_single_spill(v.fp.st(0),v.fp.status);}'if fresh else'[](auto&v){return halo2::physics_step_detail::u(halo2::physics_step_detail::spill(v.fp.st(0)));}'
    estimate='[](auto&v,std::uint32_t bits){return source_estimate(v,bits);}'
    children='[&](auto&v,std::uint32_t target,std::uint32_t site){if(target==0x313470u){run_313470(v);return;}if(target==0x313290u){run_313290(v);return;}throw std::logic_error("Different hull-basis native child");}'
    return '\n    switch(halo2::native_solver_hull_basis_owner_detail::bridge<'+('true'if prepared else'false')+','+('true'if fresh else'false')+'>(s,'+spill+','+estimate+','+children+')){case 0u:break;case 1u:return;'+''.join('case 0x'+format(pc,'x')+'u:goto '+label[:-2]+';'for pc,label in LABELS.items())+'default:throw std::logic_error("Unsupported native hull-basis continuation");}'
PREFIXES={(prepared,fresh):prefix(prepared,fresh)for prepared in(False,True)for fresh in(False,True)}
def instruction(pc,prepared):
    return 's.instruction_literal(0x'+format(pc,'x')+'u);'if prepared else'if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
def overlay(body,prepared,fresh):
    for pc,label in LABELS.items():
        needle=instruction(pc,prepared)
        if body.count(needle)!=1:raise ValueError('Hull-basis authored continuation instruction drift')
        body=body.replace(needle,label+needle,1)
    opening=body.index('{')+1
    return body[:opening]+PREFIXES[prepared,fresh]+body[opening:]
def split_body(body,intermediate=False):
    found=[key for key,value in PREFIXES.items()if value in body]
    if not found:
        if RESUME_MARKER in body or OWNER_MARKER in body:raise ValueError('Partial hull-basis overlay')
        return body,None
    if len(found)!=1:raise ValueError('Ambiguous hull-basis source profile')
    key=found[0];prepared,fresh=key;value=PREFIXES[key];opening=body.index('{')+1
    if not body.startswith(value,opening)or body.count(value)!=1:raise ValueError('Hull-basis prefix moved from exact opening')
    result=body[:opening]+body[opening+len(value):]
    source_format=True if intermediate else prepared
    for pc,label in LABELS.items():
        needle=instruction(pc,source_format)
        if result.count(label+needle)!=1:raise ValueError('Hull-basis label moved from authored instruction')
        result=result.replace(label,'',1)
    expected=(PREPARED_CONTEXTS if source_format else RAW_CONTEXTS)[int(fresh)][0]
    if digest(result)!=expected:raise ValueError('Complete hull-basis body/source-format/spill profile drift')
    if overlay(result,source_format,fresh).replace(PREFIXES[source_format,fresh],value,1)!=body:raise ValueError('Hull-basis exact overlay reconstruction drift')
    return result,key
def canonical_body(body):return split_body(body)[0]
def context(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing/ambiguous actual hull-basis owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete hull-basis context')
    return matches[0],resolver[0],dispatcher[0]
def helper(source,fresh):
    estimates=ESTIMATE.findall(source)
    if len(estimates)!=1 or digest(estimates[0])!=ESTIMATE_PIN:raise ValueError('Complete source-estimate helper drift')
    if fresh:
        values=HELPER.findall(source)
        if len(values)!=1 or digest(values[0])!=HELPER_PIN:raise ValueError('Complete native-single-spill source helper drift')
def validate(source,prepared_only=None,intermediate=False):
    match,resolver,dispatcher=context(source);body,key=split_body(match[0],intermediate)
    prepared=key[0]if key else False
    if prepared_only is not None and prepared!=prepared_only:raise ValueError('Wrong hull-basis source format')
    contexts=PREPARED_CONTEXTS if intermediate or prepared else RAW_CONTEXTS
    triple=tuple(digest(value)for value in(body,resolver,dispatcher))
    if triple not in contexts:raise ValueError('Complete hull-basis body/resolver/dispatcher drift')
    fresh=contexts.index(triple)==1
    if key is not None and key[1]!=fresh:raise ValueError('Wrong hull-basis source spill policy')
    states=STATE.findall(source)
    if len(states)!=1 or digest(states[0])!=STATE_PINS[int(fresh)]:raise ValueError('Complete hull-basis LiveState wrapper drift')
    helper(source,fresh)
    if source.count(INCLUDE)!=int(key is not None)or(key is not None and source.count(INCLUDE+INCLUDE_ANCHOR)!=1):raise ValueError('Incomplete hull-basis include ownership')
    if any(source.count(value)!=int(key==other)for other,value in PREFIXES.items()):raise ValueError('Foreign hull-basis prefix')
    if source.count(OWNER_MARKER)!=int(key is not None)or source.count(RESUME_MARKER)!=2*len(LABELS)*int(key is not None):raise ValueError('Foreign hull-basis owner or continuation marker')
    return match,body,key,fresh

def authenticate(source):
    if not BODY.search(source):
        if INCLUDE in source or OWNER_MARKER in source or RESUME_MARKER in source:raise ValueError('Missing actual hull-basis owner')
        return None
    return validate(source,False)
def replace_solver_hull_basis(source):
    found=authenticate(source)
    if found is None:return source
    match,body,key,fresh=found
    if key is not None:return source
    result=source[:match.start()]+overlay(body,False,fresh)+source[match.end():]
    if result.count(INCLUDE_ANCHOR)!=1:raise ValueError('Hull-basis preamble anchor drift')
    at=result.index(INCLUDE_ANCHOR)
    return result[:at]+INCLUDE+result[at:]
def remove_solver_hull_basis(source):
    found=authenticate(source)
    if found is None:return source
    match,body,_,_=found
    return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def rebind_prepared(source):
    match,body,key,fresh=validate(source,intermediate=True)
    if key is None:raise ValueError('Missing hull-basis prepared output prefix')
    result=overlay(body,True,fresh)
    return source[:match.start()]+result+source[match.end():]
def installed_entries(source):
    _,_,key,_=validate(source)
    if key is None:raise ValueError('Hull-basis default not installed')
    return ['313790']
