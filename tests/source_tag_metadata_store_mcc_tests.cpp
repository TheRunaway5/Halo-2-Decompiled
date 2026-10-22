#include "halo2/source_tag_metadata_store.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>

namespace {
using namespace halo2;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F&& f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Unavailable source metadata was admitted");}
bool selected(std::string_view group){return group=="shad"||group=="stem"||group=="spas"||group=="vrtx";}
bool contains(const Tag& tag,std::uint32_t address,std::size_t bytes){return tag.address&&address>=tag.address&&std::uint64_t(address)-tag.address<=tag.size&&bytes<=tag.size-(std::uint64_t(address)-tag.address);}
bool equal(std::span<const std::byte> a,std::span<const std::byte> b){return std::ranges::equal(a,b);}
}
int main(int argc,char** argv)try{
  using namespace halo2;
  const char* donor=argc>1&&*argv[1]?argv[1]:std::getenv("HALO2_MCC_SOURCE_TAG_DONOR");
  if(!donor||!std::filesystem::is_regular_file(donor))return 77;
  auto cache=std::make_shared<Cache>(donor,std::filesystem::path(donor).parent_path());
  check(cache->is_mcc()&&!cache->is_vista()&&cache->name()=="00a_introduction","Regression requires the actual MCC introduction cache");
  std::weak_ptr<const Cache> lifetime=cache;
  SourceTagMetadataProviders provider;
  std::function<std::uint32_t(std::uint16_t)> index_address;
  std::uint32_t forwarded=0,canonical=0,canonical_datum=0;
  std::vector<std::byte> forwarded_bytes;
  std::size_t canonical_count=0,retained_reads=0,retained_bytes=0,excluded_groups=0,unresolved=0;
  {
    SourceTagMetadataStore store(cache,{"shad","stem","spas","vrtx"});provider=store.providers();index_address=store.index_address_provider();
    for(const auto& tag:cache->select_tags())if(tag.address&&selected(tag.group)){
      if(!tag.size&&!cache->query_tag({.group=tag.type(),.address=tag.address,.minimum_size=1}))continue;
      const auto payload=cache->tag_payload(tag.handle());const auto& owned=provider.definition(tag.datum);
      check(owned.datum()==tag.datum&&owned.group()==tag.group&&owned.source_address()==tag.address&&owned.size()==payload.size(),"Canonical directory definition changed");
      check(index_address(tag.handle().index())==tag.address&&provider.address(tag.datum)==tag.address,"Canonical index/address callback changed");
      check(equal(owned.bytes_at(tag.address,payload.size()),payload)&&equal(provider.read(tag.address,payload.size()),payload),"Canonical payload bytes changed");
      ++canonical_count;
    }
    check(store.tag_count()==canonical_count,"Retained allocations changed canonical tag count");
    for(const auto& tag:cache->retained_tag_allocations())if(tag.address&&tag.size){
      const auto* current=cache->find_tag(tag.handle());
      if(!selected(tag.group)){
        rejects([&]{(void)provider.read(tag.address,tag.size);});++excluded_groups;continue;
      }
      check(current&&current->group==tag.group&&current->name==tag.name,"Retained allocation lost its full typed identity");
      if(!current->address){rejects([&]{(void)provider.read(tag.address,tag.size);});++unresolved;continue;}
      const auto& definition=provider.definition(tag.datum);
      check(definition.source_address()==current->address&&definition.group()==current->group,"Retained payload replaced canonical definition");
      check(index_address(tag.handle().index())==current->address,"Retained payload replaced canonical index binding");
      check(equal(provider.read(tag.address,tag.size),cache->read_tag_bytes(tag.address,tag.size)),"Actual retained allocation bytes changed");
      ++retained_reads;retained_bytes+=tag.size;
    }
    check(retained_reads&&excluded_groups,"Actual cache lacks retained/group-negative coverage");
    // This is the literal forwarded LOD read that failed ordinary MCC startup.
    // Derive each pointer from the actual cache; no addresses are injected.
    const auto* gauge=cache->query_tag({.group=tag_groups::shader,.name="objects\\weapons\\rifle\\plasma_rifle\\shaders\\plasma_rifle_gauge",.require_address=true});
    check(gauge,"Opening cache lacks the actual plasma-rifle gauge shader");
    const Bytes shad(provider.read(gauge->address,0x28));
    canonical_datum=Bytes(provider.read(shad.u32(0x24),4)).u32(0);canonical=provider.address(canonical_datum);
    const Bytes stem(provider.read(canonical,0x60));const Bytes post(provider.read(stem.u32(0x5c),8));
    check(post.u32(0)>0,"Gauge stem has no actual LOD rows");forwarded=post.u32(4);
    const bool native_gauge=cache->is_preserved_native_shader(*gauge);
    if(native_gauge) {
      // The gauge now owns its authentic original template and LOD bank.
      // Preserve the general retained-packet coverage below using an actual
      // moved stem, rather than requiring the repaired gauge to stay uncooked.
      const auto *native=cache->find_tag({canonical_datum},tag_groups::shader_template);
      check(native && native->name=="shaders\\shader_templates\\transparent\\meter_active_camo" &&
          contains(*native,forwarded,10),"Native gauge lost its canonical original LOD bank");
      (void)provider.read(forwarded,10);
      const auto retained=std::find_if(cache->retained_tag_allocations().begin(),cache->retained_tag_allocations().end(),[&](const auto& tag) {
        const auto *current=cache->find_tag(tag.handle(),tag.type());
        return tag.group=="stem" && tag.address && tag.size>=10 && current && current->address &&
            std::none_of(cache->tags().begin(),cache->tags().end(),[&](const auto& row){return contains(row,tag.address,10);});
      });
      check(retained!=cache->retained_tag_allocations().end(),"Actual cache no longer exercises an uncovered moved stem");
      forwarded=retained->address;
    }
    check(std::none_of(cache->tags().begin(),cache->tags().end(),[&](const auto& tag){return contains(tag,forwarded,10);}),"Regression no longer crosses a retained allocation");
    const Tag* owner=nullptr;
    for(const auto& tag:cache->retained_tag_allocations())if(contains(tag,forwarded,10)){check(!owner,"Forwarded LOD has ambiguous retained ownership");owner=&tag;}
    check(owner&&owner->group=="stem"&&(native_gauge||owner->name=="shaders\\shader_templates\\transparent\\sky_two_alpha_clouds"),"Literal pointer lost its actual retained template owner");
    const auto bytes=cache->read_tag_bytes(forwarded,10);forwarded_bytes.assign(bytes.begin(),bytes.end());
    check(equal(provider.read(forwarded,10),forwarded_bytes),"Actual gauge forwarded LOD read failed");
    {SourceTagMetadataStore restricted(cache,{"shad"});rejects([&]{(void)restricted.providers().read(forwarded,10);});}
    rejects([&]{(void)provider.definition(0xffffffffu);});rejects([&]{(void)provider.address(0xffffffffu);});
    rejects([&]{(void)index_address(0xffffu);});rejects([&]{(void)provider.read(0,1);});
    rejects([&]{(void)provider.read(0xffffffffu,1);});
    rejects([&]{(void)provider.read(forwarded,std::numeric_limits<std::size_t>::max());});
    const auto& original=provider.definition(gauge->datum);
    rejects([&]{(void)original.bytes_at(original.source_address()-1,1);});
    rejects([&]{(void)original.bytes_at(original.source_address()+std::uint32_t(original.size()),1);});
    rejects([&]{SourceTagMetadataStore invalid(cache,{});});rejects([&]{SourceTagMetadataStore invalid(cache,{"sbsp"});});
    rejects([&]{SourceTagMetadataStore invalid(nullptr,{"stem"});});
    cache.reset();
  }
  check(!lifetime.expired(),"Callbacks lost the real cache backing after facades were released");
  check(equal(provider.read(forwarded,10),forwarded_bytes)&&provider.definition(canonical_datum).source_address()==canonical,"Retained callback bytes/canonical identity changed after facade release");
  auto last_read=provider.read;provider={};index_address={};
  check(!lifetime.expired()&&equal(last_read(forwarded,10),forwarded_bytes),"Last read callback lost its owned backing");
  last_read={};check(lifetime.expired(),"Released metadata callbacks leaked the cache");
  std::cout<<"PASS actual MCC canonicalTags="<<canonical_count<<" retainedAllocations="<<retained_reads<<" retainedBytes="<<retained_bytes<<" excludedGroups="<<excluded_groups<<" unresolvedRejected="<<unresolved<<" forwarding/group/missing/overflow/outside/lifetime=passed World/GPU=not-run\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
