#include "audio/native_tu15_sound_kernel.hpp"
#include "native_tu15_sound_test_support.hpp"
#include "halo2/source_loop_combined_publication_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
using Json=nlohmann::json;
std::vector<std::byte> decode(const std::string& text) {
    if(text.size()%2)throw std::runtime_error("Incomplete original backing bytes");
    std::vector<std::byte> result;
    for(unsigned i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
void populate(sound_test::Memory& memory,const Json& writes) {
    for(const auto& write:writes) {
        const auto address=write[0].get<std::uint32_t>();const auto bytes=decode(write[1]);
        memory.map(address,bytes.size());memory.write(address,bytes);
    }
}
}
int main(int argc,char** argv)try {
    if(argc!=2)throw std::runtime_error("Supply the independent original combined-publication fixture");
    const auto fixture=Json::parse(std::ifstream(argv[1]));
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
        fixture.at("entry")!=0x2216d0u || fixture.at("cases").size()!=320 ||
        fixture.at("reference_backend")!="unchanged original Linux i386 CPU; Unicorn ordered-write provenance" ||
        fixture.at("hardware_unmapped_rejection_cases")!=200 ||
        !fixture.at("source_instruction_patches").empty() || !fixture.at("substituted_calls").empty())
        throw std::runtime_error("Combined publication reference authority/scope differs");
    const auto kernels=native_tu15_sound::gain_kernels();
    const auto kernel=std::ranges::find(kernels,0x2216d0u,&native_tu15_sound::Kernel::entry);
    if(kernel==kernels.end() || kernel->calls_out || kernel->contract.gp_full!=0xff || kernel->contract.xmm!=0xff)
        throw std::runtime_error("Complete combined publication kernel is not registered with its full contract");
    const auto stack=fixture.at("stack").get<std::uint32_t>();
    const auto output=fixture.at("output_address").get<std::uint32_t>();
    unsigned failures{},index{},accepted{},rejected{};
    for(const auto& row:fixture.at("cases")) {
      sound_test::Memory input;input.map(stack-0x2000,0x3000);
      populate(input,fixture.at("memory"));populate(input,row.at("memory"));
      for(const bool literal:{false,true}) {
        auto memory=input.clone();
        SourcePhysicsContactRuntimeAbi abi{};
        abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();
        abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
        const SourcePhysicsFactoryFloatingPoint fp{row.at("control"),row.at("mxcsr")};abi.floating_point=fp;
        sound_test::Memory guarded;
        for(auto address:row.at("unmapped_guard_pages")) {
            const auto page=address.get<std::uint32_t>()>>12;
            guarded.pages.emplace(page,memory.pages.at(page));memory.pages.erase(page);
        }
        Json stores=Json::array();
        const auto observer=[&](const SourcePhysicsContactRecordStore& store) {
            if((store.address>=output && store.address<output+80) || (store.address>=stack+4 && store.address<stack+20)) {
                std::uint32_t value{};for(unsigned i=0;i<store.written.size();++i)
                    value|=std::to_integer<std::uint32_t>(store.written[i])<<(8*i);
                stores.push_back({store.instruction,store.address,store.written.size(),value});
            }
        };
        try {
            if(literal)source_run_loop_combined_publication_tu15(memory,0x2216d0u,abi,fp,{},observer);
            else kernel->run(memory,abi,fp,observer,{});
        }catch(const std::exception& error) {
            throw std::runtime_error(std::string(literal?"Literal":"Native")+" combined publication case "+
                std::to_string(index)+" faulted: "+error.what());
        }
        memory.pages.insert(guarded.pages.begin(),guarded.pages.end());
        std::vector<std::byte> backing(80);memory.read(output,backing);
        const bool success=(row.at("expected_gp")[0].get<std::uint32_t>()&255u)!=0;
        if(!literal){accepted+=success;rejected+=!success;}
        const auto expected_gp=row.at("expected_gp").get<std::array<std::uint32_t,8>>();
        const auto expected_xmm=row.at("expected_xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
        if(abi.gp!=expected_gp || abi.xmm!=expected_xmm || backing!=decode(row.at("output")) ||
            stores!=row.at("stores") || abi.x87_depth!=0 || abi.x87_status!=row.at("expected_status") ||
            abi.floating_point->x87_control_word!=row.at("expected_control") || abi.floating_point->mxcsr!=row.at("expected_mxcsr")) {
            if(failures++<6)std::cerr<<(literal?"Literal":"Native")<<" combined publication case "<<index<<" differs: GP "<<Json(abi.gp)
                <<" expected "<<Json(expected_gp)<<" XMM equal="<<(abi.xmm==expected_xmm)
                <<" backing equal="<<(backing==decode(row.at("output")))<<" stores equal="<<(stores==row.at("stores"))
                <<" status="<<abi.x87_status<<" expected="<<row.at("expected_status")
                <<" mxcsr="<<abi.floating_point->mxcsr<<" expected="<<row.at("expected_mxcsr")<<'\n';
        }
      }
        ++index;
    }
    if(failures)throw std::runtime_error(std::to_string(failures)+" native combined publications differ from original execution");
    if(accepted!=120 || rejected!=200)throw std::runtime_error("Original generation/key rejection coverage differs");
    std::cout<<"PASS "<<index<<" complete original combined publications; "<<accepted
        <<" accepted parents, "<<rejected<<" stale/NONE/key/range rejections with inaccessible banks; native and literal full GP/XMM/FP and ordered output stores\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
