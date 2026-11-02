std::optional<UiVariant> matched_variant(const FrontendContent &content, const OnlineMatch &match) {
  const auto &name=match.variant;
  if(match.settings) {
    constexpr std::array<int,7> types{2,3,7,4,1,9,8};
    const auto found=std::find(types.begin(),types.end(),source_game_variant_type(*match.settings));
    if(found==types.end()||!matchmaking_variant_valid(match.settings->bytes))return std::nullopt;
    UiVariant result{name,unsigned(found-types.begin()),{}};result.source_settings=match.settings;
    return result;
  }
  const auto variant=std::find_if(content.variants.begin(),content.variants.end(),[&](const auto &v){
    std::string_view value=v.name;
    if(value.ends_with(" \xee\x80\x88"))value.remove_suffix(4);
    return value==name;
  });
  if(variant!=content.variants.end())return *variant;
  const auto extra=std::find_if(content.bundled_variants.begin(),content.bundled_variants.end(),[&](const auto &v){return v.name==name;});
  if(extra==content.bundled_variants.end())return std::nullopt;
  return *extra;
}
