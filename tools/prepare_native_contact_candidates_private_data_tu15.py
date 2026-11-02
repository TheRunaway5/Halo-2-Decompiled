#!/usr/bin/env python3
"""Prepare a private shared-algorithm/retained-observer specialization.

Writes only a new immutable analysis stage. Canonical sources are never edited.
The public numeric/default-observer path remains the same algorithm and keeps
its full ordinary supplied-DATA contract. Qualification is deliberately false.
"""
import argparse,difflib,hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def replace(s,a,b):
 assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
DOMAIN=r'''#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Optional typed retained-DATA observation contract. It never authorizes a
// read or a Source store. A private owner can suppress ONLY its bounded local
// preimages/events; ordinary reads still pass its definition/lifetime guard.
// Default public/numeric memory keeps every original observer event.
class NativeTu15RetainedStoreObservationDomain {
public:
    virtual ~NativeTu15RetainedStoreObservationDomain()=default;
    virtual bool observes_retained_store(std::uint32_t instruction,
        std::uint32_t address,std::size_t bytes) const=0;
    virtual void require_retained_extent(std::uint32_t,std::size_t) const {}
    // Optional same-domain bitwise DATA transfer. False means ordinary
    // storage: the public caller keeps its existing source copy algorithm.
    virtual bool copy_defined_data(std::uint32_t,std::uint32_t,std::uint32_t,
        std::size_t,const SourcePhysicsContactRecordObserver& = {}) {return false;}
    virtual bool data_is_defined(std::uint32_t,std::size_t) const {return true;}
};
inline bool native_tu15_observes_retained_store(const SourcePhysicsContactMemory&m,
    const SourcePhysicsContactRecordObserver&o,std::uint32_t pc,std::uint32_t p,std::size_t n){
    // Consult even with no observer, so an owned private range cannot conceal
    // an aliased retained heap publication in the read-only qualification pass.
    const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&m);
    const bool permitted=!domain||domain->observes_retained_store(pc,p,n);
    return bool(o)&&permitted;
}
} // namespace halo2
'''
PRIVATE=r'''#pragma once
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <algorithm>
#include <stdexcept>
namespace halo2 {
struct NativeTu15ContactPrivateRange {std::uint32_t first{},bytes{};};
// Owned call-local DATA. Bytes have no value until a real semantic producer
// writes them. They are never borrowed from original stack/GP/control images.
// All native providers must use THIS SAME memory view. Raw private preimages
// and private address-fault equivalence are explicitly outside this contract.
class NativeTu15ContactCandidatePrivateData final:public SourcePhysicsContactMemory,
    public NativeTu15RetainedStoreObservationDomain {
    struct Range {NativeTu15ContactPrivateRange extent;std::vector<std::byte>bytes;
        std::vector<bool>defined;};
    SourcePhysicsContactMemory&outer_;std::vector<Range>ranges_;
    const Range*range(std::uint32_t p,std::size_t n)const{
        const auto end=std::uint64_t(p)+n;
        if(end>0x100000000ull)throw std::runtime_error("Native private DATA wraps");
        for(const auto&r:ranges_){const std::uint64_t first=r.extent.first,last=first+r.extent.bytes;
            if(std::uint64_t(p)<last&&std::uint64_t(first)<end){
                if(p<first||end>last)throw std::runtime_error("Native private DATA crossing operand");
                return &r;
            }}return nullptr;
    }
    static bool retained_publication(std::uint32_t pc){
        // Authenticated2F parent heap/owner fields, never its local packet.
        switch(pc){case 0x2f37e3:case 0x2f37e7:case 0x2f38c1:
        case 0x2f38f9:case 0x2f38fe:case 0x2f3904:case 0x2f3a08:
        case 0x2f3a0d:case 0x2f3a13:case 0x2f3a32:case 0x2f3a9b:
        case 0x2f3aa0:case 0x2f3d4a:case 0x2f3d4d:case 0x2f3d55:
        case 0x2f3d66:case 0x2f3d6b:case 0x2f3d78:return true;default:break;}
        switch(pc){case 0x2f3b9d:case 0x2f3ba2:case 0x2f3bae:
        case 0x2f3c1f:case 0x2f3c36:case 0x2f3c24:case 0x2f3c3b:
        case 0x2f3ce7:case 0x2f3cf0:return true;default:break;}
        // Convex-agent allocation/constructor and actual allocator metadata
        // are retained heap stores; they may never become hidden local DATA.
        if((pc>=0x233250u&&pc<0x233500u)||(pc>=0x2e4eb0u&&pc<0x2e4f20u)||
            (pc>=0x2f8300u&&pc<0x2f8400u))return true;
        // Cache header/tag publications target the retained agent, unlike
        // its separately owned vector workspace.
        switch(pc){case 0x3090e0:case 0x309117:case 0x309120:case 0x309124:
        case 0x30912a:case 0x30912d:case 0x309137:case 0x309142:
        case 0x309162:case 0x309168:case 0x30916e:case 0x309272:
        case 0x309275:case 0x309279:case 0x309285:case 0x309288:return true;
        default:return false;}
    }
public:
    NativeTu15ContactCandidatePrivateData(SourcePhysicsContactMemory&outer,
        std::span<const NativeTu15ContactPrivateRange>owned,
        std::span<const NativeTu15ContactPrivateRange>retained):outer_(outer){
        std::uint64_t total{};
        for(const auto&e:owned){const auto end=std::uint64_t(e.first)+e.bytes;
            if(!e.bytes||end>0x100000000ull||(total+=e.bytes)>64u*1024u)
                throw std::runtime_error("Native private DATA requires bounded owned ranges");
            for(const auto&r:ranges_)if(std::uint64_t(e.first)<std::uint64_t(r.extent.first)+r.extent.bytes&&std::uint64_t(r.extent.first)<end)
                throw std::runtime_error("Native private DATA ranges overlap");
            for(const auto&r:retained)if(std::uint64_t(e.first)<std::uint64_t(r.first)+r.bytes&&std::uint64_t(r.first)<end)
                throw std::runtime_error("Native private DATA aliases retained input/output");
            ranges_.push_back({e,std::vector<std::byte>(e.bytes),std::vector<bool>(e.bytes,false)});
        }
    }
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        if(out.empty())return;
        if(const auto*r=range(p,out.size())){const auto at=p-r->extent.first;
            for(std::size_t i=0;i<out.size();++i)if(!r->defined[at+i])
                throw std::runtime_error("Native candidate requires unprovided private DATA at="+std::to_string(p)+" bytes="+std::to_string(out.size()));
            std::copy_n(r->bytes.begin()+at,out.size(),out.begin());return;}
        outer_.read(p,out);
    }
    void read_vector(std::uint32_t p,std::span<std::byte,16>out)const override{
        if(range(p,16)){read(p,out);return;}outer_.read_vector(p,out);
    }
    void write(std::uint32_t p,std::span<const std::byte>in)override{
        if(in.empty())return;
        if(const auto*found=range(p,in.size())){auto&r=*const_cast<Range*>(found);const auto at=p-r.extent.first;
            std::copy(in.begin(),in.end(),r.bytes.begin()+at);std::fill_n(r.defined.begin()+at,in.size(),true);return;}
        outer_.write(p,in);
    }
    bool observes_retained_store(std::uint32_t pc,std::uint32_t p,std::size_t n)const override{
        if(range(p,n)){if(retained_publication(pc))throw std::runtime_error("Native retained publication aliases private DATA");return false;}
        const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&outer_);
        return !domain||domain->observes_retained_store(pc,p,n);
    }
    void require_retained_extent(std::uint32_t p,std::size_t n)const override{
        if(n&&range(p,n))throw std::runtime_error("Native allocator output aliases private DATA");
        const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&outer_);
        if(domain)domain->require_retained_extent(p,n);
    }
    bool data_is_defined(std::uint32_t p,std::size_t n)const override{
        if(!n)return true;
        if(const auto*r=range(p,n)){const auto at=p-r->extent.first;
            return std::all_of(r->defined.begin()+at,r->defined.begin()+at+n,[](bool x){return x;});}
        const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&outer_);
        return !domain||domain->data_is_defined(p,n);
    }
    bool copy_defined_data(std::uint32_t pc,std::uint32_t destination,
        std::uint32_t source,std::size_t n,const SourcePhysicsContactRecordObserver&o={})override{
        if(!n||n>16u||(n&(n-1u)))throw std::runtime_error("Native defined DATA copy requires its actual scalar/vector operand granularity");
        const auto*input=range(source,n);const auto*output=range(destination,n);
        if(!input&&!output){auto*domain=dynamic_cast<NativeTu15RetainedStoreObservationDomain*>(&outer_);
            return domain&&domain->copy_defined_data(pc,destination,source,n,o);}
        // Validate retained-publication aliasing before touching any bytes.
        const bool observed=bool(o)&&observes_retained_store(pc,destination,n);
        if(!o)(void)observes_retained_store(pc,destination,n);
        std::array<std::byte,16>values;std::array<bool,16>defined{};
        if(input){const auto at=source-input->extent.first;for(std::size_t i=0;i<n;++i){
            defined[i]=input->defined[at+i];if(defined[i])values[i]=input->bytes[at+i];}}
        else{if(n==16u)outer_.read_vector(source,std::span<std::byte,16>(values));
            else outer_.read(source,std::span(values).first(n));std::fill_n(defined.begin(),n,true);}
        if(output){auto&r=*const_cast<Range*>(output);const auto at=destination-r.extent.first;
            for(std::size_t i=0;i<n;++i){r.defined[at+i]=defined[i];if(defined[i])r.bytes[at+i]=values[i];}return true;}
        if(!std::all_of(defined.begin(),defined.begin()+n,[](bool x){return x;}))
            throw std::runtime_error("Native undefined DATA cannot escape to retained storage");
        SourcePhysicsContactRecordStore event{pc,destination,{},{}};
        if(observed){event.before.resize(n);outer_.read(destination,event.before);event.written.assign(values.begin(),values.begin()+n);}
        outer_.write(destination,std::span(values).first(n));if(observed)o(event);return true;
    }
};
} // namespace halo2
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);staged=out/'staged';base=out/'base';changes={}
 def emit(name,text):
  target=staged/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text);original=ROOT/name
  if original.exists():b=base/name;b.parent.mkdir(parents=True,exist_ok=True);b.write_bytes(original.read_bytes())
  changes[name]=dict(before=sha(original)if original.exists()else None,after=sha(target))
 emit('include/halo2/native_tu15_retained_store_observation.hpp',DOMAIN)
 emit('include/halo2/native_tu15_contact_candidate_private_data.hpp',PRIVATE)
 # Each entry identifies one Source store helper. Its arithmetic, reads,
 # writes, fault phase and public/default observer behavior stay unchanged.
 edits={
 'contact_candidates':('memory_','observer_','site','p','b.size()'),
 'inverse_geometry':('memory_','observer_','pc','p','16'),
 'collision_key_query':('memory_','observe_','pc','p','4'),
 'collision_component':('memory_','observe_','pc','p','size'),
 'collision_surface_shape':('memory_','observe_','pc','p','size'),
 'shape_bounds':('m_','o_','pc','p','16'),
 'compound_shape_bounds':('m_','o_','pc','p','16'),
 'geometry':('memory_','observer_','site','p','16'),
 'convex_agent_factory':('memory_','observer_','pc','p','n'),
 'contact_cache':('memory_','observe_','pc','p','b.size()'),
 'collection_filter':('memory_','observe_','pc','p','1'),
 'array_growth':('memory_','observer_','site','address','bytes'),
 'array_allocator':('memory_','observer_','site','p','4'),
 'contact_candidate_buffers':('memory_','observer_','site','p','4'),
 'mopp_aabb_traversal':('memory_','observe_','pc','p','4'),
 }
 for name,(memory,observer,pc,address,n)in edits.items():
  path=f'src/physics/native_tu15_{name}.cpp';s=(ROOT/path).read_text();s='#include "halo2/native_tu15_retained_store_observation.hpp"\n'+s
  start=s.index('SourcePhysicsContactRecordStore ');end=s.index('\n}',start)if name=='array_growth'else len(s)
  # These owners have one writer helper; both preimage and poststore tests
  # consult the same already-computed predicate exactly once.
  assert s.count('if('+observer+')')==2,(name,s.count('if('+observer+')'))
  declaration='const bool observed=native_tu15_observes_retained_store('+','.join((memory,observer,pc,address,n))+');'
  s=s[:start]+declaration+s[start:];s=s.replace('if('+observer+')','if(observed)')
  if name=='contact_candidates':
   signature='void native_tu15_update_contact_candidates(SourcePhysicsContactMemory&memory,'
   s=replace(s,signature,'static void update_contact_candidates(SourcePhysicsContactMemory&memory,')
   s=replace(s,'const SourcePhysicsContactRecordObserver&observer,NativeTu15ContactCandidateStats*stats){','const SourcePhysicsContactRecordObserver&observer,NativeTu15ContactCandidateStats*stats,bool defined_private_data){')
   old='const auto comparator=m.word(p+0x24u),data=m.word(keys);s.sort_keys(data,0,key_count-1u,comparator,fp);'
   new='if(defined_private_data){require(s.sort_defined_keys,"Native candidate requires its semantic key sorter");const auto data=m.word(keys);s.sort_defined_keys(data,0,key_count-1u,fp);}else{'+old+'}'
   s=replace(s,old,new)
   wrapper='''
