#include "halo2/gameplay_font_source.hpp"
#include "halo2/cache.hpp"
#include "halo2/frontend.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <openssl/evp.h>
#include <span>
#include <string_view>

namespace halo2 {
namespace {
struct FontFile { std::string_view name; std::size_t bytes; std::string_view sha256; std::uint64_t fingerprint; };
struct FontLookup { unsigned character, record; };
struct FontMetric { unsigned record, character, x, y, width, height, advance; int bearing_x, bearing_y; };
struct FontKerning { unsigned first, second; int value; };
struct FontProfile {
    std::string_view name, source_sha256;
    std::uint64_t fingerprint;
    int ascent, descent, leading, horizontal_offset;
    unsigned default_record;
    std::span<const FontLookup> lookup;
    std::span<const FontMetric> metrics;
    std::span<const FontKerning> kerning;
};
#include "ui/gameplay_font_reference.inc"
struct ActionButtonMetric {
    unsigned character, record;
    FontGlyph raw, authored;
    FontGlyphSample sample;
};
struct ActionButtonProfile {
    unsigned selector;
    std::uint64_t fingerprint;
    std::span<const ActionButtonMetric> metrics;
};
#include "ui/hud/mcc_action_button_metrics.inc"

std::vector<std::string> table_names(const std::filesystem::path& directory) {
    std::ifstream file(directory/"font_table.txt");
    if(!file)throw FormatError("Missing gameplay font table: "+directory.string());
    std::vector<std::string> result;
    for(std::string name;std::getline(file,name);) {
        if(!name.empty() && name.back()=='\r')name.pop_back();
        result.push_back(std::move(name));
        if(result.size()>12)throw FormatError("Excessive gameplay font selectors");
    }
    return result;
}
std::vector<std::byte> font_bytes(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file || file.tellg()<0x40400 || file.tellg()>16*1024*1024)
        throw FormatError("Invalid gameplay font resource: "+path.string());
    std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));
    file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))
        throw FormatError("Truncated gameplay font resource");
    return bytes;
}
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> hash{};unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),hash.data(),&size,EVP_sha256(),nullptr)!=1 || size!=hash.size())
        throw FormatError("Cannot authenticate gameplay font resource");
    constexpr char hex[]="0123456789abcdef";std::string result;
    for(const auto value:hash){result+=hex[value>>4];result+=hex[value&15];}
    return result;
}
const FontFile& mcc_file(std::string_view name) {
    const auto found=std::find_if(mcc_files.begin(),mcc_files.end(),[&](const auto& row){return row.name==name;});
    if(found==mcc_files.end())throw FormatError("Unknown MCC gameplay font owner");
    return *found;
}
std::shared_ptr<const std::vector<std::byte>> authenticate_mcc_file(
    const std::filesystem::path& directory,const FontFile& owner) {
    auto bytes=font_bytes(directory/owner.name);
    // Retain one authenticated payload per fixed profile, rather than hashing
    // the same files for every HUD selector. Still read and compare every byte
    // on each validation, including unused fonts and files with restored dates.
    static std::mutex mutex;
    static std::array<std::shared_ptr<const std::vector<std::byte>>,mcc_files.size()> verified;
    const auto index=std::size_t(&owner-mcc_files.data());
    std::shared_ptr<const std::vector<std::byte>> previous;
    {std::lock_guard lock(mutex);previous=verified.at(index);}
    if(previous && bytes==*previous)return previous;
    if(bytes.size()!=owner.bytes || Bytes(bytes).u32(0x200)!=0xf0000001 || digest(bytes)!=owner.sha256)
        throw FormatError("MCC gameplay font is outside the authenticated layout profile: "+std::string(owner.name));
    auto accepted=std::make_shared<const std::vector<std::byte>>(std::move(bytes));
    {std::lock_guard lock(mutex);verified.at(index)=accepted;}
    return accepted;
}
const FontProfile* profile(unsigned selector) {
    const auto found=std::find_if(profiles.begin(),profiles.end(),[&](const auto& row){return row.name==original_names.at(selector);});
    return found==profiles.end()?nullptr:&*found;
}
void validate_source(const GameplayFontSource& source) {
    const auto current=load_gameplay_font_source(source.directory);
    if(current.domain!=source.domain || current.directory!=source.directory || current.names!=source.names)
        throw FormatError("Gameplay font source changed after selection");
}
void validate_selector(const GameplayFontSource& source,unsigned selector) {
    if(selector>=11 || selector>=source.names.size())throw FormatError("Gameplay font selector outside original table");
}
} // namespace

