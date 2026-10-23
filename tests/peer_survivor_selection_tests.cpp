#include "halo2/peer_survivor_selection.hpp"
#include "halo2/peer_prospective_role.hpp"
#include "halo2/peer_common_cut.hpp"
#include "halo2/peer_host_agreement.hpp"
#include <algorithm>
#include <array>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <tuple>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F &&f){bool rejected{};try{f();}catch(const std::exception &){rejected=true;}check(rejected,"Invalid selector input accepted");}
auto context(std::uint16_t peers=0x111,unsigned host=0){AuthorityTransferContext c;c.session[0]=std::byte{0x15};c.epoch=2;c.membership_revision=5;c.peers=peers;c.host=host;return c;}
auto seal(std::uint64_t id=7){AuthorityCutSeal out{id,0x9abcdef012345678ULL,64ull*1024*1024,{}};
    for(unsigned i=0;i<out.digest.size();++i)out.digest[i]=std::byte(0x80+i);return out;}
auto inventory(std::uint64_t lease,std::initializer_list<std::pair<AuthorityCutSeal,bool>> entries){PeerSurvivorInventory out;out.lease=lease;
    check(entries.size()<=out.cuts.size(),"Test inventory overflow");unsigned index{};
    for(const auto &[cut,irreversible]:entries)out.cuts[index++]=PeerSurvivorInventoryEntry{cut,irreversible};return out;}
unsigned type(std::span<const std::byte> bytes){check(bytes.size()>=9&&bytes.size()<=246,"Selector packet extent invalid");return std::to_integer<unsigned>(bytes[8]);}
auto outgoing(PeerSurvivorSelection &owner,unsigned target,unsigned kind,std::uint64_t now=1000){
    auto packets=owner.poll(now,128);const auto found=std::ranges::find_if(packets,[&](const auto &p){return p.peer==target&&type(p.bytes)==kind;});
    check(found!=packets.end(),"Actual selector encoder omitted required control");return found->bytes;
}
struct Mesh {
    struct Packet {unsigned from,to;std::vector<std::byte> bytes;};
    AuthorityTransferContext ctx;std::array<std::unique_ptr<PeerSurvivorSelection>,16> peers;
    std::set<std::tuple<unsigned,unsigned,std::vector<std::byte>>> lost;
    std::uint64_t now{};unsigned serial{},losses{},duplicates{},reorders{};
    explicit Mesh(std::uint16_t mask=0x111,unsigned host=0):ctx(context(mask,host)){
        for(unsigned p=0;p<16;++p)if(p!=host&&(mask&(1u<<p)))peers[p]=std::make_unique<PeerSurvivorSelection>(p,ctx);
    }
    void step(bool lossy=true,unsigned budget=4){now+=101;std::deque<Packet> wire;
        for(unsigned p=0;p<16;++p)if(peers[p])for(auto &out:peers[p]->poll(now,budget)){
            check(PeerSurvivorSelection::is_packet(out.bytes)&&out.bytes.size()<=246,"Unbounded selector control");
            check(out.peer<16&&out.peer!=p&&out.peer!=ctx.host&&(ctx.peers&(1u<<out.peer)),"Selector control escaped fixed physical survivors");
            Packet packet{p,out.peer,std::move(out.bytes)};
            // First loss per exact datagram is bounded. A periodic global
            // pattern must not silently partition one retry route forever.
            if(lossy&&lost.emplace(p,packet.to,packet.bytes).second){++losses;continue;}
            ++serial;if(lossy&&serial%2==0){wire.push_front(packet);++reorders;}else wire.push_back(packet);
            if(lossy&&serial%3==0){wire.push_back(packet);++duplicates;}
        }
        while(!wire.empty()){auto packet=std::move(wire.front());wire.pop_front();
            check(peers[packet.to]->receive(packet.from,packet.bytes,now),"Actual selector datagram was not dispatched");}
    }
    void complete(unsigned budget=4){for(unsigned i=0;i<1500;++i){step(true,budget);bool done=true;
        for(const auto &peer:peers)if(peer){check(!peer->state().failed,"Honest selector schedule failed");done&=peer->state().selection_complete;}
        if(done)return;}throw std::runtime_error("Fixed survivor selection did not converge");}
};

