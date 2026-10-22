#include <juice/juice.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

namespace {
using Json=nlohmann::json;
std::mutex log_mutex;
Json logs={{"levels",Json::object()},{"events",Json::object()}};
void log_message(juice_log_level_t level,const char* raw) {
    const std::string message(raw);
    std::lock_guard lock(log_mutex);
    const auto severity=std::to_string(int(level));
    logs["levels"][severity]=logs["levels"].value(severity,0)+1;
    // Retain only recognized categories. Never retain raw descriptions,
    // ICE usernames/passwords or arbitrary log messages.
    for(const char* event:{"Sending STUN Binding","Received STUN datagram","Received STUN Binding request",
        "Received STUN Binding success response","Received datagram","Sending datagram",
        "STUN integrity check failed","STUN local ufrag check failed","STUN remote ufrag check failed",
        "Missing remote ICE credentials","STUN message verification failed","STUN message send failed",
        "Network unreachable","poll failed","Agent receive failed"})
        if(message.find(event)!=std::string::npos)logs["events"][event]=logs["events"].value(event,0)+1;
}
struct Peer {
    std::atomic<juice_state_t> state{JUICE_STATE_DISCONNECTED};
    std::atomic<unsigned> received{},mismatch{};
    std::mutex mutex;
    std::vector<std::string> candidates;
};
void changed(juice_agent_t*,juice_state_t state,void* owner){static_cast<Peer*>(owner)->state=state;}
void candidate(juice_agent_t*,const char* text,void* owner){
    auto& peer=*static_cast<Peer*>(owner);std::lock_guard lock(peer.mutex);peer.candidates.emplace_back(text);
}
void received(juice_agent_t*,const char* data,std::size_t size,void* owner){
    auto& peer=*static_cast<Peer*>(owner);
    if(std::string_view(data,size)=="halo2-ice-control")++peer.received;else ++peer.mismatch;
}
bool connected(const Peer& peer){const auto state=peer.state.load();return state==JUICE_STATE_CONNECTED||state==JUICE_STATE_COMPLETED;}
void require(bool ok){if(!ok)throw std::runtime_error("Pinned ICE control API rejected an operation");}
}
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Supply the owned small JSON report path\n";return 1;}
    Json report={{"schema","H2_MACOS_PINNED_ICE_CHECK_V1"},{"raw_signaling_retained",false},{"concurrency","poll"}};
    report["binding"]="automatic dual-stack";
    juice_set_log_handler(log_message);juice_set_log_level(JUICE_LOG_LEVEL_VERBOSE);
    bool passed=false;
    try {
        Peer peers[2];
        using Agent=std::unique_ptr<juice_agent_t,decltype(&juice_destroy)>;
        Agent agents[2]={{nullptr,juice_destroy},{nullptr,juice_destroy}};
        for(unsigned i=0;i<2;++i){
            juice_config_t config{};config.concurrency_mode=JUICE_CONCURRENCY_MODE_POLL;
            config.cb_state_changed=changed;config.cb_candidate=candidate;config.cb_recv=received;config.user_ptr=&peers[i];
            agents[i].reset(juice_create(&config));require(bool(agents[i]));
        }
        char description[JUICE_MAX_SDP_STRING_LEN];
        require(juice_get_local_description(agents[0].get(),description,sizeof(description))==JUICE_ERR_SUCCESS);
        require(juice_set_remote_description(agents[1].get(),description)==JUICE_ERR_SUCCESS);
        require(juice_get_local_description(agents[1].get(),description,sizeof(description))==JUICE_ERR_SUCCESS);
        require(juice_set_remote_description(agents[0].get(),description)==JUICE_ERR_SUCCESS);
        require(juice_gather_candidates(agents[0].get())==JUICE_ERR_SUCCESS);
        require(juice_gather_candidates(agents[1].get())==JUICE_ERR_SUCCESS);
        unsigned candidates{},ipv4{},ipv6{};
        bool sent=false;
        const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(6);
        while(std::chrono::steady_clock::now()<end){
            for(unsigned i=0;i<2;++i){
                std::vector<std::string> pending;
                {std::lock_guard lock(peers[i].mutex);pending.swap(peers[i].candidates);}
                for(const auto& text:pending){
                    std::istringstream fields(text);std::string foundation,component,protocol,priority,address;
                    fields>>foundation>>component>>protocol>>priority>>address;
                    ++candidates;ipv4+=address.find('.')!=std::string::npos;ipv6+=address.find(':')!=std::string::npos;
                    require(juice_add_remote_candidate(agents[1-i].get(),text.c_str())==JUICE_ERR_SUCCESS);
                }
            }
            if(!sent&&connected(peers[0])&&connected(peers[1])){
                for(auto& agent:agents)require(juice_send(agent.get(),"halo2-ice-control",17)==JUICE_ERR_SUCCESS);
                sent=true;
            }
            if(sent&&peers[0].received&&peers[1].received){passed=!peers[0].mismatch&&!peers[1].mismatch;break;}
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        report["candidates"]={{"total",candidates},{"ipv4",ipv4},{"ipv6",ipv6}};
        report["states"]={int(peers[0].state.load()),int(peers[1].state.load())};
        report["received"]={peers[0].received.load(),peers[1].received.load()};
        report["passed"]=passed;
    } catch(const std::exception& error){report["error"]=error.what();}
    juice_set_log_level(JUICE_LOG_LEVEL_NONE);juice_set_log_handler(nullptr);
    {std::lock_guard lock(log_mutex);report["logs"]=logs;}
    std::ofstream(argv[1])<<report.dump(2)<<'\n';
    std::cout<<report.dump()<<'\n';
    return passed?0:1;
}
