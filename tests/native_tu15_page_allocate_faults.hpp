// Owned atomic-provider/observer prefix proof. Original normal-execution
// packets and stores determine the expected state; no candidate replay does.
namespace {
struct ParentProviderFault:std::runtime_error {ParentProviderFault():std::runtime_error("injected parent provider fault") {}};
struct ParentFaultStats {unsigned cases{},provider_prefixes{},observer_prefixes{},push_provider_prefixes{},push_observer_prefixes{},instruction_callbacks{};};
ParentFaultStats page_parent_faults(const J &fixture,const std::filesystem::path &directory){
    using namespace halo2;using physics_world_runtime_detail::WorldState;
    ParentFaultStats stats;
    const std::set<std::string> selected{"empty_strategy0","fragmented_strategy2","fragmented_strategy1","empty_strategy0__DF_set","genuine_owner_5_4096","full_capacity_strategy2","middle_strategy2","genuine_busy_queued","genuine_owner_15_4096","budget_1_2","page_shift_31"};
    const std::set<unsigned> pushes{0x13fe55,0x13fe61,0x13fe62,0x13fe72,0x13fec3,0x13ffa6,0x13ffa7,0x1400e6,0x1401e5,0x140231,0x140242};
    for(const auto &c:fixture.at("cases")){
        if(!selected.contains(c.at("name").get<std::string>())||c.at("parent_boundaries").empty())continue;
        ++stats.cases;const auto input=load(c,directory);const auto initial=page_abi(c);
        std::set<unsigned> io=c.at("parent_boundaries").get<std::set<unsigned>>();J points=J::array();
        for(auto packet:c.at("entry_abis"))if(io.contains(packet.at("entry").get<unsigned>())){packet["abi"]["eflags"]=packet["abi"]["eflags"].get<unsigned>()&0xcd5u;points.push_back(packet);}
        std::map<unsigned,std::vector<unsigned>> positions;
        for(unsigned n=0;n<points.size();++n)positions[points[n].at("entry")].push_back(n);
        std::map<unsigned,unsigned> occurrences;std::vector<unsigned> write_positions;
        for(const auto &event:c.at("writes")){
            const auto pc=event.at("instruction").get<unsigned>();
            if(io.contains(pc)){const auto occurrence=occurrences[pc]++;check(occurrence<positions.at(pc).size(),"Original write exceeds parent packets");write_positions.push_back(positions.at(pc)[occurrence]);}
            else write_positions.push_back(0xffffffffu);
        }
        const auto replay=[&](unsigned target,bool include_current){
            Memory memory=input;J events=J::array();std::set<unsigned> feedback;
            for(unsigned n=0;n<c.at("writes").size();++n){const auto position=write_positions[n];
                if(position!=0xffffffffu&&(position>target||(position==target&&!include_current)))break;
                const auto &event=c.at("writes")[n];const auto address=event.at("address").get<unsigned>();const auto bytes=unhex(event.at("written"));
                const auto bank=memory.at(address,bytes.size());const auto old=std::span(bank->second).subspan(address-bank->first,bytes.size());
                check(hex(old)==event.at("before").get<std::string>(),"Original prefix replay before-bytes differ");
                std::copy(bytes.begin(),bytes.end(),memory.banks.at(bank->first).begin()+address-bank->first);events.push_back(event);
                unsigned m{};for(const auto &mutation:c.value("mutations",J::array())){
                    if(mutation.at("instruction")==event.at("instruction")&&feedback.insert(m).second){const auto at=mutation.at("address").get<unsigned>();const auto value=unhex(mutation.at("written"));const auto storage=memory.at(at,value.size());std::copy(value.begin(),value.end(),memory.banks.at(storage->first).begin()+at-storage->first);}++m;
                }
                if(position==target)break;
            }
            return std::pair{std::move(memory.banks),std::move(events)};
        };
        for(unsigned target=0;target<points.size();++target){
            const auto pc=points[target].at("entry").get<unsigned>();const bool write=std::find(write_positions.begin(),write_positions.end(),target)!=write_positions.end();
            const auto pre=replay(target,false);const auto post=write?replay(target,true):pre;
            for(unsigned protocol=0;protocol<4;++protocol){if(protocol==3&&!write)continue;
                struct FaultMemory:Memory {
                    const J &input;const std::uint32_t &position;unsigned protocol;bool armed{},fired{},allow_peek{};std::set<unsigned> feedback;
                    FaultMemory(const Memory &initial,const J &c,const std::uint32_t &pc,unsigned mode):Memory(initial),input(c),position(pc),protocol(mode){}
                    void read(std::uint32_t address,std::span<std::byte> bytes)const override{
                        auto &self=const_cast<FaultMemory&>(*this);
                        if(armed&&protocol<3){if(protocol==2&&allow_peek)self.allow_peek=false;else{self.fired=true;throw ParentProviderFault{};}}
                        Memory::read(address,bytes);
                    }
                    void write(std::uint32_t address,std::span<const std::byte> bytes)override{
                        if(armed&&protocol<3){fired=true;throw ParentProviderFault{};}
                        Memory::write(address,bytes);unsigned n{};
                        for(const auto &event:input.value("mutations",J::array())){
                            if(event.at("instruction")==position&&feedback.insert(n).second){const auto at=event.at("address").get<unsigned>();const auto value=unhex(event.at("written"));const auto bank=Memory::at(at,value.size());std::copy(value.begin(),value.end(),banks.at(bank->first).begin()+at-bank->first);}++n;
                        }
                    }
                };
                unsigned current{};std::uint32_t cursor_pc{};FaultMemory memory{input,c,cursor_pc,protocol};J observed=J::array();
                SourcePhysicsContactRecordObserver observer;
                if(protocol)observer=[&](const auto &event){observed.push_back(J{{"instruction",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});if(protocol==3&&memory.armed){memory.fired=true;throw ParentProviderFault{};}};
                SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){cursor_pc=pc;++stats.instruction_callbacks;};cursor(services,cursor_pc);
                WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(initial);
                const auto before=[&](WorldState&,unsigned site){if(io.contains(site)&&current++==target){memory.armed=true;memory.allow_peek=protocol==2&&write;}};
                publish_entry={};bool threw{};
                try{check(native_page_allocate_detail::run(state,[](auto &s,auto owner,auto site,auto tail){page_reference_dispatch(s,owner,site,tail);},before),"Parent prefix candidate refused admission");}
                catch(const ParentProviderFault&){threw=true;}
                check(threw&&memory.fired,"Requested parent provider/observer prefix did not stop");
                auto expected=points[target];if(protocol==3&&pushes.contains(pc))expected["abi"]["gp"][4]=expected["abi"]["gp"][4].get<unsigned>()-4u;
                const auto actual=page_packet(state,cursor_pc);const auto &wanted=protocol==3?post:pre;
                if(actual!=expected||memory.banks!=wanted.first||(protocol&&observed!=wanted.second)){
                    std::cerr<<c.at("name")<<" protocol="<<protocol<<" parent-point="<<target<<" pc="<<std::hex<<pc<<std::dec<<" packet="<<(actual==expected)<<" banks="<<(memory.banks==wanted.first)<<" stores="<<(observed==wanted.second)<<'\n';
                    if(actual!=expected)std::cerr<<"native="<<actual.dump()<<"\noriginal-prefix="<<expected.dump()<<'\n';
                    throw std::runtime_error("Atomic parent Memory/store-observer prefix differs");
                }
                if(protocol==3){++stats.observer_prefixes;if(pushes.contains(pc))++stats.push_observer_prefixes;}
                else{++stats.provider_prefixes;if(pushes.contains(pc))++stats.push_provider_prefixes;}
            }
        }
    }
    return stats;
}
} // namespace
