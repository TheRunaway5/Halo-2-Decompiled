#include "halo2/vista_loading_thread.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original loading scheduler fixture");
    std::ifstream file(argv[1]);const auto original=nlohmann::json::parse(file);
    if(original.at("schema")!="H2V_LOADING_THREAD_ORIGINAL_V1" ||
       original.at("source_sha256")!="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109")
      throw std::runtime_error("Unexpected original loading scheduler source");
    unsigned count{};
    for(const auto& row:original.at("cases")) {
      const auto& supplied=row.at("supplied");
      std::uint8_t mode=supplied.at("initial_mode"),active=supplied.at("initial_active");
      unsigned draws{},active_queries{};nlohmann::json events=nlohmann::json::array();
      halo2::VistaLoadingThreadServices services;
      services.sleep=[&](auto duration){events.push_back({"sleep",duration});};
      services.mode=[&]{return mode;};
      services.present=[&](auto argument)->std::uint8_t {
        const auto& item=supplied.at("draws").at(draws++);
        const std::uint32_t result=item.at("result_word");
        events.push_back({"present",argument,result});
        mode=item.at("next_mode");active=item.at("next_active");return std::uint8_t(result);
      };
      services.fade_active=[&]{++active_queries;return active;};
      const auto result=halo2::vista_loading_thread_run(services);
      if(result!=row.at("result") || events!=row.at("events") ||
         mode!=row.at("final_mode") || active!=row.at("final_active") || !active_queries)
        throw std::runtime_error("Original loading scheduler mismatch in case "+std::to_string(count));
      ++count;
    }
    std::cout<<"PASS original Vista loading scheduler cases="<<count<<'\n';
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
