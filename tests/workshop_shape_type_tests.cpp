#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_contact_agent_factory.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace halo2;
int main(int argc,char** argv)try {
    if(argc!=2)throw std::runtime_error("Supply unchanged original shape-type reference");
    std::ifstream file(argv[1]);std::string line;unsigned count{};
    if(!std::getline(file,line) || line!="# unchanged TU1.5 21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("Shape-type reference lost its original image identity");
    while(std::getline(file,line)) {
        if(line.empty() || line.front()=='#')continue;
        std::istringstream row(line);unsigned owner{},expected{};std::string bytes;
        if(!(row>>std::hex>>owner>>std::dec>>expected>>bytes))throw std::runtime_error("Incomplete original shape type");
        auto actual=native_tu15_activation_type(owner);
        if(owner==0xa9520)actual=native_tu15_contact_child_class_23();
        if(owner==0xab4b0)actual=native_tu15_contact_child_class_24();
        if(!actual || *actual!=expected)throw std::runtime_error("Missing or differing native shape type owner="+std::to_string(owner));
        ++count;
    }
    if(count!=17 || native_tu15_activation_type(0xa9be1) ||
        native_tu15_activation_type(0xad711) || native_tu15_activation_type(0xffffffff))
        throw std::runtime_error("Shape types lost their finite original owner boundary");
    std::cout<<"PASS seventeen complete original constant getters; unknown owners refused\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