void exact_fields_and_wide_indices(){const auto c=context();
    for(auto self:{0u,1u,16u,260u,std::numeric_limits<unsigned>::max()})rejects([&]{PeerSurvivorSelection p(self,c);});
    for(auto host:{16u,256u,std::numeric_limits<unsigned>::max()}){auto bad=c;bad.host=host;rejects([&]{PeerSurvivorSelection p(4,bad);});}
    auto maximum=c;maximum.epoch=UINT32_MAX;maximum.membership_revision=UINT64_MAX;PeerSurvivorSelection max_context(4,maximum);
    check(max_context.state().context==maximum,"CXX narrowed maximum context fields");
    const auto full=seal(0xfedcba9876543211ULL);Mesh m;
    for(unsigned p:{4u,8u})check(m.peers[p]->publish_inventory(inventory(UINT64_MAX-p,{{full,true}})),"Full-width frozen inventory refused");
    m.complete();const auto digest=m.peers[4]->state().transcript;
    check(std::ranges::any_of(digest,[](auto value){return value!=std::byte{};}),"CXX lost transcript digest");
    for(unsigned p:{4u,8u}){const auto state=m.peers[p]->state();
        check(state.context==c&&state.survivors==0x110&&state.inventory_peers==0x110&&state.accepted==0x110&&state.selected==full&&state.transcript==digest,
            "CXX lost full seal/context/membership/lease transcript fields");
        check(!state.ready_observed,"Inventory/Accept fabricated election Ready");
        for(auto invalid:{0u,1u,16u,260u,std::numeric_limits<unsigned>::max()}){
            check(!m.peers[p]->allows_ready(invalid,full)&&!m.peers[p]->observe_ready(invalid,full),"Wide/couch/old-host index narrowed to a valid survivor");}
    }
    for(unsigned change=0;change<6;++change){PeerSurvivorSelection bad(4,c);auto value=inventory(1,{{full,true}});
        if(change==0)value.lease=0;if(change==1)value.cuts[0]->cut.id=0;if(change==2)value.cuts[0]->cut.bytes=0;
        if(change==3)value.cuts[0]->cut.bytes=64ull*1024*1024+1;if(change==4)value.cuts[0]->cut.digest={};if(change==5)value.cuts[1]=value.cuts[0];
        rejects([&]{bad.publish_inventory(value);});check(!bad.state().inventory_peers,"Invalid inventory changed the authenticated row bank");}
    std::cout<<"PASS real CXX selector: full-width fields, fixed physical membership, invalid unsigned endpoints and no implicit Ready\n";
}

void prospective_metadata_boundary(){auto previous=context(0xffff,15);
    previous.epoch=UINT32_MAX-1;previous.membership_revision=UINT64_MAX-1;
    const auto selected=seal(UINT64_MAX);
    const auto descriptor=derive_peer_prospective_host_role(previous,selected,0);
    check(descriptor.previous==previous&&descriptor.cut==selected&&descriptor.next.session==previous.session&&
        descriptor.next.epoch==UINT32_MAX&&descriptor.next.membership_revision==UINT64_MAX&&descriptor.next.peers==0x7fff&&descriptor.next.host==0,
        "Rust-derived prospective descriptor narrowed/relabelled fields or changed fixed survivors");
    for(auto self:{15u,16u,256u,260u,std::numeric_limits<unsigned>::max()})
        rejects([&]{derive_peer_prospective_host_role(previous,selected,self);});
    for(auto host:{16u,256u,std::numeric_limits<unsigned>::max()}){auto invalid=previous;invalid.host=host;
        rejects([&]{derive_peer_prospective_host_role(invalid,selected,0);});}
    for(unsigned field=0;field<4;++field){auto invalid=previous;
        if(field==0)invalid.epoch=UINT32_MAX;if(field==1)invalid.membership_revision=UINT64_MAX;
        if(field==2)invalid.peers=1u<<15;if(field==3)invalid.session={};
        rejects([&]{derive_peer_prospective_host_role(invalid,selected,0);});}
    for(unsigned field=0;field<4;++field){auto invalid=selected;
        if(field==0)invalid.id=0;if(field==1)invalid.bytes=0;if(field==2)invalid.bytes=64ull*1024*1024+1;if(field==3)invalid.digest={};
        rejects([&]{derive_peer_prospective_host_role(previous,invalid,0);});}
    auto copy=descriptor;copy.cut.digest[31]^=std::byte{1};
    check(copy!=derive_peer_prospective_host_role(previous,selected,0),"Prospective metadata was aliased or silently authorized a changed cut");
    const auto client=derive_peer_prospective_client_role(previous,selected,0);
    check(client.previous==previous&&client.cut==selected&&client.self==0&&client.next_epoch==UINT32_MAX&&
        client.next_membership_revision==UINT64_MAX&&client.survivors==0x7fff,
        "Rust-derived neutral client descriptor narrowed generation, cut or fixed membership");
    for(auto self:{15u,16u,256u,260u,std::numeric_limits<unsigned>::max()})
        rejects([&]{derive_peer_prospective_client_role(previous,selected,self);});
    for(auto host:{16u,256u,std::numeric_limits<unsigned>::max()}){auto invalid=previous;invalid.host=host;
        rejects([&]{derive_peer_prospective_client_role(invalid,selected,0);});}
    rejects([&]{derive_peer_prospective_client_role(context(0x11,0),selected,4);});
    std::cout<<"PASS real CXX neutral client metadata: full-width generation/cut, no unsigned endpoint narrowing, no invented client alternative for a sole survivor\n";
    std::cout<<"PASS real CXX prospective role metadata: checked successor arithmetic/full seal and unsigned endpoints; no fabricated certificate or authorization\n";
}

