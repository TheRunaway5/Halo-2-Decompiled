#include "online_account.cpp.inc"
namespace halo2 {
bool online_party_follower(const OnlineParty &p) { return !p.leader && p.members.size()>1; }
bool online_party_leader_left_postgame(const OnlineParty &p) {
  if(!online_party_follower(p))return false;
  bool self{},leader{};
  for(const auto &m:p.members) {
    if(m.self)self=m.postgame==true;
    if(m.leader)leader=m.postgame==false;
  }
  return self && leader;
}
}
