#include "halo2/model.hpp"
#include "halo2/frontend_model_locals.hpp"
#include "halo2/frontend_model_pose.hpp"
#include "halo2/transform.hpp"
#include <bit>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>

using namespace halo2;
namespace {
void require(bool value, const char *message) { if (!value) throw FormatError(message); }
std::uint32_t hex(std::istream &input) {
  std::string word;
  require(bool(input >> word), "incomplete captured pose row");
  std::size_t used;
  const auto value = std::stoul(word, &used, 16);
  require(used == word.size() && value <= 0xfffffffful, "invalid captured float word");
  return std::uint32_t(value);
}
void number(std::ostream &output, double value) {
  if (std::isfinite(value)) output << value;
  else output << "null";
}
}
int main(int argc, char **argv) {
  try {
    require(argc == 4 || argc == 5 || argc==6, "captured pose comparison needs fixture, maps, report path, optional stages/shared mode");
    const bool shared=argc==6 && std::string(argv[5])=="--shared";
    require(argc!=6 || shared,"invalid captured pose mode");
    std::ifstream fixture(argv[1]);
    require(bool(fixture), "captured original menu pose fixture missing");
    Cache cache(std::filesystem::path(argv[2])/"mainmenu.map");
    struct Stage { std::uint32_t flags{};std::vector<std::uint32_t> mask;std::optional<std::uint32_t> aim; };
    std::unordered_map<std::string,Stage> stages;
    if(argc>=5) {
      std::ifstream input(argv[4]);require(bool(input),"captured pose stage fixture missing");
      for(std::string line;std::getline(input,line);) {
        if(line.empty() || line.front()=='#')continue;
        std::istringstream row(line);std::string label;unsigned count;Stage stage;
        require(bool(row>>label),"invalid captured stage label");stage.flags=hex(row);
        require(bool(row>>count) && (count==0 || count==8),"invalid captured stage mask count");
        stage.mask.resize(count);for(auto&word:stage.mask)word=hex(row);
        require(bool(stage.flags&(1u<<30))==!stage.mask.empty(),"captured deferred flag/mask mismatch");
        std::string slot;if(row>>slot){std::istringstream raw(slot);stage.aim=hex(raw);}
        require(stages.emplace(label,std::move(stage)).second,"duplicate captured pose stage");
      }
    }
    std::ofstream report(argv[3]);
    require(bool(report), "cannot write captured pose comparison report");
    report << std::setprecision(10) << "{\n\"scope\":\"Exact paused original clip/frame/root versus native world-node bank; no offset or tolerance tuning\",\n\"objects\":[\n";
    std::size_t objects = 0, total_words = 0, differing_words = 0;
    for (std::string row; std::getline(fixture, row);) {
      if (row.empty() || row.front() == '#') continue;
      std::istringstream input(row);
      std::string label;
      unsigned clip_index, count;
      require(bool(input >> label), "invalid captured object label");
      const auto unit = hex(input), graph = hex(input);
      require(bool(input >> clip_index), "missing captured clip index");
      const auto frame_word = hex(input);
      const auto frame = std::bit_cast<float>(frame_word);
      require(bool(input >> count) && count > 0 && count < 256, "invalid captured node count");
      std::array<std::uint32_t, 13> root_words;
      for (auto &word : root_words) word = hex(input);
      const auto root = std::bit_cast<Transform>(root_words);
      std::vector<std::array<std::uint32_t, 13>> expected(count);
      for (auto &node : expected) for (auto &word : node) word = hex(input);
      std::string extra;
      require(!(input >> extra), "extra captured pose data");
      const auto model = cache.load_model(unit, false);
      const auto clip = cache.load_animation(graph, clip_index);
      std::vector<Transform> actual;
      const Stage*stage=nullptr;
      if(argc>=5) {
        const auto found=stages.find(label);require(found!=stages.end(),"captured pose stage absent");
        stage=&found->second;
        const auto primary=cache.load_frontend_model_primary_animation(graph,clip_index);
        const auto grip=cache.load_frontend_model_grip(unit);
        if(shared) {
          require(stage->mask.empty() && stage->aim,"shared comparator needs real post-admission bank and captured aim slot");
          const auto combat=cache.load_frontend_model_animation(unit,false);
          const auto ui=cache.load_frontend_model_animation(unit,true);
          require(combat && ((combat->source_graph==graph && combat->source_index==clip_index) ||
              (ui && ui->source_graph==graph && ui->source_index==clip_index)),"captured primary is outside proven menu clips");
          const bool ui_mode=ui && ui->source_graph==graph && ui->source_index==clip_index &&
              (ui->source_graph!=combat->source_graph || ui->source_index!=combat->source_index);
          const auto ik=cache.load_frontend_model_weapon_ik(unit,ui_mode?0x020001b3u:0x06000086u);
          const auto deferred=cache.load_frontend_model_deferred_mask(unit);
          const auto weapon=cache.load_frontend_model_weapon(unit);require(bool(weapon),"captured menu weapon absent");
          const auto weapon_model=cache.load_model(*weapon,false);
          const auto weapon_clip=cache.load_initialized_object_idle(*weapon);
          require(!weapon_clip || weapon_clip->frame_count==1,"captured held weapon clock unresolved");
          const auto attachment=select_weapon_attachment(cache.load_weapon_attachment(unit,*weapon,0),{},{});
          const auto startup=cache.load_initialized_object_idle(unit);
          require(bool(startup),"captured unit startup idle absent");
          const auto startup_primary=cache.load_frontend_model_primary_animation(startup->source_graph,startup->source_index);
          const auto retained_attachment=frontend_model_initialize_weapon_attachment(model,root,*startup,startup_primary,attachment);
          const auto aim=std::bit_cast<FirstPersonAimBlend>(*stage->aim);
          actual=frontend_model_pose(model,root,clip,frame,deferred,grip?&*grip:nullptr,
              &weapon_model,weapon_clip?&*weapon_clip:nullptr,&attachment,&ik,source_aim_blend_weight(aim),&primary,&retained_attachment).body;
        } else {
          auto locals=frontend_model_locals(model,clip,frame,stage->mask,&primary);
          if(grip)apply_frontend_model_grip(model,locals,*grip,stage->mask);
          actual=pose_model_locals_nodes_world(model,root,locals);
        }
      } else actual=pose_model_nodes_world_frame(model, root, &clip, frame);
      require(actual.size() == count, "native/original model node counts differ");
      std::size_t object_errors = 0, node_errors = 0;
      double max_error = 0, max_position_error = 0;
      if (objects) report << ",\n";
      report << "{\"label\":" << std::quoted(label) << ",\"definition\":\"" << std::hex << unit
             << "\",\"graph\":\"" << graph << "\",\"frame_bits\":\"" << frame_word
             << "\",\"clip_index\":" << std::dec << clip_index << ",\"frame\":" << frame
             << ",\"stage\":\"" << (shared?"production-composite-after-deferred":stage?(stage->mask.empty()?"after-deferred-sampling":"deferred-object-update"):"primary-only-unrestricted") << "\"";
      if(stage)report<<",\"object_flags\":\""<<std::hex<<stage->flags<<std::dec<<"\"";
      report << ",\"nodes\":[\n";
      for (unsigned i = 0; i < count; ++i) {
        const auto words = std::bit_cast<std::array<std::uint32_t, 13>>(actual[i]);
        unsigned errors = 0;
        double node_max = 0, position_max = 0;
        if (i) report << ",\n";
        report << "{\"index\":" << i << ",\"name_id\":\"" << std::hex << model.bones[i].name
               << "\",\"expected_words\":[";
        for (unsigned j = 0; j < 13; ++j)
          report << (j ? ",\"" : "\"") << std::setw(8) << std::setfill('0') << expected[i][j] << "\"";
        report << "],\"actual_words\":[";
        for (unsigned j = 0; j < 13; ++j) {
          report << (j ? ",\"" : "\"") << std::setw(8) << words[j] << "\"";
          if (words[j] != expected[i][j]) ++errors;
          const double error = std::abs(double(std::bit_cast<float>(words[j]))-
                                        double(std::bit_cast<float>(expected[i][j])));
          if (std::isfinite(error)) {
            node_max = std::max(node_max, error);
            if (j >= 10) position_max = std::max(position_max, error);
          }
        }
        report << std::dec << "],\"different_words\":" << errors << ",\"max_abs_error\":";
        number(report, node_max);
        report << ",\"max_translation_abs_error\":";
        number(report, position_max);
        report << "}";
        object_errors += errors;
        node_errors += errors != 0;
        max_error = std::max(max_error, node_max);
        max_position_error = std::max(max_position_error, position_max);
      }
      report << "],\"node_count\":" << count << ",\"different_nodes\":" << node_errors
             << ",\"different_words\":" << object_errors << ",\"max_abs_error\":";
      number(report, max_error);
      report << ",\"max_translation_abs_error\":";
      number(report, max_position_error);
      report << "}";
      std::cout << label << ": " << node_errors << '/' << count << " nodes, " << object_errors
                << '/' << count*13 << " float words differ; max=" << max_error
                << ", translation=" << max_position_error << '\n';
      ++objects;
      total_words += count*13;
      differing_words += object_errors;
    }
    require(objects > 0, "captured pose fixture has no objects");
    report << "],\"object_count\":" << objects << ",\"total_words\":" << total_words
           << ",\"different_words\":" << differing_words << ",\"byte_exact\":"
           << (differing_words ? "false" : "true") << "}\n";
    report.close();
    require(bool(report), "captured pose report write failed");
    return differing_words ? 2 : 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