void independent_rows_accept_reorder_and_ready_bank(){const auto c=context();const auto cut=seal();
    PeerSurvivorSelection left(4,c),right(8,c);check(left.publish_inventory(inventory(4,{{cut,true}}))&&right.publish_inventory(inventory(8,{{cut,true}})),"Frozen row refused");
    const auto left_row=outgoing(left,8,0),right_row=outgoing(right,4,0);
    right.receive(4,left_row,1001);const auto early_accept=outgoing(right,4,1,1001);
    left.receive(8,early_accept,1002);check(left.state().inventory_peers==0x10&&!left.state().accepted&&!left.state().selection_complete,
        "Accept supplied a missing independently authenticated inventory row");
    for(auto source:{0u,4u,260u,std::numeric_limits<unsigned>::max()})left.receive(source,right_row,1003);
    check(left.state().inventory_peers==0x10,"Foreign/wide authenticated source supplied another physical inventory");
    left.receive(8,right_row,1004);const auto left_accept=outgoing(left,8,1,1004);
    right.receive(4,left_accept,1005);check(right.state().selection_complete&&!left.state().selection_complete,"Final Accept reorder fixture did not split completion");
    PeerHostAgreement actual_left(4,c,0),actual_right(8,c,0);
    check(right.allows_ready(8,cut)&&actual_right.publish_ready({8,c,cut}),"Actual complete-side HostAgreement Ready refused");
    const auto actual_controls=actual_right.poll(1006,128);
    const auto ready=std::ranges::find_if(actual_controls,[](const auto &p){return p.peer==4&&p.bytes.size()>=9&&p.bytes[8]==std::byte{2};});
    check(ready!=actual_controls.end(),"Real HostAgreement encoder did not produce Ready");
    actual_left.receive(8,ready->bytes,1006);const auto banked=actual_left.state(1006).ready[8];
    check(banked==cut&&!left.state().failed&&!left.state().ready_observed&&!left.allows_ready(8,cut),
        "Legitimately reordered Ready was lost or prematurely reported to incomplete selector");
    left.receive(8,early_accept,1007);check(left.state().selection_complete&&left.observe_ready(8,*banked),"Delayed Accept could not release the actual stored Ready observation");
    check(left.state().ready_observed==0x100&&!actual_left.state(1007).ready[4],"Deferred actual Ready created a local native preparation claim");
    auto changed=right_row;changed[66]^=std::byte{1};left.receive(8,changed,1008);
    check(left.state().failed&&!left.state().selection_complete&&!left.allows_ready(4,cut),"Conflicting immutable lease did not fail frozen");
    check(!left.publish_inventory(inventory(99,{{seal(9),true}})),"Failed selector silently reset its inventory");
    std::cout<<"PASS independent real rows, early Accept retry and actual HostAgreement Ready bank across reversed control ordering\n";
}

