"""Restore one authenticated Sandbox owner boundary in fresh switch emission."""
import hashlib,re
RESOLVER_PIN='0fddb23fd6f15344aa915c1c50b845847dd34460782d49467474e9e1cfed5fed'
CANONICAL_PIN='961134517848136bf248393b533a030ae2607735d71fec8f570e0332e5ce4bc6'
RAW_PIN='21bcd5b2ab9610c42014d2a58911db342c5af8b84936fd2a1a946070f3e4a161'
PRESERVED_PIN='d46a1635c618577534bfdd091f45ecea819b4886662ca1309400154271d5a987'
RAW_CASE='case 0xcabc0u:run_cabc0(s);return;'
PRESERVED_CASE='case 0xcabc0u:if(s.image==SourcePhysicsContactImage::TitleUpdate15){s.required(site,target,tail);return;}run_cabc0(s);return;'
PATTERN=re.compile(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}')
# Only these four independently diffed old/fresh whole bodies are repaired.
# The canonical runtime is preserved; unrelated future emission drift refuses.
BODY_CONTEXTS={
 'd91d0':('4157e261b0a77f2ad771ebb6156266b56fa1424311e483c986c069b03d74cd49','c5016f9c47c35450f94bbf77f7232b3c60c54f4300e204fae1f08c0d9f810063','e616add6877a843503964e83f3f5895f44162909bbb4bf4f93dc283aeda47496'),
 '108080':('ba82ea0a78b7b3fb0d5505c105dfb61c22ef7c6391c0edfcc55dc91d05ee0301','bea96239677dd612c1b00a044694d17b3bb5c03f9ba41ec7d249196d7aac511e','b1205101f9d162cf3d83c1e3c51e52b9c3a4da839ffa9b952845d2bb10a373f5'),
 '15e1e0':('127c976ce9331e30b4ad3e2bf05343b4dcc9a8b9ba4a4978aa77ee156f633733','791e01d955396d4ccdea50820a0189a6bd153f156d25bfc4b50ae6c63aaaa876','a3af66ff3cff8150f6af8d2abed746e4ebeee1b91ccbbb31733c5bc84499e635'),
 '15f8b0':('783d21c92ddf504dad8db5e0dc26579fb4024d9c542d3833cacba799fac38935','6578a4501a31e884a09ef8f4be7baa132df36820dcf270f354e382fe0ba06cf6','b127aceba81ee7ed554b75025f18dd3e9b64bced1a727c9e24cacf9480b2c83c'),
}
BODY_CHANGES={
 'd91d0':(('SourcePhysicsEntryObservation area_diagnostic(s.memory,0xd91d0u,[&]{return s.save();});\n',''),),
 '108080':(('s.push(0x1088e5u,0x1088eau);run_cabc0(s);','s.push(0x1088e5u,0x1088eau);if(s.image==SourcePhysicsContactImage::TitleUpdate15)s.required(0x1088e5u,0xcabc0u,false);else run_cabc0(s);'),),
 '15e1e0':(('s.push(0x15e326u,0x15e32bu);s.required(0x15e326u,0x15ddc0u,false);','{const auto target=0x15ddc0u;s.push(0x15e326u,0x15e32bu);s.required(0x15e326u,target,false);}'),),
 '15f8b0':tuple((f's.push(0x{site:x}u,0x{return_pc:x}u);s.required(0x{site:x}u,0x{target:x}u,false);',f'{{const auto target=0x{target:x}u;s.push(0x{site:x}u,0x{return_pc:x}u);dispatch(s,target,0x{site:x}u,false);}}')for site,return_pc,target in((0x15fdf4,0x15fdf9,0x1a8740),(0x1600ad,0x1600b2,0x1a87a0))),
}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def dispatcher(source):
    found=list(PATTERN.finditer(source));resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if len(found)!=1 or not resolver or digest(resolver[0])!=RESOLVER_PIN:
        raise ValueError('Complete Sandbox dispatcher/resolver context drift')
    return found[0]
def preserve(source):
    match=dispatcher(source);pin=digest(match[0])
    if pin not in(CANONICAL_PIN,PRESERVED_PIN,RAW_PIN):
        raise ValueError('Complete fresh Sandbox dispatcher drift')
    if pin==RAW_PIN:
        if match[0].count(RAW_CASE)!=1:raise ValueError('Exact Sandbox owner case missing')
        body=match[0].replace(RAW_CASE,PRESERVED_CASE,1)
        if digest(body)!=PRESERVED_PIN:raise ValueError('Exact Sandbox owner preservation drift')
        source=source[:match.start()]+body+source[match.end():]
    for name,pins in BODY_CONTEXTS.items():
        pattern=re.compile(r'static void run_'+name+r'\(WorldState &s\) \{.*?\n\}',re.S)
        bodies=list(pattern.finditer(source))
        if len(bodies)!=1:raise ValueError('Complete Sandbox target body missing '+name)
        match=bodies[0];body=match[0];body_pin=digest(body)
        if body_pin in pins[:2]:continue
        if body_pin!=pins[2]:raise ValueError('Complete Sandbox target body drift '+name)
        for old,new in BODY_CHANGES[name]:
            if body.count(old)!=1:raise ValueError('Exact Sandbox target correction missing '+name)
            body=body.replace(old,new,1)
        if digest(body)!=pins[1]:raise ValueError('Exact Sandbox target preservation drift '+name)
        source=source[:match.start()]+body+source[match.end():]
    return source
def mappings(body):
    opening='static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){'
    if not body.startswith(opening):raise ValueError('Sandbox dispatcher argument contract drift')
    rest=body[len(opening):];mapping={}
    if rest.startswith('switch(target){'):
        if not rest.endswith('default:s.required(site,target,tail);return;}}'):
            raise ValueError('Sandbox switch fallback drift')
        cases=rest[len('switch(target){'):-len('default:s.required(site,target,tail);return;}}')]
        matches=list(re.finditer(r'case (0x[0-9a-f]+)u:',cases))
        if not matches or matches[0].start()!=0:raise ValueError('Unexpected Sandbox switch preamble')
        for index,match in enumerate(matches):
            target=int(match[1],16)
            if target in mapping:raise ValueError('Duplicate Sandbox dispatch target')
            mapping[target]=cases[match.end():matches[index+1].start()if index+1<len(matches)else len(cases)]
    else:
        clause=re.compile(r'if\(target==selected\((0x[0-9a-f]+)u,s\.image\)\)\{')
        while match:=clause.match(rest):
            depth=1;end=match.end()
            while depth:
                depth+=1 if rest[end]=='{'else -1 if rest[end]=='}'else 0;end+=1
            target=int(match[1],16)
            if target in mapping:raise ValueError('Duplicate Sandbox dispatch target')
            mapping[target]=rest[match.end():end-1];rest=rest[end:]
        if rest!='s.required(site,target,tail);}':raise ValueError('Sandbox selected fallback drift')
    return mapping
