// The exact preserved Session caller is run against declared protocol/resource
// observations and compared to production Rust dispatch. The lock is traced and
// native error paths are exercised. These fixtures do not create a native world,
// authenticate a provider or qualify installed delivery.
#include "halo2/online_match_session.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <mutex>
#include <stdexcept>
using Json=nlohmann::json;
namespace actual=halo2::online_protocol_bridge;
namespace fixture {
class Session;
void record(Session &,const std::string &,std::uint64_t=0,std::uint64_t=0);
struct Context{std::uint32_t epoch;};
struct State{Context context;};
struct Mutex {
    Session &s;bool held{};void lock();void unlock();
};
template<unsigned K>struct Leaf {
    Session &s;std::uint32_t value;std::optional<unsigned> historical;
    bool receive(unsigned source,std::span<const std::byte> p,std::uint64_t=0){
        std::uint64_t digest=14695981039346656037ull;for(auto b:p){digest^=std::to_integer<unsigned>(b);digest*=1099511628211ull;}
        record(s,historical?"history":"receive",historical?*historical:K,source);record(s,"payload",p.size(),digest);return true;
    }
    State state()const{record(s,"epoch",K,value);return {{value}};}
    Context context()const{record(s,"epoch",K,value);return {value};}
    void observe_host(std::uint64_t){record(s,"observe");}
    static bool is_packet(std::span<const std::byte> p){
        if constexpr(K==0)return halo2::PeerSurvivorSelection::is_packet(p);
        if constexpr(K==1)return halo2::PeerSnapshotCandidate::is_packet(p);
        if constexpr(K==2)return halo2::PeerCommonCutAgreement::is_packet(p);
        if constexpr(K==3)return halo2::PeerHostAgreement::is_packet(p);
        if constexpr(K==4)return halo2::PeerMeshQuality::is_packet(p);
        if constexpr(K==5)return halo2::PeerSourceAdmissions::is_packet(p);
        if constexpr(K==6)return halo2::PeerAuthorityTransfer::is_packet(p);
        return false;
    }
};
using PeerSurvivorSelection=Leaf<0>;using PeerSnapshotCandidate=Leaf<1>;
using PeerCommonCutAgreement=Leaf<2>;using PeerHostAgreement=Leaf<3>;
using PeerMeshQuality=Leaf<4>;using PeerSourceAdmissions=Leaf<5>;using PeerAuthorityTransfer=Leaf<6>;
struct Controller{Session *session;};
namespace bridge {bool native_session_datagram(Controller &,unsigned,std::uint32_t,std::span<const std::byte>);}
std::span<const std::byte> wire(std::span<const std::byte> p){return p;}
std::uint64_t transfer_time(){return 0;}
class Session {
public:
    Json trace=Json::array();Mutex mutex{*this};unsigned flags{},capabilities{};std::string fail;
    struct{bool direct_udp;}transport_options;
    std::shared_ptr<PeerSurvivorSelection> survivor_selection;
    std::shared_ptr<PeerSnapshotCandidate> snapshot_candidate;
    std::shared_ptr<PeerCommonCutAgreement> common_cut;
    std::shared_ptr<PeerHostAgreement> agreement;
    std::shared_ptr<PeerMeshQuality> mesh_quality;
    std::shared_ptr<PeerSourceAdmissions> source_admissions;
    std::shared_ptr<PeerAuthorityTransfer> authority;
    std::optional<Context> agreement_context;
    struct Past{Context context;std::shared_ptr<PeerHostAgreement> owner;};std::vector<Past> agreement_history;
    std::unique_ptr<Controller> controller=std::make_unique<Controller>(Controller{this});
    bool general_result;
    explicit Session(const Json &v):flags(v.value("flags",31u)),capabilities(v.value("capabilities",7u)),fail(v.value("fail",std::string{})),general_result(v.value("general",true)){
        transport_options.direct_udp=flags&8;const unsigned owners=v.value("owners",127u);const auto epochs=v.value("epochs",std::vector<std::uint32_t>(7,3));
        if(epochs.size()!=7)throw std::invalid_argument("epochs");
        if(owners&1)survivor_selection=std::make_shared<PeerSurvivorSelection>(PeerSurvivorSelection{*this,epochs[0],{}});
        if(owners&2)snapshot_candidate=std::make_shared<PeerSnapshotCandidate>(PeerSnapshotCandidate{*this,epochs[1],{}});
        if(owners&4)common_cut=std::make_shared<PeerCommonCutAgreement>(PeerCommonCutAgreement{*this,epochs[2],{}});
        if(owners&8)agreement=std::make_shared<PeerHostAgreement>(PeerHostAgreement{*this,epochs[3],{}});
        if(owners&16)mesh_quality=std::make_shared<PeerMeshQuality>(PeerMeshQuality{*this,epochs[4],{}});
        if(owners&32)source_admissions=std::make_shared<PeerSourceAdmissions>(PeerSourceAdmissions{*this,epochs[5],{}});
        if(owners&64)authority=std::make_shared<PeerAuthorityTransfer>(PeerAuthorityTransfer{*this,epochs[6],{}});
        if(!v.contains("binding")||!v.at("binding").is_null())agreement_context=Context{v.value("binding",3u)};
        for(auto epoch:v.value("history",std::vector<std::uint32_t>{1,2,3,3,4})){
            const auto i=unsigned(agreement_history.size());agreement_history.push_back({{epoch},std::make_shared<PeerHostAgreement>(PeerHostAgreement{*this,epoch,i})});
        }
    }
    bool common_phase()const{return flags&1;}bool native_control_phase()const{return flags&2;}
    bool peer_handoff_phase()const{return flags&4;}bool epoch_packets()const{return flags&16;}
    bool selection_supported()const{return (capabilities&1)&&bool(survivor_selection);}
    bool snapshot_supported()const{return (capabilities&2)&&bool(snapshot_candidate);}
    bool common_supported()const{return (capabilities&4)&&bool(common_cut);}
    void selection_abort(){record(*this,"abort");}void selection_progress(){record(*this,"progress");}
    void fence_peer_generation(){record(*this,"fence");}void refresh(){record(*this,"refresh");}
#include "reference/native_receive_dispatch_before_rust_20261101.inc"
};
void record(Session &s,const std::string &kind,std::uint64_t a,std::uint64_t b){
    s.trace.push_back({{"event",kind},{"a",a},{"b",b},{"locked",s.mutex.held}});
    if(s.fail==kind)throw std::runtime_error("declared native IO refusal");
}
void Mutex::lock(){if(held)throw std::runtime_error("duplicate lock");held=true;record(s,"lock");}
void Mutex::unlock(){if(!held)throw std::runtime_error("missing lock");record(s,"unlock");held=false;}
namespace bridge {bool native_session_datagram(Controller &c,unsigned source,std::uint32_t epoch,std::span<const std::byte> p){record(*c.session,"general",source,epoch);std::uint64_t digest=14695981039346656037ull;for(auto b:p){digest^=std::to_integer<unsigned>(b);digest*=1099511628211ull;}record(*c.session,"payload",p.size(),digest);return c.session->general_result;}}
}
class Io final:public actual::NativeReceiveIo {
    fixture::Session &s;
    using Target=actual::NativeReceiveTarget;
    mutable std::shared_ptr<fixture::PeerSurvivorSelection> selection;
    mutable std::shared_ptr<fixture::PeerSnapshotCandidate> snapshot;
    mutable std::shared_ptr<fixture::PeerCommonCutAgreement> common;
    mutable std::shared_ptr<fixture::PeerHostAgreement> agreement;
    mutable std::shared_ptr<fixture::PeerMeshQuality> quality;
    mutable std::shared_ptr<fixture::PeerSourceAdmissions> admissions;
    mutable std::shared_ptr<fixture::PeerAuthorityTransfer> authority;
    static std::span<const std::byte> bytes(rust::Slice<const std::uint8_t> p){return {reinterpret_cast<const std::byte*>(p.data()),p.size()};}
public:
    explicit Io(fixture::Session &value):s(value){}
    ~Io(){if(s.mutex.held)s.mutex.unlock();}
    void receive_lock()const override{s.mutex.lock();}void receive_unlock()const override{s.mutex.unlock();}
    actual::NativeReceiveSession receive_session()const override{actual::NativeReceiveSession out;out.common=s.common_phase();out.native_control=s.native_control_phase();out.handoff=s.peer_handoff_phase();out.direct=s.transport_options.direct_udp;out.epochs=s.epoch_packets();return out;}
    bool receive_supported(Target t)const override{switch(t){case Target::Selection:return s.selection_supported();case Target::Snapshot:return s.snapshot_supported();case Target::Common:return s.common_supported();default:throw std::invalid_argument("support target");}}
    void receive_pin(Target t)const override{switch(t){case Target::Selection:selection=s.survivor_selection;break;case Target::Snapshot:snapshot=s.snapshot_candidate;break;case Target::Common:common=s.common_cut;break;case Target::Agreement:agreement=s.agreement;break;case Target::Quality:quality=s.mesh_quality;break;case Target::Admissions:admissions=s.source_admissions;break;case Target::Authority:authority=s.authority;break;default:throw std::invalid_argument("pin");}}
    bool receive_present(Target t)const override{switch(t){case Target::Selection:return bool(selection);case Target::Snapshot:return bool(snapshot);case Target::Common:return bool(common);case Target::Agreement:return bool(agreement);case Target::Quality:return bool(quality);case Target::Admissions:return bool(admissions);case Target::Authority:return bool(authority);default:throw std::invalid_argument("present");}}
    std::uint32_t receive_epoch(Target t)const override{switch(t){case Target::Selection:return selection->state().context.epoch;case Target::Snapshot:return snapshot->state().context.epoch;case Target::Common:return common->context().epoch;case Target::Agreement:return s.agreement_context->epoch;case Target::Quality:return quality->context().epoch;case Target::Admissions:return admissions->context().epoch;case Target::Authority:return authority->context().epoch;default:throw std::invalid_argument("epoch");}}
    bool receive_binding_present()const override{return s.agreement_context.has_value();}std::uint32_t receive_binding_epoch()const override{return s.agreement_context->epoch;}
    std::uint64_t receive_history_len()const override{return s.agreement_history.size();}std::uint32_t receive_history_epoch(std::uint64_t i)const override{return s.agreement_history.at(i).context.epoch;}
    void receive_history_packet(std::uint64_t i,std::uint32_t source,rust::Slice<const std::uint8_t> p)const override{s.agreement_history.at(i).owner->receive(source,bytes(p),0);}
    void receive_packet(Target t,std::uint32_t source,rust::Slice<const std::uint8_t> p)const override{const auto b=bytes(p);switch(t){case Target::Selection:selection->receive(source,b);break;case Target::Snapshot:snapshot->receive(source,b);break;case Target::Common:common->receive(source,b);break;case Target::Agreement:agreement->receive(source,b);break;case Target::Quality:quality->receive(source,b);break;case Target::Admissions:admissions->receive(source,b);break;case Target::Authority:authority->receive(source,b);break;default:throw std::invalid_argument("receive");}}
    void receive_abort()const override{s.selection_abort();}void receive_progress()const override{s.selection_progress();}void receive_fence()const override{s.fence_peer_generation();}
    bool receive_general(std::uint32_t source,std::uint32_t epoch,rust::Slice<const std::uint8_t> p)const override{return fixture::bridge::native_session_datagram(*s.controller,source,epoch,bytes(p));}
    void receive_observe_host()const override{agreement->observe_host(0);}void receive_refresh()const override{s.refresh();}
};
std::vector<std::byte> bytes(const std::string &s){if(s.size()%2)throw std::invalid_argument("odd hex");std::vector<std::byte> out;for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return out;}
Json run(fixture::Session &s,bool rust_owner,unsigned source,std::uint32_t epoch,std::span<const std::byte> packet){bool error{};try{if(rust_owner){Io io(s);actual::native_receive_dispatch(io,source,{reinterpret_cast<const std::uint8_t*>(packet.data()),packet.size()},epoch);}else s.datagram(source,packet,epoch);}catch(const std::exception &){error=true;}return {{"trace",s.trace},{"error",error},{"held",s.mutex.held}};}
int main(){std::string line;while(std::getline(std::cin,line)){try{const auto v=Json::parse(line);fixture::Session before(v),after(v);const auto packet=bytes(v.at("packet"));const unsigned source=v.value("source",4u);const auto epoch=v.value("epoch",3u);const auto a=run(before,false,source,epoch,packet),b=run(after,true,source,epoch,packet);Json out={{"matched",a==b},{"result",a}};if(a!=b)out["rust"]=b;std::cout<<out.dump()<<std::endl;}catch(const std::exception &e){std::cout<<Json{{"fixture_error",e.what()}}.dump()<<std::endl;}}}
