#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_execution_lens.hpp"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <thread>

using namespace halo2;
namespace {
constexpr std::uint32_t base=0x52dfd8u,bytes=0x40c0u;
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
template<class F>void rejects(F run){bool caught{};try{run();}catch(const FormatError &){caught=true;}
    check(caught,"Invalid native lens handoff was accepted");}
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t milliseconds()const noexcept override{return 1000;}
    std::uint64_t cycles()const noexcept override{return 7000000;}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
std::string hash(std::span<const std::byte> input){std::array<unsigned char,SHA256_DIGEST_LENGTH> digest{};
    check(SHA256(reinterpret_cast<const unsigned char*>(input.data()),input.size(),digest.data()),"Cannot hash native query bank");
    constexpr char hex[]="0123456789abcdef";std::string out;for(auto n:digest){out+=hex[n>>4];out+=hex[n&15];}return out;}
void put(std::span<std::byte> pool,unsigned offset,std::uint32_t value){for(unsigned n=0;n<4;++n)pool[offset+n]=std::byte(value>>(n*8));}
void replace_bsp_with_pending_query(PhysicsLiveWorldTu15 &world) {
    constexpr std::uint32_t driver=0xf7100000u,metadata=0xf7140000u,stamp=0x4922c0u;
    auto &source=world.memory();
    const auto previous_stamp=source.view(stamp,4);
    std::array<std::byte,4> saved_stamp;
    std::ranges::copy(previous_stamp,saved_stamp.begin());
    std::array<std::byte,4> frame{};
    put(frame,0,2);source.write(stamp,frame);
    std::vector<std::byte> pending(bytes);
    pending[0]=pending[1]=std::byte{1}; // Query processing enabled.
    pending[0x52dfe0u-base]=std::byte{2}; // Slot zero has an issued query for view zero.
    put(pending,0x531fbcu-base,1);
    put(pending,0x531fc0u-base,1); // (presentation stamp + 1) % 3 is ring zero.
    source.write(base,pending);

    // A fresh device has fence zero and no result in the prior world's slot.
    SourceRuntimeMemory device;
    put(frame,0,driver);device.mount(0x411568u,frame,SourceRuntimeMemory::Access::ReadOnly,"new lens driver pointer");
    std::vector<std::byte> state(4096),queries(4096);
    put(state,0x7d4,metadata);put(queries,12,0xffffffffu);
    device.mount(driver,state,SourceRuntimeMemory::Access::ReadOnly,"new lens driver");
    device.mount(metadata,queries,SourceRuntimeMemory::Access::ReadOnly,"new pending query metadata");
    const auto poll=[&] {
        auto abi=world.prepare_source_call();source_execute_lens_query(world,0x20750u,abi,&device);
    };
    bool reached_kick{};
    try {poll();}catch(const FormatError &e) {
        reached_kick=std::string_view(e.what())=="Original lens query reached unowned graphics service 4212576";
    }
    check(reached_kick,"Keeping old query slots did not reproduce the fresh-device fence collision");
    check(source_retire_lens_queries(world)==1u,"BSP transition did not retire the previous device's query");
    poll();
    source_require_retired_lens_queries(source);
    source.write(stamp,saved_stamp);
}
std::string outside(const SourceRuntimeMemory &memory,std::uint32_t stack,std::size_t stack_bytes){
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> context(EVP_MD_CTX_new(),EVP_MD_CTX_free);
    check(context&&EVP_DigestInit_ex(context.get(),EVP_sha256(),nullptr)==1,"Cannot start native conservation digest");
    memory.visit_committed([&](std::uint32_t address,std::span<const std::byte> data){
        std::uint64_t from=address,to=std::uint64_t(address)+data.size();
        const std::array<std::pair<std::uint64_t,std::uint64_t>,2> excluded{{{base,base+bytes},{stack,std::uint64_t(stack)+stack_bytes}}};
        for(const auto &[low,high]:excluded){
            if(from>=to)break;
            if(high<=from||low>=to)continue;
            if(from<low)check(EVP_DigestUpdate(context.get(),data.data()+(from-address),low-from)==1,"Cannot hash native prefix");
            from=std::min(to,high);
        }
        if(from<to)check(EVP_DigestUpdate(context.get(),data.data()+(from-address),to-from)==1,"Cannot hash native suffix");
    });
    std::array<unsigned char,SHA256_DIGEST_LENGTH> result{};unsigned size{};
    check(EVP_DigestFinal_ex(context.get(),result.data(),&size)==1&&size==result.size(),"Cannot finish native conservation digest");
    return {reinterpret_cast<const char*>(result.data()),result.size()};
}
}
int main(int argc,char **argv)try{
    check(argc==4,"Supply XBE MAP ORIGINAL_RETIREMENT_FIXTURE");
    if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2])||!std::filesystem::exists(argv[3]))return 77;
    nlohmann::json fixture;{std::ifstream file(argv[3]);file>>fixture;}
    const auto original=OriginalData::load_xbe(argv[1]);check(original->image==OriginalData::Image::TitleUpdate15,"Lens handoff requires TU1.5");
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Lens handoff requires owned scratch");
    MultiplayerOptions options;options.map=argv[2];options.shared_maps=std::filesystem::path(argv[2]).parent_path();
    options.gameplay_xbe=argv[1];options.local_players=0;options.dedicated=true;
    options.variant={"Slayer",0,{{0x59,5},{10,0},{2,0}}};
    MultiplayerPreparedWorld prepared(options,std::nullopt,std::make_shared<Clock>());
    check(prepared.source_objects&&prepared.source_objects->owns_physics(),"Lens handoff lacks its native match owner");
    unsigned cases{};
    prepared.source_objects->render_model_view([&](auto &world){
    auto &memory=world.memory();const auto abi=world.prepare_source_call();
    auto metadata=memory.checkpoint();const auto found=std::ranges::find_if(metadata.regions,[&](const auto &region){
        return abi.gp[4]>=region.address&&std::uint64_t(abi.gp[4])<std::uint64_t(region.address)+region.bytes.size();});
    check(found!=metadata.regions.end()&&found->owner=="native TU1.5 multiplayer main call stack","Unknown native caller scratch owner");
    const auto stack=found->address;const auto stack_bytes=found->bytes.size();metadata={};
    const auto original_pool=memory.view(base,bytes);std::vector<std::byte> retained(original_pool.begin(),original_pool.end());
    for(const auto &row:fixture.at("cases")){
        const unsigned count=row.at("count"),seed=row.at("seed");std::vector<std::byte> pool(bytes);
        for(unsigned n=0;n<bytes;++n)pool[n]=std::byte((n*17u+seed)&255u);
        pool[0]=std::byte{};put(pool,0x531fbcu-base,count);
        check(hash(pool)==row.at("initial_sha256").get<std::string>(),"Native/original retirement inputs differ");memory.write(base,pool);
        const auto conserved=outside(memory,stack,stack_bytes);
        check(source_retire_lens_queries(world)==count,"Native retirement lost its original request count");
        check(hash(memory.view(base,bytes))==row.at("retired_sha256").get<std::string>(),"Complete native retirement differs from unchanged original20700");
        check(outside(memory,stack,stack_bytes)==conserved,"Native retirement modified non-presentation state");
        source_require_retired_lens_queries(memory);
        check(source_retire_lens_queries(world)==count&&hash(memory.view(base,bytes))==row.at("retired_sha256").get<std::string>(),
            "Repeated native retirement changed its ring");++cases;
    }
    auto invalid=retained;put(invalid,0x531fbcu-base,512);memory.write(base,invalid);
    const auto before=hash(memory.view(base,bytes));rejects([&]{source_retire_lens_queries(world);});
    check(hash(memory.view(base,bytes))==before,"Malformed native retirement partially changed its ring");
    replace_bsp_with_pending_query(world);
    memory.write(base,retained);
    bool foreign_rejected{};std::thread foreign([&]{try{source_retire_lens_queries(world);}catch(const FormatError &){foreign_rejected=true;}});foreign.join();
    check(foreign_rejected,"Native retirement admitted a foreign executor");
    });
    std::cout<<"PASS "<<cases<<" complete unchanged-original retirement differentials, all three rings, 32-bit boundaries, full capacity, unused-bit preservation, idempotence and non-presentation/native-state conservation\n";
    std::cout<<"PASS stale CPU ring reproduces the new-device kick and the normal retirement removes it\n";
    std::cout<<"LIMIT declared CPU query banks and actual TU1.5 executor; GPU and live migration acceptance are separate\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
