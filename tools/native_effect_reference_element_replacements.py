"""Exact complete current/fresh effect-reference source ownership."""
import hashlib,importlib.util,re
from pathlib import Path
def _sibling(name):
    spec=importlib.util.spec_from_file_location('_effect_reference_'+name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
function=_sibling('native_model_marker_lookup_replacements').function

def digest(text):return hashlib.sha256(text.encode()).hexdigest()
SIGN_EXTEND_PIN='cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b'
INCLUDE='#include "rendering/effects/native_tu15_effect_reference_element_admission.hpp"\n'
PUBLIC={
 'combat':('source_run_combat_action_tu15','source_combat_action_tu15_owns_entry'),
 'death':('source_run_death_cycle_runtime_tu15','source_death_cycle_runtime_tu15_owns_entry'),
 'particle':('source_run_effect_particle_frame_tu15','source_effect_particle_frame_tu15_owns_entry'),
}
BODY=re.compile(r'static void run_(178070|13a6b0|22b790|22aca0|22b710)\(WorldState &s\) \{.*?\n\}',re.S)
def prefix(entry):
    if isinstance(entry,str):entry=int(entry,16)
    if entry not in PINS:raise ValueError('Unknown effect-reference native owner')
    return ('\n    if(halo2::native_effect_reference_element_detail::bridge(s,0x'+f'{entry:x}'+'u,'
        '[](auto&child,std::uint32_t target,std::uint32_t site){'
        'if(site==0x1780ecu){run_178070(child);return;}'
        'if(site==0x1780f4u){run_13a6b0(child);return;}'
        'dispatch(child,target,site,false);}))return;')
def canonical_body(body,entry):
    opening=body.index('{')+1;marker=prefix(entry)
    return body[:opening]+body[opening+len(marker):]if body[opening:].startswith(marker)else body
def authenticate(source):
    require_selected_identity=_sibling('native_selected_operand_binding').require_selected_identity
    matches=list(BODY.finditer(source))
    if not matches:raise ValueError('No complete effect-reference owners')
    owners=tuple(sorted(int(match[1],16)for match in matches))
    if len(set(owners))!=len(owners):raise ValueError('Duplicate effect-reference owner')
    resolver=function(source,'selected');dispatcher=function(source,'dispatch')
    if digest(function(source,'sign_extend'))!=SIGN_EXTEND_PIN:raise ValueError('Complete effect-reference sign-extension helper drift')
    state=re.search(r'^struct LiveState :.*?^\};',source,re.M|re.S)
    if not state or source.count('using WorldState=LiveState;')!=1:raise ValueError('Complete effect-reference LiveState drift')
    identities=[]
    for name,(run,owns)in PUBLIC.items():
        try:public=(function(source,run),function(source,owns))
        except ValueError:continue
        context=(digest(resolver),digest(dispatcher),digest(state[0]),*(digest(text)for text in public))
        if context in CONTEXTS:identities.append((context,CONTEXTS[context]))
    if len(identities)!=1:raise ValueError('Complete effect-reference resolver/dispatcher/State/public context drift')
    context,(name,origin,expected)=identities[0]
    if owners!=tuple(sorted(expected)):raise ValueError('Effect-reference owner set drift')
    if any(prefix(entry)in match[0]for match in matches for entry in(int(match[1],16),))and source.count(INCLUDE)!=1:
        raise ValueError('Bound effect-reference native include missing/duplicated')
    if source.count(INCLUDE)>1:raise ValueError('Duplicate effect-reference native include')
    result=[]
    for match in matches:
        entry=int(match[1],16);body=canonical_body(match[0],entry)
        if function(source,f'run_{entry:x}')!=match[0]or digest(body)not in PINS[entry]:raise ValueError('Complete effect-reference body drift '+hex(entry))
        require_selected_identity(source,body,{context[0]})
        result.append((match,entry,body))
    return result
def replace_effect_reference_element(source):
    if not BODY.search(source):return source
    for match,entry,body in reversed(authenticate(source)):
        opening=body.index('{')+1;bound=body[:opening]+prefix(entry)+body[opening:]
        source=source[:match.start()]+bound+source[match.end():]
    return source if INCLUDE in source else INCLUDE+source
PINS={1287856: ('3c3275d3f7ee492ee3252e4ca80291a1e7dc9ccf5f5a2abcdb5c52c518cb3dd2',
           '688cabc723fd018d89e8238c0cbde03f3e62723863950f32c3df08ed45deedf3'),
 1540208: ('55bd2f3a2dac02f979f5b5ac6e1db2b32e6dfadb481a8d1690d93b9e99cd4341',
           'f1fab1c1a844a92245f86bc0f4f9840a6534a49eb7185a28aa6ca6cccb5ed1dc'),
 2272416: ('fb00950ef4e4f8a834a2e0361e21bf3a217944c4671b93b2dbe3b35a25617151',),
 2275088: ('67be61af7042e1a28ddb6e2cab91cc947073b71eeb4911ee9ac7825e2b0f5102',),
 2275216: ('45ff8bebfa275af7bfa17e1afc0275dcc916cc02d536614bedb65f4ffeefd6cc',
           'f907fd7b88fa3ee8a65bdbfd7ba7f3c73671a5d0fe307510b5796630f35c2fbd')}
CONTEXTS={('03e2514fde9d92aa28b30dc7c13ef0cc2fcaf7c320d20644d66603ba6e6b640a', '00e22d4bf71ec7037cc263e95aca1dd0d6b4e40433abeaaaa17998e101fd1b29', '68accf0054ca64fec14dc244a1c1d00e6015e79ef2e0182717e8b2fe4cf99fb3', 'ec22a414aa0f01a7cf2fbf16deec71f20abf6f1caadb3c5ef2dfd9fe1920c1ca', '8fec9b443681931925cadcb113f9e458a026e5dd36a988d23cc9be4f44e28cba'): ('particle',
                                                                                                                                                                                                                                                                                                                                                        'current',
                                                                                                                                                                                                                                                                                                                                                        (1540208,
                                                                                                                                                                                                                                                                                                                                                         1287856,
                                                                                                                                                                                                                                                                                                                                                         2275216,
                                                                                                                                                                                                                                                                                                                                                         2272416,
                                                                                                                                                                                                                                                                                                                                                         2275088)),
 ('1e70ace48573a64c67d51a9c7cde9f01c560e73795f8b089a918001d6bb1fbd6', '1940da2e3890172390375a282ec9b1827d5a001fc02f5b85d0367efb9666ac45', 'c3e351629369a688a5ca40be8dfcaf725482d008a3050759060cacd2a2bed275', '756f21822724abbf43554d0db4ee99793f788ed0ec5d74fb2424d15388f273ba', '269ea56cac029b21798798ae989aa9a99b4e055fdc86c959d545e6cb3b8d3ce9'): ('death',
                                                                                                                                                                                                                                                                                                                                                        'fresh',
                                                                                                                                                                                                                                                                                                                                                        (1540208,
                                                                                                                                                                                                                                                                                                                                                         1287856,
                                                                                                                                                                                                                                                                                                                                                         2275216)),
 ('35ee53916a87b4f8aa52fbba09c2df76777e09e0cc19264072b405d2a6189468', '1a64412793921cc17f92b4b0986c4c2cb8b1163629e2cb7b581371eb717bb6a6', 'c3e351629369a688a5ca40be8dfcaf725482d008a3050759060cacd2a2bed275', '07f20648abb6f1ef588f8e938f3611fa2949fe3ea6cbb54caf26a1b95b179d1f', '8dde07ff32aae9b6deb836b53976970c6508929adb9f60c0c3863697a7b9b39a'): ('particle',
                                                                                                                                                                                                                                                                                                                                                        'fresh',
                                                                                                                                                                                                                                                                                                                                                        (1540208,
                                                                                                                                                                                                                                                                                                                                                         1287856,
                                                                                                                                                                                                                                                                                                                                                         2275216,
                                                                                                                                                                                                                                                                                                                                                         2272416,
                                                                                                                                                                                                                                                                                                                                                         2275088)),
 ('7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75', '2f716e064af3a72690bbe766be88580a1927daa39319590d18444e94e00114ef', '68accf0054ca64fec14dc244a1c1d00e6015e79ef2e0182717e8b2fe4cf99fb3', '8babe9ef20df7e057fde057724996687cced5f124bc6fc80910b5a777ad39132', '5ffaf3d432c95512967e86067d95492571ed9f7989aa4b897ccc5457c5de9135'): ('combat',
                                                                                                                                                                                                                                                                                                                                                        'current',
                                                                                                                                                                                                                                                                                                                                                        (1540208,
                                                                                                                                                                                                                                                                                                                                                         1287856)),
 ('7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75', 'f434da6ab6b8f8239fe575d6e75efb8e00f4a67a5188ac30cfff70e1f8d3a0ef', 'c3e351629369a688a5ca40be8dfcaf725482d008a3050759060cacd2a2bed275', '8babe9ef20df7e057fde057724996687cced5f124bc6fc80910b5a777ad39132', '5ffaf3d432c95512967e86067d95492571ed9f7989aa4b897ccc5457c5de9135'): ('combat',
                                                                                                                                                                                                                                                                                                                                                        'fresh',
                                                                                                                                                                                                                                                                                                                                                        (1540208,
                                                                                                                                                                                                                                                                                                                                                         1287856)),
 ('cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61', '2ddcb4ca932da7abcdbbbf8050bead73a3a18e4a4153021913f0555281b8aa20', '68accf0054ca64fec14dc244a1c1d00e6015e79ef2e0182717e8b2fe4cf99fb3', '2a991a9c2fe08cfe7b819dca11f49e352be74fd0afe781f4cfed9eadbea34292', '006c8d4b1ef535a74a05825002e6ceeded8498269d7140d5ab56b87b661530db'): ('death',
                                                                                                                                                                                                                                                                                                                                                        'current',
                                                                                                                                                                                                                                                                                                                                                        (1540208,
                                                                                                                                                                                                                                                                                                                                                         1287856,
                                                                                                                                                                                                                                                                                                                                                         2275216))}
