// Compile the preserved modern C++ parser itself, not a rewritten oracle.
#include "reference/account_client_before_rust_20261101/online_account.cpp.inc"
#include <iostream>
namespace {
using Json = nlohmann::json;
Json appearance(const std::optional<halo2::FrontendAppearance> &a) {
  if (!a)
    return nullptr;
  return std::bit_cast<std::array<std::uint8_t, 9>>(*a);
}
Json invites(const std::vector<halo2::OnlineClanInvite> &rows) {
  Json out = Json::array();
  for (const auto &r : rows)
    out.push_back({{"id", r.id}, {"name", r.name}});
  return out;
}
Json countdown(const std::optional<halo2::OnlineParty::Countdown> &c) {
  if (!c)
    return nullptr;
  return Json{{"serial", c->serial},
              {"seconds", c->seconds ? Json(*c->seconds) : Json(nullptr)},
              {"mode", c->mode},
              {"complete", c->complete},
              {"requester", c->requester}};
}
Json project(const halo2::OnlineAccountSnapshot &s, const std::string &kind) {
  if (kind == "social") {
    Json clan = nullptr;
    if (s.clan) {
      const auto &c = *s.clan;
      Json members = Json::array(), messages = Json::array();
      for (const auto &m : c.members)
        members.push_back({{"id", m.id},
                           {"name", m.name},
                           {"role", m.role},
                           {"self", m.self},
                           {"presence", unsigned(m.presence)}});
      for (const auto &m : c.messages)
        messages.push_back({{"id", m.id}, {"name", m.name}, {"body", m.body}});
      clan = {{"id", c.id},
              {"name", c.name},
              {"role", c.role},
              {"members", members},
              {"outgoing", invites(c.outgoing)},
              {"messages", messages}};
    }
    return Json{{"clan", clan}, {"invitations", invites(s.invitations)}};
  }
  if (kind == "friends") {
    const auto row = [](const halo2::OnlineFriend &f) {
      return Json{{"id", f.id},
                  {"name", f.name},
                  {"tag", f.tag},
                  {"presence", f.presence},
                  {"state", f.state},
                  {"party_joinable",
                   f.party_joinable ? Json(*f.party_joinable) : Json(nullptr)},
                  {"party_context", f.party_context}};
    };
    Json rows = Json::array();
    for (const auto &f : s.friends)
      rows.push_back(row(f));
    return Json{{"friend_tag", s.friend_tag},
                {"friends", rows},
                {"friends_available", s.friends_available},
                {"friends_message", s.friends_message},
                {"friend_search",
                 s.friend_search ? row(*s.friend_search) : Json(nullptr)}};
  }
  Json party = nullptr, invitations = Json::array();
  for (const auto &r : s.party_invitations)
    invitations.push_back(
        {{"id", r.id}, {"name", r.name}, {"version", r.version}});
  if (s.party) {
    const auto &p = *s.party;
    Json members = Json::array();
    for (const auto &m : p.members) {
      Json locals = Json::array();
      for (const auto &l : m.local_players)
        locals.push_back({{"name", l.name},
                          {"guest", l.guest},
                          {"appearance", appearance(l.appearance)}});
      members.push_back(
          {{"handle", m.handle},
           {"name", m.name},
           {"ready_playlist", m.ready_playlist},
           {"self", m.self},
           {"leader", m.leader},
           {"online", m.online},
           {"appearance", appearance(m.appearance)},
           {"postgame", m.postgame ? Json(*m.postgame) : Json(nullptr)},
           {"local_players", locals}});
    }
    party = {{"version", p.version},
             {"leader", p.leader},
             {"members", members},
             {"outgoing", invites(p.outgoing)},
             {"countdown_supported", p.countdown_supported},
             {"countdown", countdown(p.countdown)},
             {"privacy", p.privacy ? Json{{"mode", p.privacy->mode},
                                          {"revision", p.privacy->revision},
                                          {"context", p.privacy->context}}
                                   : Json(nullptr)}};
  }
  return Json{{"party", party}, {"party_invitations", invitations}};
}
} // namespace
int main() {
  std::string line;
  while (std::getline(std::cin, line)) {
    Json output;
    try {
      const auto input = Json::parse(line);
      const auto kind = input.at("kind").get<std::string>();
      const auto &root = input.at("root");
      halo2::OnlineAccountSnapshot state;
      Json value;
      if (kind == "countdown")
        value = countdown(halo2::parse_party_countdown(root));
      else {
        if (kind == "social")
          halo2::social(root, state);
        else if (kind == "friends")
          halo2::parse_friends(root, state);
        else if (kind == "party")
          halo2::parse_party(root, state);
        else
          throw std::runtime_error("Unknown replay kind");
        value = project(state, kind);
      }
      output = {{"accepted", true}, {"value", value}};
    } catch (const std::exception &) {
      output = {{"accepted", false}};
    }
    std::cout << output.dump() << '\n';
  }
}
