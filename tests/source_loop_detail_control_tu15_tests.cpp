#include "halo2/source_loop_detail_control_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char**argv)try{
#if !((defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__)))
    std::cout<<"Original detail arithmetic requires its qualified x87 owner\n";return 77;
#endif
    if(argc!=2)throw std::runtime_error("Supply original complete detail arithmetic fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("Detail arithmetic authority differs");
    const auto scalar=[](const auto& input){return std::bit_cast<float>(input.template get<std::uint32_t>());};
    unsigned failures{},index{};
    for(const auto& row:fixture.at("positions")){
        auto seed=row.at(0).get<std::uint32_t>();const auto& b=row.at(1);
        const auto result=source_loop_detail_offset_tu15({{scalar(b.at(0)),scalar(b.at(1))},
            {scalar(b.at(2)),scalar(b.at(3))},{scalar(b.at(4)),scalar(b.at(5))}},seed);
        const auto output=nlohmann::json::array({std::bit_cast<std::uint32_t>(result.x),
            std::bit_cast<std::uint32_t>(result.y),std::bit_cast<std::uint32_t>(result.z)});
        if(output!=row.at(2) || seed!=row.at(3))if(failures++<8)
            std::cerr<<"Offset "<<index<<" expected "<<row.dump()<<" got "<<output.dump()<<" seed="<<seed<<'\n';
        ++index;
    }
    index=0;
    for(const auto& row:fixture.at("spatial_requests")){
        auto seed=row.at(0).get<std::uint32_t>();const auto& b=row.at(1);
        const auto hex=row.at(2).get<std::string>();std::array<std::byte,68> parent{};
        for(unsigned n=0;n<68;++n)parent[n]=std::byte(std::stoul(hex.substr(n*2,2),nullptr,16));
        const auto value=source_loop_detail_spatial_tu15(parent,{{scalar(b.at(0)),scalar(b.at(1))},
            {scalar(b.at(2)),scalar(b.at(3))},{scalar(b.at(4)),scalar(b.at(5))}},seed);
        const auto output=nlohmann::json::array({std::bit_cast<std::uint32_t>(value.control),
            std::bit_cast<std::uint32_t>(value.gain_db),std::bit_cast<std::uint32_t>(value.position.x),
            std::bit_cast<std::uint32_t>(value.position.y),std::bit_cast<std::uint32_t>(value.position.z)});
        if(output!=row.at(3) || seed!=row.at(4))if(failures++<8)
            std::cerr<<"Spatial request "<<index<<" expected "<<row.dump()<<" got "<<output.dump()<<" seed="<<seed<<'\n';
        ++index;
    }
    index=0;
    for(const auto& row:fixture.at("deadlines")){
        auto seed=row.at(4).get<std::uint32_t>();const auto& b=row.at(1);
        const auto result=source_loop_detail_deadline_tu15({scalar(b.at(0)),scalar(b.at(1))},row.at(3).get<bool>()?4u:0u,
            std::bit_cast<std::int32_t>(row.at(2).get<std::uint32_t>()),seed,row.at(0));
        if(std::bit_cast<std::uint32_t>(result)!=row.at(5) || seed!=row.at(6))if(failures++<8)
            std::cerr<<"Deadline "<<index<<" expected "<<row.dump()<<" got "<<std::bit_cast<std::uint32_t>(result)<<" seed="<<seed<<'\n';
        ++index;
    }
    if(failures)throw std::runtime_error(std::to_string(failures)+" original detail arithmetic cases differ");
    std::cout<<"PASS original detail offsets and distinct deadline paths\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