std::filesystem::path gameplay_font_directory(const Cache& cache) {
    return cache.is_mcc()?cache.shared_directory().parent_path()/"h2_fonts":cache.shared_directory()/"fonts";
}
GameplayFontSource load_gameplay_font_source(const std::filesystem::path& requested) {
    auto directory=requested;
    bool legacy_mcc=false;
    if(!std::filesystem::exists(directory) && directory.filename()=="fonts" &&
       directory.parent_path().filename()=="h2_maps_win64_dx11" &&
       directory.parent_path().parent_path().filename()=="halo2" &&
       std::filesystem::is_directory(directory.parent_path())) {
        directory=directory.parent_path().parent_path()/"h2_fonts";
        legacy_mcc=true;
    }
    auto names=table_names(directory);
    const bool mcc_directory=directory.filename()=="h2_fonts" && directory.parent_path().filename()=="halo2";
    if(names.size()==11 && !legacy_mcc && !mcc_directory) {
        for(const auto& name:names)
            if(name.find_first_of("/\\")!=name.npos || name=="..")throw FormatError("Invalid gameplay font owner");
        return {GameplayFontDomain::Original,directory,std::move(names)};
    }
    if(!mcc_directory ||
       names.size()!=mcc_names.size() || !std::equal(names.begin(),names.end(),mcc_names.begin()))
        throw FormatError("Unrecognized MCC gameplay font table");
    for(const auto& owner:mcc_files)authenticate_mcc_file(directory,owner);
    return {GameplayFontDomain::Mcc,directory,std::move(names)};
}
namespace {
RetailFont project_gameplay_font(RetailFont result,const GameplayFontSource& source,unsigned selector) {
    if(source.domain==GameplayFontDomain::Original)return result;
    const auto& owner=mcc_file(source.names[selector]);
    if(result.file_fingerprint!=owner.fingerprint)
        throw FormatError("MCC font changed during decoding");
    const auto* layout=profile(selector);
    if(!layout)return result; // Fixedsys9 and MSLCD14 are byte-identical.
    // Only physical glyph locations are needed below. Moving them avoids a
    // complete glyph-map and atlas copy just before replacing logical metrics.
    const auto sampled_glyphs=std::move(result.glyphs);
    result.ascent=layout->ascent;result.descent=layout->descent;result.leading=layout->leading;
    result.glyph_record_indices.assign(65536,layout->default_record);
    for(const auto& row:layout->lookup)result.glyph_record_indices.at(row.character)=row.record;
    result.kerning.clear();
    for(const auto& row:layout->kerning)result.kerning[{row.first,row.second}]=row.value;
    std::map<unsigned,const FontMetric*> metrics;
    for(const auto& row:layout->metrics)metrics.emplace(row.record,&row);
    result.glyphs.clear();result.sampled_glyphs.clear();
    for(unsigned c=32;c<65536;++c) {
        const auto& logical=*metrics.at(result.glyph_record_indices[c]);
        const auto& physical=sampled_glyphs.at(logical.character);
        if(logical.width && logical.height && (!physical.width || !physical.height))
            throw FormatError("MCC font lacks original glyph raster owner");
        result.glyphs.emplace_hint(result.glyphs.end(),c,FontGlyph{logical.x,logical.y,logical.width,logical.height,
            int(logical.advance),logical.bearing_x,logical.bearing_y});
        if(logical.width && logical.height)
            result.sampled_glyphs.emplace_hint(result.sampled_glyphs.end(),c,FontGlyphSample{physical.x,physical.y,physical.width,physical.height});
    }
    return result;
}
} // namespace
RetailFont load_gameplay_font(const GameplayFontSource& source,unsigned selector) {
    validate_selector(source,selector);
    validate_source(source);
    return project_gameplay_font(load_retail_font(source.directory/source.names[selector]),source,selector);
}
struct GameplayFontBank::Impl {
    std::filesystem::path requested;
    GameplayFontSource source;
    std::mutex mutex;
    std::map<std::string,std::shared_ptr<const std::vector<std::byte>>> encoded;
    std::array<std::shared_ptr<const RetailFont>,11> raw,projected;
    explicit Impl(const std::filesystem::path& directory)
        :requested(directory.lexically_normal()),source(load_gameplay_font_source(directory)) {
        // Decode the exact authenticated snapshot, even if source files are
        // replaced while a match is loading. Never trust a filename or date.
        if(source.domain==GameplayFontDomain::Mcc)
            for(const auto& owner:mcc_files)
                encoded.emplace(owner.name,authenticate_mcc_file(source.directory,owner));
    }
    std::shared_ptr<const RetailFont> raw_font(unsigned selector) {
        validate_selector(source,selector);
        auto& retained=raw.at(selector);
        if(!retained) {
            auto decoded=source.domain==GameplayFontDomain::Mcc?
                decode_retail_font(*encoded.at(source.names[selector])):
                load_retail_font(source.directory/source.names[selector]);
            if(source.domain==GameplayFontDomain::Mcc &&
               decoded.file_fingerprint!=mcc_file(source.names[selector]).fingerprint)
                throw FormatError("MCC font changed during decoding");
            retained=std::make_shared<const RetailFont>(std::move(decoded));
        }
        return retained;
    }
};
GameplayFontBank::GameplayFontBank(const std::filesystem::path& directory)
    :impl_(std::make_unique<Impl>(directory)) {}
