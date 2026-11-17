"""Private full actual313470 context pins; virtual dispatch remains the owning source dispatcher."""
import hashlib
import re
OWNER_MARKER='native_solver_hull_edge_owner_detail'
RESUME_MARKER='native_hull_edge_resume_'
BODY=re.compile(r'static void run_313470\(WorldState &s\) \{.*?\n\}',re.S)
OLD_PREFIX='\n    switch(halo2::native_solver_hull_edge_owner_detail::bridge(s,[&](auto&v,std::uint32_t target,std::uint32_t site){if(target!=0x313390u)throw std::logic_error("Different hull-edge native child");run_313390(v);})){case 0u:break;case 1u:return;case 0x313471u:goto native_hull_edge_resume_313471;case 0x313480u:goto native_hull_edge_resume_313480;case 0x313481u:goto native_hull_edge_resume_313481;case 0x313484u:goto native_hull_edge_resume_313484;case 0x313487u:goto native_hull_edge_resume_313487;case 0x313488u:goto native_hull_edge_resume_313488;case 0x313489u:goto native_hull_edge_resume_313489;case 0x31348eu:goto native_hull_edge_resume_31348e;case 0x313492u:goto native_hull_edge_resume_313492;case 0x313499u:goto native_hull_edge_resume_313499;case 0x31349eu:goto native_hull_edge_resume_31349e;case 0x3134a1u:goto native_hull_edge_resume_3134a1;case 0x3134a2u:goto native_hull_edge_resume_3134a2;case 0x3134a7u:goto native_hull_edge_resume_3134a7;case 0x3134aeu:goto native_hull_edge_resume_3134ae;case 0x3134b1u:goto native_hull_edge_resume_3134b1;case 0x3134b4u:goto native_hull_edge_resume_3134b4;case 0x3134b9u:goto native_hull_edge_resume_3134b9;case 0x3134beu:goto native_hull_edge_resume_3134be;case 0x3134c1u:goto native_hull_edge_resume_3134c1;case 0x3134e3u:goto native_hull_edge_resume_3134e3;case 0x3134ecu:goto native_hull_edge_resume_3134ec;case 0x3134f4u:goto native_hull_edge_resume_3134f4;case 0x3134f8u:goto native_hull_edge_resume_3134f8;case 0x3134fbu:goto native_hull_edge_resume_3134fb;case 0x313525u:goto native_hull_edge_resume_313525;case 0x313529u:goto native_hull_edge_resume_313529;case 0x313535u:goto native_hull_edge_resume_313535;case 0x313539u:goto native_hull_edge_resume_313539;case 0x31353cu:goto native_hull_edge_resume_31353c;case 0x313540u:goto native_hull_edge_resume_313540;case 0x313544u:goto native_hull_edge_resume_313544;case 0x313547u:goto native_hull_edge_resume_313547;case 0x313548u:goto native_hull_edge_resume_313548;case 0x313549u:goto native_hull_edge_resume_313549;case 0x31354au:goto native_hull_edge_resume_31354a;case 0x31354du:goto native_hull_edge_resume_31354d;case 0x313554u:goto native_hull_edge_resume_313554;case 0x313555u:goto native_hull_edge_resume_313555;case 0x313558u:goto native_hull_edge_resume_313558;case 0x313559u:goto native_hull_edge_resume_313559;case 0x31355au:goto native_hull_edge_resume_31355a;case 0x31355du:goto native_hull_edge_resume_31355d;default:throw std::logic_error("Unsupported native hull-edge continuation");}'
RAW_PREFIX='\n    switch(halo2::native_solver_hull_edge_owner_detail::bridge<false>(s,[&](auto&v,std::uint32_t target,std::uint32_t site){if(target!=0x313390u)throw std::logic_error("Different hull-edge native child");run_313390(v);})){case 0u:break;case 1u:return;case 0x313471u:goto native_hull_edge_resume_313471;case 0x313480u:goto native_hull_edge_resume_313480;case 0x313481u:goto native_hull_edge_resume_313481;case 0x313484u:goto native_hull_edge_resume_313484;case 0x313487u:goto native_hull_edge_resume_313487;case 0x313488u:goto native_hull_edge_resume_313488;case 0x313489u:goto native_hull_edge_resume_313489;case 0x31348eu:goto native_hull_edge_resume_31348e;case 0x313492u:goto native_hull_edge_resume_313492;case 0x313499u:goto native_hull_edge_resume_313499;case 0x31349eu:goto native_hull_edge_resume_31349e;case 0x3134a1u:goto native_hull_edge_resume_3134a1;case 0x3134a2u:goto native_hull_edge_resume_3134a2;case 0x3134a7u:goto native_hull_edge_resume_3134a7;case 0x3134aeu:goto native_hull_edge_resume_3134ae;case 0x3134b1u:goto native_hull_edge_resume_3134b1;case 0x3134b4u:goto native_hull_edge_resume_3134b4;case 0x3134b9u:goto native_hull_edge_resume_3134b9;case 0x3134beu:goto native_hull_edge_resume_3134be;case 0x3134c1u:goto native_hull_edge_resume_3134c1;case 0x3134e3u:goto native_hull_edge_resume_3134e3;case 0x3134ecu:goto native_hull_edge_resume_3134ec;case 0x3134f4u:goto native_hull_edge_resume_3134f4;case 0x3134f8u:goto native_hull_edge_resume_3134f8;case 0x3134fbu:goto native_hull_edge_resume_3134fb;case 0x313525u:goto native_hull_edge_resume_313525;case 0x313529u:goto native_hull_edge_resume_313529;case 0x313535u:goto native_hull_edge_resume_313535;case 0x313539u:goto native_hull_edge_resume_313539;case 0x31353cu:goto native_hull_edge_resume_31353c;case 0x313540u:goto native_hull_edge_resume_313540;case 0x313544u:goto native_hull_edge_resume_313544;case 0x313547u:goto native_hull_edge_resume_313547;case 0x313548u:goto native_hull_edge_resume_313548;case 0x313549u:goto native_hull_edge_resume_313549;case 0x31354au:goto native_hull_edge_resume_31354a;case 0x31354du:goto native_hull_edge_resume_31354d;case 0x313554u:goto native_hull_edge_resume_313554;case 0x313555u:goto native_hull_edge_resume_313555;case 0x313558u:goto native_hull_edge_resume_313558;case 0x313559u:goto native_hull_edge_resume_313559;case 0x31355au:goto native_hull_edge_resume_31355a;case 0x31355du:goto native_hull_edge_resume_31355d;default:throw std::logic_error("Unsupported native hull-edge continuation");}'
PREPARED_PREFIX='\n    switch(halo2::native_solver_hull_edge_owner_detail::bridge<true>(s,[&](auto&v,std::uint32_t target,std::uint32_t site){if(target!=0x313390u)throw std::logic_error("Different hull-edge native child");run_313390(v);})){case 0u:break;case 1u:return;case 0x313471u:goto native_hull_edge_resume_313471;case 0x313480u:goto native_hull_edge_resume_313480;case 0x313481u:goto native_hull_edge_resume_313481;case 0x313484u:goto native_hull_edge_resume_313484;case 0x313487u:goto native_hull_edge_resume_313487;case 0x313488u:goto native_hull_edge_resume_313488;case 0x313489u:goto native_hull_edge_resume_313489;case 0x31348eu:goto native_hull_edge_resume_31348e;case 0x313492u:goto native_hull_edge_resume_313492;case 0x313499u:goto native_hull_edge_resume_313499;case 0x31349eu:goto native_hull_edge_resume_31349e;case 0x3134a1u:goto native_hull_edge_resume_3134a1;case 0x3134a2u:goto native_hull_edge_resume_3134a2;case 0x3134a7u:goto native_hull_edge_resume_3134a7;case 0x3134aeu:goto native_hull_edge_resume_3134ae;case 0x3134b1u:goto native_hull_edge_resume_3134b1;case 0x3134b4u:goto native_hull_edge_resume_3134b4;case 0x3134b9u:goto native_hull_edge_resume_3134b9;case 0x3134beu:goto native_hull_edge_resume_3134be;case 0x3134c1u:goto native_hull_edge_resume_3134c1;case 0x3134e3u:goto native_hull_edge_resume_3134e3;case 0x3134ecu:goto native_hull_edge_resume_3134ec;case 0x3134f4u:goto native_hull_edge_resume_3134f4;case 0x3134f8u:goto native_hull_edge_resume_3134f8;case 0x3134fbu:goto native_hull_edge_resume_3134fb;case 0x313525u:goto native_hull_edge_resume_313525;case 0x313529u:goto native_hull_edge_resume_313529;case 0x313535u:goto native_hull_edge_resume_313535;case 0x313539u:goto native_hull_edge_resume_313539;case 0x31353cu:goto native_hull_edge_resume_31353c;case 0x313540u:goto native_hull_edge_resume_313540;case 0x313544u:goto native_hull_edge_resume_313544;case 0x313547u:goto native_hull_edge_resume_313547;case 0x313548u:goto native_hull_edge_resume_313548;case 0x313549u:goto native_hull_edge_resume_313549;case 0x31354au:goto native_hull_edge_resume_31354a;case 0x31354du:goto native_hull_edge_resume_31354d;case 0x313554u:goto native_hull_edge_resume_313554;case 0x313555u:goto native_hull_edge_resume_313555;case 0x313558u:goto native_hull_edge_resume_313558;case 0x313559u:goto native_hull_edge_resume_313559;case 0x31355au:goto native_hull_edge_resume_31355a;case 0x31355du:goto native_hull_edge_resume_31355d;default:throw std::logic_error("Unsupported native hull-edge continuation");}'
PREFIX=RAW_PREFIX
INCLUDE='#include "physics/runtime/native_tu15_solver_hull_edge_source_owner.hpp"\n'
BODY_PIN='3c1f5c38e0c036e8d2b1df5c44703ed4fe403c375e45b1409e20893db138de22'
FRESH_BODY_PIN='0bd51f12221b5596e0af35fe725e5cfd962824f40159cf737f19f4a15304c4fc'
RESOLVER_PIN='1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4'
DISPATCHERS=('5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0','aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849')
RAW_CONTEXTS=(('3c1f5c38e0c036e8d2b1df5c44703ed4fe403c375e45b1409e20893db138de22', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'), ('0bd51f12221b5596e0af35fe725e5cfd962824f40159cf737f19f4a15304c4fc', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'))
OLD_PREPARED_CONTEXTS=(('9941772b5b8408fcf18ccee2b6f6f5c8c462dd58dae4ed6953c360d45aa49a56', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'), ('a43719b51e8ebe951fbb365d71059db167edc8885c6c5d04e7f3cf03fbf78cfc', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'))
OLD_INSTALLED_BODY_PINS=('4e38b5dffec664c1282c0880503fa9e36c680f7f1c062d1343f18c487af9c463', '035972330d701220c567011632a28ae66baaae1284134e64f7918d4286245c10', '9941772b5b8408fcf18ccee2b6f6f5c8c462dd58dae4ed6953c360d45aa49a56', 'a43719b51e8ebe951fbb365d71059db167edc8885c6c5d04e7f3cf03fbf78cfc')
FRONTIERS=((3224688, 3224689), (3224697, 3224704), (3224704, 3224705), (3224705, 3224708), (3224708, 3224711), (3224711, 3224712), (3224712, 3224713), (3224717, 3224718), (3224721, 3224722), (3224724, 3224729), (3224729, 3224734), (3224734, 3224737), (3224737, 3224738), (3224742, 3224743), (3224745, 3224750), (3224750, 3224753), (3224753, 3224756), (3224756, 3224761), (3224761, 3224766), (3224766, 3224769), (3224799, 3224803), (3224807, 3224812), (3224816, 3224820), (3224820, 3224824), (3224824, 3224827), (3224865, 3224869), (3224869, 3224873), (3224880, 3224885), (3224885, 3224889), (3224889, 3224892), (3224892, 3224896), (3224896, 3224900), (3224900, 3224903), (3224903, 3224904), (3224904, 3224905), (3224905, 3224906), (3224908, 3224909), (3224912, 3224916), (3224916, 3224917), (3224917, 3224920), (3224920, 3224921), (3224921, 3224922), (3224924, 3224925))
LABELS={3224689: 'native_hull_edge_resume_313471: ', 3224704: 'native_hull_edge_resume_313480: ', 3224705: 'native_hull_edge_resume_313481: ', 3224708: 'native_hull_edge_resume_313484: ', 3224711: 'native_hull_edge_resume_313487: ', 3224712: 'native_hull_edge_resume_313488: ', 3224713: 'native_hull_edge_resume_313489: ', 3224718: 'native_hull_edge_resume_31348e: ', 3224722: 'native_hull_edge_resume_313492: ', 3224729: 'native_hull_edge_resume_313499: ', 3224734: 'native_hull_edge_resume_31349e: ', 3224737: 'native_hull_edge_resume_3134a1: ', 3224738: 'native_hull_edge_resume_3134a2: ', 3224743: 'native_hull_edge_resume_3134a7: ', 3224750: 'native_hull_edge_resume_3134ae: ', 3224753: 'native_hull_edge_resume_3134b1: ', 3224756: 'native_hull_edge_resume_3134b4: ', 3224761: 'native_hull_edge_resume_3134b9: ', 3224766: 'native_hull_edge_resume_3134be: ', 3224769: 'native_hull_edge_resume_3134c1: ', 3224803: 'native_hull_edge_resume_3134e3: ', 3224812: 'native_hull_edge_resume_3134ec: ', 3224820: 'native_hull_edge_resume_3134f4: ', 3224824: 'native_hull_edge_resume_3134f8: ', 3224827: 'native_hull_edge_resume_3134fb: ', 3224869: 'native_hull_edge_resume_313525: ', 3224873: 'native_hull_edge_resume_313529: ', 3224885: 'native_hull_edge_resume_313535: ', 3224889: 'native_hull_edge_resume_313539: ', 3224892: 'native_hull_edge_resume_31353c: ', 3224896: 'native_hull_edge_resume_313540: ', 3224900: 'native_hull_edge_resume_313544: ', 3224903: 'native_hull_edge_resume_313547: ', 3224904: 'native_hull_edge_resume_313548: ', 3224905: 'native_hull_edge_resume_313549: ', 3224906: 'native_hull_edge_resume_31354a: ', 3224909: 'native_hull_edge_resume_31354d: ', 3224916: 'native_hull_edge_resume_313554: ', 3224917: 'native_hull_edge_resume_313555: ', 3224920: 'native_hull_edge_resume_313558: ', 3224921: 'native_hull_edge_resume_313559: ', 3224922: 'native_hull_edge_resume_31355a: ', 3224925: 'native_hull_edge_resume_31355d: '}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def normalized_installed_body(body,prepared):
    prefix=PREPARED_PREFIX if prepared else RAW_PREFIX
    opening=body.index('{')+1
    if not body.startswith(prefix,opening):raise ValueError('Hull-edge prefix is not at its exact owner entry')
    normalized=body[:opening]+OLD_PREFIX+body[opening+len(prefix):]
    allowed=tuple(row[0]for row in OLD_PREPARED_CONTEXTS)if prepared else OLD_INSTALLED_BODY_PINS[:2]
    if digest(normalized)not in allowed:raise ValueError('Complete tagged hull-edge body, source format or continuation placement drift')
    return normalized

