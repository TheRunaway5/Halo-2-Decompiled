#include "halo2/native_tu15_arena_bootstrap.hpp"
#include "halo2/native_tu15_arena_data.hpp"
#include "halo2/native_threads.hpp"
#include "halo2/source_runtime_image.hpp"
#include <openssl/evp.h>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#if defined(__linux__)
#include <unistd.h>
#endif

namespace {
using namespace halo2;
using Json=nlohmann::json;
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
std::uint64_t integer(std::span<const std::byte> bytes) {
    std::uint64_t result=0;for(unsigned i=0;i<bytes.size();++i)result|=std::uint64_t(std::to_integer<unsigned>(bytes[i]))<<(8*i);return result;
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t p){return integer(memory.view(p,4));}
void put(SourcePhysicsContactMemory& memory,std::uint32_t p,std::uint32_t value) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));memory.write(p,bytes);
}
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,EVP_MAX_MD_SIZE> out{};unsigned count=0;
    check(EVP_Digest(bytes.data(),bytes.size(),out.data(),&count,EVP_sha256(),nullptr)==1,"SHA256 unavailable");
    std::ostringstream result;for(unsigned i=0;i<count;++i)result<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(out[i]);return result.str();
}
std::string file_digest(const std::filesystem::path& path) {
    std::ifstream in(path,std::ios::binary);check(bool(in),"Missing reference file");
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> context(EVP_MD_CTX_new(),EVP_MD_CTX_free);
    check(context && EVP_DigestInit_ex(context.get(),EVP_sha256(),nullptr)==1,"SHA256 context unavailable");
    std::array<char,65536> buffer{};
    while(in.read(buffer.data(),buffer.size()) || in.gcount())check(EVP_DigestUpdate(context.get(),buffer.data(),in.gcount())==1,"SHA256 update unavailable");
    check(in.eof(),"Reference read failed");std::array<unsigned char,EVP_MAX_MD_SIZE> out{};unsigned count=0;
    check(EVP_DigestFinal_ex(context.get(),out.data(),&count)==1,"SHA256 finish unavailable");
    std::ostringstream result;for(unsigned i=0;i<count;++i)result<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(out[i]);return result.str();
}
struct Directory {
    std::filesystem::path path;
    Directory(){
#if defined(__linux__)
        std::string name="/tmp/halo2-native-arena-XXXXXX";check(::mkdtemp(name.data()),"Cannot create private native arena test directory");path=name;
#else
        throw std::runtime_error("Native arena test file owner unavailable");
#endif
    }
    ~Directory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
// A scheduler input for comparison with the cooperative original-CPU oracle.
// It never supplies job return values, completion bytes or queue contents.
struct Schedule {
    std::mutex mutex;std::condition_variable_any changed;unsigned idle=0,epoch=0;
    SourcePhysicsGameArenaSchedule services(){return {
        [this](auto token,std::stop_token stop){
            if(token!=0xf3000003u)return;
            std::unique_lock lock(mutex);++idle;changed.notify_all();const auto seen=epoch;
            if(!changed.wait(lock,stop,[&]{return epoch>seen;}))throw NativeWorkerStopped();
        },
        [this]{
            std::unique_lock lock(mutex);
            check(changed.wait_for(lock,std::chrono::seconds(5),[&]{return idle>0;}),"Source worker did not reach its first idle wait");
            const auto target=idle+1;++epoch;changed.notify_all();
            check(changed.wait_for(lock,std::chrono::seconds(5),[&]{return idle>=target;}),"Source worker did not retire its actual file job");
        }};}
};
struct Step{std::uint32_t entry,argument;bool has_argument;std::size_t stores;};
constexpr std::array steps{
    Step{0x12cb70,0,false,18},Step{0x1221b0,0,false,2728},Step{0x125280,0,false,1049638},Step{0x125280,0,false,1049638},
    Step{0x125e50,0,true,1049640},Step{0x125e50,1,true,1049642},Step{0x125e50,3,true,1049644},
    Step{0x125e50,4,true,1049646},Step{0x125e50,5,true,1049648},Step{0x125e50,16,true,1049650},
    Step{0x125e50,257,true,1049652},Step{0x125e50,1024,true,1049654},Step{0x125e50,5120,true,1049656}};
}
int main(int argc,char** argv)try {
#if !defined(__linux__)
    return 77;
#endif
    std::filesystem::path reference,fixture_path,capture;
    for(int i=1;i<argc;i+=2) {
        check(i+1<argc,"Missing arena test argument");const std::string_view option=argv[i];
        if(option=="--reference-xbe")reference=argv[i+1];
        else if(option=="--original-fixture")fixture_path=argv[i+1];
        else if(option=="--capture")capture=argv[i+1];
        else throw std::runtime_error("Unknown arena test argument");
    }
    check(reference.empty()==fixture_path.empty(),"Original arena comparison needs both authenticated reference and fixture");
    if(!reference.empty() && (!std::filesystem::is_regular_file(reference) || !std::filesystem::is_regular_file(fixture_path)))return 77;
    Json fixture,report{{"schema","H2_NATIVE_TU15_ARENA_BOOTSTRAP_V1"},{"world_startup_complete",false},
        {"reference_executable_is_runtime_input",false},{"normal_gameplay_admission",false},{"steps",Json::array()}};
    std::ifstream expected;
    SourceRuntimeMemory declarations;mount_native_tu15_arena_data(declarations);
    std::size_t declared_bytes=0;Json fields=Json::array();
    declarations.visit_committed([&](auto p,auto bytes){declared_bytes+=bytes.size();fields.push_back({{"address",p},{"bytes",bytes.size()},{"initial_sha256",digest(bytes)}});});
    check(declared_bytes==43562 && fields.size()==20,"Native arena data includes unclassified storage");
    if(!reference.empty()) {
        const auto original=load_source_runtime_image(reference,SourcePhysicsContactImage::TitleUpdate15);
        declarations.visit_committed([&](auto p,auto bytes){check(std::ranges::equal(bytes,original.view(p,bytes.size())),"Native arena cold declaration differs from the authenticated reference at "+std::to_string(p));});
        std::ifstream input(fixture_path);fixture=Json::parse(input);
        check(fixture.at("source_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
            fixture.at("instruction_patches").empty() && fixture.at("supplied_gameplay_results").empty(),"Arena fixture is not the unchanged authenticated CPU reference");
        const auto stores=fixture_path.parent_path()/fixture.at("store_file").get<std::string>();
        check(file_digest(stores)=="73ae9913a4e1be6066fff2fb3fad28cc156b2ca4f4e6eb0371a8c3cdb06a79ba","Original arena store stream fingerprint differs");
        expected.open(stores,std::ios::binary);check(bool(expected),"Missing original arena stores");
        report["original_instructions"]=fixture.at("executed_original_instructions");
    }
    std::size_t stores=0;NativeRuntimeAddress native_arena{};
    {
        NativeTu15HeapBootstrap process;auto& bank=process.memory();mount_native_tu15_arena_data(bank);
        Directory files;Schedule schedule;
        const auto observe=[&](const SourcePhysicsContactRecordStore& store){
            if(store.address>=0x60000000u && store.address<0x60200000u)return;
            if(expected.is_open()) {
                std::array<std::byte,25> raw{};check(bool(expected.read(reinterpret_cast<char*>(raw.data()),raw.size())),"Original arena store stream exhausted");
                const auto bytes=std::span(raw);std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(8)<<store.instruction;
                check(fixture.at("canonical_instruction_map").at(key.str())==integer(bytes.first(4)) &&
                    store.address==integer(bytes.subspan(4,4)) && store.before.size()==integer(bytes.subspan(8,1)) &&
                    integer(store.before)==integer(bytes.subspan(9,8)) && integer(store.written)==integer(bytes.subspan(17,8)),
                    "Native typed arena differs from original ordered store "+std::to_string(stores));
            }
            ++stores;
        };
        {
            SourcePhysicsGameArenaNative arena(bank,{files.path,0x60100000u,{0x27f,0x1f80},schedule.services()},{},observe);
            for(std::size_t i=0;i<steps.size();++i) {
                const auto& step=steps[i];constexpr auto sp=0x6000f000u;
                SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=sp;abi.eflags=2;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
                put(arena.memory(),sp,0x60200000u);if(step.has_argument)put(arena.memory(),sp+4,step.argument);
                arena.run(step.entry,abi);check(abi.gp[4]==sp+4+(step.has_argument?4:0),"Native arena lost its source caller frame");
                check(stores==step.stores,"Native arena source write count differs");
                if(!fixture.is_null()) {
                    const auto& row=fixture.at("steps").at(i);const auto& final=row.at("exit_abi");
                    check(std::stoul(row.at("entry").get<std::string>(),nullptr,16)==step.entry &&
                        row.at("arguments")== (step.has_argument?Json::array({step.argument}):Json::array()),"Arena reference program differs");
                    auto gp=final.at("gp").get<decltype(abi.gp)>();
                    // The priority BOOL is the actual host outcome, ignored by
                    // this source caller. No original kernel clobber is claimed.
                    if(i==1){check(abi.gp[0]<=1,"Invalid native priority BOOL");gp[0]=abi.gp[0];}
                    check(abi.gp==gp && (abi.eflags&0xcd5u)==(final.at("eflags").get<std::uint32_t>()&0xcd5u),"Native arena returned GP/defined flags differ");
                    check(!abi.x87_depth && !abi.active_mmx && !abi.x87_status && abi.xmm==decltype(abi.xmm){},"Native arena changed retained FP state");
                }
                report["steps"].push_back({{"entry",step.entry},{"argument",step.has_argument?Json(step.argument):Json(nullptr)},{"returned_gp",abi.gp},{"ordered_stores",stores}});
            }
            if(expected.is_open()){char extra;check(!expected.read(&extra,1) && expected.eof(),"Unused original arena store suffix");}
            arena.stop();
            check(word(bank,0x4f2950u)==0x82d47000u && word(bank,0x4f2954u)==11188u &&
                word(bank,0x4f295cu)==2217386973u,"Native arena allocator/cursor/CRC differs from TU1.5");
            native_arena=bank.native_address(word(bank,0x4f2950u),0x3fe000u);
            check(native_arena.value>0xffffffffu,"Native arena backing was narrowed into a source pointer");
            const auto saved=arena.ownership_checkpoint();
            check(saved.files.size()==2 && saved.synchronization.objects.size()==3 && saved.physical.allocations.size()==1,
                "Native arena lacks its actual file, synchronization or page owners");
            for(unsigned i=0;i<2;++i)check(std::filesystem::file_size(files.path/("savegame"+std::to_string(i)+".bin"))==4239360u,"Native arena file job created the wrong extent");
            const auto physical_sha=digest(bank.view(0x80061000u,0x30e4000u));
            if(!fixture.is_null())check(physical_sha==fixture.at("final_banks").back().at("sha256").get<std::string>(),"Complete original physical arena differs");
            report["physical_sha256"]=physical_sha;report["physical_bytes"]=0x30e4000u;
            report["physical_bytes_compared"]=reference.empty()?0u:0x30e4000u;
        }
        bool released=false;try{bank.source_address(native_arena,1);}catch(const std::exception&){released=true;}
        check(released,"Native arena retained a released host address");
    }
    // Production startup composition uses an ungated real worker and the same
    // caller/storage policy as the native live world. No reference is passed.
    {
        Directory files,other_files;NativeTu15ArenaBootstrap startup(files.path),other(other_files.path);
        check(startup.heap().value>0xffffffffu && startup.arena().value>0xffffffffu,"Native startup lost its checked 64-bit owners");
        check(word(startup.memory(),0x4f2954u)==4748u && word(startup.memory(),0x4f295cu)==1696638025u &&
            integer(startup.memory().view(0x50ebccu,1))==1u,"Ungated native arena startup did not finish its source file jobs/CRC");
        check(word(startup.memory(),0x4ef1bcu)==0,"Native startup handed off with queued source jobs");
        // The ungated worker may recycle a previous job before or after the
        // main thread takes its next one. The resulting free-list order is
        // source scheduling state; require the complete owned free set.
        std::set<std::uint32_t> free_jobs;
        for(auto job=word(startup.memory(),0x4ef1b8u);job;job=word(startup.memory(),job+0x3cu)) {
            check(job>=0x4ecc38u && job<0x4ef1b8u && (job-0x4ecc38u)%0x40u==0 && free_jobs.insert(job).second,
                "Native startup free-job chain escapes its owner or cycles");
            check(word(startup.memory(),job+4u)==0xffffffffu,"Native startup retained an active source job");
        }
        check(free_jobs.size()==150,"Native startup handed off before recycling all source jobs");
        bool rejected=false;try{startup.memory().source_address(other.arena(),1);}catch(const std::exception&){rejected=true;}
        check(rejected,"Native startup accepted another arena owner's host address");
        rejected=false;try{put(startup.memory(),0x47c76cu,0);}catch(const std::exception&){rejected=true;}
        check(rejected,"Native startup made an immutable filename binding writable");
    }
    report["declared_bytes"]=declared_bytes;report["cold_fields"]=fields;report["ordered_stores"]=stores;
    report["native_64_bit_ownership"]=true;report["ungated_worker_startup"]=true;report["released_and_foreign_addresses_rejected"]=true;
    report["original_cpu_comparison"]=!reference.empty();report["passed"]=true;
    if(!capture.empty()){std::filesystem::create_directories(capture.parent_path());std::ofstream out(capture);out<<report.dump(2)<<'\n';check(bool(out),"Cannot write native arena evidence");}
    std::cout<<"Native TU1.5 arena bootstrap: "<<declared_bytes<<" classified cold/operand bytes; 13 complete calls, "<<stores
        <<" ordered writes; real worker, two files and checked 64-bit arena; "<<(reference.empty()?"no executable input":"unchanged original CPU comparison passed")<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
