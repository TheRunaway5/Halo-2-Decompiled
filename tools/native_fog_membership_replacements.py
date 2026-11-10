"""Strict complete 171070 fog membership owners in the two real renderer units."""
import hashlib,re
SELECTED_PIN='0c3b62f036e905dd2c7631554b0f539e28b31e1e7b81ffe1b3ec22aa2297ba1c'
LITERAL_PIN='d54131935860f584d28698770ba1b5a60ec59705a23295f3d2d7fbb28b964a37'
CONTEXTS={
    SELECTED_PIN:{('dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981','fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e')},
    LITERAL_PIN:{('892cf508106b0c182d59e4dba45804e434a1833003cfb3c322f9a8875c54d7fb','c0e112fb90d7ea32e73e6b3b37d333b6a375456791c5f2eaf9a08906d613f6a9')},
}
PREFIX='\n    if(halo2::native_fog_membership_detail::bridge(s))return;'
INCLUDE='#include "rendering/native_tu15_fog_membership.hpp"\n'
BODY=re.compile(r'static void run_171070\(WorldState &s\) \{.*?\n\}',re.S)
SHIFTS=('s.set(1,s.flags.shift(s.reg(1,4,0),selected(0x4u,s.image),4),4,0);','s.set(1,s.flags.shift(s.reg(1,4,0),0x4u,4),4,0);')
ORDERED_SHIFT='halo2::native_fog_membership_detail::retained_datum_offset(s);'
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    if body[opening:].startswith(PREFIX):body=body[:opening]+body[opening+len(PREFIX):]
    return body.replace(ORDERED_SHIFT,SHIFTS[0]if 'selected('in body else SHIFTS[1])
def ordered_reference(body):
    if sum(body.count(old)for old in SHIFTS)!=1:raise ValueError('Expected exact original171070 shift')
    for old in SHIFTS:body=body.replace(old,ORDERED_SHIFT)
    return body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete171070 fog owner')
    match=matches[0];body=canonical_body(match[0]);pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('Complete171070 fog owner body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatch or(digest(resolver[0]),digest(dispatch[0]))not in CONTEXTS[pin]:raise ValueError('Complete171070 fog resolver/dispatcher drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{context[0]for context in CONTEXTS[pin]})
    return match,body
def replace_fog_membership(source):
    if not BODY.search(source):return source
    match,body=authenticate(source)
    expected=ordered_reference(body);opening=expected.index('{')+1;expected=expected[:opening]+PREFIX+expected[opening:]
    if match[0]!=expected:source=source[:match.start()]+expected+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate fog-membership include')
    if INCLUDE not in source:source=INCLUDE+source
    return source

PUBLIC_PIN='db17a5d4c04824d16a1479abd91be6407a495a0a7ca78c13b542c91b6ac7f3f4'
PUBLIC_INCLUDE='#include "halo2/native_tu15_fog_membership_value.hpp"\n'
PUBLIC_BODY=re.compile(r'bool BspAtmosphere::contains_planar\(int cluster,std::uint32_t datum\) const \{.*?\n\}',re.S)
PUBLIC_REPLACEMENT='''bool BspAtmosphere::contains_planar(int cluster,std::uint32_t datum) const {
    if(cluster<=-2) {
        const auto instance=std::size_t(-(std::int64_t(cluster)+2));
        if(instance>=instance_membership.size())throw FormatError("planar instance outside BSP");
        return std::any_of(instance_membership[instance].begin(),instance_membership[instance].end(),
            [&](unsigned member){return contains_planar(int(member),datum);});
    }
    if(cluster<0)return false;
    if(std::size_t(cluster)>=clusters.size())throw FormatError("planar cluster outside BSP");
    return native_fog_membership_detail::planar_member(*this,cluster,datum);
}'''
def replace_public(source):
    matches=list(PUBLIC_BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete public fog membership owner')
    match=matches[0]
    if match[0]!=PUBLIC_REPLACEMENT:
        if digest(match[0])!=PUBLIC_PIN:raise ValueError('Complete public fog membership owner drift')
        source=source[:match.start()]+PUBLIC_REPLACEMENT+source[match.end():]
    if source.count(PUBLIC_INCLUDE)>1:raise ValueError('Duplicate public fog membership include')
    if PUBLIC_INCLUDE not in source:source=PUBLIC_INCLUDE+source
    return source
