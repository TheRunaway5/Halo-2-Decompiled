"""Complete source/owner authentication for model marker lookup contexts."""
import hashlib,importlib.util,re
from pathlib import Path

def _sibling(name):
    spec=importlib.util.spec_from_file_location('_marker_'+name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

NON_CODE=re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',re.S)
FUNCTION=re.compile(r'^[ \t]*(?:\[\[[^\]]+\]\][ \t]*)?(?:(?:inline|static|constexpr)\s+)*(?:[\w:<>]+(?:\s*[&*])?\s+)+(?P<name>[\w:]+)\s*\((?P<parameters>[^;{}]*?)\)\s*\{',re.M)
def function(source,name):
    # Only the audited C++ owner grammar is needed here. Keep this admission
    # module importable by file path without importing CLI instrumentation or
    # modifying its caller's sys.path/module registry.
    mask=NON_CODE.sub(lambda match:' '*len(match[0]),source);found=[]
    for match in FUNCTION.finditer(mask):
        if match['name']!=name:continue
        depth=1
        for brace in re.finditer(r'[{}]',mask[match.end():]):
            depth+=1 if brace[0]=='{'else-1
            if not depth:found.append(source[match.start():match.end()+brace.end()]);break
    if len(found)!=1:raise ValueError('Expected one complete marker ownership function '+name)
    return found[0]
def digest(text):return hashlib.sha256(text.encode()).hexdigest()

def matches_effect_body(body, pin):
    if digest(body) == pin:
        return True
    # The accounting build added this guard to the identical literal callback.
    # Authenticate its position and the entire underlying body together.
    guarded = 'if(s.instruction&&!halo2::native_census_detail::skip_report(s,pc))s.instruction(pc);'
    return body.count(guarded) == 1 and digest(body.replace(
        guarded, 'if(s.instruction)s.instruction(pc);', 1)) == pin
INCLUDE='#include "gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp"\n'
BODY=re.compile(r'(?:\[\[gnu::noinline\]\] )?static void run_(1e11b0|1dc8f0)\((WorldState|MotionState|State) &s\) \{.*?\n\}',re.S)
CONTEXTS={
 ('1e9da052f012884648480efa4a25a6a87505871ce21f6c50b8fc0a5db61a96ce','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'):('actual','primary'),
 ('455760eb53cfbb209d956b2fa5edfc04088200476830c19b6b5bd61e6180411f','5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657','1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b'):('actual','ai'),
 ('455760eb53cfbb209d956b2fa5edfc04088200476830c19b6b5bd61e6180411f','618bc56800beb42be63e189783dd7cf9a98d7b904e56fa92de6523bf86490d7a','1a68d583722bac0a950594a436cbb9f1b28b09624517589c9ee8ad507712f605'):('actual','scene'),
 ('455760eb53cfbb209d956b2fa5edfc04088200476830c19b6b5bd61e6180411f','0fddb23fd6f15344aa915c1c50b845847dd34460782d49467474e9e1cfed5fed','961134517848136bf248393b533a030ae2607735d71fec8f570e0332e5ce4bc6'):('actual','sandbox'),
 ('8a29912f46c282fe00c3aae904a1e6af48a2397ada4919b03fac4a3766bbcffa','771300930484d824648b4efc363e270e680eae6c45ec6ff73b04312731cab022','256b7dcb6f209e0f424b18208ae7f67b530185f594c7805a947a3afc45736624'):('canonical','motion'),
 ('c2b56220968b2afa1dec6417c255296f6e5df48c3c00381a4e28497f8acc7d3e','fixed1.1','e1026953a2014c87e9830cc0e2699fedc508b629f3412f469213d57d2da26680'):('canonical','death-startup'),
 # Exact fresh complete contexts, separately replayed from genuine references.
 ('1e9da052f012884648480efa4a25a6a87505871ce21f6c50b8fc0a5db61a96ce','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'):('actual','primary'),
 ('1e9da052f012884648480efa4a25a6a87505871ce21f6c50b8fc0a5db61a96ce','b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8','974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a'):('actual','ai'),
 ('1e9da052f012884648480efa4a25a6a87505871ce21f6c50b8fc0a5db61a96ce','0fddb23fd6f15344aa915c1c50b845847dd34460782d49467474e9e1cfed5fed','d46a1635c618577534bfdd091f45ecea819b4886662ca1309400154271d5a987'):('actual','sandbox'),
}
DEATH_STATE_PIN='c76b02b00b991ca6af9f81e5ace29ec73d56985f69269c8d7c1498486a6e92a6'
DEATH_PUBLIC_PIN='1474c37a1ffb0fdfb7a23465497f660150b7224096cfc7dca329da77d279fc69'
EFFECT_BODY_PIN='a049498a99dc15635da5f9f849de2f0359a7cc5c76bcc0a0250a39cc1f62a495'
EFFECT_ROUTE_PINS={'native_weapon_effect_dispatch_tu15_owns_entry':'e57801cc206e04cc9eb9d0795f886281dd9e2ae90ae733b728fa14eb535e16ec','native_weapon_effect_dispatch_tu15_run':'206700cab7683b4858b18940d2db2f0ab15dc1bc0d06b2507a34697dee349b20'}
EFFECT_PREFIX='\n    if(halo2::native_model_marker_lookup_detail::effect(m,a,s,o))return;'
def prefix(mode):return '\n    if(halo2::native_model_marker_lookup_detail::bridge(s,halo2::native_model_marker_lookup_detail::Observation::'+mode+'))return;'
def canonical_body(body):
    opening=body.index('{')+1
    for marker in(prefix('actual'),prefix('canonical'),EFFECT_PREFIX):
        if body[opening:].startswith(marker):return body[:opening]+body[opening+len(marker):]
    return body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete model marker lookup owner')
    match=matches[0];body=canonical_body(match[0]);state=match[2]
    dispatcher=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Complete marker lookup dispatcher missing')
    if state=='State':
        owner=re.search(r'struct State : physics_contact_runtime_detail::State \{.*?\n\};',source,re.S)
        public=function(source,'source_run_death_startup_bodies_1_1')
        if not owner or digest(owner[0])!=DEATH_STATE_PIN or digest(public)!=DEATH_PUBLIC_PIN:
            raise ValueError('Complete marker lookup fixed1.1 State/public owner drift')
        resolver_pin='fixed1.1'
    else:
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
        if not resolver:raise ValueError('Complete marker lookup resolver missing')
        resolver_pin=digest(resolver[0])
    context=(digest(body),resolver_pin,digest(dispatcher[0]))
    if context not in CONTEXTS:raise ValueError('Complete marker lookup body/resolver/dispatcher context drift '+repr(context))
    mode,owner=CONTEXTS[context]
    if owner=='sandbox':
        if _sibling('native_sandbox_dispatch_preservation').preserve(source)!=source:
            raise ValueError('Sandbox canonical required-owner preservation missing before marker binding')
    if owner=='primary'and context[2]=='aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849':
        case='case 0xfa5d0u:run_fa5d0(s);return;'
        halfword=_sibling('native_halfword_bit_query_replacements')
        if dispatcher[0].count(case)!=1 or digest(dispatcher[0].replace(case,'',1))!='5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0' or digest(halfword.canonical_body(function(source,'run_fa5d0')))!=halfword.PIN:
            raise ValueError('Complete fresh marker FA owner routing/body drift')
    if match[0]!=body and not match[0][match[0].index('{')+1:].startswith(prefix(mode)):
        raise ValueError('Marker lookup observation prefix drift')
    return match,body,mode,owner
def replace_model_marker_lookup(source,*,defer_sandbox=False):
    if not BODY.search(source):return source
    if defer_sandbox:
        # The owning Sandbox wrapper restores its exact historical reset
        # boundary after common lowering. Never bind the unpreserved switch.
        helper=_sibling('native_sandbox_dispatch_preservation')
        try:dispatcher=helper.dispatcher(source)[0]
        except ValueError:dispatcher=''
        if digest(dispatcher)==helper.RAW_PIN:
            authenticate(helper.preserve(source));return source
    match,body,mode,owner=authenticate(source)
    if match[0]==body:
        opening=body.index('{')+1;bound=body[:opening]+prefix(mode)+body[opening:]
        source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate marker lookup native include')
    if owner=='scene':
        anchor='#include "physics/runtime/native_tu15_closest_segments_source_owner.hpp"\n'
        if source.count(anchor)!=1:raise ValueError('Complete Scene sibling include contract drift')
        return source.replace(INCLUDE,'',1).replace(anchor,anchor+INCLUDE,1)
    return source if INCLUDE in source else INCLUDE+source
def effect_body(source):return function(source,'marker_lookup')
def replace_effect(source,dispatcher):
    body=effect_body(source);original=canonical_body(body)
    if not matches_effect_body(original,EFFECT_BODY_PIN):raise ValueError('Complete Effect marker lookup body drift')
    aliases='using Memory=SourcePhysicsContactMemory;\nusing Abi=SourcePhysicsWorldRuntimeAbi;\nusing Observer=SourcePhysicsContactRecordObserver;\nusing Services=SourcePhysicsWorldRuntimeServices;'
    if dispatcher.count(aliases)!=1 or dispatcher.count('constexpr auto none=0xffffffffu;')!=1:
        raise ValueError('Complete Effect marker API alias/sentinel contract drift')
    for name,pin in EFFECT_ROUTE_PINS.items():
        if digest(function(dispatcher,name))!=pin:raise ValueError('Complete Effect marker production route drift '+name)
    if body==original:
        opening=body.index('{')+1;source=source.replace(body,body[:opening]+EFFECT_PREFIX+body[opening:],1)
    return source
