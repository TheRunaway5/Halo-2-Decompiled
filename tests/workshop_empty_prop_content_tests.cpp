#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/prop_sandbox.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
}
int main(int argc,char** argv)try{
    if(argc!=7)throw FormatError("Supply MAP SHARED_MAPS PREPARED_ROOT PROP_COUNT EMPTY_COUNT REPORT");
    const Cache cache(argv[1],argv[2],{},argv[3]);
    const auto content=cache.load_content(0,true);
    unsigned props{},empty{},nonempty{};
    for(unsigned n=0;n<content.objects.size();++n){
        const auto& object=content.objects[n];if(object.type!=11)continue;++props;
        need(n<content.prop_sandbox.size(),"Authored prop lost its prepass placement ordinal");
        if(object.definition==null_datum){
            ++empty;
            need(!content.prop_sandbox[n]&&!load_prop_sandbox_definition(cache,object),
                "Authored empty prop acquired a sandbox definition");
        }else{
            ++nonempty;
            need(cache.find_tag(TagHandle{object.definition})!=nullptr,"Nonempty prop lost its definition");
        }
    }
    need(props==std::stoul(argv[4])&&empty==std::stoul(argv[5]),"Content reader dropped or changed an authored prop placement");
    // Only the actual NONE sentinel is empty. A nonempty reference with the
    // wrong class must still reach the ordinary definition owner's rejection.
    ScenarioObject invalid;invalid.type=11;invalid.definition=cache.scenario_tag().value;
    bool rejected=false;
    try{(void)load_prop_sandbox_definition(cache,invalid);}catch(const FormatError&){rejected=true;}
    need(rejected,"Empty prop handling swallowed a nonempty wrong-class definition");
    nlohmann::json report{{"passed",true},{"props",props},{"empty",empty},{"nonempty",nonempty},
        {"objects",content.objects.size()},{"placement_alignment_preserved",true},
        {"nonempty_wrong_class_rejected",true},{"scope","Actual prepared Workshop content and prop prepass only; no gameplay or render acceptance"}};
    std::ofstream(argv[6])<<report.dump(2)<<'\n';
    std::cout<<"PASS actual Workshop empty-prop content: props="<<props<<" empty="<<empty
        <<" nonempty="<<nonempty<<"; all placement slots retained and wrong-class references rejected\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