def canonical_body(body):
    prefixes=[prefix for prefix in(RAW_PREFIX,PREPARED_PREFIX)if prefix in body]
    if not prefixes:
        if OLD_PREFIX in body or any(label in body for label in LABELS.values()):raise ValueError('Untagged or partial hull-edge overlay')
        return body
    if len(prefixes)!=1 or body.count(prefixes[0])!=1:raise ValueError('Ambiguous tagged hull-edge prefix')
    prefix=prefixes[0];normalized_installed_body(body,prefix==PREPARED_PREFIX)
    opening=body.index('{')+1;result=body[:opening]+body[opening+len(prefix):]
    for label in LABELS.values():
        if result.count(label)!=1:raise ValueError('Incomplete hull-edge continuation labels')
        result=result.replace(label,'',1)
    return result

def context(source):
    bodies=list(BODY.finditer(source))
    if len(bodies)!=1:raise ValueError('Missing or ambiguous actual solver-hull edge owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete solver-hull edge context')
    return bodies[0],resolver[0],dispatcher[0]

def require_owned_markers(source,installed):
    if source.count(OWNER_MARKER)!=int(installed) or source.count(RESUME_MARKER)!=2*len(LABELS)*int(installed):raise ValueError('Foreign hull-edge owner or continuation marker')

