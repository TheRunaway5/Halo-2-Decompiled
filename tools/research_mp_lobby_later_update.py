#!/usr/bin/env python3
"""Independent genuine-byte lobby/options correspondence in owned u5English.

Prefixes only locate candidates. Complete instructions, internal branch/switch
topology, owning direct-call constraints and every image-address pair are kept.
No annotation labels, old address reuse or supplied runtime state is used.
"""
import argparse,collections,csv,difflib,hashlib,json,re,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_IMM,X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN
from oracle_mp_spawn_lobby_source import BASE_PIN,ENTRIES as BASE_ENTRIES,OUT as BASE_OUT
from oracle_mp_spawn_local_map_clock import FROZEN as OLDER,ENTRIES as CLOCK_ENTRIES,OUT as CLOCK_OUT
from functions import _trace
NEW=Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
NEW_PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
OUT=ROOT/'analysis/multiplayer/title-update-1.5/lobby-owners'
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_local_map_clock.py':'bf8904ed88b6d71ac950d53a3247b5cd2ad5ecc0af3ea54efae5ec1ba6abb882',
 'docs/MP_LOCAL_MAP_CLOCK_RESEARCH.md':'621a3d59d48586fd577fab4b7a7010ec5d49dc8182d23a7a3a70885d2bd8dd9b',
 'analysis/multiplayer/integration/spawn-source/local-map-clock/report.json':'f4b6533c314d04505f07a270df9e5fb5d204da31aa57e70af1e477ef91276372',
 'analysis/multiplayer/integration/spawn-source/local-map-clock/independent-freeze-verification.json':'26313b3bc8af8440678e67eebade3adfd7424623b3bb707d42d9cbffccfcac65',
 'analysis/multiplayer/integration/spawn-source/local-map-clock/measured-native-clock-inputs.json':'8746659256c8ea69de32f67cd144bb406357fbc64dd633b0f5bb4be0d01e9316',
 'tools/reference/halo2-decompiled/tools/functions.py':'dbfcde80005b4cd8ddee18289ce3c6aaccdc7e8f69423b4d983628fed82a353c',
 'tools/reference/halo2-decompiled/tools/xbe.py':'3bc3717d8a78450690ad9efa9eabf9ffa45a68e1207dde554a820c4cd80413d4'}
_clock=json.loads((CLOCK_OUT/'report.json').read_text())
for _r in _clock['instruction_listings']+[_clock['executed_original_listing'],_clock['observed_map_clock_store_listing']]:FROZEN[str((CLOCK_OUT/'instructions'/_r['file']).relative_to(ROOT))]=_r['sha256']
for _n,_h in _clock['compiled_compatibility_timer']['primary_sources'].items():FROZEN[str((CLOCK_OUT/'primary-source'/_n).relative_to(ROOT))]=_h
for _n,_key in [('compiled-timer.cpp','source_sha256'),('compiled-timer.exe','executable_sha256')]:FROZEN[str((CLOCK_OUT/_n).relative_to(ROOT))]=_clock['compiled_compatibility_timer'][_key]
_base=json.loads((BASE_OUT/'report.json').read_text())
for _r in _base['instruction_listings']:FROZEN[str((BASE_OUT/'instructions'/_r['file']).relative_to(ROOT))]=_r['sha256']
TUP_ENTRIES=(0x139880,0x139540,0x123b0,0x169d0,0x190ba0,0x190bf0,0x23855a,0x6f190,
 0x190a30,0x139410,0x1395b0,0x11d780,0x68300,0x69170,0x69e90,0x6b650,0x6b750,
 0x188ad0,0x1884e0,0x187610,0x69280,0x852a0,0x147d20,0x147cc0,0x337a40)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def containing(image,p):return next((s for s in image.sections if s.va<=p<s.va+s.vsize),None)
