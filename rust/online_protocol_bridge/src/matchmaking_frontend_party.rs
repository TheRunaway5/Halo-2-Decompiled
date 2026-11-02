use crate::ffi;
use halo2_online_protocol::matchmaking_frontend_party as core;
pub(super) fn member(p: &ffi::PartyMemberData) -> core::Member {
    core::Member {
        handle: p.handle.clone(),
        name: p.name.clone(),
        ready: p.ready.clone(),
        is_self: p.is_self,
        leader: p.leader,
        online: p.online,
        locals: p
            .locals
            .iter()
            .map(|s| core::Local {
                name: s.name.clone(),
                guest: s.guest,
                appearance: s.appearance_present.then_some(s.appearance),
            })
            .collect(),
        controller: p.controller,
        guest: p.guest,
        appearance: p.appearance_present.then_some(p.appearance),
        postgame: p.postgame_present.then_some(p.postgame),
    }
}
fn encoded(p: core::Member) -> ffi::PartyMemberData {
    ffi::PartyMemberData {
        handle: p.handle,
        name: p.name,
        ready: p.ready,
        is_self: p.is_self,
        leader: p.leader,
        online: p.online,
        locals: p
            .locals
            .into_iter()
            .map(|s| ffi::PartyLocalData {
                name: s.name,
                guest: s.guest,
                appearance_present: s.appearance.is_some(),
                appearance: s.appearance.unwrap_or_default(),
            })
            .collect(),
        controller: p.controller,
        guest: p.guest,
        appearance_present: p.appearance.is_some(),
        appearance: p.appearance.unwrap_or_default(),
        postgame_present: p.postgame.is_some(),
        postgame: p.postgame.unwrap_or_default(),
    }
}
struct Io<'a>(&'a ffi::NativeFrontendPartyIo);
impl core::Io for Io<'_> {
    fn primary_appearance(&self) -> core::Result<core::Appearance> {
        self.0
            .party_primary_appearance()
            .map(|p| p.bytes)
            .map_err(|e| e.to_string())
    }
    fn local_players(&self) -> core::Result<Vec<core::Seat>> {
        self.0
            .party_local_players()
            .map(|rows| {
                rows.into_iter()
                    .map(|p| core::Seat {
                        controller: p.controller,
                        name: p.name,
                        appearance: p.appearance,
                    })
                    .collect()
            })
            .map_err(|e| e.to_string())
    }
    fn authorization(&self, controller: u32) -> core::Result<Option<bool>> {
        self.0
            .party_authorization(controller)
            .map(|a| a.present.then_some(a.authorized))
            .map_err(|e| e.to_string())
    }
}
pub fn frontend_party_members(
    present: bool,
    party: &[ffi::PartyMemberData],
    io: &ffi::NativeFrontendPartyIo,
) -> core::Result<Vec<ffi::PartyMemberData>> {
    let party = party.iter().map(member).collect::<Vec<_>>();
    core::members(present.then_some(party.as_slice()), &Io(io))
        .map(|rows| rows.into_iter().map(encoded).collect())
}
pub fn frontend_party_column(
    party: &[ffi::PartyMemberData],
    voice: &ffi::PartyVoiceData,
    screen: u32,
    row: u32,
    column: u32,
) -> Vec<u8> {
    let party = party.iter().map(member).collect::<Vec<_>>();
    let voice = core::Voice {
        party: voice.party,
        joined: voice.joined,
        transmitting: voice.transmitting,
        muted: voice.muted.iter().map(|s| s.value.clone()).collect(),
        talking: voice.talking.iter().map(|s| s.value.clone()).collect(),
    };
    core::column(&party, &voice, screen, row, column)
}

pub(crate) fn notification(f: &ffi::PartyNotificationData) -> core::Notification {
    core::Notification {
        authenticated: f.authenticated,
        busy: f.busy,
        message_empty: f.message_empty,
        invitations: f.invitations.iter().map(|s| s.value.clone()).collect(),
        party: f.party.then(|| {
            f.members
                .iter()
                .map(|p| core::Membership {
                    handle: p.handle.clone(),
                    local_count: p.local_count,
                })
                .collect()
        }),
    }
}
pub fn frontend_party_sounds(
    before: &ffi::PartyNotificationData,
    after: &ffi::PartyNotificationData,
) -> Vec<u32> {
    core::sounds(&notification(before), &notification(after))
}
pub fn frontend_party_notice(f: &ffi::PartyNoticeFactsData) -> ffi::PartyNoticeData {
    let f = core::NoticeFacts {
        authenticated: f.authenticated,
        signed_in: f.signed_in.clone(),
        invitations: f
            .invitations
            .iter()
            .map(|p| core::Invitation {
                id: p.id.clone(),
                name: p.name.clone(),
            })
            .collect(),
        dismissed: f.dismissed.iter().map(|s| s.value.clone()).collect(),
        busy: f.busy,
        party: f.party,
        launch: f.launch,
        status: f.status,
    };
    match core::notice(&f) {
        Some(n) => ffi::PartyNoticeData {
            present: true,
            id: n.invitation.id,
            name: n.invitation.name,
            busy: n.busy,
            can_accept: n.can_accept,
        },
        None => ffi::PartyNoticeData {
            present: false,
            id: Vec::new(),
            name: Vec::new(),
            busy: false,
            can_accept: false,
        },
    }
}
