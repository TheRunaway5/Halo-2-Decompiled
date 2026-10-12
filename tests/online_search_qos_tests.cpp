#include "halo2/online_search_qos.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace halo2;
int main(int argc,char **argv) {
  try {
    const std::string token(43,'s');
    for(const auto &url:{"http://queue.example","http://127.0.0.1:8080","https://user@queue.example",
        "https://queue.example/path","https://queue.example?x","https://queue.example\n"}) {
      bool rejected=false;
      try { OnlineSearchQos invalid(url,token,false); } catch(...) { rejected=true; }
      if(!rejected)throw std::runtime_error("Invalid search origin accepted");
    }
    for(const auto &bearer:{std::string("short"),std::string(129,'s'),std::string(43,'s')+"\r\nX: forged"}) {
      bool rejected=false;
      try { OnlineSearchQos invalid("https://queue.example",bearer,false); } catch(...) { rejected=true; }
      if(!rejected)throw std::runtime_error("Invalid search bearer accepted");
    }
    if(argc!=2)throw std::runtime_error("Missing fixture origin");
    {
      OnlineSearchQos client(argv[1],token,true);
      std::this_thread::sleep_for(std::chrono::seconds(6));
    }
    std::cout<<"PASS search origin and bearer validation, bounded native worker teardown\n";
  } catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
