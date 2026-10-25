#include "halo2/prepared_original_sound_bank.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("Supply the owned prepared-bank control manifest");
        std::ifstream input(argv[1]);const auto cases=nlohmann::json::parse(input);
        unsigned rejected{};
        for(const auto& row:cases.at("rejections")) {
            bool failed=false;
            try {const auto bank=halo2::PreparedOriginalSoundBank::load(row.at("directory").get<std::string>(),row.at("sha256").get<std::string>());}
            catch(const std::exception&){failed=true;}
            if(!failed)throw std::runtime_error("Malformed original-bank input admitted: "+row.at("name").get<std::string>());
            ++rejected;
        }
        const auto expected=cases.at("expected_refusals").get<unsigned>();
        if(rejected!=expected||(expected!=10&&expected!=12))throw std::runtime_error("Incomplete prepared-bank refusal cohort");
        const auto bank=halo2::PreparedOriginalSoundBank::load(cases.at("directory").get<std::string>(),cases.at("sha256").get<std::string>());
        if(bank->metadata().permutations.size()!=6)throw std::runtime_error("Actual AP bank lost its six permutations");
        if(cases.at("has_initial_flags").get<bool>()&&bank->metadata().used!=std::vector<std::byte>{std::byte{0}})
            throw std::runtime_error("Actual AP bank lost its authenticated initial permutation state");
        for(const auto index:{std::pair{1u,0u},std::pair{0u,6u}}) {
            bool failed=false;try{(void)bank->encoded(index.first,index.second);}catch(const halo2::FormatError&){failed=true;}
            if(!failed)throw std::runtime_error("Original-bank pitch/permutation bounds were ignored");
        }
        std::cout<<"PASS native prepared-bank refusals="<<rejected<<" original_permutations=6 boundary_controls=2\n";
        return 0;
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
