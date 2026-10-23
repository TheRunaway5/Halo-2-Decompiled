#include "halo2/online_match_session.hpp"
#include "halo2/frontend.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace halo2;
using namespace std::chrono_literals;
using Json=nlohmann::json;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void wait(F condition,const char *why){const auto end=std::chrono::steady_clock::now()+25s;
    while(!condition()){check(std::chrono::steady_clock::now()<end,why);std::this_thread::sleep_for(2ms);}}
}
int main(int argc,char **argv)try{
    check(argc==3,"Supply ORIGIN PRIVATE_DESCRIPTORS_FILE");
    const std::string mode=std::getenv("HALO2_PEER_SELECTION_CASE")?std::getenv("HALO2_PEER_SELECTION_CASE"):"normal";
    check(mode=="normal"||mode=="mixed"||mode=="abort"||mode=="preparation","Unknown selection fixture");
    Json descriptors;{std::ifstream in(argv[2]);in>>descriptors;}
    std::vector<OnlineAdmission> roster;
    for(const auto &row:descriptors.at(0).at("players")){
        OnlineAdmission p;p.slot=roster.size();p.name=row.at("name");p.team=row.at("team");p.peer=row.at("peer");p.guest=row.at("guest");
        const std::string identity=row.at("identity");check(identity.size()==16,"Invalid fixture identity");
        for(unsigned i=0;i<8;++i)p.identity[i]=std::byte(std::stoul(identity.substr(i*2,2),nullptr,16));roster.push_back(p);
    }
    std::array<std::unique_ptr<OnlineMatchSession>,16> sessions;
    for(const auto &d:descriptors){const unsigned self=d.at("slot");OnlineMatchTransportOptions options;
        options.direct_udp=true;options.peer_host_migration=true;options.survivor_cut_selection=mode!="mixed"||self!=4;
        sessions[self]=std::make_unique<OnlineMatchSession>(argv[1],d.at("id"),d.at("token"),self,roster,true,false,false,options);
    }
    const auto healthy=[&]{for(auto &s:sessions)if(s)check(s->phase()!=OnlineMatchSession::Phase::Failed,s->error().c_str());};
    wait([&]{healthy();for(auto &s:sessions)if(s&&s->phase()!=OnlineMatchSession::Phase::Loading)return false;return true;},"Selection loading timeout");
    for(auto &s:sessions)if(s)s->loaded(0x12345678,UiVariant{"Slayer",0,{}});
    wait([&]{healthy();for(auto &s:sessions)if(s&&s->phase()!=OnlineMatchSession::Phase::Running)return false;return true;},"Selection Running timeout");
    const unsigned host=sessions[0]->host();const auto context=*sessions[host]->authority_context();
    std::vector<unsigned> survivors;for(unsigned p=0;p<16;++p)if(sessions[p]&&p!=host)survivors.push_back(p);
    std::vector<SourcePlayerAdmission> admitted;
    for(unsigned slot=0;slot<16;++slot){SourcePlayerAdmission p{slot,std::uint32_t((0x100u+slot)<<16)|slot};p.identity.front()=slot+1;admitted.push_back(p);}
    check(sessions[host]->publish_source_players(context,admitted),"Source fixture refused");
    wait([&]{healthy();for(auto &s:sessions)if(s){const auto players=s->source_players();if(!players||players->players!=admitted||!s->common_cut())return false;}return true;},"All predecessor Hello/admission timeout");
    PeerCommonCut round{context,{1,123,4096,{std::byte{0x41}}},std::uint16_t(context.peers&~(1u<<host))};
    const auto finish=[&]{
        check(sessions[host]->offer_common_cut(round),"Common Offer refused");
        wait([&]{healthy();for(auto &s:sessions)if(s&&s->common_cut()->pending!=round)return false;return true;},"Common Offer timeout");
        for(auto p:survivors)check(sessions[p]->associate_common_cut(round),"Common local association refused");
        check(sessions[host]->confirm_common_cut_native(round),"Declared native validation fixture refused");
        wait([&]{healthy();for(auto &s:sessions)if(s){const auto c=*s->common_cut();if(!c.commit||c.associated!=round.recipients)return false;}return true;},"Genuine CommonCommit timeout");
        for(auto p:survivors)check(sessions[p]->apply_common_cut(round),"Common local apply refused");
        wait([&]{healthy();for(auto &s:sessions)if(s){const auto c=*s->common_cut();if(!c.release||c.applied!=round.recipients)return false;}return true;},"Genuine CommonRelease timeout");
    };
    finish();
    check(sessions[survivors.front()]->release_common_cut(round),"First explicit Common Release refused");
    if(mode!="normal"){
        for(auto p:survivors)check(sessions[p]->release_common_cut(round),"Common Release refused");
        wait([&]{healthy();for(auto &s:sessions)if(s&&s->common_cut()->committed!=round)return false;return true;},"All Common Released timeout");
    }else{
        check(sessions[survivors.front()]->common_cut()->committed==round&&
            !sessions[survivors.back()]->common_cut()->committed,"Partial Release fixture lost distinct committed labels");
    }
    auto pending=round;++pending.seal.id;pending.seal.digest[0]=std::byte{0x42};
    if(mode=="abort"){
        check(sessions[host]->offer_common_cut(pending),"Pending Abort Offer refused");
        wait([&]{healthy();for(auto &s:sessions)if(s&&s->common_cut()->pending!=pending)return false;return true;},"Pending Abort Offer timeout");
    }
    const auto inventory=[&](unsigned peer){PeerSurvivorInventory value;value.lease=100+peer;
        value.cuts[0]=PeerSurvivorInventoryEntry{round.seal,peer==survivors.front()};
        if(mode=="abort")value.cuts[1]=PeerSurvivorInventoryEntry{pending.seal,false};return value;};
    if(mode!="mixed")for(auto p:survivors){
        auto fabricated=inventory(p);fabricated.cuts[0]->cut.id=999;fabricated.cuts[0]->irreversible_common=true;
        check(!sessions[p]->freeze_survivor_selection(context,fabricated)&&
            !(sessions[p]->survivor_selection()->inventory_peers&(1u<<p))&&!sessions[p]->peer_migration()->ready[p],
            "Unobserved CommonCommit witness created a frozen inventory or Ready");
    }
    for(auto p:survivors)if(sessions[p]->survivor_selection())
        check(!sessions[p]->prepare_peer_migration({p,context,round.seal}),"Ready bypassed explicit frozen inventory and selection");
    unsigned pulse_sequence{};
    const auto pulse_host=[&](bool require_delivery=true){
        const auto marker=std::byte(++pulse_sequence);
        const std::vector<std::byte> packet{std::byte{'H'},std::byte{'2'},std::byte{'N'},std::byte{'2'},
            std::byte(native_multiplayer_protocol),std::byte(native_multiplayer_protocol>>8),std::byte{3},marker};
        std::uint16_t received{};
        wait([&]{healthy();for(auto p:survivors)sessions[host]->send(packet,p);
            for(auto p:survivors){std::vector<std::byte> frame;unsigned source;
                while(sessions[p]->receive(frame,source))if(source==host&&frame==packet)received|=std::uint16_t(1u<<p);}
            for(auto p:survivors)if(sessions[p]->peer_migration()->suspected)return false;
            return !require_delivery||received==round.recipients;
        },"Actual authenticated host traffic did not service suspicion");
    };
    const auto suspect_host=[&]{
        const auto started=std::chrono::steady_clock::now();
        wait([&]{healthy();if(std::chrono::steady_clock::now()-started<510ms)return false;
            for(auto p:survivors)if(!sessions[p]->peer_migration()->suspected)return false;return true;},
            "Genuine host gameplay silence did not reach HostAgreement suspicion");
    };
    if(mode=="preparation"){
        using Kind=OnlineMatchSession::MigrationPreparationKind;
        using Outcome=OnlineMatchSession::MigrationPreparationOutcome;
        const auto peer=survivors.front(),foreign=survivors.back();auto &session=*sessions[peer];
        pulse_host();
        const auto healthy_step=session.migration_preparation_request();
        check(healthy_step.outcome==Outcome::Inactive&&!healthy_step.request,"Healthy host created a native preparation request");
        suspect_host();
        const auto preparing=session.migration_preparation_request();const auto request=preparing.request;
        check(preparing.outcome==Outcome::Preparing&&request&&request->kind()==Kind::Prepare&&request->context()==context&&request->self()==peer&&request->id()!=0,
            "Atomic genuine suspicion did not create a bound preparation request");
        const auto repeated=session.migration_preparation_request().request;
        check(repeated&&repeated->id()==request->id()&&repeated->kind()==Kind::Prepare&&
            sessions[foreign]->complete_migration_preparation(*request,inventory(peer))==Outcome::Refused,
            "Preparation request was replaced or accepted by another actual Session");
        pulse_host();
        check(session.complete_migration_preparation(*request,inventory(peer))==Outcome::Retiring,
            "A stale suspected observation published after an actual healthy host packet");
        const auto retiring=session.migration_preparation_request();const auto retirement=retiring.request;
        check(retiring.outcome==Outcome::Retiring&&retirement&&retirement->kind()==Kind::Retire&&retirement->id()==request->id()&&
            !(session.survivor_selection()->inventory_peers&(1u<<peer))&&!session.peer_migration()->ready[peer]&&
            !session.peer_migration()->fenced&&session.phase()==OnlineMatchSession::Phase::Running&&
            !session.freeze_survivor_selection(context,inventory(peer)),
            "Unpublished retirement manufactured a row, Ready, fence or racing publication");
        suspect_host();
        const auto reserved=session.migration_preparation_request().request;
        check(reserved&&reserved->kind()==Kind::Retire&&reserved->id()==request->id(),
            "New silence replaced an outstanding real retirement reservation");
        // This metadata fixture created no native owners. Native/outer owner
        // refusal and actual destruction are covered by the paired MCC case.
        check(session.complete_migration_preparation(*retirement)==Outcome::Retryable,
            "Unpublished metadata retirement could not complete after the genuine reservation");
        const auto retry=session.migration_preparation_request().request;
        check(retry&&retry->kind()==Kind::Prepare&&retry->id()>request->id()&&
            session.complete_migration_preparation(*retirement)==Outcome::Refused&&
            session.complete_migration_preparation(*request,inventory(peer))==Outcome::Refused,
            "Stale completion reset or consumed the fresh actual preparation request");
        check(session.complete_migration_preparation(*retry,inventory(peer))==Outcome::Published,
            "Fresh genuine suspicion and current Common provenance did not publish the declared inventory");
        const auto published=*session.survivor_selection();
        check((published.inventory_peers&(1u<<peer))&&!published.selection_complete&&
            session.phase()==OnlineMatchSession::Phase::AwaitingWorld&&!session.peer_migration()->ready[peer],
            "A single published inventory manufactured the other survivor or native Ready");
        pulse_host(false);
        auto replacement=inventory(peer);++replacement.lease;
        const auto after_publication=session.migration_preparation_request();
        check(after_publication.outcome==Outcome::Published&&!after_publication.request&&!session.freeze_survivor_selection(context,replacement)&&
            session.survivor_selection()->inventory_peers==published.inventory_peers&&
            session.phase()==OnlineMatchSession::Phase::AwaitingWorld&&!session.peer_migration()->ready[peer]&&
            !session.peer_migration()->fenced,
            "Healthy return reset an already installed local inventory or resumed the generation");
        Json result{{"survivor_selection_case",mode},{"metadata_and_declared_native_fixture",true},
            {"master_gameplay_relay",false},{"native_or_rendered_migration",false}};
        {std::ofstream out(std::filesystem::path(argv[2]).parent_path()/"result.json");out<<result.dump(2);}
        std::cout<<"PASS authenticated preparation scheduling: actual host silence, stale healthy completion refusal, opaque retirement reservation, fresh retry and immutable local inventory; no Ready\n";
        std::cout<<"LIMIT local DTLS metadata and declared inventory only; no native owner retirement, renderer/input/audio preparation or WAN acceptance\n";
        return 0;
    }
    if(mode=="mixed"){
        for(auto p:survivors)if(sessions[p]->survivor_selection()){
            check(!sessions[p]->freeze_survivor_selection(context,inventory(p)),"Mixed predecessor capabilities silently downgraded selection");
            check(!sessions[p]->prepare_peer_migration({p,context,round.seal}),"Mixed capability group admitted legacy Ready");
            check(!sessions[p]->peer_migration()->ready[p],"Mixed capability refusal published Ready");
        }
    }else{
        const unsigned last=survivors.back();
        pulse_host();suspect_host();
        for(auto p:survivors)if(p!=last)check(sessions[p]->freeze_survivor_selection(context,inventory(p)),"Explicit native inventory fixture refused");
        wait([&]{healthy();const auto s=sessions[last]->survivor_selection();return s&&s->inventory_peers;},"Authenticated inventory did not reach unfrozen survivor");
        check(!(sessions[last]->survivor_selection()->inventory_peers&(1u<<last))&&
            !sessions[last]->survivor_selection()->selection_complete&&sessions[last]->phase()==OnlineMatchSession::Phase::Running,
            "Incoming inventory manufactured a local freeze, own inventory or unanimous selection");
        check(sessions[last]->freeze_survivor_selection(context,inventory(last)),"Last explicit inventory refused");
        wait([&]{healthy();for(auto p:survivors){const auto s=sessions[p]->survivor_selection();if(!s||!s->selection_complete)return false;}return true;},"Unanimous exact transcript timeout");
        for(auto p:survivors){const auto s=*sessions[p]->survivor_selection();
            check(s.selected==round.seal&&s.accepted==round.recipients&&s.inventory_peers==round.recipients&&!s.failed,
                "Selection confused partial Release labels or shrank physical membership");
            check(sessions[p]->phase()==OnlineMatchSession::Phase::AwaitingWorld&&!sessions[p]->peer_migration()->ready[p],
                "Selection metadata manufactured native Ready or resumed gameplay");
        }
        if(mode=="abort"){
            check(sessions[host]->abort_common_cut(pending),"Real delayed predecessor Abort refused");
            wait([&]{healthy();for(auto p:survivors)if(!sessions[p]->survivor_selection()->failed)return false;return true;},"Delayed genuine Common Abort failed to invalidate advertised frozen inventory");
        }else{
            auto wrong=round.seal;++wrong.id;
            for(auto p:survivors){check(!sessions[p]->prepare_peer_migration({p,context,wrong}),"Nonselected cut reached Ready");
                check(sessions[p]->prepare_peer_migration({p,context,round.seal}),"Exact selected declared preparation could not publish Ready");}
            wait([&]{healthy();for(auto p:survivors)if(sessions[p]->survivor_selection()->ready_observed!=round.recipients)return false;return true;},"Actual authenticated Ready observation timeout");
            check(sessions[survivors.front()]->invalidate_survivor_selection(context),"Native failure could not terminally invalidate selection");
            wait([&]{healthy();for(auto p:survivors)if(!sessions[p]->survivor_selection()->failed)return false;return true;},"Terminal invalidation did not reach all survivors");
        }
        for(auto p:survivors){check(!sessions[p]->freeze_survivor_selection(context,inventory(p))&&
            !sessions[p]->prepare_peer_migration({p,context,round.seal})&&sessions[p]->authority_context()==context&&
            sessions[p]->phase()==OnlineMatchSession::Phase::AwaitingWorld,
            "Failed selection restarted, replaced Ready, resumed gameplay or changed generation");}
    }
    Json result{{"survivor_selection_case",mode},{"metadata_and_declared_native_fixture",true},
        {"master_gameplay_relay",false},{"native_or_rendered_migration",false}};
    {std::ofstream out(std::filesystem::path(argv[2]).parent_path()/"result.json");out<<result.dump(2);}
    std::cout<<"PASS authenticated survivor-selection "<<mode<<": explicit frozen inventory, fixed all-peer transcript, exact Ready gate and terminal failure\n";
    std::cout<<"LIMIT metadata/declared native fixture and local DTLS; no actual native freeze, renderer/input/audio preparation or WAN acceptance\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