struct CommonMesh {
    AuthorityTransferContext ctx=context();std::array<std::unique_ptr<PeerCommonCutAgreement>,16> peers;std::uint64_t now{};
    std::function<bool(unsigned,unsigned,unsigned)> allowed=[](auto,auto,auto){return true;};
    CommonMesh(){for(unsigned p:{0u,4u,8u})peers[p]=std::make_unique<PeerCommonCutAgreement>(p,ctx);}
    void pump(){for(unsigned step=0;step<32;++step){now+=101;for(unsigned p:{0u,4u,8u})for(const auto &out:peers[p]->poll(now,128))
        if(allowed(p,out.peer,std::to_integer<unsigned>(out.bytes[8])))peers[out.peer]->receive(p,out.bytes,now);}}
    void complete(const PeerCommonCut &cut){check(peers[0]->offer(cut),"Actual common offer refused");pump();
        for(unsigned p:{4u,8u})check(peers[p]->associate(cut),"Actual common association refused");pump();
        check(peers[0]->confirm_native(cut),"Explicit common native confirmation refused");pump();
        for(unsigned p:{4u,8u})check(peers[p]->apply(cut),"Actual common apply refused");pump();
        for(unsigned p:{4u,8u})check(peers[p]->release(cut),"Actual common release refused");pump();}
};
void actual_common_missed_offer_and_partial_release(){CommonMesh common;const PeerCommonCut prior{common.ctx,seal(7),0x110},next{common.ctx,seal(9),0x110};common.complete(prior);
    common.allowed=[](unsigned from,unsigned to,unsigned kind){return !(from==0&&to==8&&kind==0);};
    check(common.peers[0]->offer(next),"Next real common offer refused");common.pump();
    check(common.peers[4]->associate(next),"First pending association refused");common.pump();
    check(common.peers[4]->state().pending==next&&common.peers[8]->state().pending==prior,"Missed real Offer fixture did not diverge pending labels");
    Mesh missed;check(missed.peers[4]->publish_inventory(inventory(4,{{prior.seal,true},{next.seal,false}}))&&
        missed.peers[8]->publish_inventory(inventory(8,{{prior.seal,true}})),"Missed-offer actual retained-pair inventory refused");missed.complete();
    check(missed.peers[4]->state().selected==prior.seal&&missed.peers[8]->state().selected==prior.seal,"Missed Offer selected an unowned pending cut");
    common.allowed=[](auto,auto,auto){return true;};common.pump();check(common.peers[8]->associate(next),"Delayed real association refused");common.pump();
    check(common.peers[0]->confirm_native(next),"Second actual native confirmation refused");common.pump();
    for(unsigned p:{4u,8u})check(common.peers[p]->apply(next),"Second actual apply refused");common.pump();
    check(common.peers[4]->release(next),"Partial real Release refused");common.pump();
    check(common.peers[4]->state().committed==next&&common.peers[8]->state().committed==prior&&common.peers[8]->state().pending==next,
        "Partial real Release did not preserve divergent committed/new pending slots");
    check(common.peers[4]->state().commit&&common.peers[8]->state().commit,"Irreversible evidence was not actually observed");
    Mesh released;check(released.peers[4]->publish_inventory(inventory(4,{{next.seal,true}}))&&
        released.peers[8]->publish_inventory(inventory(8,{{prior.seal,true},{next.seal,true}})),"Partial-release retained inventories refused");released.complete();
    check(released.peers[4]->state().selected==next.seal&&released.peers[8]->state().selected==next.seal,
        "Different local committed slots did not select the shared exact irreversible pair");
    std::cout<<"PASS actual CommonCut packet schedules: missed Offer retains prior shared cut; partial Release selects shared new pending/committed cut\n";
}

