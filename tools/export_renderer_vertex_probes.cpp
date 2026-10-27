// Offline native-emitter differential inputs. Never linked into the game.
#include "halo2/menu_world_vertex_source.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc,char**argv)try {
  using namespace halo2;namespace fs=std::filesystem;using Json=nlohmann::json;
  if(argc!=3)throw FormatError("export_renderer_vertex_probes INPUT_MATRIX OUTPUT");
  std::ifstream input(argv[1]);Json document;input>>document;
  fs::path output=argv[2];fs::create_directories(output);Json rows=Json::array();
  for(const auto& row:document.at("rows")) {
    if(!row.value("basic_raw_semantics",false)&&!row.value("raw_renderer_extension",false))continue;
    VertexProgram program;const auto instructions=row.at("vertex_instructions").get<std::vector<VertexInstruction>>();
    if(instructions.empty()||instructions.size()>program.instructions.size())throw FormatError("invalid probe program extent");
    program.count=instructions.size();program.relative_constants=row.at("relative_constants").get<bool>();
    std::copy(instructions.begin(),instructions.end(),program.instructions.begin());
    const auto classification=row.at("class_index").get<unsigned>();
    const auto semantics=row.at("semantics").get<std::vector<unsigned>>();
    const bool skinned=classification>=2;
    bool old_rejected=false;
    try{(void)menu_world_material_vertex_source(program,skinned);}catch(const FormatError&){old_rejected=true;}
    const auto source=menu_world_renderer_vertex_source(program,skinned);
    const auto identity=row.at("program_sha256").get<std::string>();
    std::string full_key=identity+"-c"+std::to_string(classification);
    for(const auto semantic:semantics)full_key+='-'+std::to_string(semantic);
    const auto path=output/(full_key+".glsl");
    std::ofstream stream(path);stream<<source.helpers<<"\nvoid runNativeVertex(){\n"<<source.statements<<"}\n";
    if(!stream)throw FormatError("cannot save native vertex probe source");
    const auto components=menu_world_renderer_vertex_input_components(program,skinned);
    std::array<unsigned,16> widened{};std::copy(components.begin(),components.end(),widened.begin());
    rows.push_back({{"program_sha256",identity},{"full_key",full_key},{"class_index",classification},{"semantics",semantics},{"source",path.filename().string()},
      {"old_corridor_rejected",old_rejected},{"consumed_inputs",source.consumed_inputs},
      {"consumed_constants",source.consumed_constants},{"consumed_input_components",widened},
      {"default_texture_w_mask",menu_world_vertex_default_texture_w_mask(program)},
      {"operations",source.helper_operations}});
  }
  std::ofstream report(output/"sources.json");report<<Json{{"schema","HALO2_RENDERER_VERTEX_ARITHMETIC_PROBES_V1"},{"entries",rows}}.dump(2)<<'\n';
  std::cout<<rows.size()<<" actual renderer vertex programs emitted\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
