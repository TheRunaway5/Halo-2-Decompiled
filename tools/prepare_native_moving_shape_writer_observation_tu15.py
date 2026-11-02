#!/usr/bin/env python3
"""Mechanical retained-store capability propagation; frozen source/private overlay."""
from pathlib import Path
import hashlib,json,re,difflib
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/physics/native-moving-shape-process-qualified-actual1479-20261102/source'
OUT=ROOT/'analysis/physics/native-moving-shape-writer-observation-stage-20261102'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def close_paren(s,start):
 assert s[start]=='(';depth=0
 for i in range(start,len(s)):
  if s[i]=='(':depth+=1
  elif s[i]==')':
   depth-=1
   if depth==0:return i
 raise AssertionError('Unbalanced expression')
def main():
 OUT.mkdir(exist_ok=True);staged=OUT/'staged';sources={};before={};details=[];unmatched=[];diffs=[]
 excluded={'native_tu15_convex_simplex_single.cpp','native_tu15_contact_point_create.cpp'}
 for path in sorted((BASE/'src/physics').glob('*.cpp')):
  if path.name in excluded:continue
  old=path.read_text()
  if 'SourcePhysicsContactRecordStore'not in old:continue
  text=old;constructors=0
  # Preserve existing explicit reference/value semantics. Only implicit
  # callbacks use the new stable provider reference or forwarding value.
  source_header=BASE/'include/halo2'/path.name.replace('.cpp','.hpp');types=old+'\n'+(source_header.read_text()if source_header.exists()else'')
  observer_fields=set(re.findall(r'SourcePhysicsContactRecordObserver\s*&?\s*(\w+_)\s*[;,{]',types))
  for match in reversed(list(re.finditer(r'\b(\w+_)\((\w+)\)',text))):
   field,arg=match.groups()
   if field not in observer_fields:continue
   prefix=text[max(0,match.start()-600):match.start()];memories=list(re.finditer(r'\b(?:memory_|m_)\((\w+)\)',prefix))
   if not memories:continue
   memory=memories[-1].group(1)
   # A semicolon or completed constructor body separates unrelated objects.
   tail=prefix[memories[-1].end():]
   if ';'in tail or '{'in tail:continue
   reference=bool(re.search(r'SourcePhysicsContactRecordObserver\s*&\s*'+re.escape(field)+r'\b',types))
   function='native_tu15_effective_store_observer'if reference else'native_tu15_copy_effective_store_observer'
   text=text[:match.start()]+field+'('+function+'('+memory+','+arg+'))'+text[match.end():];constructors+=1
  events=list(re.finditer(r'SourcePhysicsContactRecordStore\s+(\w+)\s*\{[^;]*?\};',text,re.S));converted=0
  for match in reversed(events):
   event=match.group(1);tail=text[match.end():];callback=re.search(r'\b(observe_|observer_|o_|observe|observer|o)\s*\(\s*'+re.escape(event)+r'\s*\)',tail)
   if not callback:unmatched.append((path.name,event,'callback'));continue
   end=match.end()+callback.end();segment=text[match.start():end];resize=re.search(re.escape(event)+r'\.before\.resize\s*\(',segment)
   if not resize:unmatched.append((path.name,event,'before resize'));continue
   left=segment.index('(',resize.start());right=close_paren(segment,left);width=segment[left+1:right]
   memory=re.search(r'\b(\w+)\.read(?:_vector)?\s*\([^;]*?\b'+re.escape(event)+r'\.before',segment,re.S)
   if not memory:unmatched.append((path.name,event,'memory'));continue
   mem=memory.group(1);obs=callback.group(1);active='observing_'+event;effective='effective_'+event
   body=segment[match.end()-match.start():]
   condition=r'if\s*\(\s*'+re.escape(obs)+r'\s*\)'
   if not re.search(condition,body):unmatched.append((path.name,event,'observer condition'));continue
   body=re.sub(condition,'if('+active+')',body)
   body=re.sub(r'\b'+re.escape(obs)+r'\s*\(\s*'+re.escape(event)+r'\s*\)',effective+'('+event+')',body)
   declaration='const auto&'+effective+'=native_tu15_effective_store_observer('+mem+','+obs+');const bool '+active+'=native_tu15_observes_retained_store('+mem+','+effective+','+event+'.instruction,'+event+'.address,'+width+');'
   text=text[:match.start()]+match.group(0)+declaration+body+text[end:];converted+=1
  if text!=old:
   text='#include "halo2/native_tu15_retained_store_observer.hpp"\n'+text;name=str(path.relative_to(BASE));q=staged/name;q.parent.mkdir(parents=True,exist_ok=True);q.write_text(text);before[name]=sha(path);sources[name]=sha(q);details.append({'source':name,'constructor_resolutions':constructors,'stores_converted':converted});diffs.extend(difflib.unified_diff(old.splitlines(True),text.splitlines(True),fromfile='a/'+name,tofile='b/'+name))
 # The selected solver cache publication used a direct private helper with no
 # observer parameter. Add its authenticated Source labels without changing
 # arithmetic, source read order, cache identities or byte widths.
 name='src/physics/native_tu15_convex_simplex_solver.cpp';path=BASE/name;old=path.read_text();text=old
 start=text.index('void narrow_store(');end=text.index('\n}',start)+2
 body='''void narrow_store(SourcePhysicsContactMemory&memory,std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned n){
    std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(value>>(k*8u));
    const SourcePhysicsContactRecordObserver none;
    const auto&observe=native_tu15_effective_store_observer(memory,none);
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    const bool observed=native_tu15_observes_retained_store(memory,observe,pc,p,n);
    if(observed){event.before.resize(n);memory.read(p,event.before);event.written.assign(b.begin(),b.begin()+n);}
    memory.write(p,std::span(b).first(n));if(observed)observe(event);
}'''
 text=text[:start]+body+text[end:]
 replacements={'narrow_store(memory,input.support_cache+8u,':'narrow_store(memory,0x30f68fu,input.support_cache+8u,','narrow_store(memory,input.support_cache+9u,':'narrow_store(memory,0x30f694u,input.support_cache+9u,','narrow_store(memory,input.support_cache+written*2u,':'narrow_store(memory,0x30f6a3u,input.support_cache+written*2u,','narrow_store(memory,target,':'narrow_store(memory,0x30f6c3u,target,'}
 for a,b in replacements.items():assert text.count(a)==1;text=text.replace(a,b)
 text='#include "halo2/native_tu15_retained_store_observer.hpp"\n'+text;target=staged/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text);before[name]=sha(path);sources[name]=sha(target);details.append({'source':name,'constructor_resolutions':0,'stores_converted':1,'source_site_labels_added':[0x30f68f,0x30f694,0x30f6a3,0x30f6c3]});diffs.extend(difflib.unified_diff(old.splitlines(True),text.splitlines(True),fromfile='a/'+name,tofile='b/'+name))
 report={'qualified':False,'source_sha256':sources,'base_sha256':before,'details':details,'unmatched':unmatched,'no_root_files_modified':True,'scope':'Private mechanical capability propagation only; actual observation/default/fault proofs remain required.'};(OUT/'source-pins.json').write_text(json.dumps(report,indent=2)+'\n');(OUT/'writers.diff').write_text(''.join(diffs));print(json.dumps({'files':len(sources),'constructors':sum(x['constructor_resolutions']for x in details),'stores':sum(x['stores_converted']for x in details),'unmatched':unmatched},indent=2))
if __name__=='__main__':main()
