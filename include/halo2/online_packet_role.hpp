#pragma once
namespace halo2 {
// Called after physical-peer, roster, protocol and generation admission.
// Voice authorizes its speaker and recipient separately; simulation packets
// retain their host/client direction even when every peer has a direct link.
constexpr bool online_packet_role(bool local_host,bool source_host,unsigned kind) {
  return kind==6 || (kind==18 && (local_host || source_host)) ||
      (local_host && (kind==1 || kind==2 || kind==4 || kind==19)) ||
      (source_host && (kind==3 || kind==5 || kind==14 || kind==20));
}
}
