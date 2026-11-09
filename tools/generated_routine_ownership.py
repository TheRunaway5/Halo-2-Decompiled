"""Source ownership for literal-instruction routines, including non-generated files.

This recognizes the audited attributed/template C++ function grammar, rather
than pretending to parse arbitrary C++. Literal callbacks outside that grammar
are rejected so a new owner shape cannot silently disappear from the census.
"""
from dataclasses import dataclass
from bisect import bisect_right
import re
from pathlib import Path
import hashlib

NON_CODE=re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',re.S)
FUNCTION=re.compile(r'^[ \t]*(?:\[\[[^\]]+\]\][ \t]*)?(?:(?:inline|static|constexpr)\s+)*(?:[\w:<>]+(?:\s*[&*])?\s+)+(?P<name>[\w:]+)\s*\((?P<parameters>[^;{}]*?)\)\s*\{',re.M)
CALLBACK=re.compile(r'\b(?:[A-Za-z_]\w*(?:\(\))?\.)*instruction(?:_literal)?\s*\(')
BRACES=re.compile(r'[{}]')
LITERAL_START=re.compile(r'(?:0x[0-9a-fA-F]+[uU]?\b|selected\s*\(|(?:world|damage)_observation_pc\s*\(|\(s\.image==SourcePhysicsContactImage::MapPack11\?)')
CANONICAL_ONLY_CALLBACK_SOURCES={
    'src/gameplay/animation/unit_animation_selection_runtime.cpp',
    'src/gameplay/weapons/unit_pickup_runtime.cpp',
    'src/gameplay/weapons/unit_weapon_routing.cpp',
}


def masked(text):return NON_CODE.sub(lambda match:' '*len(match[0]),text)


def literal_callbacks(mask):
    for callback in CALLBACK.finditer(mask):
        at=callback.end()
        while at<len(mask) and mask[at].isspace():at+=1
        if LITERAL_START.match(mask,at):
            yield callback


def literal_source_paths(root):
    """Generated units plus audited literal owners, independent of filename."""
    for path in sorted((Path(root)/'src').rglob('*')):
        if not path.is_file() or path.suffix not in {'.cpp','.hpp','.h','.c'}:continue
        if path.name.endswith('_generated.cpp') or any(literal_callbacks(masked(path.read_text()))):yield path


def qualified_dispatch_entries(text,image):
    """Actual owner targets from the complete simple internal dispatcher grammar.

    Public entry selectors and callback labels can use a canonical namespace.
    Accept only the internal actual-target dispatcher, with no extra statements,
    and a complete standard selected() resolver where an operand needs it.
    """
    if image not in {'tu15','map-pack11'}:return {}
    mask=masked(text);functions={}
    for match in FUNCTION.finditer(mask):
        if match['name'] not in {'dispatch','selected'}:continue
        depth=1
        for brace in BRACES.finditer(mask,match.end()):
            depth+=1 if brace[0]=='{'else -1
            if not depth:
                functions[match['name']]=text[match.start():brace.end()];break
    dispatcher=functions.get('dispatch','')
    signature=re.fullmatch(r'\s*static void dispatch\(\w+ &s,std::uint32_t target,std::uint32_t site,bool tail\)(\{.*\})',dispatcher,re.S)
    if not signature:return {}
    expression=r'(?:selected\(0x[0-9a-f]+u,s\.image\)|0x[0-9a-f]+u)'
    branch=rf'if\(target==({expression})\)\{{run_([0-9a-f]+)\(s\);return;\}}'
    case=rf'case ({expression}):run_([0-9a-f]+)\(s\);return;'
    body=signature[1]
    if re.fullmatch(r'\{(?:'+branch+r')+s\.required\(site,target,tail\);(?:return;)?\}',body):pairs=re.findall(branch,body)
    elif re.fullmatch(r'\{switch\(target\)\{(?:'+case+r')+default:s\.required\(site,target,tail\);return;\}\}',body):pairs=re.findall(case,body)
    else:return {}
    resolver=functions.get('selected','');mapping=None
    if any(value.startswith('selected(')for value,symbol in pairs):
        grammar=(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{'
            r'if\(image==SourcePhysicsContactImage::MapPack11\)return p;switch\(p\)\{'
            r'((?:case 0x[0-9a-f]+u:return 0x[0-9a-f]+u;)+)'
            r'default:throw std::runtime_error\("(?:[^"\\]|\\.)*"\);\}\}')
        parsed=re.fullmatch(grammar,resolver)
        if not parsed:return {}
        cases=re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',parsed[1]);mapping={int(a,16):int(b,16)for a,b in cases}
        if len(mapping)!=len(cases):return {}
    candidates={}
    for value,symbol in pairs:
        operand=int(re.search(r'0x([0-9a-f]+)u',value)[1],16)
        actual=mapping.get(operand)if value.startswith('selected(')and image=='tu15'else operand
        if actual is not None:candidates.setdefault(symbol,set()).add(actual)
    return {symbol:dict(entry=next(iter(values)),dispatcher_sha256=hashlib.sha256(dispatcher.encode()).hexdigest(),
        selected_resolver_sha256=hashlib.sha256(resolver.encode()).hexdigest()if mapping is not None else None,
        basis='Complete simple internal actual-target dispatcher; selected resolver authenticated by exact grammar.')
        for symbol,values in candidates.items()if len(values)==1}


@dataclass(frozen=True)
class Signature:
    begin:int
    opening_end:int
    function_name:str
    state_type:str|None
    @property
    def symbol(self):
        match=re.fullmatch(r'run_([0-9a-f]+)',self.function_name)
        return match[1] if match else self.function_name
    def __getitem__(self,index):return {1:self.symbol,2:self.state_type or 'local-state'}[index]
    def start(self):return self.begin
    def end(self):return self.opening_end


def function_bodies(text):
    mask=masked(text);owners=[]
    for match in FUNCTION.finditer(mask):
        if owners and match.start()<owners[-1][1]:continue
        if match['name'] in {'if','for','while','switch','catch'}:continue
        depth=1;end=None
        for brace in BRACES.finditer(mask,match.end()):
            depth+=1 if brace[0]=='{' else -1
            if not depth:end=brace.end();break
        if end is None:raise ValueError('Unclosed generated function '+match['name'])
        body=mask[match.start():end]
        if not re.fullmatch(r'run_[0-9a-f]+',match['name']) and not any(literal_callbacks(body)):continue
        state=re.search(r'\b(\w+)\s*&\s*s\b',match['parameters'])
        signature=Signature(match.start(),match.end(),match['name'],state[1] if state else None)
        owners.append((match.start(),end))
        yield signature,text[match.start():end]
    starts=[begin for begin,end in owners]
    for callback in literal_callbacks(mask):
        index=bisect_right(starts,callback.start())-1
        if index<0 or callback.start()>=owners[index][1]:
            raise ValueError('Literal instruction callback has no recognized generated function owner at offset '+str(callback.start()))
