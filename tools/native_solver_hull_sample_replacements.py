"""Exact actual313300 raw/prepared source overlays, including source spill policy."""
import hashlib,re
OWNER_MARKER='native_solver_hull_sample_owner_detail'
RESUME_MARKER='native_hull_sample_resume_'
BODY=re.compile(r'static void run_313300\(WorldState &s\) \{.*?\n\}',re.S)
HELPER=re.compile(r'std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?\n\}',re.S)
STATE=re.compile(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',re.S)
STATE_PINS=('086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575')
HELPER_PIN='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1'
INCLUDE='#include "physics/runtime/native_tu15_solver_hull_sample_source_owner.hpp"\n'
INCLUDE_ANCHOR='#include "halo2/source_vehicle_input_tu15.hpp"\n'
RAW_CONTEXTS=(('abb1e9c9f80b2d38224c12612dc1fbde9091798d3579728bcd998980f7264703', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'), ('91402da6613cb2690b9e5ed94da0739e35c0e51f178af36c5ef27c0d5baeb70a', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'))
PREPARED_CONTEXTS=(('e3cf3851190da4d8751eb8fe1231094a038b4712b19c2302730ce9916f9c5324', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'), ('3631b251739862bf63b8e9c4caffe48d3d8b11265aa05afdde01bb7b9b371f4d', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'))
FRONTIERS=((3224320, 3224324), (3224324, 3224328), (3224328, 3224332), (3224338, 3224342), (3224354, 3224358), (3224364, 3224368), (3224368, 3224374), (3224379, 3224383), (3224393, 3224397), (3224403, 3224405), (3224405, 3224409), (3224409, 3224415), (3224420, 3224424), (3224433, 3224437), (3224437, 3224440), (3224440, 3224444), (3224444, 3224450), (3224450, 3224454), (3224456, 3224459))
LABELS={after:'native_hull_sample_resume_'+format(after,'x')+': 'for _,after in FRONTIERS}
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def prefix(prepared,fresh):
    spill='[](auto&v){return native_single_spill(v.fp.st(0),v.fp.status);}'if fresh else'[](auto&v){return halo2::physics_step_detail::u(halo2::physics_step_detail::spill(v.fp.st(0)));}'
    return '\n    switch(halo2::native_solver_hull_sample_owner_detail::bridge<'+('true'if prepared else'false')+','+('true'if fresh else'false')+'>(s,'+spill+')){case 0u:break;case 1u:return;'+''.join('case 0x'+format(pc,'x')+'u:goto '+label[:-2]+';'for pc,label in LABELS.items())+'default:throw std::logic_error("Unsupported native hull-sample continuation");}'
PREFIXES={(prepared,fresh):prefix(prepared,fresh)for prepared in(False,True)for fresh in(False,True)}
def instruction(pc,prepared):
    return 's.instruction_literal(0x'+format(pc,'x')+'u);'if prepared else'if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
def overlay(body,prepared,fresh):
    for pc,label in LABELS.items():
        needle=instruction(pc,prepared)
        if body.count(needle)!=1:raise ValueError('Hull-sample authored continuation instruction drift')
        body=body.replace(needle,label+needle,1)
    opening=body.index('{')+1
    return body[:opening]+PREFIXES[prepared,fresh]+body[opening:]
def split_body(body,intermediate=False):
    found=[key for key,value in PREFIXES.items()if value in body]
    if not found:
        if 'native_hull_sample_resume_'in body or'native_solver_hull_sample_owner_detail'in body:raise ValueError('Partial hull-sample overlay')
        return body,None
    if len(found)!=1:raise ValueError('Ambiguous hull-sample source profile')
    key=found[0];prepared,fresh=key;value=PREFIXES[key];opening=body.index('{')+1
    if not body.startswith(value,opening)or body.count(value)!=1:raise ValueError('Hull-sample prefix moved from exact opening')
    result=body[:opening]+body[opening+len(value):]
    source_format=True if intermediate else prepared
    for pc,label in LABELS.items():
        needle=instruction(pc,source_format)
        if result.count(label+needle)!=1:raise ValueError('Hull-sample label moved from authored instruction')
        result=result.replace(label,'',1)
    expected=(PREPARED_CONTEXTS if source_format else RAW_CONTEXTS)[int(fresh)][0]
    if digest(result)!=expected:raise ValueError('Complete hull-sample body/source-format/spill profile drift')
    if overlay(result,source_format,fresh).replace(PREFIXES[source_format,fresh],value,1)!=body:raise ValueError('Hull-sample exact overlay reconstruction drift')
    return result,key
def canonical_body(body):return split_body(body)[0]
def context(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing/ambiguous actual hull-sample owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete hull-sample context')
    return matches[0],resolver[0],dispatcher[0]
def helper(source,fresh):
    if fresh:
        values=HELPER.findall(source)
        if len(values)!=1 or digest(values[0])!=HELPER_PIN:raise ValueError('Complete native-single-spill source helper drift')
def validate(source,prepared_only=None,intermediate=False):
    match,resolver,dispatcher=context(source);body,key=split_body(match[0],intermediate)
    prepared=key[0]if key else False
    if prepared_only is not None and prepared!=prepared_only:raise ValueError('Wrong hull-sample source format')
    contexts=PREPARED_CONTEXTS if intermediate or prepared else RAW_CONTEXTS
    triple=tuple(digest(value)for value in(body,resolver,dispatcher))
    if triple not in contexts:raise ValueError('Complete hull-sample body/resolver/dispatcher drift')
    fresh=contexts.index(triple)==1
    if key is not None and key[1]!=fresh:raise ValueError('Wrong hull-sample source spill policy')
    states=STATE.findall(source)
    if len(states)!=1 or digest(states[0])!=STATE_PINS[int(fresh)]:raise ValueError('Complete hull-sample LiveState wrapper drift')
    helper(source,fresh)
    if source.count(INCLUDE)!=int(key is not None)or(key is not None and source.count(INCLUDE+INCLUDE_ANCHOR)!=1):raise ValueError('Incomplete hull-sample include ownership')
    if any(source.count(value)!=int(key==other)for other,value in PREFIXES.items()):raise ValueError('Foreign hull-sample prefix')
    if source.count(OWNER_MARKER)!=int(key is not None) or source.count(RESUME_MARKER)!=2*len(LABELS)*int(key is not None):raise ValueError('Foreign hull-sample owner or continuation marker')
    return match,body,key,fresh

def authenticate(source):
    if not BODY.search(source):
        if INCLUDE in source or OWNER_MARKER in source or RESUME_MARKER in source:raise ValueError('Missing actual hull-sample owner')
        return None
    return validate(source,False)
def replace_solver_hull_sample(source):
    found=authenticate(source)
    if found is None:return source
    match,body,key,fresh=found
    if key is not None:return source
    result=source[:match.start()]+overlay(body,False,fresh)+source[match.end():]
    if result.count(INCLUDE_ANCHOR)!=1:raise ValueError('Hull-sample preamble anchor drift')
    at=result.index(INCLUDE_ANCHOR)
    return result[:at]+INCLUDE+result[at:]
def remove_solver_hull_sample(source):
    found=authenticate(source)
    if found is None:return source
    match,body,_,_=found
    return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def rebind_prepared(source):
    match,body,key,fresh=validate(source,intermediate=True)
    if key is None:raise ValueError('Missing hull-sample prepared output prefix')
    result=overlay(body,True,fresh)
    return source[:match.start()]+result+source[match.end():]
def installed_entries(source):
    _,_,key,_=validate(source)
    if key is None:raise ValueError('Hull-sample default not installed')
    return ['313300']