void delayed_abort_codec_and_terminal_failure(){const auto c=context();const auto cut=seal();Mesh m;
    for(unsigned p:{4u,8u})check(m.peers[p]->publish_inventory(inventory(p,{{cut,true}})),"Abort fixture refused");m.complete();
    check(!m.peers[4]->invalidate(seal(901))&&m.peers[4]->allows_ready(4,cut),"Unrelated pending Abort invalidated another selected cut");
    check(m.peers[4]->invalidate(cut)&&!m.peers[4]->allows_ready(4,cut),"Exact selected Abort allowed Ready before publication");
    for(unsigned i=0;i<32;++i)m.step();
    for(unsigned p:{4u,8u}){check(m.peers[p]->state().failed&&!m.peers[p]->state().selection_complete&&!m.peers[p]->state().ready_observed,
        "Delayed Abort resurrected selection or manufactured Ready");check(!m.peers[p]->publish_inventory(inventory(99,{{seal(11),true}})),"Abort permitted silent restart");}
    PeerSurvivorSelection publisher(4,c);check(publisher.publish_inventory(inventory(1,{{cut,true},{seal(9),false},{seal(11),true}})),"Maximum packet fixture refused");
    const auto bytes=outgoing(publisher,8,0);check(bytes.size()==246,"Maximum selector wire bound changed");
    PeerSurvivorSelection parser(8,c);for(std::size_t size=0;size<bytes.size();++size)parser.receive(4,std::span(bytes).first(size),0);
    auto trailing=bytes;trailing.push_back(std::byte{});parser.receive(4,trailing,0);
    for(unsigned offset:{7u,8u,58u,74u,131u}){auto bad=bytes;bad[offset]=std::byte{255};parser.receive(4,bad,0);}
    check(!parser.state().inventory_peers&&!parser.state().selection_complete&&parser.state().rejected>200,"Malformed CXX codec path admitted a row");
    std::cout<<"PASS exact delayed Abort and immutable terminal failure; real bounded decoder rejects truncation/extension/version/bool/attempt controls\n";
}

void full_physical_room_and_no_quorum_substitution(){const auto chosen=seal(0xfedcba9876543211ULL);
    for(auto budget:{1u,2u,4u,128u,std::numeric_limits<unsigned>::max()}){Mesh m(0xffff,15);
        for(unsigned p=0;p<15;++p){auto row=inventory(UINT64_MAX-p,{{chosen,p==0}});
            if(p%2==0)row.cuts[1]=PeerSurvivorInventoryEntry{seal(UINT64_MAX),true};
            check(m.peers[p]->publish_inventory(row),"Full physical-room row refused");check(m.peers[p]->poll(1,0).empty(),"Zero budget emitted a selector packet");}
        m.complete(budget);check(m.losses&&m.duplicates&&m.reorders,"Full physical-room loss/reorder fixture was not exercised");
        for(unsigned p=0;p<15;++p){const auto state=m.peers[p]->state();check(state.survivors==0x7fff&&state.inventory_peers==0x7fff&&state.accepted==0x7fff&&
            state.selected==chosen&&!state.ready_observed,"Sixteen-PC predecessor room lost a fixed survivor or fabricated Ready");}
    }
    PeerSurvivorSelection missing(4,context());check(missing.publish_inventory(inventory(1,{{chosen,true}})),"Missing survivor fixture refused");
    missing.poll(UINT64_MAX,128);check(missing.state().survivors==0x110&&missing.state().inventory_peers==0x10&&!missing.state().accepted&&!missing.state().selection_complete,
        "Timeout or couch seats substituted for the missing physical survivor");
    for(auto rows:{std::pair{inventory(4,{{seal(7),false}}),inventory(8,{{seal(7),false}})},
        std::pair{inventory(4,{{seal(7),true}}),inventory(8,{{seal(9),true}})}}){Mesh none;
        none.peers[4]->publish_inventory(rows.first);none.peers[8]->publish_inventory(rows.second);for(unsigned i=0;i<32;++i)none.step();
        check(none.peers[4]->state().failed&&none.peers[8]->state().failed,"Association-only/no-intersection inventories produced selection");}
    std::cout<<"PASS all15 fixed survivors of16 physical PCs with loss, retry, duplicates, reordering and budgets1/2/4/128/MAX; no timeout-created quorum\n";
}
}
int main()try{exact_fields_and_wide_indices();prospective_metadata_boundary();independent_rows_accept_reorder_and_ready_bank();actual_common_missed_offer_and_partial_release();
    delayed_abort_codec_and_terminal_failure();full_physical_room_and_no_quorum_substitution();
    std::cout<<"LIMIT declared frozen-owner inventories and component control receipts; no native preparation, production driver, rendered gameplay, EOS/WAN or hardware acceptance\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
