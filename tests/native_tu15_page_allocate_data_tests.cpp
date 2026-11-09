#include "native_tu15_allocator_abi_fixture.hpp"
#include "halo2/native_tu15_page_allocate_kernel.hpp"
#include "halo2/native_tu15_stack_probe_kernel.hpp"
#include "halo2/native_tu15_page_window_kernel.hpp"
#include "halo2/native_tu15_page_retire_kernel.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include <iostream>
#include <set>

namespace halo2::page_data_callback_owner {
using namespace physics_world_runtime_detail;
struct RequiredOwner {std::uint32_t entry;};
static void dispatch(WorldState&,std::uint32_t entry,std::uint32_t,bool){throw RequiredOwner{entry};}
#include "page_data_busy_reference.inc"
}
namespace {
using namespace native_allocator_fixture;
using P=NativeTu15PageAllocatePhase;using F=NativeTu15PageFrameSlot;using D=NativeTu15PageAllocation;
bool caller_word(std::uint32_t address,std::size_t size){return address>=0x2000000u&&std::uint64_t(address)+size<=0x2010000u;}
struct DataMemory:Memory {
    J writes=J::array();
    void write(std::uint32_t address,std::span<const std::byte> input)override{
        const auto bank=at(address,input.size());const auto old=std::span(bank->second).subspan(address-bank->first,input.size());
        if(!caller_word(address,input.size()))writes.push_back(J{{"address",address},{"before",hex(old)},{"written",hex(input)}});
        Memory::write(address,input);
    }
};
struct Storage {
    DataMemory &memory;const J &input;std::array<std::uint32_t,8> caller;std::set<unsigned> feedback;
    std::uint32_t base()const{return caller[4]-0x1064u;}
    std::uint32_t word(std::uint32_t address,unsigned width=4u){std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;}
    void store(std::uint32_t address,std::uint32_t value,unsigned width=4u){std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));memory.write(address,std::span(bytes).first(width));}
    void apply_feedback(std::uint32_t instruction){unsigned n{};for(const auto &m:input.value("mutations",J::array())){
        if(m.at("instruction")==instruction&&feedback.insert(n).second){const auto p=m.at("address").get<std::uint32_t>();const auto bytes=unhex(m.at("written"));const auto bank=memory.at(p,bytes.size());std::copy(bytes.begin(),bytes.end(),memory.banks.at(bank->first).begin()+p-bank->first);}++n;}}
    template<class Phase,class Data>std::uint32_t read(Phase,const Data&,std::uint32_t address,unsigned width=4u){return word(address,width);}
    template<class Phase,class Data>void write(Phase,const Data&,std::uint32_t address,std::uint32_t value,unsigned width=4u){store(address,value,width);}
    template<class Phase,class Data>void prepare(Phase,const Data&){}
    template<class Phase,class Data>void complete(Phase,const Data&){}
    void prepare(P phase,D&){
        if(phase==P::begin){
            // Test-only caller-frame transport. The shared probe still owns
            // all page touches and genuine return-word relocation. This DATA
            // gate does not claim a retained GP/flags or callee-entry adapter.
            store(caller[4]-4u,0x13fe5au);
            struct Probe {
                Storage &memory;std::uint32_t counter;
                void prepare(NativeTu15StackProbePhase p,const NativeTu15StackProbe &s){if(p==NativeTu15StackProbePhase::large_begin)memory.store(s.saved_cursor,counter);}
                std::uint32_t read(NativeTu15StackProbePhase,const NativeTu15StackProbe&,std::uint32_t at){return memory.word(at);}
                void write(NativeTu15StackProbePhase,const NativeTu15StackProbe&,std::uint32_t at,std::uint32_t value){memory.store(at,value);}
                void complete(NativeTu15StackProbePhase,const NativeTu15StackProbe &s){(void)memory.word(s.cursor-4u);}
            } probe{*this,caller[1]};NativeTu15StackProbe reservation;reservation.entry_sp=caller[4]-4u;reservation.bytes=0x1054u;native_tu15_probe_stack(probe,reservation);
        }else if(phase==P::begin_scan)store(base(),caller[7]);
    }
    std::uint32_t argument(P phase,const D&){
        if(phase==P::request_bytes)return word(caller[4]+8u);
        if(phase==P::owner){store(base()+12u,caller[3]);store(base()+8u,caller[5]);return word(caller[4]+4u);}
        return word(caller[4]+12u);
    }
    static std::uint32_t offset(F slot){switch(slot){
        case F::pages:return 0x1cu;case F::result:return 0x20u;case F::policy:return 0x2cu;
        case F::chosen:return 0x13u;case F::ring_begin:return 0x14u;case F::ring_end:return 0x18u;
        case F::current_handle:return 0x28u;case F::previous:return 0x48u;case F::span_tick:return 0x24u;
        case F::span_pages:return 0x30u;case F::victim:return 0x34u;case F::victim_tick:return 0x50u;
        case F::best_previous:return 0x38u;case F::best_tick:return 0x3cu;case F::best_start:return 0x40u;
        case F::best_pages:return 0x44u;case F::next_index:return 0x4cu;case F::datum_cursor:return 0x60u;}
        throw std::runtime_error("Unknown typed page frame field");}
    std::uint32_t frame_read(P,const D&,F slot){return word(base()+offset(slot),slot==F::chosen?1u:4u);}
    void frame_write(P phase,const D&,F slot,std::uint32_t value){if(phase==P::pages_initial)store(base()+4u,caller[6]);store(base()+offset(slot),value,slot==F::chosen?1u:4u);if(phase==P::ring_end_publish)apply_feedback(0x13ff52u);}
    std::uint32_t window_address(const D&,std::uint32_t index)const{return base()+0x64u+index*16u;}
    std::uint32_t best_window_address(const D&)const{return base()+0x38u;}
    void write(P phase,const D&,std::uint32_t address,std::uint32_t value,unsigned width){
        store(address,value,width);
        if(phase==P::window_pages_publish)apply_feedback(0x140093u);
    }
};
struct Services {
    Storage &storage;unsigned allocations{},retirements{},preferences{},retained_busy_callbacks{};J busy_inputs=J::array();
    NativeTu15PageBusyResume busy(std::uint32_t owner,std::uint32_t datum,const D &s){
        check(owner==0x12fcb0u,"Unexpected page DATA busy owner");
        busy_inputs.push_back(J::array({s.owner,s.record,datum}));
        // Test-only genuine canonical provider. These arguments are derived
        // from the DATA caller; the retained child computes its own result.
        // It stays explicitly translated and never enters production.
        auto state=abi(storage.input);state.gp={owner,s.rows,s.record_tick,s.span_end,storage.base()-8u,s.owner,s.record,datum};
        storage.store(state.gp[4]+4u,datum);storage.store(state.gp[4],0x13ffa9u);
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto){++retained_busy_callbacks;};SourcePhysicsContactRecordObserver observe;
        physics_world_runtime_detail::WorldState child{storage.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};child.load(state);
        page_data_callback_owner::run_12fcb0(child);return {child.gp[5],child.gp[6],child.gp[7],bool(child.gp[0]&255u)};
    }
    NativeTu15PageWindowResume prefer(std::uint32_t owner,std::uint32_t candidate,std::uint32_t best,const D &s){
        ++preferences;NativeTu15PageWindow data;data.owner=owner;data.candidate=candidate;data.best=best;
        return {owner,candidate,s.span_end,native_tu15_prefer_page_window(storage,data)};
    }
    void retire(std::uint32_t owner,std::uint32_t datum){
        ++retirements;NativeTu15PageRetirement data;data.owner=owner;data.datum=datum;
        struct Children {
            Storage &storage;
            NativeTu15PageRetireResume callback(std::uint32_t,std::uint32_t,const NativeTu15PageRetirement&){throw std::runtime_error("DATA-only page cohort reached a real retirement callback");}
            void erase(std::uint32_t pool,std::uint32_t datum,const NativeTu15PageRetirement&){native_tu15_data_array_delete(storage.memory,pool,datum);}
        } children{storage};native_tu15_retire_page(storage,data,children);
    }
    NativeTu15PageOverlapResume retire_overlap(std::uint32_t owner,std::uint32_t datum,const D &s){retire(owner,datum);return {owner,s.descriptor};}
    NativeTu15PageVictimResume retire_victim(std::uint32_t owner,std::uint32_t datum,const D &s){retire(owner,datum);return {owner,s.predecessor};}
    NativeTu15PageAllocationResume allocate(std::uint32_t pool,const D &s){++allocations;return {s.owner,s.predecessor,native_tu15_data_array_allocate(storage.memory,pool)};}
};
}
int main(int argc,char **argv)try{
    check(argc==2,"Supply page-allocation DATA fixture");const std::filesystem::path path=argv[1];std::ifstream file(path);const auto fixture=J::parse(file);unsigned cases{},reads{},stores{},allocations{},retirements{},preferences{},retained_busy_callbacks{},semantic_callback_inputs{},boundary_cases{};
    for(const auto &c:fixture.at("cases")){
        DataMemory memory;static_cast<Memory&>(memory)=load(c,path.parent_path());memory.logging=true;
        Storage storage{memory,c,c.at("before_abi").at("gp").get<std::array<std::uint32_t,8>>(),{}};Services services{storage};D data;
        std::uint32_t result{},stopped{};try{result=native_tu15_allocate_pages(storage,data,services);}catch(const page_data_callback_owner::RequiredOwner &stop){stopped=stop.entry;}
        const auto name=c.at("name").get<std::string>();check(stopped==c.at("required_entry"),name+" DATA genuine child boundary differs");
        if(stopped)++boundary_cases;else check(result==c.at("after_abi").at("gp").at(0),name+" page DATA result differs");
        J expected_reads=J::array(),actual_reads=J::array(),expected_stores=J::array();
        for(const auto &e:c.at("reads"))if(!caller_word(e.at("address"),e.at("bytes")))expected_reads.push_back(J::array({e.at("address"),e.at("bytes")}));
        for(const auto &e:memory.reads)if(!caller_word(e.at(0),e.at(1)))actual_reads.push_back(e);
        if(expected_reads!=actual_reads){unsigned n{};while(n<expected_reads.size()&&n<actual_reads.size()&&expected_reads[n]==actual_reads[n])++n;
            throw std::runtime_error(name+" heap DATA read order differs at"+std::to_string(n)+" native="+(n<actual_reads.size()?actual_reads[n].dump():"end")+" original="+(n<expected_reads.size()?expected_reads[n].dump():"end"));}
        for(auto e:c.at("writes"))if(!caller_word(e.at("address"),unhex(e.at("written")).size())){e.erase("instruction");expected_stores.push_back(e);}
        check(memory.writes==expected_stores,name+" heap DATA stores differ");
        for(const auto &bank:c.at("banks"))if(!caller_word(bank.at("address"),memory.banks.at(bank.at("address")).size()))
            check(memory.banks.at(bank.at("address"))==native_allocator_fixture::file(path.parent_path()/bank.at("after_file").get<std::string>()),name+" complete heap DATA bank differs");
        J expected_inputs=J::array();for(const auto &entry:c.at("entry_abis"))if(entry.at("entry")==0x12fcb0u){const auto &g=entry.at("abi").at("gp");expected_inputs.push_back(J::array({g.at(5),g.at(6),g.at(7)}));}
        check(expected_inputs==services.busy_inputs,name+" semantic busy callback inputs differ");semantic_callback_inputs+=expected_inputs.size();
        ++cases;reads+=actual_reads.size();stores+=memory.writes.size();allocations+=services.allocations;retirements+=services.retirements;preferences+=services.preferences;retained_busy_callbacks+=services.retained_busy_callbacks;
    }
    std::cout<<J{{"passed",true},{"cases",cases},{"genuine_child_boundary_cases",boundary_cases},{"semantic_callback_inputs",semantic_callback_inputs},{"retained_busy_instruction_callbacks",retained_busy_callbacks},{"ordered_heap_reads",reads},{"ordered_heap_stores",stores},{"shared_datum_allocations",allocations},{"shared_page_retirements",retirements},{"shared_window_preferences",preferences},{"full_retained_abi_qualified",false},{"callback_service_closure_qualified",false}}.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