def normalized(i,image):
 op=i.op_str
 if(i.mnemonic.startswith('j')or i.mnemonic=='call')and op.startswith('0x'):op='DIRECT'
 else:
  # Absolute/indexed image-memory displacements and MOV/PUSH image-address
  # immediate candidates may relocate. Arithmetic/compare immediates stay exact.
  def replacement(m):
   before=op[:m.start()];inside=before.rfind('[')>before.rfind(']')
   return'IMAGE_ADDRESS'if(inside or i.mnemonic in('mov','push'))and containing(image,int(m.group(),16))else m.group()
  op=re.sub(r'0x[0-9a-f]+',replacement,op)
 return i.mnemonic+' '+op
def complete_tokens(ins,image):
 positions={i.address:n for n,i in enumerate(ins)};tokens=[]
 for i in ins:
  token=normalized(i,image)
  if i.mnemonic.startswith('j')and len(i.operands)==1 and i.operands[0].type==X86_OP_IMM:
   target=i.operands[0].imm;token+=' INDEX='+str(positions[target])if target in positions else' EXTERNAL'
  tokens.append(token)
 return tokens
def backing(image,p,n):
 s=containing(image,p);assert s
 o=p-s.va;return dict(section=s.name,section_virtual_address=f'{s.va:08x}',raw_file_backed=o+n<=s.rsize,bytes_hex=image.read(p,n).hex(),zero_fill_bytes=max(0,n-max(0,s.rsize-o)))
def references(left,right,old,new):
 rows=[]
 for a,b in zip(left,right):
  for k,(oa,ob)in enumerate(zip(a.operands,b.operands)):
   if oa.type==ob.type==X86_OP_MEM and containing(old,oa.mem.disp)and containing(new,ob.mem.disp):
    assert oa.size==ob.size;rows.append(dict(old_site=f'{a.address:08x}',later_site=f'{b.address:08x}',operand=k,kind='image_memory_displacement',old_address=f'{oa.mem.disp:08x}',later_address=f'{ob.mem.disp:08x}',width=oa.size,old=backing(old,oa.mem.disp,oa.size),later=backing(new,ob.mem.disp,ob.size)))
   elif oa.type==ob.type==X86_OP_IMM and a.mnemonic in('mov','push')and containing(old,oa.imm)and containing(new,ob.imm):
    rows.append(dict(old_site=f'{a.address:08x}',later_site=f'{b.address:08x}',operand=k,kind='image_immediate_candidate',old_address=f'{oa.imm:08x}',later_address=f'{ob.imm:08x}',width=4,old=backing(old,oa.imm,4),later=backing(new,ob.imm,4)))
 return rows
def calls(ins):return[(n,i.address,i.operands[0].imm)for n,i in enumerate(ins)if i.mnemonic=='call'and len(i.operands)==1 and i.operands[0].type==X86_OP_IMM]
def alignment(oi,ni,old,new):
 left=complete_tokens(oi,old);right=complete_tokens(ni,new)
 matcher=difflib.SequenceMatcher(None,left,right,autojunk=False)
 pairs={a+j:b+j for a,b,n in matcher.get_matching_blocks()for j in range(n)}
 changes=[dict(kind=tag,old_instruction_indices=[a,b],later_instruction_indices=[c,d],old_instructions=[f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in oi[a:b]],later_instructions=[f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in ni[c:d]])for tag,a,b,c,d in matcher.get_opcodes()if tag!='equal']
 return pairs,changes
def raw_operands(ins,image):
 refs=[];immediates=[];cc=[]
 for i in ins:
  if i.mnemonic=='call':cc.append(dict(site=f'{i.address:08x}',bytes_hex=i.bytes.hex(),operand=i.op_str,direct_target=f'{i.operands[0].imm:08x}'if len(i.operands)==1 and i.operands[0].type==X86_OP_IMM else None))
  for k,o in enumerate(i.operands):
   if o.type==X86_OP_IMM:immediates.append(dict(site=f'{i.address:08x}',operand=k,value=o.imm,hex=f'{o.imm&0xffffffff:08x}'))
   address=o.mem.disp if o.type==X86_OP_MEM else o.imm if o.type==X86_OP_IMM and not(i.mnemonic.startswith('j')or i.mnemonic=='call')else None
   if address is not None and containing(image,address):
    n=o.size if o.type==X86_OP_MEM else 4
    refs.append(dict(site=f'{i.address:08x}',operand=k,instruction=f'{i.mnemonic} {i.op_str}',kind='image_memory_displacement'if o.type==X86_OP_MEM else'image_valued_immediate_candidate_not_semantic_type',address=f'{address:08x}',width=n,backing=backing(image,address,n)))
 return dict(image_reference_operands=refs,immediate_operands=immediates,calls=cc)
