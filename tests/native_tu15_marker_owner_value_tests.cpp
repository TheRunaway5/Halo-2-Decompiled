#include "native_tu15_marker_owner_test_support.hpp"
#include "halo2/native_campaign_object_motion_refs_tu15.hpp"
namespace frozen_marker_owner{
std::uint32_t native_campaign_object_marker_owner_tu15(const halo2::SourcePhysicsContactMemory&,std::uint32_t);
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply inherited-marker DATA fixture");std::ifstream file(argv[1]);const auto fixture=J::parse(file);
    unsigned comparisons{},interruptions{};
    for(const auto&row:fixture.at("cases")){
        if(row.contains("fault")||!row.value("value_domain",true))continue;
        for(bool noisy:{false,true}){
            std::optional<J>fault;
            J reference_reads;std::uint32_t reference_value{};native_raw_x87_detail::Image reference_fp;
            unsigned count{};
            for(int stop=-1;;++stop){
                if(stop>=0&&unsigned(stop)>=count)break;
                bool old_stopped{};J old_reads;std::uint32_t old_value{};native_raw_x87_detail::Image old_fp;
                for(bool old:{true,false}){
                    OwnerMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
                    memory.noisy=noisy;memory.capture=[]{return J{};};
                    if(stop>=0)memory.fault=J{{"phase","before_read"},{"ordinal",stop}};
                    asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
                    const auto start=native_raw_x87_detail::capture();bool stopped{};std::uint32_t value{};
                    try{value=old?frozen_marker_owner::native_campaign_object_marker_owner_tu15(memory,row.at("matrix")):native_campaign_object_marker_owner_tu15(memory,row.at("matrix"));}
                    catch(const Fault&){stopped=true;}
                    const auto finished=native_raw_x87_detail::capture();native_raw_x87_detail::restore(start);
                    if(old){old_stopped=stopped;old_reads=memory.reads;old_value=value;old_fp=finished;}
                    else{
                        check(stopped==old_stopped&&memory.reads==old_reads&&value==old_value,"Public inherited-marker predecessor/DATA policy differs");
                        check(native_raw_x87_detail::equivalent(finished,old_fp),"Public inherited-marker provider FP effects differ");
                        if(stop<0){check(value==row.at("expected").at("gp")[0].get<std::uint32_t>(),"Public inherited-marker original result differs");count=memory.reads.size();}
                        else ++interruptions;
                        ++comparisons;
                    }
                }
            }
        }
    }
    std::cout<<J{{"passed",true},{"predecessor_comparisons",comparisons},{"read_interruptions",interruptions},{"provider_fp_effects_preserved",true},{"physical_result_comparison_on_ordinary_backing",true}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