GameplayFontBank::~GameplayFontBank()=default;
const GameplayFontSource& GameplayFontBank::source()const noexcept {return impl_->source;}
bool GameplayFontBank::matches_directory(const std::filesystem::path& directory)const {
    return directory.lexically_normal()==impl_->requested;
}
std::shared_ptr<const RetailFont> GameplayFontBank::raw(unsigned selector) {
    std::lock_guard lock(impl_->mutex);
    return impl_->raw_font(selector);
}
std::shared_ptr<const RetailFont> GameplayFontBank::font(unsigned selector) {
    std::lock_guard lock(impl_->mutex);
    validate_selector(impl_->source,selector);
    auto& retained=impl_->projected.at(selector);
    if(!retained) {
        const auto raw=impl_->raw_font(selector);
        if(impl_->source.domain==GameplayFontDomain::Original || !profile(selector))retained=raw;
        else retained=std::make_shared<const RetailFont>(project_gameplay_font(*raw,impl_->source,selector));
    }
    return retained;
}
int gameplay_font_horizontal_offset(const GameplayFontSource& source,unsigned selector) {
    validate_selector(source,selector);validate_source(source);
    if(source.domain==GameplayFontDomain::Mcc)if(const auto* layout=profile(selector))return layout->horizontal_offset;
    // Preserve the original signed word, independently of atlas placement.
    std::ifstream file(source.directory/source.names[selector],std::ios::binary);
    file.seekg(0x20a);std::array<unsigned char,2> bytes{};
    if(!file.read(reinterpret_cast<char*>(bytes.data()),2))throw FormatError("Missing original font horizontal offset");
    return std::bit_cast<std::int16_t>(std::uint16_t(bytes[0]|(unsigned(bytes[1])<<8)));
}
void require_mcc_gameplay_font_sample(const GameplayFontSource& source,unsigned selector,
                                     const std::filesystem::path& sample) {
    validate_selector(source,selector);validate_source(source);
    if(source.domain!=GameplayFontDomain::Mcc)throw FormatError("MCC font sample has no native source owner");
    const auto& owner=mcc_file(source.names[selector]);
    const auto bytes=font_bytes(sample);
    if(bytes.size()!=owner.bytes || digest(bytes)!=owner.sha256)
        throw FormatError("Custom MCC gameplay font redraw needs an explicit original-metric projection");
}
namespace {
std::size_t apply_authored_button_metrics(RetailFont& font,unsigned selector,
                                        std::span<const ActionButtonProfile> profiles) {
    const auto profile=std::find_if(profiles.begin(),profiles.end(),
        [&](const auto& row){return row.selector==selector;});
    if(profile==profiles.end() || font.file_fingerprint!=profile->fingerprint ||
       font.glyph_record_indices.size()!=65536 || font.atlas.width!=2049 ||
       !font.atlas.height || font.atlas.height>4096 || font.atlas.format!=11 ||
       font.atlas.layers!=1 || font.atlas.depth || !font.atlas.mips.empty() ||
       font.atlas.pixels.size()!=std::size_t(font.atlas.width)*font.atlas.height*4)
        throw FormatError("Unqualified MCC action-button metric owner");
    const auto matches=[](const FontGlyph& a,const FontGlyph& b) {
        return a.width==b.width && a.height==b.height && a.advance==b.advance &&
            a.bearing_x==b.bearing_x && a.bearing_y==b.bearing_y;
    };
    std::array<FontGlyph*,20> targets{};
    bool raw=true,authored=true;
    for(std::size_t i=0;i<profile->metrics.size();++i) {
        const auto& row=profile->metrics[i];
        const auto glyph=font.glyphs.find(row.character);
        const auto sample=font.sampled_glyphs.find(row.character);
        if(font.glyph_record_indices[row.character]!=row.record ||
           std::count(font.glyph_record_indices.begin(),font.glyph_record_indices.end(),row.record)!=1 ||
           glyph==font.glyphs.end() || sample==font.sampled_glyphs.end() ||
           sample->second.x!=row.sample.x || sample->second.y!=row.sample.y ||
           sample->second.width!=row.sample.width || sample->second.height!=row.sample.height ||
           row.sample.x>font.atlas.width || row.sample.y>font.atlas.height ||
           row.sample.width>font.atlas.width-row.sample.x ||
           row.sample.height>font.atlas.height-row.sample.y)
            throw FormatError("Unqualified MCC action-button glyph or authored sample");
        targets[i]=&glyph->second;
        raw&=matches(glyph->second,row.raw);
        authored&=matches(glyph->second,row.authored);
    }
    if(!raw && !authored)throw FormatError("Partial or custom MCC action-button metric state");
    if(authored)return 0;
    // All map lookups, validation and possible throws precede these primitive
    // assignments. Atlas coordinates and every sampled rectangle stay intact.
    std::size_t changed{};
    for(std::size_t i=0;i<profile->metrics.size();++i) {
        auto& glyph=*targets[i];const auto& row=profile->metrics[i].authored;
        changed+=!matches(glyph,row);
        glyph.width=row.width;glyph.height=row.height;glyph.advance=row.advance;
        glyph.bearing_x=row.bearing_x;glyph.bearing_y=row.bearing_y;
    }
    return changed;
}
} // namespace
std::size_t apply_mcc_action_button_metrics(RetailFont& font,unsigned selector) {
    return apply_authored_button_metrics(font,selector,action_button_profiles);
}
std::size_t apply_mcc_menu_button_metrics(RetailFont& font,unsigned selector) {
    return apply_authored_button_metrics(font,selector,menu_button_profiles);
}
} // namespace halo2