def table_pair(of,nf,oi,ni,old,new):
 if len(of.tables)!=len(nf.tables):return None
 op={i.address:n for n,i in enumerate(oi)};np={i.address:n for n,i in enumerate(ni)};rows=[]
 for(a,w,n),(b,ww,nn)in zip(of.tables,nf.tables):
  if(w,n)!=(ww,nn):return None
  left,right=old.read(a,w*n),new.read(b,w*n)
  if w==4:
   lv=struct.unpack('<'+'I'*n,left);rv=struct.unpack('<'+'I'*n,right)
   lt=[op.get(v,'ZERO'if v==0 else'EXTERNAL')for v in lv];rt=[np.get(v,'ZERO'if v==0 else'EXTERNAL')for v in rv];same=lt==rt
  else:lt=rt=None;same=left==right
  rows.append(dict(old_address=f'{a:08x}',later_address=f'{b:08x}',width=w,entries=n,old_hex=left.hex(),later_hex=right.hex(),old_target_instruction_indices=lt,later_target_instruction_indices=rt,topology_equal=same))
 return rows

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--new',type=Path,default=NEW);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
 for p,h in FROZEN.items():assert sha(ROOT/p)==h,p
 assert sha(args.new)==NEW_PIN;new=Xbe(args.new);nc=_Code(new,'.text')
 images={};codes={};old_starts={};entries={'base':BASE_ENTRIES+(0x199f34,0x199bbf,0x59280),'tu1_1':TUP_ENTRIES}
 for group,p,pin,starts in [('base',ROOT/'assets/disc/default.xbe',BASE_PIN,ROOT/'analysis/retail/functions.tsv'),('tu1_1',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',PIN,ROOT/'analysis/map-pack-update/functions.tsv')]:
  assert sha(p)==pin;images[group]=Xbe(p);codes[group]=_Code(images[group],'.text');old_starts[group]={int(r['address'],16)for r in csv.DictReader(starts.open(),delimiter='\t')}|set(BASE_ENTRIES if group=='base'else CLOCK_ENTRIES)|set(entries[group])
 # Linear decoding supplies candidate locations/direct call target boundaries
 # only. Selected routines are then recursively decoded from actual new bytes.
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.skipdata=True;sec=new.section('.text');linear=list(md.disasm(new.section_bytes(sec),sec.va));tokens=[normalized(i,new)for i in linear]
 wanted=collections.defaultdict(set);old_functions={};old_traces={};old_sources={}
 for group,ee in entries.items():
  for e in ee:
   key=(group,e);f=_trace(codes[group],e,old_starts[group]);ins=[codes[group].at(p)for p in sorted(f.code)];old_functions[key]=ins;old_traces[key]=f
   old_sources[key]=next((r for r in(_base if group=='base'else _clock)['instruction_listings']if int(r['entry'],16)==e),None)
   if old_sources[key]:
    olddir=BASE_OUT if group=='base'else CLOCK_OUT;p=olddir/'instructions'/old_sources[key]['file'];literal=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins);assert p.read_text()==literal,(group,hex(e))
   for n in sorted({min(v,len(ins))for v in(16,12,8)},reverse=True):wanted[n].add(tuple(normalized(i,images[group])for i in ins[:n]))
 index={n:collections.defaultdict(list)for n in wanted}
 for n,keys in wanted.items():
  for p in range(len(tokens)-n+1):
   t=tuple(tokens[p:p+n])
   if t in keys:index[n][t].append(linear[p].address)
 new_starts=set()
 detail=Cs(CS_ARCH_X86,CS_MODE_32);detail.detail=True;detail.skipdata=True
 for i in detail.disasm(new.section_bytes(sec),sec.va):
  if i.mnemonic=='call'and len(i.operands)==1 and i.operands[0].type==X86_OP_IMM and nc.inside(i.operands[0].imm):new_starts.add(i.operands[0].imm)
 rows={};candidates={};nf_cache={}
 for key,oi in old_functions.items():
  group,e=key;locations=[];prefix=0
  for n in sorted({min(v,len(oi))for v in(16,12,8)},reverse=True):
   found=index[n].get(tuple(normalized(i,images[group])for i in oi[:n]),[])
   if found:locations=found;prefix=n
   if len(found)==1:break
  rows[key]=dict(source_group=group,old_entry=f'{e:08x}',old_instruction_rows=len(oi),frozen_old_listing_sha256=old_sources[key]['sha256']if old_sources[key]else None,prefix_instructions=prefix,prefix_candidates=[f'{p:08x}'for p in locations]);candidates[key]=[]
  for p in locations:
   if p not in nf_cache:
    f=_trace(nc,p,new_starts|{p});nf_cache[p]=(f,[nc.at(a)for a in sorted(f.code)])
   nf,ni=nf_cache[p];tables=table_pair(old_traces[key],nf,oi,ni,images[group],new)
   if complete_tokens(oi,images[group])==complete_tokens(ni,new)and tables is not None and all(r['topology_equal']for r in tables):candidates[key].append(p)
  rows[key]['complete_shape_and_branch_candidates']=[f'{p:08x}'for p in candidates[key]]
 # Small repeated getters are admitted only when actual owning direct callers
 # and already proved callee identities leave exactly one candidate.
 mapped={k:v[0]for k,v in candidates.items()if len(v)==1};resolution=[];exact_keys=set(mapped)
 for turn in range(len(rows)):
  changed=False
  for key,cc in candidates.items():
   if key in mapped:continue
   group,e=key;valid=[];evidence=[]
   for p in cc:
    ni=nf_cache[p][1];good=True;rules=[]
    # Actual global aliases already proved by complete routine correspondence
    # constrain repeated allocator wrappers without choosing a familiar entry.
    aliases=collections.defaultdict(set)
    for owner,laterowner in mapped.items():
     if owner[0]==group:
      for ref in references(old_functions[owner],nf_cache[laterowner][1],images[group],new):aliases[int(ref['old_address'],16)].add(int(ref['later_address'],16))
    for ref in references(old_functions[key],ni,images[group],new):
     address=int(ref['old_address'],16);actual=int(ref['later_address'],16)
     if len(aliases[address])==1:
      expected=next(iter(aliases[address]));good&=actual==expected;rules.append(dict(kind='proved_shared_image_address',old_address=f'{address:08x}',actual_later_address=f'{actual:08x}',required_later_address=f'{expected:08x}'))
    for n,a,target in calls(old_functions[key]):
     if(group,target)in mapped:
      actual=ni[n].operands[0].imm;expected=mapped[(group,target)];good&=actual==expected;rules.append(dict(kind='owning_outgoing_call',old_site=f'{a:08x}',later_site=f'{ni[n].address:08x}',old_target=f'{target:08x}',actual_later_target=f'{actual:08x}',required_later_target=f'{expected:08x}'))
    for caller,newcaller in mapped.items():
     if caller[0]!=group:continue
     for n,a,target in calls(old_functions[caller]):
      if target==e:
       actual=nf_cache[newcaller][1][n].operands[0].imm;good&=actual==p;rules.append(dict(kind='proved_incoming_caller',old_caller=f'{caller[1]:08x}',later_caller=f'{newcaller:08x}',old_site=f'{a:08x}',actual_later_target=f'{actual:08x}'))
    if good:valid.append(p);evidence.append(dict(candidate=f'{p:08x}',actual_call_constraints=rules))
   rows[key]['after_owning_call_candidates']=[f'{p:08x}'for p in valid]
   if len(valid)==1:
    mapped[key]=valid[0];exact_keys.add(key);rows[key]['identity_resolution']=evidence[0];resolution.append(dict(pass_index=turn,source_group=group,old_entry=f'{e:08x}',later_entry=f'{valid[0]:08x}'));changed=True
  if not changed:break
 # Changed bodies are identified separately from exact correspondence. A
 # proved caller's aligned actual CALL destination is stronger than a prefix;
 # unique prefix candidates require multiple already proved callee relations.
 for turn in range(len(rows)):
  changed=False
  for key,row in rows.items():
   if key in mapped:continue
   group,e=key;incoming=[]
   for caller,latercaller in mapped.items():
    if caller[0]!=group:continue
    ni=nf_cache[latercaller][1];pairs,_=alignment(old_functions[caller],ni,images[group],new)
    for n,a,target in calls(old_functions[caller]):
     if target==e and n in pairs:
      latercall=ni[pairs[n]]
      if latercall.mnemonic=='call'and len(latercall.operands)==1 and latercall.operands[0].type==X86_OP_IMM:incoming.append(dict(kind='proved_owning_caller_actual_aligned_destination',old_caller=f'{caller[1]:08x}',later_caller=f'{latercaller:08x}',old_site=f'{a:08x}',later_site=f'{latercall.address:08x}',actual_later_target=f'{latercall.operands[0].imm:08x}',caller_complete_shape_equal=caller in exact_keys))
   targets={int(r['actual_later_target'],16)for r in incoming}
   candidate=None;evidence=[]
   if len(targets)==1:candidate=next(iter(targets));evidence=incoming
   elif not targets and len(row['prefix_candidates'])==1:
    p=int(row['prefix_candidates'][0],16);ni=nf_cache[p][1];pairs,_=alignment(old_functions[key],ni,images[group],new);rules=[];good=True
    for n,a,target in calls(old_functions[key]):
     if(group,target)in mapped and n in pairs:
      latercall=ni[pairs[n]];actual=latercall.operands[0].imm;expected=mapped[(group,target)];good&=actual==expected;rules.append(dict(kind='unique_prefix_aligned_proved_callee',old_site=f'{a:08x}',later_site=f'{latercall.address:08x}',old_target=f'{target:08x}',actual_later_target=f'{actual:08x}',required_later_target=f'{expected:08x}'))
    if good and len(rules)>=2:candidate=p;evidence=rules
    elif good and rules and len(old_functions[key])==len(ni):
     # A remaining ADD/CMP literal can be a real relocated address, but is
     # never normalized merely because its number lies inside the image.
     # Require complete instruction/branch equality except such operands,
     # actual already-proved shared globals, and a proved callee relation.
     aliases=collections.defaultdict(set)
     for owner,laterowner in mapped.items():
      if owner[0]==group and owner in exact_keys:
       for ref in references(old_functions[owner],nf_cache[laterowner][1],images[group],new):aliases[int(ref['old_address'],16)].add(int(ref['later_address'],16))
     lt=complete_tokens(old_functions[key],images[group]);rt=complete_tokens(ni,new);relocs=[];equal=True
     for a,b,l,r in zip(old_functions[key],ni,lt,rt):
      if l==r:continue
      evidence_pair=None
      if a.mnemonic==b.mnemonic and a.mnemonic in('add','cmp')and len(a.operands)==len(b.operands)==2 and a.operands[1].type==b.operands[1].type==X86_OP_IMM:
       oa,ob=a.operands[1].imm,b.operands[1].imm
       if containing(images[group],oa)and containing(new,ob)and l.replace(hex(oa),'PROVED_ADDRESS')==r.replace(hex(ob),'PROVED_ADDRESS'):
        possible=[(anchor,next(iter(v)))for anchor,v in aliases.items()if len(v)==1 and containing(images[group],anchor)==containing(images[group],oa)and ob-next(iter(v))==oa-anchor]
        if possible:
         anchor,relocated=min(possible,key=lambda v:abs(v[0]-oa));evidence_pair=dict(kind='arithmetic_address_relocation_from_proved_shared_global',old_site=f'{a.address:08x}',later_site=f'{b.address:08x}',old_address=f'{oa:08x}',later_address=f'{ob:08x}',proved_old_anchor=f'{anchor:08x}',proved_later_anchor=f'{relocated:08x}',same_byte_offset=oa-anchor)
      if evidence_pair is None:equal=False;break
      relocs.append(evidence_pair)
     if equal and relocs:candidate=p;evidence=rules+relocs
   if candidate is not None:
    if candidate not in nf_cache:
     f=_trace(nc,candidate,new_starts|{candidate});nf_cache[candidate]=(f,[nc.at(a)for a in sorted(f.code)])
    mapped[key]=candidate;row['identity_resolution']=dict(candidate=f'{candidate:08x}',actual_instruction_context=evidence);resolution.append(dict(pass_index=turn,source_group=group,old_entry=f'{e:08x}',later_entry=f'{candidate:08x}',changed_body_identity=True));changed=True
  if not changed:break
 insdir=args.output.parent/'instructions';insdir.mkdir(exist_ok=True)
 for key,row in rows.items():
  group,e=key;oi=old_functions[key]
  q=insdir/f'{group}-{e:08x}-old-{e:08x}.asm';q.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in oi));row['old_listing']=dict(file=q.name,sha256=sha(q),rows=len(oi));row['old_literal_operands']=raw_operands(oi,images[group])
  if key not in mapped:
   row['status']='unresolved_no_unique_original_owner_correspondence';row['retained_prefix_candidate_listings']=[]
   for phex in row['prefix_candidates']:
    p=int(phex,16);ni=nf_cache[p][1];q=insdir/f'{group}-{e:08x}-unadmitted-prefix-{p:08x}.asm';q.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ni));row['retained_prefix_candidate_listings'].append(dict(candidate=phex,file=q.name,sha256=sha(q),rows=len(ni),instruction_differences=alignment(oi,ni,images[group],new)[1],literal_operands=raw_operands(ni,new)))
   continue
  p=mapped[key];nf,ni=nf_cache[p];pairs,diffs=alignment(oi,ni,images[group],new);isexact=complete_tokens(oi,images[group])==complete_tokens(ni,new);paired_old=[oi[a]for a in pairs];paired_later=[ni[b]for b in pairs.values()]
  row.update(status='complete_instruction_and_owning_call_correspondence'if isexact else'changed_body_identified_through_original_owning_instruction_context',later_entry=f'{p:08x}',later_instruction_rows=len(ni),complete_normalized_instructions_equal=isexact,internal_branch_topology_equal=isexact,switch_tables=table_pair(old_traces[key],nf,oi,ni,images[group],new),paired_image_references=references(paired_old,paired_later,images[group],new),instruction_differences=diffs,paired_direct_calls=[])
  row['later_literal_operands']=raw_operands(ni,new)
  for n,a,target in calls(oi):
   if n not in pairs:continue
   latercall=ni[pairs[n]];actual=latercall.operands[0].imm;expected=mapped.get((group,target));row['paired_direct_calls'].append(dict(old_site=f'{a:08x}',later_site=f'{latercall.address:08x}',old_target=f'{target:08x}',later_target=f'{actual:08x}',expected_later_target=f'{expected:08x}'if expected is not None else None,mapped_target_matches=expected==actual if expected is not None else None))
  for label,ii,image in [('old',oi,images[group]),('later',ni,new)]:
   for i in ii:assert image.read(i.address,len(i.bytes))==i.bytes
   q=insdir/f'{group}-{e:08x}-{label}-{e if label=="old"else p:08x}.asm';q.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ii));row[label+'_listing']=dict(file=q.name,sha256=sha(q),rows=len(ii))
 authpath=ROOT/'analysis/multiplayer/title-update-1.5/dlc-authentication.json';auth=json.loads(authpath.read_text());owned=next(r for r in auth['executables']if r['path'].endswith('u5English/default.xbe'));assert owned['sha256']==NEW_PIN and owned['retail_authenticated']and owned['all_section_hashes_valid']
 report=dict(schema=2,tool_sha256=sha(__file__),frozen_pins=FROZEN,source_images={'base':BASE_PIN,'tu1_1':PIN,'later_owned':NEW_PIN},later_original_path=str(args.new),later_text_sha256=hashlib.sha256(new.section_bytes(sec)).hexdigest(),authentication_source_sha256=sha(authpath),authentication=dict(retail_header_signature_valid=owned['retail_header_signature_valid'],all_section_hashes_valid=owned['all_section_hashes_valid'],retail_authenticated=owned['retail_authenticated']),new_boundaries_source='actual later direct CALL destinations; selected own entry for each candidate; no annotation or decompiler name applied',source_groups={k:len(v)for k,v in entries.items()},routines=list(rows.values()),owning_call_identity_resolution=resolution,summary=dict(requested=len(rows),mapped=len(mapped),complete_shape_matches=sum(r.get('complete_normalized_instructions_equal',False)for r in rows.values()),changed_body_owner_identities=sum(r.get('status','').startswith('changed_body_')for r in rows.values()),unresolved=len(rows)-len(mapped),complete_old_and_later_instruction_rows=sum(r['old_instruction_rows']+r.get('later_instruction_rows',0)for r in rows.values()),mapped_call_constraints_verified=sum(c['mapped_target_matches']is True for r in rows.values()for c in r.get('paired_direct_calls',[])),mapped_call_mismatches=sum(c['mapped_target_matches']is False for r in rows.values()for c in r.get('paired_direct_calls',[]))),executed_guest_instructions=0,instruction_patches=[],runtime_or_platform_values_supplied=[],native_gameplay_update_applied=False,boundaries=[
 'The authenticated owned u5English originalXBE bytes are primary authority. Its unseeded decompiler/annotation names, comments and author address labels are not used to select or admit any mapping.',
 'All frozen1.1/base source suites remain read-only and every prior pin is verified before/after. Prefix shapes only locate candidates. Exact instruction/branch/switch correspondence is a separate status from changed-body owner identity through actual proved caller destinations or unique prefix plus independently proved callee relations. Every changed body is retained with its complete literal difference list.',
 'Only image-address candidates in MOV/PUSH or memory operands may normalize; arithmetic/compare/count/timing literals remain exact. An ADD/CMP address may independently identify a changed body only through the exact same offset from an already proved shared global and a proved callee relation; it remains a literal reported difference. All raw addresses, instruction bytes, calls, immediate operands and static image-reference bytes are retained, including unpaired changed operands. Image-valued immediate candidates do not imply semantic pointer types. Zero-filled virtual section tails are not runtime initialization evidence.',
 'Matching instruction/control-flow correspondence is bounded static evidence, not full call-closure behavior/runtime parity. External unpaired callee bodies, local profiles/device/session ownership, clock scheduling, world readiness and original countdown lifecycle remain unexecuted. No admitted lobby or local participant is supplied.',
 'The Xbox executable does not establish SDL desktop window ownership. This suite supports original menu/count/options/readiness instruction contracts only; the host window implementation remains a separate native concern.'])
 report['instruction_listing_manifest']={p.name:sha(p)for p in sorted(insdir.glob('*.asm'))}
 report['summary']['complete_retained_instruction_rows']=sum(len(p.read_text().splitlines())for p in insdir.glob('*.asm'))
 assert report['summary']['mapped_call_mismatches']==0
 raw=(json.dumps(report,indent=2,sort_keys=True)+'\n').encode()
 if args.output.exists():assert args.output.read_bytes()==raw,('Refusing changed frozen later report',str(args.output))
 else:args.output.write_bytes(raw)
 for p,h in FROZEN.items():assert sha(ROOT/p)==h,p
 print(json.dumps(dict(output=str(args.output),report_sha256=sha(args.output),tool_sha256=sha(__file__),summary=report['summary'],unresolved=[(r['source_group'],r['old_entry'],r.get('prefix_candidates'))for r in rows.values()if r['status'].startswith('unresolved')])),flush=True)
if __name__=='__main__':main()
