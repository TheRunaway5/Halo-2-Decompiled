#include "halo2/campaign_text_layout.hpp"
#include <iostream>
#include <fstream>
using namespace halo2;
void check(bool okay,const char* message){if(!okay)throw std::runtime_error(message);}
int main(int argc,char** argv)try {
    if(argc!=2)throw std::runtime_error("Supply the unchanged-original text-control fixture");
    std::ifstream fixture(argv[1]);check(bool(fixture),"Missing original text-control fixture");
    unsigned c,value,found,count=0;
    while(fixture>>c>>value>>found){const auto actual=campaign_text_layout::escape(c);
        check(actual.codepoint==value && unsigned(actual.found)==found,"Text-control output differs from original TU1.5 execution");++count;}
    check(fixture.eof()&&count==7,"Incomplete original text-control fixture");
    RetailFont font;font.ascent=10;font.descent=2;font.leading=1;
    for(unsigned c=32;c<127;++c){FontGlyph g{};g.advance=6;font.glyphs.emplace(c,g);}
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
