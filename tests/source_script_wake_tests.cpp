#define HALO2_WAKE_STATE_CHECKS 1
#include "source_script_wake_fixture.hpp"
using namespace halo2::script_wake_fixture;
int main(int argc,char **argv) {
    try {
        if(argc!=2)throw std::runtime_error("expected fixture directory");
        const std::filesystem::path root=argv[1];std::ifstream cases(root/"cases.tsv");
        std::string name;unsigned ticks{},failures{},case_count{};
        while(cases>>name>>ticks) {
            ++case_count;
            auto p=load(root/(name+".hsw"));unsigned tick{};std::ostringstream observed;
            ScriptVm vm(p,[&](const auto &builtin,const auto &args)->ScriptValue {
                if(builtin!="print" || args.size()!=1)throw std::runtime_error("undeclared host boundary");
                observed<<tick<<' '<<args[0].text<<'\n';return {};
            });
            vm.start_level();
#ifdef HALO2_WAKE_STATE_CHECKS
            std::ifstream states(root/(name+".states"));
            std::optional<ScriptVm::AuthorityCheckpoint> authority;
#endif
            // Checkpoint after external sleep has established the source flag
            // and saved schedule; restore must preserve that continuation too.
            std::optional<ScriptVm::Snapshot> saved;
            std::string prefix;
            for(tick=0;tick<ticks;++tick){
                vm.tick(tick);
#ifdef HALO2_WAKE_STATE_CHECKS
                const auto current=vm.authority_checkpoint();
                for(const auto &t:current.threads) {
                    unsigned expected_tick{},script{},active{};std::int64_t wake{},previous{};
                    if(!(states>>expected_tick>>script>>wake>>active>>previous))throw std::runtime_error(name+": missing original state");
                    const auto actual=t.stopped?-1:t.dormant?-2:std::int64_t(t.wake);
                    const auto saved=t.external_sleep?(t.external_sleep->dormant?-2:std::int64_t(t.external_sleep->wake)):0;
                    if(expected_tick!=tick || script!=t.script || actual!=wake || active!=bool(t.external_sleep) || (active && saved!=previous))
                        throw std::runtime_error(name+": schedule/first external wake differs at tick "+std::to_string(tick));
                }
                if(tick==1)authority=current;
#endif
                if(tick==1){saved=vm.snapshot();prefix=observed.str();}
            }
            if(!vm.diagnostics().empty())throw std::runtime_error(name+": "+vm.diagnostics().front());
            const auto expected=read(root/(name+".events"));const auto first=observed.str();
            if(first!=expected){++failures;std::cout<<"FAIL "<<name<<"\nexpected:\n"<<expected<<"observed:\n"<<first;}
            else std::cout<<"PASS "<<name<<'\n';
            if(saved){vm.restore(*saved);observed.str(prefix);observed.clear();observed.seekp(0,std::ios::end);for(tick=2;tick<ticks;++tick)vm.tick(tick);
                if(observed.str()!=first)throw std::runtime_error(name+": restored dispatch diverged");}
#ifdef HALO2_WAKE_STATE_CHECKS
            if(authority){auto prepared=vm.prepare_authority(*authority);if(!vm.commit_authority(std::move(prepared)))throw std::runtime_error("authority commit");
                observed.str(prefix);observed.clear();observed.seekp(0,std::ios::end);
                for(tick=2;tick<ticks;++tick)vm.tick(tick);
                if(observed.str()!=first)throw std::runtime_error(name+": imported authority diverged");
                auto bad=*authority;if(!bad.threads.empty()){
                    bad.threads[0].external_sleep=ScriptVm::AuthorityWakeState{1,true};bool rejected=false;
                    try{vm.prepare_authority(bad);}catch(const FormatError &){rejected=true;}
                    if(!rejected)throw std::runtime_error("invalid saved dormant wake accepted");
                }
            }
#endif
        }
        if(!cases.eof() || case_count!=23)throw std::runtime_error("invalid or incomplete case list");
        return failures?1:0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 2;}
}
