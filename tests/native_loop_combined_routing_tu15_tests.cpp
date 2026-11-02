#include "audio/native_tu15_sound_kernel.hpp"
#include "native_tu15_sound_test_support.hpp"
#include "halo2/source_loop_combined_routing_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
std::vector<std::byte> decode(const std::string& text){
    if(text.size()%2)throw std::runtime_error("Incomplete original backing");
    std::vector<std::byte> bytes;
    for(unsigned n=0;n<text.size();n+=2)bytes.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return bytes;
}
void populate(sound_test::Memory& memory,const Json& writes){
    for(const auto& write:writes){const auto at=write[0].get<std::uint32_t>();const auto bytes=decode(write[1]);
        memory.map(at,bytes.size());memory.write(at,bytes);}
}
}
int main(int argc,char** argv)try{
    if(argc!=2)throw std::runtime_error("Supply the independent original combined-routing fixture");
    const auto fixture=Json::parse(std::ifstream(argv[1]));
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
        fixture.at("entry")!=0x2b9450u || fixture.at("cases").size()!=384 ||
        fixture.at("reference_backend")!="unchanged original Linux i386 CPU; Unicorn ordered-write provenance" ||
        fixture.at("hardware_guarded_plugin_cases")!=320 || !fixture.at("source_instruction_patches").empty() ||
        !fixture.at("substituted_calls").empty())throw std::runtime_error("Combined routing reference authority/scope differs");
    const auto kernels=native_tu15_sound::gain_kernels();
    const auto kernel=std::ranges::find(kernels,0x2b9450u,&native_tu15_sound::Kernel::entry);
    if(kernel==kernels.end() || kernel->calls_out || kernel->contract.gp_full!=0xff || kernel->contract.xmm!=0xff)
        throw std::runtime_error("Complete native routing kernel is not registered with its full contract");
    const auto stack=fixture.at("stack").get<std::uint32_t>(),output=fixture.at("output_address").get<std::uint32_t>();
    unsigned index{},failures{},dry{},double_send{},no_send{};
    for(const auto& row:fixture.at("cases")){
        sound_test::Memory input;input.map(stack-0x2000,0x3000);populate(input,fixture.at("memory"));populate(input,row.at("memory"));
        const auto prefix=row.at("prefix").get<unsigned>(),kind=row.at("kind").get<std::uint32_t>();
        const auto expected=decode(row.at("output"));
        const auto word=[&](unsigned at){std::uint32_t value{};for(unsigned n=0;n<4;++n)
            value|=std::to_integer<std::uint32_t>(expected.at(at+n))<<(8*n);return value;};
        const std::array<std::uint32_t,4> bins=row.at("bypass").get<bool>()?
            std::array<std::uint32_t,4>{0,1,4,5}:std::array<std::uint32_t,4>{6,7,8,9};
        for(unsigned n=0;n<4;++n)if(word((prefix+n)*8)!=bins[n])throw std::runtime_error("Original dry route inventory changed");
        const auto sends=kind==0?1u:kind==1?2u:0u;
        if(word(0x40)!=prefix+4+sends)throw std::runtime_error("Original class route count changed");
        for(unsigned n=0;n<sends;++n)if(word((prefix+4+n)*8)!=10u)throw std::runtime_error("Original class send changed");
        if(sends==2 && word((prefix+4)*8+4)!=word((prefix+5)*8+4))throw std::runtime_error("Original repeated send differs");
        dry+=row.at("bypass").get<bool>();double_send+=sends==2;no_send+=sends==0;
        for(const bool literal:{false,true}){
            auto memory=input.clone();SourcePhysicsContactRuntimeAbi abi{};
            abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();
            abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
            const SourcePhysicsFactoryFloatingPoint fp{row.at("control"),row.at("mxcsr")};abi.floating_point=fp;
            sound_test::Memory guarded;
            for(auto address:row.at("unmapped_guard_pages")){const auto page=address.get<std::uint32_t>()>>12;
                guarded.pages.emplace(page,memory.pages.at(page));memory.pages.erase(page);}
            Json stores=Json::array();
            const auto observer=[&](const SourcePhysicsContactRecordStore& store){
                if((store.address>=output && store.address<output+80) || (store.address>=stack+4 && store.address<stack+20)){
                    std::uint32_t value{};for(unsigned n=0;n<store.written.size();++n)value|=std::to_integer<std::uint32_t>(store.written[n])<<(8*n);
                    stores.push_back({store.instruction,store.address,store.written.size(),value});}
            };
            try{
                if(literal)source_run_loop_combined_routing_tu15(memory,0x2b9450u,abi,fp,{},observer);
                else kernel->run(memory,abi,fp,observer,{});
            }catch(const std::exception& error){throw std::runtime_error(std::string(literal?"Literal":"Native")+
                " combined routing case "+std::to_string(index)+" faulted: "+error.what());}
            memory.pages.insert(guarded.pages.begin(),guarded.pages.end());
            std::vector<std::byte> backing(80),caller(16);memory.read(output,backing);memory.read(stack+4,caller);
            const auto gp=row.at("expected_gp").get<std::array<std::uint32_t,8>>();
            const auto xmm=row.at("expected_xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
            if(abi.gp!=gp || abi.xmm!=xmm || backing!=expected || caller!=decode(row.at("caller")) ||
                stores!=row.at("stores") || abi.x87_depth!=0 || abi.x87_status!=row.at("expected_status") ||
                abi.floating_point->x87_control_word!=row.at("expected_control") || abi.floating_point->mxcsr!=row.at("expected_mxcsr")){
                if(failures++<6)std::cerr<<(literal?"Literal":"Native")<<" combined routing case "<<index<<" differs: GP "<<Json(abi.gp)
                    <<" expected "<<Json(gp)<<" XMM equal="<<(abi.xmm==xmm)<<" backing equal="<<(backing==expected)
                    <<" caller equal="<<(caller==decode(row.at("caller")))<<" stores equal="<<(stores==row.at("stores"))
                    <<" status="<<abi.x87_status<<" expected="<<row.at("expected_status")
                    <<" mxcsr="<<abi.floating_point->mxcsr<<" expected="<<row.at("expected_mxcsr")<<'\n';
            }
        }
        ++index;
    }
    if(failures)throw std::runtime_error(std::to_string(failures)+" native/literal combined routes differ from original CPU execution");
    if(dry!=192 || double_send!=64 || no_send!=256)throw std::runtime_error("Original route category coverage differs");
    std::cout<<"PASS "<<index<<" complete original combined routes; native/literal full GP/XMM/FP and ordered output/caller stores; "
        <<dry<<" bypasses, "<<double_send<<" duplicate class sends, "<<no_send<<" no-send categories\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
