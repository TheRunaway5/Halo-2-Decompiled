#pragma once
struct Prefix {};
struct Missing {std::uint32_t address;std::size_t width;};
struct Memory final:SourcePhysicsContactMemory {
    mutable std::map<std::uint32_t,std::vector<std::byte>> banks;
    Json timeline=Json::array();mutable Json applied=Json::array(),reads=Json::array();
    mutable std::map<std::uint32_t,unsigned> occurrences;
    bool prefix{};unsigned ordinal{};mutable unsigned read_attempts{};
    auto locate(std::uint32_t address,std::size_t width)const {
        auto found=banks.upper_bound(address);if(found==banks.begin())throw Missing{address,width};--found;
        if(std::uint64_t(address-found->first)+width>found->second.size())throw Missing{address,width};return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(prefix && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
        reads.push_back(Json::array({address,out.size()}));const auto occurrence=++occurrences[address];
        for(const auto &change:timeline)if(change.at("on_read").at("address")==address && change.at("on_read").at("occurrence")==occurrence){
            for(const auto &edit:change.at("writes")){const auto data=bytes(edit.at("bytes"));const auto target=edit.at("address").get<unsigned>();const auto region=locate(target,data.size());std::copy(data.begin(),data.end(),region->second.begin()+target-region->first);}applied.push_back(change);
        }
    }
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("Speech envelope attempted retained publication");}
    explicit Memory(const Json &row){
        for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));timeline=row.at("input_timeline");
    }
};
