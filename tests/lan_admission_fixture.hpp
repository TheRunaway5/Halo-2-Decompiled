// Included inside multiplayer_tests.cpp's fixture namespace.
void raw_lan_admission_security(){
#ifndef _WIN32
  using Packet=std::vector<std::byte>;
  const auto put=[](Packet &p,std::uint64_t v,unsigned bytes){for(unsigned i=0;i<bytes;++i)p.push_back(std::byte(v>>(8*i)));};
  const auto header=[&](unsigned kind){Packet p;put(p,0x324e3248,4);put(p,native_multiplayer_protocol,2);put(p,kind,1);return p;};
  auto content=arena();auto selected=rules();auto variant=*selected.session_configuration;
  variant.name=std::string(1024,'x');for(unsigned i=0;i<3500;++i)variant.values.append(76,1);
  MultiplayerMatch match(content,0,starts(),selected);LanHost host(match,0,123,variant);
  struct Socket {int fd=::socket(AF_INET,SOCK_DGRAM,0);~Socket(){if(fd>=0)close(fd);}} a,b;
  check(a.fd>=0&&b.fd>=0,"Create raw admission fixture sockets");
  sockaddr_in to{};to.sin_family=AF_INET;to.sin_addr.s_addr=htonl(INADDR_LOOPBACK);to.sin_port=htons(host.port());
  std::array<Controls,16> controls{};
  const auto send=[&](int fd,const Packet &p){check(sendto(fd,p.data(),p.size(),0,reinterpret_cast<sockaddr*>(&to),sizeof(to))==int(p.size()),"Send admission fixture");};
  const auto receive=[&](int fd,unsigned rounds=20){
    std::vector<Packet> out;
    for(unsigned tick=0;tick<rounds;++tick){host.receive(controls);
      for(;;){std::array<std::byte,1501> bytes{};const auto n=recv(fd,bytes.data(),bytes.size(),MSG_DONTWAIT);
        if(n<0)break;out.emplace_back(bytes.begin(),bytes.begin()+n);}
      std::this_thread::sleep_for(std::chrono::milliseconds(2));}
    return out;
  };
  auto hello=header(1);put(hello,456,8);put(hello,123,8);put(hello,4,1);
  for(unsigned i=0;i<4;++i)put(hello,255,1);for(unsigned i=0;i<4;++i)put(hello,0,1);
  send(a.fd,hello);receive(a.fd);
  check(host.peers()==0&&match.available_player_slots()==16,"First unauthenticated hello allocated live player slots");
  const auto wrap=[&](const Packet &body,const Packet &cookie){auto p=header(9);p.insert(p.end(),cookie.begin(),cookie.end());p.insert(p.end(),body.begin(),body.end());return p;};
  const Packet empty_cookie(16);
  auto initial=wrap(hello,empty_cookie);send(a.fd,initial);auto challenge=receive(a.fd);
  check(challenge.size()==1&&challenge[0].size()==31&&challenge[0][6]==std::byte{8},"Hello must receive only a bounded challenge");
  Packet cookie(challenge[0].begin()+15,challenge[0].end());
  send(b.fd,wrap(hello,cookie));receive(b.fd);
  check(host.peers()==0,"Another endpoint echoed a stolen cookie");
  auto changed=hello;changed[7]^=std::byte{1};send(a.fd,wrap(changed,cookie));receive(a.fd);
  check(host.peers()==0,"Cookie authorized another hello");
  for(unsigned i=0;i<100;++i){auto spoof=hello;spoof[7]=std::byte(i);send(b.fd,wrap(spoof,empty_cookie));}
  receive(b.fd);check(host.peers()==0,"Unverified hello flood consumed lobby capacity");
  send(a.fd,wrap(hello,cookie));const auto initial_snapshot=receive(a.fd);
  check(host.peers()==1&&match.available_player_slots()==12,"Valid cookie did not admit exactly the requested couch");
  const auto admitted_hello=wrap(hello,cookie);
  for(unsigned i=0;i<100;++i)send(a.fd,admitted_hello);
  check(receive(a.fd,1).size()<=initial_snapshot.size(),"Repeated admitted hellos regenerated unbounded snapshots");
  auto query=header(6);put(query,789,8);
  send(b.fd,query);check(receive(b.fd).empty(),"Legacy unauthenticated query amplified a reply");
  const auto request=wrap(query,empty_cookie);send(b.fd,request);challenge=receive(b.fd);
  std::size_t first_bytes{};for(const auto &p:challenge)first_bytes+=p.size();
  check(challenge.size()==1&&first_bytes<=request.size()&&challenge[0][6]==std::byte{8},"Session query amplified before proof");
  cookie.assign(challenge[0].begin()+15,challenge[0].end());
  send(b.fd,wrap(query,cookie));const auto description=receive(b.fd);
  std::size_t bytes{};for(const auto &p:description){check(p[6]==std::byte{7},"Verified query did not return configuration");bytes+=p.size();}
  check(bytes>28000&&host.peers()==1,"Verified fragmented configuration or player census changed");
  std::this_thread::sleep_for(std::chrono::milliseconds(5100));
  send(a.fd,hello);receive(a.fd);
  check(host.peers()==0,"Unverified hello refreshed an established player's liveness");
#endif
}
