use crate::ffi;
use crate::matchmaking_frontend_party_flow::FrontendPartyFlow;
use halo2_online_protocol::matchmaking_frontend_party::Member;
use halo2_online_protocol::matchmaking_frontend_party_view as core;

fn member(p: ffi::PartyViewMember) -> Member {
    Member {
        handle: p.handle,
        name: p.name,
        is_self: p.is_self,
        leader: p.leader,
        controller: p.controller,
        ..Member::default()
    }
}
fn named(p: ffi::PartyViewNamed) -> core::Named {
    core::Named {
        id: p.id,
        name: p.name,
    }
}
fn entry(e: core::Entry) -> ffi::PartyViewEntry {
    ffi::PartyViewEntry {
        label: e.label,
        destination: e.destination,
        value: e.value,
    }
}
struct Io<'a>(&'a ffi::NativeFrontendPartyViewIo);
impl core::Io for Io<'_> {
    fn facts(&self) -> core::Result<core::Facts> {
        let f = self.0.party_view_facts().map_err(|e| e.to_string())?;
        Ok(core::Facts {
            screen: f.screen,
            context: f.context,
            description: f.description,
            party: f.party.then(|| core::Party {
                leader: f.leader,
                countdown_supported: f.countdown_supported,
                members: f.members.into_iter().map(member).collect(),
                outgoing: f.outgoing.into_iter().map(named).collect(),
            }),
            invitations: f.invitations.into_iter().map(named).collect(),
            friends: f
                .friends
                .into_iter()
                .map(|p| core::Friend {
                    name: p.name,
                    state: p.state,
                    presence: p.presence,
                })
                .collect(),
            clan: f.clan.then(|| core::Clan {
                role: f.clan_role,
                members: f
                    .clan_members
                    .into_iter()
                    .map(|p| core::ClanMember {
                        name: p.name,
                        is_self: p.is_self,
                        presence: p.presence,
                    })
                    .collect(),
            }),
            friends_message: f.friends_message,
            party_message: f.party_message,
            account_message: f.account_message,
            voice_client: f.voice_client,
            voice_party: f.voice_party,
            voice_joined: f.voice_joined,
            voice_connecting: f.voice_connecting,
            microphone_muted: f.microphone_muted,
            muted: f.muted.into_iter().map(|p| p.value).collect(),
        })
    }
    fn layout(
        &self,
        screen: u32,
        pane: u32,
        direct: bool,
        theme: bool,
    ) -> core::Result<core::Layout> {
        self.0
            .party_view_layout(screen, pane, direct, theme)
            .map(|p| core::Layout {
                original: p.original,
                buttons: p.buttons,
            })
            .map_err(|e| e.to_string())
    }
    fn read(&self, original: bool, kind: core::Read, index: u32) -> core::Result<Vec<u8>> {
        self.0
            .party_view_read(original, kind as u32, index)
            .map_err(|e| e.to_string())
    }
    fn members(&self) -> core::Result<Vec<Member>> {
        self.0
            .party_view_members()
            .map(|p| p.into_iter().map(member).collect())
            .map_err(|e| e.to_string())
    }
    fn seconds(&self) -> core::Result<i32> {
        self.0.party_view_seconds().map_err(|e| e.to_string())
    }
    fn matchmaking(&self) -> core::Result<bool> {
        self.0.party_view_matchmaking().map_err(|e| e.to_string())
    }
    fn selected(&self) -> core::Result<Vec<u8>> {
        self.0.party_view_selected().map_err(|e| e.to_string())
    }
    fn view(&self) -> core::Result<Option<core::View>> {
        self.0
            .party_view_snapshot()
            .map(|v| {
                v.present.then(|| core::View {
                    input: crate::matchmaking_frontend_lobby_view::input(&v.input),
                    protocol: v.protocol,
                    machines: v.machines,
                })
            })
            .map_err(|e| e.to_string())
    }
    fn has_stats(&self) -> core::Result<bool> {
        self.0.party_view_has_stats().map_err(|e| e.to_string())
    }
    fn start_error(&self) -> core::Result<Vec<u8>> {
        self.0.party_view_start_error().map_err(|e| e.to_string())
    }
    fn expand(&self, text: &[u8]) -> core::Result<Vec<u8>> {
        self.0.party_view_expand(text).map_err(|e| e.to_string())
    }
    fn entries_empty(&self) -> core::Result<bool> {
        self.0.party_view_entries_empty().map_err(|e| e.to_string())
    }
    fn effect(&self, e: core::Effect) -> core::Result<()> {
        let mut out = ffi::PartyViewEffect {
            kind: 0,
            text: Vec::new(),
            entry: ffi::PartyViewEntry {
                label: Vec::new(),
                destination: 0,
                value: 0,
            },
            entries: Vec::new(),
        };
        match e {
            core::Effect::ClearDescription => {}
            core::Effect::Title(text) => {
                out.kind = 1;
                out.text = text;
            }
            core::Effect::Description(text) => {
                out.kind = 2;
                out.text = text;
            }
            core::Effect::Append(e) => {
                out.kind = 3;
                out.entry = entry(e);
            }
            core::Effect::Replace(entries) => {
                out.kind = 4;
                out.entries = entries.into_iter().map(entry).collect();
            }
            core::Effect::FormatError(text) => {
                out.kind = 5;
                out.text = text;
            }
        }
        self.0.party_view_effect(&out).map_err(|e| e.to_string())
    }
}
pub fn frontend_party_view_populate(
    controller: &FrontendPartyFlow,
    io: &ffi::NativeFrontendPartyViewIo,
) -> core::Result<()> {
    core::populate(controller.controller(), &Io(io))
}
pub fn frontend_party_view_text(
    index: u32,
    io: &ffi::NativeFrontendPartyViewIo,
) -> core::Result<Vec<u8>> {
    core::text(index, &Io(io))
}