void native_tu15_update_contact_candidates(SourcePhysicsContactMemory&m,
 NativeTu15ContactProducer p,NativeTu15ContactPayload a,NativeTu15ContactPayload b,
 NativeTu15ContactContext c,NativeTu15ContactVirtualOwner f,NativeTu15ContactCandidateScratch s,
 NativeTu15PhysicsFrameFp&fp,unsigned d,const NativeTu15ContactCandidateServices&services,
 const SourcePhysicsContactRecordObserver&o,NativeTu15ContactCandidateStats*stats){
 update_contact_candidates(m,p,a,b,c,f,s,fp,d,services,o,stats,false);
}
void native_tu15_update_contact_candidates_defined_data(SourcePhysicsContactMemory&m,
 NativeTu15ContactProducer p,NativeTu15ContactPayload a,NativeTu15ContactPayload b,
 NativeTu15ContactContext c,NativeTu15ContactVirtualOwner f,NativeTu15ContactCandidateScratch s,
 NativeTu15PhysicsFrameFp&fp,unsigned d,const NativeTu15ContactCandidateServices&services,
 const SourcePhysicsContactRecordObserver&o,NativeTu15ContactCandidateStats*stats){
 update_contact_candidates(m,p,a,b,c,f,s,fp,d,services,o,stats,true);
}
'''
   s=replace(s,'} // namespace halo2',wrapper+'} // namespace halo2')
  emit(path,s)
 # Contact shape has byte and scalar-pop writers, each with its own output.
 name='src/physics/native_tu15_contact_shape.cpp';s=(ROOT/name).read_text();s='#include "halo2/native_tu15_retained_store_observation.hpp"\n'+s
 marker='SourcePhysicsContactRecordStore event{pc,p,{},{}};';positions=[m.start()for m in re.finditer(re.escape(marker),s)];assert len(positions)==2
 for at,n in reversed(list(zip(positions,('b.size()','4')))):
  end=s.index('observe_(event);',at)+len('observe_(event);');segment=s[at:end];segment=segment.replace('if(observe_)','if(observed)');s=s[:at]+'const bool observed=native_tu15_observes_retained_store(memory_,observe_,pc,p,'+n+');'+segment+s[end:]
 emit(name,s)
 # The two generic array reserve publications keep their public contract.
 name='src/physics/native_tu15_array_reserve.cpp';s=(ROOT/name).read_text();s='#include "halo2/native_tu15_retained_store_observation.hpp"\n'+s
 at=s.index('SourcePhysicsContactRecordStore ');s=s[:at]+'const bool observed=native_tu15_observes_retained_store(m,observe,instruction,address,4);'+s[at:];assert s.count('if(observe)')==2;s=s.replace('if(observe)','if(observed)');emit(name,s)
 # Observation domain must survive the existing nested cache view.
 name='include/halo2/native_tu15_mopp_aabb_private_cache.hpp';s=(ROOT/name).read_text();s='#include "halo2/native_tu15_retained_store_observation.hpp"\n'+s
 s=replace(s,'final:public SourcePhysicsContactMemory {','final:public SourcePhysicsContactMemory,public NativeTu15RetainedStoreObservationDomain {')
 s=replace(s,'public:\n    NativeTu15MoppAabbPrivateCache','public:\n    bool observes_retained_store(std::uint32_t pc,std::uint32_t p,std::size_t n)const override{\n        if(touches(p,n))throw std::runtime_error("Private MOPP cache raw observation requires its owner");\n        const auto*d=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&memory_);return !d||d->observes_retained_store(pc,p,n);\n    }\n    void require_retained_extent(std::uint32_t p,std::size_t n)const override{\n        if(n&&touches(p,n))throw std::runtime_error("Retained output aliases private MOPP cache");\n        const auto*d=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&memory_);if(d)d->require_retained_extent(p,n);\n    }\n    bool copy_defined_data(std::uint32_t pc,std::uint32_t dst,std::uint32_t src,std::size_t n,const SourcePhysicsContactRecordObserver&o={})override{\n        if(touches(dst,n)||touches(src,n))throw std::runtime_error("Private MOPP cache transfer requires its typed owner");\n        auto*d=dynamic_cast<NativeTu15RetainedStoreObservationDomain*>(&memory_);return d&&d->copy_defined_data(pc,dst,src,n,o);\n    }\n    bool data_is_defined(std::uint32_t p,std::size_t n)const override{\n        if(touches(p,n)){const auto at=offset(p,n);return std::all_of(defined_.begin()+at,defined_.begin()+at+n,[](bool x){return x;});}\n        const auto*d=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&memory_);return !d||d->data_is_defined(p,n);\n    }\n    NativeTu15MoppAabbPrivateCache');emit(name,s)
 # Share the same key sort, omitting only the proven dead fourth value.
 name='src/physics/native_tu15_contact_candidate_buffers.cpp';s=(staged/name).read_text();s=replace(s,'std::uint32_t first,std::uint32_t last,\n std::uint32_t forwarded){','std::uint32_t first,std::uint32_t last){')
 s=s.replace('sort(m,keys,first,high,forwarded)','sort(m,keys,first,high)')
 s=replace(s,'sort(m,keys,first,last,forwarded);','(void)forwarded;sort(m,keys,first,last);')
 s=replace(s,'NativeTu15ContactEntryBuffer native_tu15_initialize_contact_entries','void native_tu15_sort_defined_contact_keys(SourcePhysicsContactMemory&memory,std::uint32_t keys,\n std::uint32_t first,std::uint32_t last,const SourcePhysicsContactRecordObserver&observer){Memory m(memory,observer);sort(m,keys,first,last);}\nNativeTu15ContactEntryBuffer native_tu15_initialize_contact_entries');emit(name,s)
 name='include/halo2/native_tu15_contact_candidate_buffers.hpp';s=(ROOT/name).read_text();s=replace(s,'// Actual2F3430/2F34D0 and317170.','// Same semantic key sorter, without the Source-dead forwarded control word.\nvoid native_tu15_sort_defined_contact_keys(SourcePhysicsContactMemory&,std::uint32_t keys,\n    std::uint32_t first,std::uint32_t last,const SourcePhysicsContactRecordObserver& = {});\n// Actual2F3430/2F34D0 and317170.');emit(name,s)
 name='include/halo2/native_tu15_contact_candidates.hpp';s=(ROOT/name).read_text();s=replace(s,'struct NativeTu15ContactCandidateServices {','struct NativeTu15ContactCandidateServices {\n    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&)> sort_defined_keys;')
 at=s.index('void native_tu15_update_contact_candidates(');declaration=s[at:s.index(';',at)+1].replace('native_tu15_update_contact_candidates','native_tu15_update_contact_candidates_defined_data');s=replace(s,'} // namespace halo2','// Host private-DATA specialization: only dead comparator forwarding is omitted.\n'+declaration+'\n} // namespace halo2');emit(name,s)
 name='include/halo2/native_tu15_contact_candidates_binding.hpp';s=(ROOT/name).read_text();s='#include "halo2/native_tu15_retained_store_observation.hpp"\n'+s
 s=replace(s,'        services.sort_keys=[&]','        services.sort_defined_keys=[&](auto keys,auto low,auto high,auto&){\n            if(activity)++activity->sorts;native_tu15_sort_defined_contact_keys(memory,keys,low,high,observe);\n        };\n        services.sort_keys=[&]')
 needle='            (void)native_tu15_initialize_contact_entries(memory,{header},count,current,providers.buffers,observe);'
 s=replace(s,needle,needle+'''\n            if(const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&memory)){
                const auto bytes=std::uint64_t(count)*12u;if(bytes>0xffffffffu)throw std::runtime_error("Native candidate entry extent wraps");
                std::array<std::byte,4>b{};memory.read(header,b);std::uint32_t data{};for(unsigned k=0;k<4;++k)data|=std::to_integer<unsigned>(b[k])<<(k*8u);
                domain->require_retained_extent(data,std::size_t(bytes));
            }''')
 s=replace(s,'        native_tu15_update_contact_candidates(memory,producer','        native_tu15_update_contact_candidates_defined_data(memory,producer');emit(name,s)
 patch=[]
 for name in changes:
  before=(ROOT/name).read_text()if(ROOT/name).exists()else'';after=(staged/name).read_text();patch.extend(difflib.unified_diff(before.splitlines(True),after.splitlines(True),'a/'+name,'b/'+name))
 (out/'native-private-data.diff').write_text(''.join(patch));(out/'source-pins.json').write_text(json.dumps(dict(qualified=False,root_reviewed=False,files=changes,scope=__doc__),indent=2)+'\n')
 print(out)
if __name__=='__main__':main()