def authenticate(source):
    if not BODY.search(source):
        if INCLUDE in source or OWNER_MARKER in source or RESUME_MARKER in source:raise ValueError('Missing actual solver-hull edge owner')
        return None
    match,resolver,dispatcher=context(source)
    if PREPARED_PREFIX in match[0]:raise ValueError('Raw hull-edge installer received a prepared tagged owner')
    body=canonical_body(match[0])
    if tuple(digest(value)for value in(body,resolver,dispatcher))not in RAW_CONTEXTS:raise ValueError('Complete hull-edge raw body/resolver/dispatcher drift')
    if source.count(INCLUDE)>1 or(RAW_PREFIX in match[0])!=(INCLUDE in source):raise ValueError('Incomplete hull-edge raw overlay')
    if any(source.count(prefix)!=int(prefix in match[0])for prefix in(RAW_PREFIX,PREPARED_PREFIX,OLD_PREFIX)):raise ValueError('Foreign hull-edge prefix outside its owner')
    require_owned_markers(source,RAW_PREFIX in match[0])
    return match,body

def replace_solver_hull_edge(source):
    found=authenticate(source)
    if not found:return source
    match,body=found;installed=body
    for pc,label in LABELS.items():
        needle='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        if installed.count(needle)!=1:raise ValueError('Hull-edge continuation site drift')
        installed=installed.replace(needle,label+needle,1)
    at=installed.index('{')+1;value=source[:match.start()]+installed[:at]+RAW_PREFIX+installed[at:]+source[match.end():]
    return value if INCLUDE in value else INCLUDE+value

