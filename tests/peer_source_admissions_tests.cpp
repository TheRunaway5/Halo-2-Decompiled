#include "halo2/peer_source_admissions.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}check(rejected,"Invalid source admission input accepted");}
AuthorityTransferContext context(){AuthorityTransferContext c;c.session[0]=std::byte{0x42};c.peers=(1u<<0)|(1u<<4);c.host=0;return c;}
std::vector<OnlineAdmission> roster(unsigned count=8){std::vector<OnlineAdmission> rows;
    for(unsigned i=0;i<count;++i){OnlineAdmission p{i,"Chosen Halo name",-1};p.peer=i/4*4;p.guest=i%4!=0;p.identity[0]=std::byte(i+1);rows.push_back(p);}return rows;}
std::vector<SourcePlayerAdmission> sources(unsigned count=8){std::vector<SourcePlayerAdmission> players;
    for(unsigned i=0;i<count;++i){SourcePlayerAdmission p{i,0x10000u+i};p.identity[0]=0x42;p.identity[11]=std::uint8_t(i);players.push_back(p);}return players;}
void resign(std::vector<std::byte> &packet){
    std::vector<std::byte> input;for(char c:std::string_view("H2 SOURCE ADMISSIONS 1"))input.push_back(std::byte(c));
    input.insert(input.end(),packet.begin()+9,packet.begin()+56);input.insert(input.end(),packet.begin()+89,packet.end());
    check(SHA256(reinterpret_cast<const unsigned char *>(input.data()),input.size(),reinterpret_cast<unsigned char *>(packet.data()+57)),"Test packet hash failed");
}
void run(){
    const auto c=context();const auto r=roster();const auto players=sources();PeerSourceAdmissions host(0,c,r),client(4,c,r);
    check(!host.accepted()&&!client.accepted()&&!host.confirmed(4),"Admission fabricated before publication");
    check(host.poll(0).empty()&&!client.publish(c,players),"Nonhost minted source admissions");
    check(host.publish(c,players)&&host.publish(c,players),"Host publication/idempotence failed");
    check(host.poll(0,0).empty(),"Zero packet budget was ignored");auto initial=host.poll(0,16);
    check(initial.size()==1&&initial[0].peer==4&&initial[0].bytes.size()==306,"Bounded eight-player offer differs");const auto offer=initial[0].bytes;
    check(host.poll(249).empty()&&!client.accepted(),"Lost offer delivered without retry");
    auto retry=host.poll(250);check(retry.size()==1,"Lost offer was not retried");client.receive(0,retry[0].bytes);
    auto accepted=client.accepted();check(accepted&&accepted->players==players&&accepted->context==c,"Accepted native allocations changed");
    check(!host.confirmed(4),"Host confirmed admission before acknowledgement");
    auto lost_ack=client.poll(251);check(lost_ack.size()==1&&lost_ack[0].bytes.size()==89,"Bounded acknowledgement differs");
    retry=host.poll(500);check(retry.size()==1,"Lost acknowledgement stopped retries");client.receive(0,retry[0].bytes);
    auto ack=client.poll(501);check(ack.size()==1,"Duplicate offer did not reacknowledge");host.receive(4,ack[0].bytes);
    check(host.confirmed(4)&&host.poll(1000).empty()&&!host.confirmed(1),"Acknowledged host state differs");

    auto changed=players;changed[0].player^=0x10000;check(!host.publish(c,changed),"Same generation replaced a native allocation");
    PeerSourceAdmissions other_host(0,c,r);check(other_host.publish(c,changed),"Conflicting offer fixture failed");
    client.receive(0,other_host.poll(0)[0].bytes);check(client.accepted()==accepted&&client.poll(502).empty(),"Conflicting host offer replaced or acknowledged accepted identities");
    for(unsigned n=7;n<offer.size();++n){PeerSourceAdmissions target(4,c,r);target.receive(0,std::span(offer).first(n));check(!target.accepted(),"Truncated offer was accepted");}
    for(unsigned source:{1u,4u,16u}){PeerSourceAdmissions target(4,c,r);target.receive(source,offer);check(!target.accepted(),"Unverified physical endpoint admitted identities");}
    for(unsigned at:{4u,7u,9u,41u,45u,53u,55u,56u,57u,89u,90u,91u,92u,93u,101u}){
        PeerSourceAdmissions target(4,c,r);auto forged=offer;forged[at]^=std::byte{0x80};
        if(at>=89)resign(forged);target.receive(0,forged);check(!target.accepted(),"Malformed/context/roster/handle offer was accepted");
    }
    {PeerSourceAdmissions target(4,c,r);auto forged=offer;
        std::copy_n(forged.begin()+105,12,forged.begin()+132);resign(forged);target.receive(0,forged);check(!target.accepted(),"Duplicate source identity was accepted");}
    {PeerSourceAdmissions target(4,c,r);auto forged=offer;forged.push_back(std::byte{});resign(forged);target.receive(0,forged);check(!target.accepted(),"Trailing bytes admitted a different roster");}
    for(auto bad:{0u,1u,7u}){auto invalid=players;invalid[bad].slot=16;rejects([&]{host.publish(c,invalid);});}
    rejects([&]{host.publish(c,std::span(players).first(7));});
    auto invalid_roster=r;invalid_roster[4].guest=true;rejects([&]{PeerSourceAdmissions invalid(0,c,invalid_roster);});
    auto invalid_context=c;invalid_context.host=1;rejects([&]{PeerSourceAdmissions invalid(0,invalid_context,r);});
    check(!PeerSourceAdmissions::is_packet({})&&!client.receive(0,std::array<std::byte,7>{}),"Non-admission datagrams were consumed");

    auto next=c;++next.epoch;next.host=4;check(host.replace_context(c,next)&&client.replace_context(c,next),"Committed host generation was not replaced");
    check(!host.accepted()&&!client.accepted()&&!host.confirmed(4),"Prior generation admission survived replacement");
    host.receive(4,offer);client.receive(0,offer);check(!host.accepted()&&!client.accepted(),"Replayed prior generation offer was accepted");
    check(!host.replace_context(c,next)&&!client.replace_context(next,c),"Stale/reversed context reset accepted");
    check(client.publish(next,players),"New committed host could not publish");auto reverse=client.poll(1000);host.receive(4,reverse[0].bytes);
    auto reverse_ack=host.poll(1000);client.receive(0,reverse_ack[0].bytes);check(client.confirmed(0)&&host.accepted()->players==players,"Reverse-host exchange failed");
    auto revision=next;++revision.membership_revision;check(host.replace_context(next,revision)&&client.replace_context(next,revision),"Agreed membership generation reset failed");
    host.receive(4,reverse[0].bytes);check(!host.accepted(),"Old membership generation was replayed");
    {auto full=context();full.peers=0x1111;const auto full_roster=roster(16);
        PeerSourceAdmissions leader(0,full,full_roster);check(leader.publish(full,sources(16)),"Sixteen-player publication failed");
        auto offers=leader.poll(0,16);check(offers.size()==3,"Sixteen-player roster confused couch seats with physical peers");
        for(const auto &packet:offers){check(packet.bytes.size()==522,"Maximum source roster exceeded its bounded packet");PeerSourceAdmissions recipient(packet.peer,full,full_roster);
            recipient.receive(0,packet.bytes);check(recipient.accepted()&&recipient.accepted()->players==sources(16),"Maximum source roster differs");
            auto receipt=recipient.poll(0);check(receipt.size()==1,"Maximum roster acknowledgement failed");leader.receive(packet.peer,receipt[0].bytes);
            check(leader.confirmed(packet.peer),"Maximum roster recipient was not confirmed");}
        check(leader.poll(1000).empty(),"Completed maximum roster continued broadcasting");
    }
    std::cout<<"PASS bounded source admission exchange: eight couch slots, offer/ack loss and duplicates, immutable identities, authenticated endpoints, room/epoch/membership/roster/handle validation and committed host replacement\n";
}
void continuity(){
    auto before=context();before.peers=0x1111;const auto r=roster(16);
    const auto rows=sources(16);PeerSourceAdmissions original(0,before,r);
    check(original.publish(before,rows),"Continuity fixture admission refused");const auto accepted=*original.accepted();
    auto next=before;++next.epoch;++next.membership_revision;next.host=4;next.peers=0x1110;
    auto host=PeerSourceAdmissions::resume_after_host_loss(4,next,r,accepted);
    auto client=PeerSourceAdmissions::resume_after_host_loss(8,next,r,accepted);
    auto delayed=PeerSourceAdmissions::resume_after_host_loss(12,next,r,accepted);
    check(host->accepted()->players==rows&&client->accepted()->players==rows&&delayed->accepted()->players==rows,
        "Host loss changed original native/PCR allocation rows");
    check(client->poll(0).empty()&&!host->confirmed(8),"Context carry fabricated receipt of a successor offer");
    auto changed=rows;changed[0].identity.front()^=0x80;check(!host->publish(next,changed),"Replacement publication changed a retained original identity");
    PeerSourceAdmissions conflicting(4,next,r);check(conflicting.publish(next,changed),"Conflicting successor fixture refused");
    for(auto &packet:conflicting.poll(0,16))if(packet.peer==8)client->receive(4,packet.bytes);
    check(client->accepted()->players==rows&&client->poll(1).empty(),"Successor replaced or acknowledged a different original allocation roster");
    for(auto &packet:host->poll(0,16))if(packet.peer==8)client->receive(4,packet.bytes);
    for(auto &packet:client->poll(1))host->receive(8,packet.bytes);
    check(host->confirmed(8)&&!host->confirmed(12),"Successor offer receipt confused a delayed endpoint");
    // PC12 can carry its independently retained original identities through
    // the next certified epoch even if the intermediate host dies before its
    // offer arrives. A timeout itself does not call this factory.
    auto last=next;++last.epoch;++last.membership_revision;last.peers=0x1100;last.host=8;
    auto survivor=PeerSourceAdmissions::resume_after_host_loss(12,last,r,*delayed->accepted());
    check(survivor->accepted()->context==last&&survivor->accepted()->players==rows,"Delayed survivor lost the original allocation chain");
    auto forged=accepted;forged.digest.front()^=std::byte{1};rejects([&]{PeerSourceAdmissions::resume_after_host_loss(4,next,r,forged);});
    auto invalid=next;invalid.peers=before.peers;rejects([&]{PeerSourceAdmissions::resume_after_host_loss(4,invalid,r,accepted);});
    invalid=next;invalid.epoch=before.epoch;rejects([&]{PeerSourceAdmissions::resume_after_host_loss(4,invalid,r,accepted);});
    rejects([&]{PeerSourceAdmissions::resume_after_host_loss(0,next,r,accepted);});
    std::cout<<"PASS certified host-loss identity carry: retained sixteen native/PCR rows, no fabricated offer receipt, immutable successor identities, delayed learner across two epochs and invalid certificate context/digest rejection\n";
}
void concurrent(){
    const auto r=roster();const auto rows=sources();PeerSourceAdmissions a(0,context(),r),b(4,context(),r);
    const auto began=std::chrono::steady_clock::now();auto now=[&]{return std::uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count());};
    std::atomic<bool> failed=false;std::mutex gate;std::exception_ptr error;
    auto pump=[&](std::stop_token stop,auto &from,auto &to,unsigned source){try{unsigned serial=0;while(!stop.stop_requested()){
        for(auto &packet:from.poll(now(),16)){if(++serial%7==0)continue;to.receive(source,packet.bytes);if(serial%11==0)to.receive(source,packet.bytes);}
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }}catch(...){std::lock_guard lock(gate);error=std::current_exception();failed=true;}};
    std::jthread out([&](std::stop_token stop){pump(stop,a,b,0);});std::jthread back([&](std::stop_token stop){pump(stop,b,a,4);});
    std::jthread observer([&](std::stop_token stop){while(!stop.stop_requested()){(void)a.accepted();(void)b.accepted();(void)a.context();(void)b.confirmed(0);std::this_thread::yield();}});
    auto wait=[&](auto ready){const auto deadline=now()+10000;while(!ready()){
        if(failed){std::lock_guard lock(gate);std::rethrow_exception(error);}check(now()<deadline,"Concurrent admission exchange timed out");std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }};
    check(a.publish(context(),rows),"Concurrent host publication refused");wait([&]{return a.confirmed(4)&&b.accepted().has_value();});
    auto next=context();++next.epoch;next.host=4;check(a.replace_context(context(),next)&&b.replace_context(context(),next),"Concurrent generation reset refused");
    check(b.publish(next,rows),"Concurrent replacement host publication refused");wait([&]{return b.confirmed(0)&&a.accepted().has_value();});
    check(a.accepted()->players==rows&&b.accepted()->players==rows,"Concurrent exchange corrupted identities");
    std::cout<<"PASS concurrent peer pumping, admission publication, observation and committed generation replacement\n";
}
}
int main(){try{run();continuity();concurrent();return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
