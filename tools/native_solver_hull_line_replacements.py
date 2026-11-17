"""Exact actual3136a0 raw/prepared source overlays, including the exact FLD C1 policy."""
import hashlib,re
OWNER_MARKER='native_solver_hull_line_owner_detail'
RESUME_MARKER='native_hull_line_resume_'
BODY=re.compile(r'static void run_3136a0\(WorldState &s\) \{.*?\n\}',re.S)
STATE=re.compile(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',re.S)
STATE_PINS=('086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575')
INCLUDE='#include "physics/runtime/native_tu15_solver_hull_line_source_owner.hpp"\n'
INCLUDE_ANCHOR='#include "physics/runtime/native_tu15_solver_hull_initialize_source_owner.hpp"\n'
RAW_CONTEXTS=(('be94c9aeae3665bdf27dcb043494f38b36637e7d7456fc19341eb4c3c064e664', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'), ('593b692fff8cbed0efb46dff77b165ea499c31f56b88658b2a89d2372c59ef10', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'))
PREPARED_CONTEXTS=(('83f82ad0d7552e134a1d842021e3db5ea06c0634a228e0ad81e21e5a576a7954', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'), ('bec07aec018b9df6488650aec6bd67d4e324274ef00877c4d50b9887d9851898', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'))
FRONTIERS=((3225248, 3225249), (3225257, 3225258), (3225258, 3225259), (3225261, 3225264), (3225268, 3225269), (3225269, 3225270), (3225270, 3225275), (3225275, 3225278), (3225278, 3225281), (3225281, 3225286), (3225286, 3225291), (3225325, 3225332), (3225336, 3225339), (3225339, 3225340), (3225348, 3225349), (3225354, 3225358), (3225358, 3225363), (3225363, 3225368), (3225368, 3225372), (3225372, 3225377), (3225377, 3225380), (3225380, 3225385), (3225385, 3225388), (3225426, 3225430), (3225430, 3225434), (3225445, 3225446), (3225448, 3225453), (3225453, 3225454), (3225454, 3225455), (3225457, 3225458), (3225463, 3225464), (3225466, 3225471), (3225471, 3225472), (3225472, 3225473), (3225475, 3225476))
LABELS={after:'native_hull_line_resume_'+format(after,'x')+': 'for _,after in FRONTIERS}
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def prefix(prepared,fresh):
    children='[&](auto&v,std::uint32_t target,std::uint32_t site){if(target==0x313390u){run_313390(v);return;}if(target==0x313290u){run_313290(v);return;}throw std::logic_error("Different hull-line native child");}'
    return '\n    switch(halo2::native_solver_hull_line_owner_detail::bridge<'+('true'if prepared else'false')+','+('true'if fresh else'false')+'>(s,'+children+')){case 0u:break;case 1u:return;'+''.join('case 0x'+format(pc,'x')+'u:goto '+label[:-2]+';'for pc,label in LABELS.items())+'default:throw std::logic_error("Unsupported native hull-line continuation");}'
PREFIXES={(prepared,fresh):prefix(prepared,fresh)for prepared in(False,True)for fresh in(False,True)}
def instruction(pc,prepared):
    return 's.instruction_literal(0x'+format(pc,'x')+'u);'if prepared else'if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
def overlay(body,prepared,fresh):
    for pc,label in LABELS.items():
        needle=instruction(pc,prepared)
        if body.count(needle)!=1:raise ValueError('Hull-line authored continuation instruction drift')
        body=body.replace(needle,label+needle,1)
    opening=body.index('{')+1
    return body[:opening]+PREFIXES[prepared,fresh]+body[opening:]
def split_body(body,intermediate=False):
    found=[key for key,value in PREFIXES.items()if value in body]
    if not found:
        if 'native_hull_line_resume_'in body or'native_solver_hull_line_owner_detail'in body:raise ValueError('Partial hull-line overlay')
        return body,None
    if len(found)!=1:raise ValueError('Ambiguous hull-line source profile')
    key=found[0];prepared,fresh=key;value=PREFIXES[key];opening=body.index('{')+1
    if not body.startswith(value,opening)or body.count(value)!=1:raise ValueError('Hull-line prefix moved from exact opening')
    result=body[:opening]+body[opening+len(value):]
    source_format=True if intermediate else prepared
    for pc,label in LABELS.items():
        needle=instruction(pc,source_format)
        if result.count(label+needle)!=1:raise ValueError('Hull-line label moved from authored instruction')
        result=result.replace(label,'',1)
    expected=(PREPARED_CONTEXTS if source_format else RAW_CONTEXTS)[int(fresh)][0]
    if digest(result)!=expected:raise ValueError('Complete hull-line body/source-format/FLD C1 profile drift')
    if overlay(result,source_format,fresh).replace(PREFIXES[source_format,fresh],value,1)!=body:raise ValueError('Hull-line exact overlay reconstruction drift')
    return result,key
def canonical_body(body):return split_body(body)[0]
def context(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing/ambiguous actual hull-line owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete hull-line context')
    return matches[0],resolver[0],dispatcher[0]
def validate(source,prepared_only=None,intermediate=False):
    match,resolver,dispatcher=context(source);body,key=split_body(match[0],intermediate)
    prepared=key[0]if key else False
    if prepared_only is not None and prepared!=prepared_only:raise ValueError('Wrong hull-line source format')
    contexts=PREPARED_CONTEXTS if intermediate or prepared else RAW_CONTEXTS
    triple=tuple(digest(value)for value in(body,resolver,dispatcher))
    if triple not in contexts:raise ValueError('Complete hull-line body/resolver/dispatcher drift')
    fresh=contexts.index(triple)==1
    if key is not None and key[1]!=fresh:raise ValueError('Wrong hull-line source FLD C1 policy')
    states=STATE.findall(source)
    if len(states)!=1 or digest(states[0])!=STATE_PINS[int(fresh)]:raise ValueError('Complete hull-line LiveState wrapper drift')
    if source.count(INCLUDE)!=int(key is not None)or(key is not None and source.count(INCLUDE+INCLUDE_ANCHOR)!=1):raise ValueError('Incomplete hull-line include ownership')
    if any(source.count(value)!=int(key==other)for other,value in PREFIXES.items()):raise ValueError('Foreign hull-line prefix')
    if source.count(OWNER_MARKER)!=int(key is not None) or source.count(RESUME_MARKER)!=2*len(LABELS)*int(key is not None):raise ValueError('Foreign hull-line owner or continuation marker')
    return match,body,key,fresh

def authenticate(source):
    if not BODY.search(source):
        if INCLUDE in source or OWNER_MARKER in source or RESUME_MARKER in source:raise ValueError('Missing actual hull-line owner')
        return None
    return validate(source,False)
def replace_solver_hull_line(source):
    found=authenticate(source)
    if found is None:return source
    match,body,key,fresh=found
    if key is not None:return source
    result=source[:match.start()]+overlay(body,False,fresh)+source[match.end():]
    if result.count(INCLUDE_ANCHOR)!=1:raise ValueError('Hull-line preamble anchor drift')
    at=result.index(INCLUDE_ANCHOR)
    return result[:at]+INCLUDE+result[at:]
def remove_solver_hull_line(source):
    found=authenticate(source)
    if found is None:return source
    match,body,_,_=found
    return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def rebind_prepared(source):
    match,body,key,fresh=validate(source,intermediate=True)
    if key is None:raise ValueError('Missing hull-line prepared output prefix')
    result=overlay(body,True,fresh)
    return source[:match.start()]+result+source[match.end():]
def installed_entries(source):
    _,_,key,_=validate(source)
    if key is None:raise ValueError('Hull-line default not installed')
    return ['3136a0']