def remove_solver_hull_edge(source):
    found=authenticate(source)
    if not found:return source
    match,body=found;return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)

def rebind_prepared(source):
    match,resolver,dispatcher=context(source);body=match[0];opening=body.index('{')+1
    prefixes=[prefix for prefix in(RAW_PREFIX,PREPARED_PREFIX)if body.startswith(prefix,opening)]
    if len(prefixes)!=1 or source.count(INCLUDE)!=1:raise ValueError('Missing exact hull-edge specialization input')
    prefix=prefixes[0];normalized=body[:opening]+OLD_PREFIX+body[opening+len(prefix):]
    if tuple(digest(value)for value in(normalized,resolver,dispatcher))not in OLD_PREPARED_CONTEXTS:raise ValueError('Complete hull-edge prepared body/resolver/dispatcher drift')
    if any(source.count(value)!=int(value==prefix)for value in(RAW_PREFIX,PREPARED_PREFIX,OLD_PREFIX)):raise ValueError('Foreign hull-edge specialization prefix')
    require_owned_markers(source,True)
    result=body[:opening]+PREPARED_PREFIX+body[opening+len(prefix):]
    return source[:match.start()]+result+source[match.end():]

def installed_entries(source):
    match,_,_=context(source)
    if PREPARED_PREFIX in match[0]:
        if rebind_prepared(source)!=source:raise ValueError('Hull-edge prepared owner was not explicitly rebound')
    else:
        found=authenticate(source)
        if not found or RAW_PREFIX not in match[0]or source.count(INCLUDE)!=1:raise ValueError('Missing tagged hull-edge raw owner')
    return['313470']
