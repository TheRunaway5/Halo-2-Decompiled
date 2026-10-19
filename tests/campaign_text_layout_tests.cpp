#include "halo2/campaign_text_layout.hpp"
#include "halo2/campaign_title.hpp"
#include <iostream>
#include <fstream>
using namespace halo2;
void check(bool okay,const char* message){if(!okay)throw std::runtime_error(message);}
int main(int argc,char** argv)try {
    if(argc!=4)throw std::runtime_error("Supply original text-control, MCC opening-title and Xbox opening-title fixtures");
    std::ifstream fixture(argv[1]);check(bool(fixture),"Missing original text-control fixture");
    unsigned c,value,found,count=0;
    while(fixture>>c>>value>>found){const auto actual=campaign_text_layout::escape(c);
        check(actual.codepoint==value && unsigned(actual.found)==found,"Text-control output differs from original TU1.5 execution");++count;}
    check(fixture.eof()&&count==7,"Incomplete original text-control fixture");
    RetailFont font;font.ascent=10;font.descent=2;font.leading=1;
    for(unsigned c=32;c<127;++c){FontGlyph g{};g.advance=6;font.glyphs.emplace(c,g);}
    std::ifstream mcc_fixture(argv[2]);std::string header,hex;
    check(bool(std::getline(mcc_fixture,header)) && header=="MCC 00a_introduction f6916833eedc5efba6250ff2bfd0d0547318572901776069ca043c0c8b4329b8", "Wrong MCC title fixture owner");
    std::vector<std::byte> bank;
    while(mcc_fixture>>hex)for(std::size_t i=0;i<hex.size();i+=2)
        bank.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
    check(bank.size()==72,"Incomplete actual MCC opening-title bank");
    const auto titles=read_campaign_titles(bank,true,"scenarios\\solo\\00a_introduction\\00a_introduction");
    const auto mcc_rows=campaign_text_layout::lines(font,"COVENANT HOLY CITY, HIGH CHARITY|nNINTH AGE OF RECLAMATION",
        float(titles[0].bounds[3]-titles[0].bounds[1]));
    std::cout<<"Actual MCC title rows="<<mcc_rows.size()<<" width="<<titles[0].bounds[3]-titles[0].bounds[1]<<'\n';
    check(mcc_rows.size()==2,"Actual MCC title wraps into a vertical column of characters");
    for(const auto& t:titles)check(t.bounds==std::array<std::int16_t,4>{350,0,480,595},"Opening title bounds differ from the original Xbox record");
    std::ifstream xbox_fixture(argv[3]);
    check(bool(std::getline(xbox_fixture,header)) && header=="Xbox 00a_introduction 5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28","Wrong Xbox title fixture owner");
    std::vector<std::byte> original_bank;
    while(xbox_fixture>>hex)for(std::size_t i=0;i<hex.size();i+=2)
        original_bank.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
    check(original_bank.size()==72,"Incomplete original Xbox title bank");
    const auto original=read_campaign_titles(original_bank,false,{});
    for(unsigned row=0;row<2;++row)check(titles[row].bounds==original[row].bounds &&
        titles[row].font==original[row].font && titles[row].justification==original[row].justification &&
        titles[row].color==original[row].color && titles[row].shadow==original[row].shadow &&
        titles[row].fade_in==original[row].fade_in && titles[row].up_time==original[row].up_time &&
        titles[row].fade_out==original[row].fade_out,"MCC projection differs from original Xbox title fields");
    check(original[0].text==0x1000087b && original[1].text==0x1000087c,"Original text IDs changed");
    check(titles[0].text==0x100009a2 && titles[1].text==0x100009a3,"MCC title text IDs were replaced");
    for(const auto& t:titles)check(t.font==2 && t.justification==1 && t.color==0xffffff && !t.shadow &&
        t.fade_in==1 && t.up_time==5 && t.fade_out==1,"Title projection changed authored style or timing");
    const auto literal=read_campaign_titles(bank,false,"scenarios\\solo\\00a_introduction\\00a_introduction");
    check(literal[0].bounds==std::array<std::int16_t,4>{225,1024,40,80},"Non-MCC title source was projected");
    check(read_campaign_titles(bank,true,"custom")[0].bounds==literal[0].bounds,"Foreign scenario was projected");
    for(std::size_t i=0;i<bank.size();++i) {
        auto changed=bank;changed[i]^=std::byte{1};
        const auto custom=read_campaign_titles(changed,true,"scenarios\\solo\\00a_introduction\\00a_introduction");
        const auto unprojected=read_campaign_titles(changed,false,{});
        for(unsigned row=0;row<2;++row)check(custom[row].bounds==unprojected[row].bounds,"Changed MCC title bank admitted a retail projection");
    }
    // Actual untouched Xbox title1000087B, bounds350/0/480/595 and justification1.
    const auto rows=campaign_text_layout::lines(font,"COVENANT HOLY CITY, HIGH CHARITY|nNINTH AGE OF RECLAMATION",595);
    check(rows.size()==2,"Xbox |n title break is printed as literal text instead of producing two lines");
    check(std::string(rows[0].text.begin(),rows[0].text.end())=="COVENANT HOLY CITY, HIGH CHARITY" &&
        std::string(rows[1].text.begin(),rows[1].text.end())=="NINTH AGE OF RECLAMATION","Location title text changed");
    check(campaign_text_layout::center(0,0,595,100)==50 && campaign_text_layout::center(1,0,595,100)==545 &&
        campaign_text_layout::center(2,0,595,100)==297.5f,"Xbox left/right/center justification ordinals are swapped");
    check(campaign_text_layout::characters("A||B")==std::vector<unsigned>({'A','|','B'}),"Escaped literal bar changed");
    check(campaign_text_layout::characters("|l|r|c|n|t")==std::vector<unsigned>({0xe405,0xe406,0xe407,13,9}),"Original recognized text-control mapping changed");
    check(campaign_text_layout::characters("|x|")==std::vector<unsigned>({'|','x','|'}),"Unknown or trailing marker consumed text");
    check(campaign_text_layout::lines(font,"A\r\nB",595).size()==2,"CR/LF produced an extra or missing source line");
    check(campaign_text_layout::lines(font,"A\nB",595).size()==1,"Original ignored LF became an authored newline");
    const auto white=campaign_text_layout::title_color(0xffffff),near_white=campaign_text_layout::title_color(0xfffffe);
    check(white.x==.8f && white.y==.8f && white.z==.8f && near_white.x==1 && near_white.z==254.f/255,
        "Original all-white title limit changed another authored color");
    std::cout<<"PASS actual Xbox location-title control text, line boundaries and left/right/center ordinals\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
