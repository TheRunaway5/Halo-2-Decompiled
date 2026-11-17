"""Exact actual3147d0 raw/prepared source overlays, including source spill policy."""
import hashlib,re
BODY=re.compile(r'static void run_3147d0\(WorldState &s\) \{.*?\n\}',re.S)
HELPER=re.compile(r'std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?\n\}',re.S)
STATE=re.compile(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',re.S)
STATE_PINS=('086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575')
HELPER_PIN='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1'
INCLUDE='#include "physics/runtime/native_tu15_solver_hull_clip_source_owner.hpp"\n'
INCLUDE_ANCHOR='#include "halo2/source_unit_speech_channel_symbols_tu15.hpp"\n'
OWNER_MARKER='native_solver_hull_clip_owner_detail'
RESUME_MARKER='native_hull_clip_resume_'
RAW_CONTEXTS=(('fb1f686d30c823296eba36bf95bea60040458217bbde6e6c4b869dfb7d1f81fd', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'), ('047f77092e14911acabb44eb57e7f83024cba069793758d05d4d2811833bd071', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'))
PREPARED_CONTEXTS=(('224d863afba2663d3054e495da827c481b8d3681aace8efcf0ac74e881310049', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'), ('bc237b4c85c1cab29da301d375e1bdf37ec62edc054c771a584d16389a344137', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'))
FRONTIERS=((0x3147d0,0x3147d1), (0x3147d9,0x3147dc), (0x3147dc,0x3147df), (0x3147df,0x3147e2), (0x3147e2,0x3147e5), (0x3147e8,0x3147eb), (0x3147eb,0x3147ee), (0x31480b,0x31480f), (0x31480f,0x314813), (0x314813,0x314815), (0x314815,0x314816), (0x314816,0x314819), (0x314819,0x31481d), (0x314829,0x31482b), (0x314832,0x314836), (0x314836,0x314838), (0x31483a,0x314840), (0x314840,0x314842), (0x314849,0x31484b), (0x314854,0x314856), (0x314856,0x314858), (0x314866,0x314867), (0x314869,0x31486a))
LABELS={after:'native_hull_clip_resume_'+format(after,'x')+': 'for _,after in FRONTIERS}
ORDERED_PC=0x3147ee
LEGACY_MULTIPLY=' s.xmm[0]=_mm_mul_ps(s.xmm[0],s.xmm[1]);'
ORDERED_MULTIPLY=' s.xmm[0]=halo2::native_solver_hull_clip_source_detail::ordered_multiply(s.xmm[0],s.xmm[1]);'
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def prefix(prepared,fresh):
    spill='[](auto&v){return native_single_spill(v.fp.st(0),v.fp.status);}'if fresh else'[](auto&v){return halo2::physics_step_detail::u(halo2::physics_step_detail::spill(v.fp.st(0)));}'
    return '\n    switch(halo2::native_solver_hull_clip_owner_detail::bridge<'+('true'if prepared else'false')+','+('true'if fresh else'false')+'>(s,'+spill+')){case 0u:break;case 1u:return;'+''.join('case 0x'+format(pc,'x')+'u:goto '+label[:-2]+';'for pc,label in LABELS.items())+'default:throw std::logic_error("Unsupported native hull-clip continuation");}'
PREFIXES={(prepared,fresh):prefix(prepared,fresh)for prepared in(False,True)for fresh in(False,True)}
def instruction(pc,prepared):
    return 's.instruction_literal(0x'+format(pc,'x')+'u);'if prepared else'if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
def overlay(body,prepared,fresh):
    for pc,label in LABELS.items():
        needle=instruction(pc,prepared)
        if body.count(needle)!=1:raise ValueError('Hull-clip authored continuation instruction drift')
        body=body.replace(needle,label+needle,1)
    # The retained continuation replicates the original MULPS operand order.
    site=LABELS[ORDERED_PC]+instruction(ORDERED_PC,prepared)
    if body.count(site+LEGACY_MULTIPLY)!=1 or ORDERED_MULTIPLY in body:raise ValueError('Complete hull-clip body original MULPS drift')
    body=body.replace(site+LEGACY_MULTIPLY,site+ORDERED_MULTIPLY,1)
    opening=body.index('{')+1
    return body[:opening]+PREFIXES[prepared,fresh]+body[opening:]
def split_body(body,intermediate=False):
    found=[key for key,value in PREFIXES.items()if value in body]
    if not found:
        if RESUME_MARKER in body or OWNER_MARKER in body or ORDERED_MULTIPLY in body:raise ValueError('Partial hull-clip overlay')
        return body,None
    if len(found)!=1:raise ValueError('Ambiguous hull-clip source profile')
    key=found[0];prepared,fresh=key;value=PREFIXES[key];opening=body.index('{')+1
    if not body.startswith(value,opening)or body.count(value)!=1:raise ValueError('Hull-clip prefix moved from exact opening')
    result=body[:opening]+body[opening+len(value):]
    source_format=True if intermediate else prepared
    for pc,label in LABELS.items():
        needle=instruction(pc,source_format)
        if result.count(label+needle)!=1:raise ValueError('Hull-clip label moved from authored instruction')
        result=result.replace(label,'',1)
    site=instruction(ORDERED_PC,source_format)
    if result.count(ORDERED_MULTIPLY)!=1 or result.count(site+ORDERED_MULTIPLY)!=1:raise ValueError('Hull-clip ordered MULPS moved')
    result=result.replace(site+ORDERED_MULTIPLY,site+LEGACY_MULTIPLY,1)
    expected=(PREPARED_CONTEXTS if source_format else RAW_CONTEXTS)[int(fresh)][0]
    if digest(result)!=expected:raise ValueError('Complete hull-clip body/source-format/spill profile drift')
    if overlay(result,source_format,fresh).replace(PREFIXES[source_format,fresh],value,1)!=body:raise ValueError('Hull-clip exact overlay reconstruction drift')
    return result,key
def canonical_body(body):return split_body(body)[0]
def context(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing/ambiguous actual hull-clip owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete hull-clip context')
    return matches[0],resolver[0],dispatcher[0]
def helper(source,fresh):
    if fresh:
        values=HELPER.findall(source)
        if len(values)!=1 or digest(values[0])!=HELPER_PIN:raise ValueError('Complete native-single-spill source helper drift')
def validate(source,prepared_only=None,intermediate=False):
    match,resolver,dispatcher=context(source);body,key=split_body(match[0],intermediate)
    prepared=key[0]if key else False
    if prepared_only is not None and prepared!=prepared_only:raise ValueError('Wrong hull-clip source format')
    contexts=PREPARED_CONTEXTS if intermediate or prepared else RAW_CONTEXTS
    triple=tuple(digest(value)for value in(body,resolver,dispatcher))
    if triple not in contexts:raise ValueError('Complete hull-clip body/resolver/dispatcher drift')
    fresh=contexts.index(triple)==1
    if key is not None and key[1]!=fresh:raise ValueError('Wrong hull-clip source spill policy')
    states=STATE.findall(source)
    if len(states)!=1 or digest(states[0])!=STATE_PINS[int(fresh)]:raise ValueError('Complete hull-clip LiveState wrapper drift')
    helper(source,fresh)
    if source.count(INCLUDE)!=int(key is not None)or(key is not None and source.count(INCLUDE+INCLUDE_ANCHOR)!=1):raise ValueError('Incomplete hull-clip include ownership')
    if any(source.count(value)!=int(key==other)for other,value in PREFIXES.items()):raise ValueError('Foreign hull-clip prefix')
    if source.count(OWNER_MARKER)!=int(key is not None)or source.count(RESUME_MARKER)!=2*len(LABELS)*int(key is not None):raise ValueError('Foreign hull-clip owner or continuation marker')
    if source.count(ORDERED_MULTIPLY)!=int(key is not None):raise ValueError('Foreign hull-clip ordered MULPS')
    return match,body,key,fresh

def authenticate(source):
    if not BODY.search(source):
        if INCLUDE in source or OWNER_MARKER in source or RESUME_MARKER in source:raise ValueError('Missing actual hull-clip owner')
        return None
    return validate(source,False)
def replace_solver_hull_clip(source):
    found=authenticate(source)
    if found is None:return source
    match,body,key,fresh=found
    if key is not None:return source
    result=source[:match.start()]+overlay(body,False,fresh)+source[match.end():]
    if result.count(INCLUDE_ANCHOR)!=1:raise ValueError('Hull-clip preamble anchor drift')
    at=result.index(INCLUDE_ANCHOR)
    return result[:at]+INCLUDE+result[at:]
def remove_solver_hull_clip(source):
    found=authenticate(source)
    if found is None:return source
    match,body,_,_=found
    return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def replace_solver_hull_clip_prepared(source):
    match,body,key,fresh=validate(source,True) if prefix_markers_present(source) else validate(source,intermediate=True)
    if key is not None:return source
    result=source[:match.start()]+overlay(body,True,fresh)+source[match.end():]
    if result.count(INCLUDE_ANCHOR)!=1:raise ValueError('Hull-clip preamble anchor drift')
    at=result.index(INCLUDE_ANCHOR)
    return result[:at]+INCLUDE+result[at:]
def prefix_markers_present(source):
    return any(marker in source for marker in(INCLUDE,OWNER_MARKER,RESUME_MARKER))
def remove_solver_hull_clip_prepared(source):
    match,body,key,_=validate(source,True)
    if key is None:raise ValueError('Missing hull-clip prepared default')
    return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def rebind_prepared(source):
    match,body,key,fresh=validate(source,intermediate=True)
    if key is None:raise ValueError('Missing hull-clip prepared output prefix')
    result=overlay(body,True,fresh)
    return source[:match.start()]+result+source[match.end():]
def installed_entries(source):
    _,_,key,_=validate(source)
    if key is None:raise ValueError('Hull-clip default not installed')
    return ['3147d0']
