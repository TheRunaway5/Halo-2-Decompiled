#include "halo2/source_object_spatial_pool.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
Words parse(std::string_view line){std::istringstream s{std::string(line)};Words w;std::uint32_t v;
    while(s>>std::hex>>v)w.push_back(v);if(!s.eof())throw FormatError("invalid spatial fixture");return w;}
std::uint32_t hash(std::span<const std::uint32_t>w){std::uint32_t h=2166136261;
    for(auto v:w)for(unsigned b=0;b<4;++b){h^=std::uint8_t(v>>(8*b));h*=16777619;}return h;}
std::uint32_t records_hash(const SourceObjectSpatialDatumPool&p){Words w;
    for(const auto&r:p.records)w.insert(w.end(),r.begin(),r.begin()+p.record_words);return hash(w);}
Words summary(const SourceObjectSpatialPool&p){return {p.entries.cursor,p.entries.highwater,p.entries.active_count,p.entries.next_generation,
    p.references.cursor,p.references.highwater,p.references.active_count,p.references.next_generation};}
void append(Words&to,std::span<const std::uint32_t>v){to.insert(to.end(),v.begin(),v.end());}
std::array<std::uint32_t,5> primitive_words(const ObjectLightSpatialPrimitive&p){return std::bit_cast<std::array<std::uint32_t,5>>(p);}
struct Probe {
    std::unique_ptr<SourceObjectSpatialPool> pool=std::make_unique<SourceObjectSpatialPool>(512);
    std::array<std::uint32_t,512> owner_heads;
    Probe(){owner_heads.fill(0xffffffff);}
    Words run(char kind,const Words&w){std::size_t at=0;
        auto take=[&](){if(at==w.size())throw FormatError("short spatial probe");return w[at++];};
        auto done=[&](){if(at!=w.size())throw FormatError("extra spatial probe words");};
        if(kind=='R') {
            const auto fill=take(),debug=take(),collide=take();done();
            pool=std::make_unique<SourceObjectSpatialPool>(512,collide!=0);
            for(auto&p:{&pool->entries,&pool->references})for(auto&r:p->records)r.fill(fill);
            pool->activate(debug!=0);owner_heads.fill(0xffffffff);
            Words out=summary(*pool);out.push_back(records_hash(pool->entries));out.push_back(records_hash(pool->references));return out;
        }
        if(kind=='T') {
            const bool collide=take()!=0;pool=std::make_unique<SourceObjectSpatialPool>(512,collide);
            for(auto*p:{&pool->entries,&pool->references}) {
                p->cursor=take();p->highwater=take();p->active_count=take();p->next_generation=std::uint16_t(take());
            }
            for(auto*p:{&pool->entries,&pool->references})for(auto&v:p->occupied)v=take();
            for(auto&v:pool->heads)v=take();for(auto&v:owner_heads)v=take();
            for(auto*p:{&pool->entries,&pool->references})for(auto&r:p->records)for(unsigned i=0;i<p->record_words;++i)r[i]=take();
            done();return summary(*pool);
        }
        if(kind=='A' || kind=='F' || kind=='G') {
            const auto which=take();if(which>1)throw FormatError("invalid spatial pool selector");
            auto&p=which?pool->references:pool->entries;
            if(kind=='G'){p.next_generation=std::uint16_t(take());done();return {p.next_generation};}
            const auto token=kind=='A'?p.allocate():take();
            if(kind=='F')p.release(token);done();
            Words out={token,p.cursor,p.highwater,p.active_count,p.next_generation};
            if(token!=0xffffffff)append(out,std::span(p.records[token&0xffff]).first(p.record_words));return out;
        }
        if(kind=='I' || kind=='U' || kind=='D') {
            const auto index=take(),owner=take();if(index>=owner_heads.size())throw FormatError("invalid spatial fixture owner");
            auto&head=owner_heads[index];std::vector<std::int16_t> members;
            if(kind=='D'){done();pool->erase(owner,head);}
            else {
                std::array<std::uint32_t,5> raw;for(auto&v:raw)v=take();
                const auto p=std::bit_cast<ObjectLightSpatialPrimitive>(raw);
                if(kind=='I'){const auto count=take();for(unsigned i=0;i<count;++i)members.push_back(std::int16_t(take()));done();pool->insert(owner,head,p,members);}
                else {done();pool->update(owner,head,p);}
            }
            Words out=summary(*pool);out.push_back(head);
            for(auto c:members)out.push_back(pool->heads.at(c));return out;
        }
        if(kind=='Q') {
            const auto c=take(),index=take();done();if(index>=owner_heads.size())throw FormatError("invalid query fixture owner");
            Words out=summary(*pool);append(out,std::array<std::uint32_t,7>{records_hash(pool->entries),records_hash(pool->references),
                hash(pool->entries.occupied),hash(pool->references.occupied),hash(pool->heads),hash(owner_heads),owner_heads[index]});
            const auto list=pool->cluster(std::int16_t(c));out.push_back(std::uint32_t(list.size()));
            for(const auto&r:list){out.push_back(r.object);append(out,primitive_words(r.primitive));}return out;
        }
        if(kind=='J') {
            SourceObjectRegistrationOwner owner;owner.full_handle=0xe2000001;owner.pool_rank=0;
            owner.primitive.object_flags=take();owner.primitive.header_flags=std::uint8_t(take());owner.primitive.type=std::int8_t(take());
            owner.primitive.unit_flags_10a=std::uint8_t(take());owner.primitive.unit_flags_348=std::uint8_t(take());owner.primitive.prop_flags_12c=take();
            owner.location.leaf=take();const auto before=take();owner.location.cluster=std::int16_t(before);owner.location.bsp=std::int16_t(before>>16);
            owner.header_cluster=std::int16_t(take());SourceObjectLocation supplied;supplied.leaf=take();const auto after=take();supplied.cluster=std::int16_t(after);supplied.bsp=std::int16_t(after>>16);
            owner.primitive.primitive_sphere={{std::bit_cast<float>(take()),std::bit_cast<float>(take()),std::bit_cast<float>(take())},std::bit_cast<float>(take())};done();
            auto journal=std::make_unique<SourceObjectRegistrationJournal>(3);journal->birth(0,owner);
            ObjectLightSpatialBsp bsp;bsp.clusters.resize(3);const auto effects=journal->register_owner(1,owner.full_handle,supplied,bsp);
            const auto&out=journal->owner(owner.full_handle);const auto&bank=(owner.primitive.object_flags&0x200)?journal->collidable():journal->noncollidable();
            Words result={out.primitive.object_flags,out.primitive.header_flags,out.location.leaf,
                std::uint32_t(std::uint16_t(out.location.cluster))|(std::uint32_t(std::uint16_t(out.location.bsp))<<16),
                std::uint32_t(std::int32_t(out.header_cluster)),out.reference_head,effects.old_none_to_valid};append(result,summary(bank));
            for(std::int16_t c=0;c<3;++c){const auto list=bank.cluster(c);result.push_back(std::uint32_t(list.size()));for(const auto&r:list){result.push_back(r.object);append(result,primitive_words(r.primitive));}}
            return result;
        }
        throw FormatError("unknown source spatial fixture kind");
    }
};
void print(const Words&w){for(std::size_t i=0;i<w.size();++i)std::cout<<(i?" ":"")<<std::hex<<w[i];std::cout<<'\n';}
void journal_regression(){
    auto j=std::make_unique<SourceObjectRegistrationJournal>(1);ObjectLightSpatialBsp bsp;bsp.clusters.resize(1);
    SourceObjectRegistrationOwner a;a.full_handle=0x10001;a.pool_rank=3;a.primitive.type=6;a.primitive.object_flags=0x200;
    a.primitive.primitive_sphere={{1,2,3},1};a.rendering_sphere=a.primitive.primitive_sphere;
    auto b=a;b.full_handle=0x10002;b.pool_rank=1;
    j->birth(10,a);j->birth(11,b);j->register_owner(12,a.full_handle,{0,0,0},bsp);j->register_owner(13,b.full_handle,{0,0,0},bsp);
    if(j->collidable().cluster(0)[0].object!=b.full_handle || j->actual_pool_order()[0].full_handle!=b.full_handle)
        throw FormatError("actual event order was replaced by current pool sorting");
    const auto saved=*j;const auto previous=j->owner(a.full_handle).location;
    j->unregister(14,a.full_handle);j->register_owner(15,a.full_handle,{0xffffffff,-1,-1},bsp);
    if(j->collidable().cluster(0)[0].object!=a.full_handle || j->owner(a.full_handle).location.leaf!=previous.leaf ||
        !(j->owner(a.full_handle).primitive.object_flags&0x40000))throw FormatError("suppliedNONE registration lost actual retained location/order");
    auto flags=j->owner(a.full_handle).primitive;flags.object_flags|=1;j->primitive_update(16,a.full_handle,flags);
    if(!(j->collidable().cluster(0)[0].primitive.flags&1))throw FormatError("primitive flags were not updated in place");
    *j=saved;
    if(j->events().size()!=4 || j->collidable().cluster(0)[0].object!=b.full_handle)
        throw FormatError("checkpoint did not restore exact source journal/spatial order");
    j->unregister(14,a.full_handle);j->free(15,a.full_handle);a.full_handle=0x20001;j->birth(16,a);
    j->register_owner(17,a.full_handle,{0,0,0},bsp);
    if(j->collidable().cluster(0)[0].object!=a.full_handle)throw FormatError("recycled actual native handle used stale membership");
    bool rejected=false;try{j->free(18,a.full_handle);}catch(const FormatError&){rejected=true;}
    if(!rejected)throw FormatError("unproved root destruction was silently synthesized");
    auto movement=std::make_unique<SourceObjectRegistrationJournal>(2);
    bsp.clusters.resize(2);BspAtmosphere collision;collision.planes.push_back({1,0,0,0});
    collision.nodes.push_back({0,0x800000,0x800001});collision.leaves={0,1};
    a.full_handle=0x30001;a.primitive.primitive_sphere={{-1,0,0},1};a.root_position={-1,0,0};
    movement->birth(0,a);movement->register_default(1,a.full_handle,collision,0,bsp);
    if(movement->owner(a.full_handle).header_cluster!=0)throw FormatError("NULL registration did not call source point location");
    const ObjectLightSpatialSphere near{{-.95f,0,0},1},far{{.5f,0,0},1};
    const auto unchanged=movement->bound_refresh(2,a.full_handle,near,near,near.center,collision,0,bsp);
    if(unchanged.relinked || movement->owner(a.full_handle).primitive.primitive_sphere.center.x!=-1 ||
        movement->owner(a.full_handle).rendering_sphere.center.x!=near.center.x)
        throw FormatError("retained primitive threshold lost independent rendering-sphere update");
    const auto changed=movement->bound_refresh(3,a.full_handle,far,far,far.center,collision,0,bsp);
    if(!changed.relinked || !changed.activation_services_required || movement->owner(a.full_handle).header_cluster!=1 ||
        !movement->collidable().cluster(0).empty() || movement->collidable().cluster(1)[0].object!=a.full_handle)
        throw FormatError("bound-change NULL relink did not update actual source membership");
    auto pressure=std::make_unique<SourceObjectRegistrationJournal>(3);bsp.clusters.resize(3);
    std::uint64_t ordinal=0;
    for(unsigned i=0;i<684;++i){
        SourceObjectRegistrationOwner full;full.full_handle=0xe1000000u|i;full.pool_rank=std::uint16_t(i);
        full.primitive.object_flags=0x200200;
        pressure->birth(ordinal++,full);pressure->register_owner(ordinal++,full.full_handle,{0,0,0},bsp);
    }
    const auto&last=pressure->owner(0xe1000000u|683);
    if(last.reference_head!=0xffffffffu || pressure->collidable().entries.active_count!=2048 ||
        pressure->collidable().references.active_count!=2048 || !(last.primitive.object_flags&0x100) ||
        !(last.primitive.header_flags&0x40))
        throw FormatError("pool pressure incorrectly prevented source registration bits");
}
}
int main(int argc,char**argv){try{
    Probe probe;if(argc==2 && std::string_view(argv[1])=="--probe") {
        std::string line;while(std::getline(std::cin,line))print(probe.run(line.at(0),parse(std::string_view(line).substr(2))));return 0;
    }
    if(argc!=2)throw FormatError("source spatial test requires untouched fixtures");std::ifstream f(argv[1]);
    if(!f)throw FormatError("missing source spatial fixtures");std::string line;unsigned cases=0,words=0;std::array<unsigned,256> count{};
    while(std::getline(f,line)){
        if(line.empty() || line[0]=='#')continue;const auto split=line.find('|');
        if(split==std::string::npos)throw FormatError("missing source spatial fixture separator");
        const auto actual=probe.run(line[0],parse(std::string_view(line).substr(2,split-2))),expected=parse(std::string_view(line).substr(split+1));
        if(actual!=expected){std::size_t i=0;while(i<actual.size() && i<expected.size() && actual[i]==expected[i])++i;
            throw FormatError("source spatial fixture differs at case"+std::to_string(cases)+" word"+std::to_string(i));}
        ++count[std::uint8_t(line[0])];++cases;words+=expected.size();
    }
    if(!f.eof() || count['R']<5 || count['T']!=2 || count['A']<6000 || count['I']<100 || count['F']<100 || count['Q']<100 || count['J']<256)
        throw FormatError("incomplete original spatial pool fixture coverage");journal_regression();
    std::cout<<cases<<" untouched source spatial fixtures passed; "<<words<<" exact words; native actual-event/restore controls passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
