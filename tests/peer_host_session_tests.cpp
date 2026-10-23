#include "halo2/online_match_session.hpp"
#include "halo2/frontend.hpp"
#include "voice_queue_fixture.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

using namespace halo2;
using namespace std::chrono_literals;
using Json=nlohmann::json;
namespace {
std::string stage="setup";
struct CommitProbe {unsigned calls{};bool accept{};};
bool commit_probe(void *opaque)noexcept{auto &probe=*static_cast<CommitProbe *>(opaque);++probe.calls;return probe.accept;}
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void wait(F ready,const char *stage){const auto end=std::chrono::steady_clock::now()+25s;
    while(!ready()){check(std::chrono::steady_clock::now()<end,stage);std::this_thread::sleep_for(2ms);}}
std::vector<std::byte> game(unsigned self,bool host){return {std::byte{'H'},std::byte{'2'},std::byte{'N'},std::byte{'2'},
    std::byte(native_multiplayer_protocol),std::byte(native_multiplayer_protocol>>8),std::byte(host?3:1),std::byte(self)};}
}
int main(int argc,char **argv)try{
    check(argc==3,"Supply ORIGIN PRIVATE_DESCRIPTORS_FILE");Json descriptors;{std::ifstream in(argv[2]);in>>descriptors;}
    std::vector<OnlineAdmission> roster;for(const auto &row:descriptors.at(0).at("players")){
        OnlineAdmission p;p.slot=roster.size();p.name=row.at("name");p.team=row.at("team");p.peer=row.at("peer");p.guest=row.at("guest");
        const std::string identity=row.at("identity");check(identity.size()==16,"Invalid fixture identity");
        for(unsigned i=0;i<8;++i)p.identity[i]=std::byte(std::stoul(identity.substr(i*2,2),nullptr,16));roster.push_back(p);}
    std::array<std::unique_ptr<OnlineMatchSession>,16> sessions;std::array<unsigned,16> games{},voices{},sequences{};
    const bool decoded_audio=std::getenv("HALO2_PEER_AUDIO_HANDOFF");
    const bool automatic=std::getenv("HALO2_PEER_AUTO_SELECT");
    const bool abort_selected=std::getenv("HALO2_PEER_ABORT_HANDOFF");
    check(!automatic||!decoded_audio,"Automatic preference and in-flight device gates use separate fixtures");
    check(!abort_selected||(!automatic&&!decoded_audio),"Aborted-cut refusal uses its own fixture");
    std::array<std::shared_ptr<GameVoice>,16> audio;
    std::array<std::shared_ptr<test::VoiceQueueState>,16> devices;
    std::array<std::optional<GameVoiceFrame>,16> old_microphone,old_received;
    for(const auto &d:descriptors){const unsigned self=d.at("slot");OnlineMatchTransportOptions options;options.direct_udp=true;options.peer_host_migration=true;
        sessions[self]=std::make_unique<OnlineMatchSession>(argv[1],d.at("id"),d.at("token"),self,roster,true,false,false,options);
        if(decoded_audio){devices[self]=std::make_shared<test::VoiceQueueState>();devices[self]->capture(true);
            audio[self]=std::make_shared<GameVoice>(std::make_unique<test::QueuedVoiceDevices>(devices[self]));
            audio[self]->enable(true);audio[self]->mute(15,true);audio[self]->blocks(1u<<14);sessions[self]->retain_voice(audio[self]);}}
    const auto healthy=[&]{for(unsigned p=0;p<16;++p)if(auto &s=sessions[p]){
        check(s->phase()!=OnlineMatchSession::Phase::Failed,s->error().c_str());
        if(audio[p])check(audio[p]->snapshot().error.empty(),audio[p]->snapshot().error.c_str());}};
    stage="initial ICE/session loading";
    wait([&]{healthy();for(auto &s:sessions)if(s&&s->phase()!=OnlineMatchSession::Phase::Loading)return false;return true;},"Direct session loading timeout");
    for(auto &s:sessions)if(s){check(s->local_slots().size()==4&&s->roster().size()==16,"Four-seat physical admission changed");s->loaded(0x12345678,UiVariant{"Slayer",0,{}});}
    wait([&]{healthy();for(auto &s:sessions)if(s&&s->phase()!=OnlineMatchSession::Phase::Running)return false;return true;},"Direct session running timeout");
    unsigned host=sessions[0]->host();for(auto &s:sessions)if(s)check(s->host()==host,"Initial measured host disagrees");
    std::vector<SourcePlayerAdmission> admitted;
    for(unsigned slot=0;slot<16;++slot){SourcePlayerAdmission p{slot,std::uint32_t((0x100u+slot)<<16)|slot};p.identity.front()=slot+1;admitted.push_back(p);}
    const auto first=*sessions[host]->authority_context();
    check(sessions[host]->publish_source_players(first,admitted),"Initial source allocation fixture refused");
    const auto source_ready=[&]{healthy();for(auto &s:sessions)if(s){const auto value=s->source_players();if(!value||value->players!=admitted)return false;}return true;};
    stage="initial source admission";wait(source_ready,"Initial direct source admission timeout");
    const auto drain=[&]{for(unsigned self=0;self<16;++self)if(auto &s=sessions[self]){
        std::vector<std::byte> bytes;unsigned from{};while(s->receive(bytes,from)){
            check(bytes.size()==8&&from==std::to_integer<unsigned>(bytes[7])&&bytes[6]==std::byte(self==host?1:3),"Direct gameplay changed source or role");++games[self];}
        for(auto &packet:s->voice_packets()){check(!packet.preferences&&packet.peer==(self==host?packet.frame.source:host),"Direct voice changed authenticated route");
            check(packet.frame.sequence!=0xff000101u,"Delayed malformed voice reached ordinary delivery");
            if(packet.frame.sequence==0xff000110u || packet.frame.sequence==0xff000111u)continue;
            ++voices[self];
            old_received[self]=packet.frame;if(audio[self])audio[self]->receive(std::move(packet.frame));}}};
    const auto exercise=[&](auto duration){const auto until=std::chrono::steady_clock::now()+duration;
        while(std::chrono::steady_clock::now()<until){healthy();for(unsigned self=0;self<16;++self)if(auto &s=sessions[self]){
            check(s->host()==host&&s->phase()==OnlineMatchSession::Phase::Running,"Committed peers lost their running generation");
            std::vector<GameVoiceFrame> captured;
            if(audio[self])captured=audio[self]->outgoing();
            else captured.push_back({self,++sequences[self],false,{1,1},{0x60}});
            for(auto &frame:captured){frame.source=self;if(audio[self])old_microphone[self]=frame;}
            if(self==host){for(unsigned to=0;to<16;++to)if(to!=self&&sessions[to]){s->send(game(self,true),to);for(const auto &frame:captured)s->voice(to,frame);}}
            else{s->send(game(self,false),host);for(const auto &frame:captured)s->voice(host,frame);}}
            drain();std::this_thread::sleep_for(10ms);}drain();};
    stage="initial gameplay/voice";exercise(decoded_audio?1200ms:800ms);
    // Real authenticated malicious-peer controls: do not infer rejection from
    // an empty queue on an unreliable channel. Require the actual dispatch
    // rejection counter AND delivery of a valid voice control for each case.
    stage="authenticated voice rejection";
    unsigned attacker=0;while(attacker==host || !sessions[attacker])++attacker;
    if(decoded_audio)check(old_microphone[attacker].has_value(),"Real attacker capture baseline was not observed");
    check(old_received[attacker]&&old_received[attacker]->source==host&&old_received[attacker]->origin_signature,
        "Wrong-seat replay lacks the host's independently signed delivered voice frame");
    auto baseline=decoded_audio?*old_microphone[attacker]:GameVoiceFrame{attacker,0,false,{1,1},{0x60}};
    for(unsigned attack=0;attack<2;++attack){
        auto poison=baseline,control=baseline;poison.sequence=0xff000100u+attack;control.sequence=0xff000110u+attack;
        // voice() correctly refuses to sign a foreign speaker locally. Replay
        // the actual host-signed body through this different authenticated PC
        // so origin verification succeeds and the receiver's seat check runs.
        if(attack==0)poison=*old_received[attacker];
        else poison.gains.left=std::numeric_limits<float>::infinity();
        const auto before=sessions[host]->rejected_direct_dispatches();bool valid_received=false;
        auto heartbeat=std::chrono::steady_clock::now();
        wait([&]{
            healthy();
            if(std::chrono::steady_clock::now()>=heartbeat){
                for(unsigned p=0;p<16;++p)if(p!=host&&sessions[p])sessions[host]->send(game(host,true),p);
                heartbeat=std::chrono::steady_clock::now()+20ms;
            }
            sessions[attacker]->voice(host,poison);sessions[attacker]->voice(host,control);
            for(auto &packet:sessions[host]->voice_packets()){
                check(packet.frame.source!=poison.source||packet.frame.sequence!=poison.sequence,
                    "Authenticated wrong-seat/malformed voice reached its consumer");
                if(packet.frame.sequence==control.sequence){
                    check(!packet.preferences&&packet.peer==attacker&&packet.frame.source==attacker&&packet.frame.origin_signature,
                        "Valid voice control lost its actual authenticated sender");valid_received=true;
                }
            }
            return valid_received&&sessions[host]->rejected_direct_dispatches()>before;
        },"Actual malicious voice rejection or healthy control was not observed");
    }
    // Retire only the synthetic high-sequence controls before ordinary audio
    // resumes; they must not poison a real decoder's sequencing baseline.
    std::this_thread::sleep_for(40ms);sessions[host]->voice_packets();
    Json history=Json::array();
    for(unsigned round=0;round<2;++round){stage="prepare handoff "+std::to_string(round+1);const auto before=*sessions[host]->authority_context();unsigned candidate=16;
        for(unsigned p=0;p<16;++p)if(sessions[p]&&p!=host){candidate=p;break;}
        // Declared native-owner completion is a fixture. The production caller
        // must actually retain/import the paired world and change native roles.
        std::array<std::optional<ValidatedAuthorityCut>,16> ready_cuts;
        for(unsigned p=0;p<16;++p)if(sessions[p]&&p!=host){
            ready_cuts[p]=ValidatedAuthorityCut{p,before,{7+round*16,100+round,8192,{std::byte(0x51+round)}}};
            if(!automatic&&p==candidate)check(sessions[p]->prepare_peer_migration(*ready_cuts[p]),"Retained paired-cut fixture refused");
        }
        if(!automatic)wait([&]{healthy();for(auto &s:sessions)if(s){const auto state=s->peer_migration();
            if(!state||!state->ready[candidate])return false;}return true;},"Candidate ready cut did not reach admitted peers");
        std::optional<PeerCommonCut> aborted;
        if(round==0){
            stage="predecessor common-cut Abort";
            // The selected-cut refusal and unrelated-cut positive control use
            // actual authenticated common controls on all four workers. Leave
            // ACKs pending until after the peer vote fences this generation.
            const auto seal=abort_selected?ready_cuts[candidate]->seal:
                AuthorityCutSeal{901,901,8192,{std::byte{0xa1}}};
            aborted=PeerCommonCut{before,seal,std::uint16_t(before.peers&~(1u<<host))};
            wait([&]{healthy();for(auto &s:sessions)if(s&&!s->common_cut())return false;return true;},"Common-cut feature negotiation timeout");
            check(sessions[host]->offer_common_cut(*aborted),"Actual predecessor refused its common-cut offer");
            wait([&]{healthy();for(auto &s:sessions)if(s){const auto state=s->common_cut();
                if(!state||state->pending!=aborted)return false;}return true;},"Common-cut offer did not reach admitted peers");
            check(sessions[host]->abort_common_cut(*aborted),"Actual predecessor refused pre-Commit Abort");
            wait([&]{healthy();for(auto &s:sessions)if(s){const auto state=s->common_cut();
                if(!state||state->pending!=aborted||!state->abort)return false;}return true;},"Authenticated common-cut Abort did not arrive");
        }
        wait([&]{
            for(unsigned p=0;p<16;++p)if(sessions[p]&&p!=host)sessions[host]->send(game(host,true),p);
            drain();
            for(auto &s:sessions)if(s&&s->peer_migration()->suspected)return false;
            for(auto &s:sessions)if(s)check(!s->migration_candidate()&&!s->propose_peer_migration(),"A healthy host was replaced by automatic preference");
            return true;
        },"Authenticated healthy-host traffic was not observed");
        if(decoded_audio){check(old_microphone[candidate]&&old_received[candidate],"Initial real microphone/decoder did not run");
            devices[candidate]->block_playback();exercise(200ms);
            wait([&]{return devices[candidate]->snapshot().playback_entered;},"Candidate playback did not enter the in-flight gate");}
        stage="host socket shutdown "+std::to_string(round+1);
        sessions[host].reset(); // Actual native session, signaling and DTLS/socket destruction.
        wait([&]{healthy();return sessions[candidate]->peer_migration()->suspected;},"Host silence was not observed");
        const auto proposer=candidate;
        if(automatic){
            // Keep survivors measuring one another after every predecessor row
            // has expired. Full old membership must now be ineligible.
            std::this_thread::sleep_for(5100ms);
            check(sessions[proposer]->host_candidates().empty(),"Departed predecessor retained full-membership eligibility");
            for(auto &s:sessions)if(s)check(!s->peer_migration()->decision,"Automatic election invented native readiness");
            // Readiness is withheld until the predecessor's measurements have
            // expired. Session workers must now initiate and retry themselves.
            for(unsigned p=0;p<16;++p)if(ready_cuts[p])
                check(sessions[p]->prepare_peer_migration(*ready_cuts[p]),"Late retained paired-cut fixture refused");
        }
        stage="peer election "+std::to_string(round+1);
        wait([&]{healthy();for(auto &s:sessions)if(s){const auto state=s->peer_migration();if(!state||!state->decision)return false;}return true;},"Direct session majority timeout");
        candidate=sessions[proposer]->peer_migration()->decision->value.host;
        check(candidate<16&&candidate!=host&&ready_cuts[candidate],"Automatic election selected an unready or departed PC");
        const auto retained=*ready_cuts[candidate];
        std::array<std::optional<PeerMigrationDecision>,16> decisions;
        for(unsigned p=0;p<16;++p)if(auto &s=sessions[p]){
            decisions[p]=s->peer_migration()->decision;const auto &d=*decisions[p];
            check(d.previous==before&&d.value.host==candidate&&d.value.cut==retained.seal&&d.next.epoch==before.epoch+1&&
                d.next.peers==std::uint16_t(before.peers&~(1u<<host)),"Session decided another cut, generation or physical membership");
            check(s->phase()==(p==candidate?OnlineMatchSession::Phase::Migrating:OnlineMatchSession::Phase::Rebinding),"Media reopened before native role completion");
            auto forged=d;++forged.next.epoch;CommitProbe untrusted;
            check(!s->finish_peer_migration(forged,commit_probe,&untrusted)&&!untrusted.calls,"Caller-invented certificate reached a native commit");
            std::vector<std::byte> bytes;unsigned from{};check(!s->receive(bytes,from)&&s->voice_packets().empty(),"Queued predecessor gameplay/voice survived its fence");}
        stage="native completion barrier "+std::to_string(round+1);const auto successor=*decisions[candidate];CommitProbe native;
        if(!automatic){
            for(unsigned p=0;p<16;++p)if(auto &s=sessions[p]){CommitProbe blocked;blocked.accept=true;
                const auto state=s->peer_migration();
                check(state.has_value(),"Candidate-only control lost its local agreement");
                bool missing{};for(unsigned q=0;q<16;++q)if(sessions[q]&&!state->ready[q])missing=true;
                check(state->ready[candidate]&&*state->ready[candidate]==decisions[p]->value.cut&&missing,
                    "Candidate-only control lacks an exact candidate cut and an unprepared physical survivor");
                check(!s->finish_peer_migration(*decisions[p],commit_probe,&blocked)&&!blocked.calls&&s->authority_context()==before,
                    "Candidate-only readiness reached native publication before every physical survivor prepared");}
            for(unsigned p=0;p<16;++p)if(sessions[p]&&p!=candidate){
                check(ready_cuts[p]&&ready_cuts[p]->seal==successor.value.cut&&
                    sessions[p]->prepare_peer_migration(*ready_cuts[p]),"Survivor exact-cut preparation fixture refused");
            }
        }
        wait([&]{healthy();for(auto &s:sessions)if(s){const auto state=s->peer_migration();
            if(!state)return false;
            for(unsigned p=0;p<16;++p)if(sessions[p]&&
                (!state->ready[p]||*state->ready[p]!=successor.value.cut))return false;
        }return true;},"Every fixed physical survivor's exact-cut preparation did not arrive");
        if(aborted){
            for(unsigned p=0;p<16;++p)if(auto &s=sessions[p]){
                if(abort_selected){CommitProbe blocked;blocked.accept=true;
                    check(!s->finish_peer_migration(*decisions[p],commit_probe,&blocked)&&!blocked.calls&&s->authority_context()==before,
                        "Aborted selected cut reached native publication or changed generation");}
                check(s->acknowledge_common_cut_abort(*aborted),"Fenced recipient could not acknowledge retired Abort");
            }
            if(abort_selected){
                Json result{{"handoffs",Json::array()},{"aborted_selected_cut_blocked",true},{"fenced_abort_acknowledgement",true},
                    {"native_cut_and_role_completion","declared fixture"},{"master_gameplay_relay",false},
                    {"native_commit_callback_guard",true},{"authenticated_voice_rejection",true}};
                {std::ofstream out(std::filesystem::path(argv[2]).parent_path()/"result.json");out<<result.dump(2);}
                std::cout<<"PASS authenticated selected-cut Abort survives host loss: exact learned certificates cannot invoke native commit or publish an epoch, and fenced recipients can acknowledge retirement\n";
                std::cout<<"LIMIT declared native preparation, local DTLS/account fixtures; no rendered/native-owner or WAN acceptance\n";
                return 0;
            }
        }
        if(decoded_audio){
            check(!sessions[candidate]->finish_peer_migration(successor,commit_probe,&native)&&!native.calls,"Native commit ran while old device playback was in flight");
            check(sessions[candidate]->phase()==OnlineMatchSession::Phase::Migrating,"Pending device retirement lost the native barrier");
            devices[candidate]->unblock_playback();
            wait([&]{(void)sessions[candidate]->finish_peer_migration(successor,commit_probe,&native);return native.calls==1;},"Candidate audio retirement did not reach native commit");
            check(!devices[candidate]->snapshot().queued_audible&&!audio[candidate]->snapshot().talking[old_received[candidate]->source],
                "New native generation retained old PCM or talker state");
        }else check(!sessions[candidate]->finish_peer_migration(successor,commit_probe,&native)&&native.calls==1,"Failed native commit was ignored");
        check(sessions[candidate]->phase()==OnlineMatchSession::Phase::Migrating&&sessions[candidate]->authority_context()==before,
            "Failed native commit published a transport generation");
        native.accept=true;
        check(sessions[candidate]->finish_peer_migration(successor,commit_probe,&native)&&native.calls==2,"Exact native commit did not precede publication");
        check(!sessions[candidate]->finish_peer_migration(successor,commit_probe,&native)&&native.calls==2,"Native commit ran twice for an old certificate");
        for(unsigned p=0;p<16;++p)if(sessions[p]&&p!=candidate){sessions[candidate]->send(game(candidate,true),p);
            if(!decoded_audio)sessions[candidate]->voice(p,GameVoiceFrame{candidate,++sequences[candidate],false,{1,1},{0x60}});}
        std::this_thread::sleep_for(100ms);
        for(unsigned p=0;p<16;++p)if(auto &s=sessions[p])if(p!=candidate){
            std::vector<std::byte> bytes;unsigned from{};check(!s->receive(bytes,from)&&s->voice_packets().empty(),"Successor bytes published before recipient's native rebind");
            check(s->finish_peer_migration(*decisions[p]),"Survivor exact learned decision could not commit");}
        host=candidate;unsigned member_count{};for(auto &s:sessions)if(s)member_count+=4;
        wait(source_ready,"Next-generation source identity continuity timeout");
        for(auto &s:sessions)if(s){const auto continuity=s->continuity();check(s->authority_context()==successor.next&&continuity&&continuity->host==host&&
            unsigned(std::popcount(continuity->members))==member_count&&s->local_slots().size()==4,"Epoch or four-seat roster did not change atomically");
            check(!s->finish_peer_migration(successor),"An old certificate committed twice");}
        auto wrong=admitted;wrong[0].identity.front()^=0x80;check(!sessions[host]->publish_source_players(successor.next,wrong),"Promoted host replaced an original native/PCR identity");
        if(decoded_audio){
            std::array<unsigned,16> audible{};
            for(unsigned p=0;p<16;++p)if(sessions[p]){audible[p]=devices[p]->snapshot().audible;
                check(old_microphone[p]&&old_received[p],"Missing predecessor batches for late-delivery check");
                audio[p]->receive(*old_received[p]);auto stale=*old_microphone[p];stale.sequence=0x7ffffff0u+round;
                unsigned target=host;if(p==host)for(unsigned q=0;q<16;++q)if(q!=p&&sessions[q]){target=q;break;}
                sessions[p]->voice(target,stale);}
            std::this_thread::sleep_for(100ms);
            for(unsigned p=0;p<16;++p)if(sessions[p]){
                check(sessions[p]->voice_packets().empty(),"Old held microphone batch was relabeled into the new peer epoch");
                check(devices[p]->snapshot().audible==audible[p],"Old held received frame reached a new decoder generation");
                const auto selected=audio[p]->snapshot();check(selected.enabled&&selected.muted==(1u<<15)&&selected.blocked==(1u<<14),"Native handoff lost voice preferences");}
        }
        const auto prior_games=games,prior_voices=voices;std::array<unsigned,16> prior_audio{};
        if(decoded_audio)for(unsigned p=0;p<16;++p)if(sessions[p])prior_audio[p]=devices[p]->snapshot().audible;
        stage="resumed gameplay/voice "+std::to_string(round+1);exercise(decoded_audio?1200ms:800ms);
        for(unsigned p=0;p<16;++p)if(sessions[p]){
            check(games[p]>prior_games[p]+10&&voices[p]>prior_voices[p]+3,"A completed handoff did not resume fresh gameplay and voice packets");
            if(decoded_audio)check(devices[p]->snapshot().audible>prior_audio[p]+3,"A completed handoff did not resume fresh decoded stereo");}
        history.push_back({{"host",host},{"epoch",successor.next.epoch},{"physical_members",successor.next.peers},{"seats",member_count}});
    }
    for(unsigned p=0;p<16;++p)if(sessions[p]){
        check(games[p]>100&&voices[p]>(decoded_audio?50u:100u),"Player payloads or encoded voice packets did not survive both handoffs");
        if(decoded_audio){const auto d=devices[p]->snapshot();check(d.starts==1&&d.stops==0&&d.discards>=3&&d.audible>10,"Decoded voice did not survive both host losses without reopening devices");}}
    if(automatic){
        stage="sole physical survivor";const auto before=*sessions[host]->authority_context();unsigned survivor=16;
        for(unsigned p=0;p<16;++p)if(sessions[p]&&p!=host)survivor=p;
        check(survivor<16,"Missing final survivor");
        const ValidatedAuthorityCut cut{survivor,before,{73,123,8192,{std::byte{0x79}}}};
        check(sessions[survivor]->prepare_peer_migration(cut),"Sole survivor's retained cut refused");
        sessions[host].reset();
        wait([&]{healthy();return bool(sessions[survivor]->peer_migration()->decision);},"Sole survivor did not automatically elect without invented speed measurements");
        const auto state=sessions[survivor]->peer_migration();
        check(state->decision&&state->decision->voters==(1u<<survivor)&&state->decision->value.cut==cut.seal,
            "Sole survivor changed the fixed physical quorum or cut");
        check(sessions[survivor]->finish_peer_migration(*state->decision)&&sessions[survivor]->phase()==OnlineMatchSession::Phase::Running,
            "Sole survivor could not finish the declared native role");
        host=survivor;const auto next=*sessions[host]->authority_context();
        history.push_back({{"host",host},{"epoch",next.epoch},{"physical_members",next.peers},{"seats",4}});
    }
    Json result{{"handoffs",history},{"native_cut_and_role_completion","declared fixture"},{"game_counts",games},{"voice_packet_counts",voices},
        {"master_gameplay_relay",false},{"audible_devices",false},{"decoded_synthetic_audio",decoded_audio},{"automatic_measured_host_choice",automatic},{"automatic_peer_proposals",true},{"native_commit_callback_guard",true},{"automatic_rendered_runtime",false},{"authenticated_voice_rejection",true}};
    {std::ofstream out(std::filesystem::path(argv[2]).parent_path()/"result.json");out<<result.dump(2);}
    std::cout<<"PASS four actual native session workers: two host socket/signaling losses, direct majority decisions, native-completion barriers, preserved 16 original allocation identities, 16/12/8 couch seats and resumed gameplay/encoded voice packets\n";
    if(decoded_audio)std::cout<<"PASS real Opus/PCM through queued synthetic devices: in-flight playback holds native completion, retired old audio queues, late microphone/received-batch rejection, preserved voice preferences, resumed decoded stereo and retained device ownership across both host losses\n";
    std::cout<<"PASS authenticated wrong-seat and non-finite voice rejection observed in dispatch, with healthy voice controls delivered\n";
    if(automatic)std::cout<<"PASS automatic ready-PC preference from reciprocal survivor measurements after predecessor telemetry expires across two host losses, then a sole physical survivor without invented speed/NAT metrics\n";
    std::cout<<"PASS session-worker election initiation and paced retries without any positive caller proposal\n";
    std::cout<<"PASS native commit admission/audio guards, refusal without epoch publication, one successful commit before publication and no repeated old-certificate commit\n";
    std::cout<<"LIMIT declared native cuts/role completion, fixture account verification, no rendered simulation, audible headset or WAN proof\n";
    return 0;
}catch(const std::exception &e){std::cerr<<stage<<": "<<e.what()<<'\n';return 1;}
