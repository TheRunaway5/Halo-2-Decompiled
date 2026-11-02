//! Party menu/text policy over cached native facts and ordered native reads.
//! Resource namespaces, real seat projection and primitive frame writes are IO.
//! This ports the replacement client's caller; it does not recover Live policy.
use crate::matchmaking_frontend_lobby_view as lobby;
use crate::matchmaking_frontend_party::Member;
use crate::matchmaking_frontend_party_flow::Controller;
pub type Result<T> = std::result::Result<T, String>;

#[derive(Clone, Debug, Default)]
pub struct Named {
    pub id: Vec<u8>,
    pub name: Vec<u8>,
}
#[derive(Clone, Debug, Default)]
pub struct Friend {
    pub name: Vec<u8>,
    pub state: Vec<u8>,
    pub presence: Vec<u8>,
}
#[derive(Clone, Debug, Default)]
pub struct ClanMember {
    pub name: Vec<u8>,
    pub is_self: bool,
    pub presence: u32,
}
#[derive(Clone, Debug, Default)]
pub struct Clan {
    pub role: u32,
    pub members: Vec<ClanMember>,
}
#[derive(Clone, Debug, Default)]
pub struct Party {
    pub leader: bool,
    pub countdown_supported: bool,
    pub members: Vec<Member>,
    pub outgoing: Vec<Named>,
}
#[derive(Clone, Debug, Default)]
pub struct Facts {
    pub screen: u32,
    pub context: u32,
    pub description: Vec<u8>,
    pub party: Option<Party>,
    pub invitations: Vec<Named>,
    pub friends: Vec<Friend>,
    pub clan: Option<Clan>,
    pub friends_message: Vec<u8>,
    pub party_message: Vec<u8>,
    pub account_message: Vec<u8>,
    pub voice_client: bool,
    pub voice_party: bool,
    pub voice_joined: bool,
    pub voice_connecting: bool,
    pub microphone_muted: bool,
    pub muted: Vec<Vec<u8>>,
}
#[derive(Clone, Debug, Default)]
pub struct Layout {
    pub original: bool,
    pub buttons: u64,
}
#[derive(Clone, Debug, Default)]
pub struct View {
    pub input: lobby::Input,
    pub protocol: i32,
    pub machines: i32,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Entry {
    pub label: Vec<u8>,
    pub destination: i32,
    pub value: i32,
}
impl Entry {
    fn new(label: impl AsRef<[u8]>, destination: i32, value: i32) -> Self {
        Self {
            label: label.as_ref().to_vec(),
            destination,
            value,
        }
    }
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Effect {
    ClearDescription,
    Title(Vec<u8>),
    Description(Vec<u8>),
    Append(Entry),
    Replace(Vec<Entry>),
    FormatError(Vec<u8>),
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Read {
    Header,
    Button,
    Authored,
    Localized,
}
pub trait Io {
    fn facts(&self) -> Result<Facts>;
    fn layout(&self, screen: u32, pane: u32, direct: bool, theme: bool) -> Result<Layout>;
    fn read(&self, original: bool, kind: Read, index: u32) -> Result<Vec<u8>>;
    fn members(&self) -> Result<Vec<Member>>;
    fn seconds(&self) -> Result<i32>;
    fn matchmaking(&self) -> Result<bool>;
    fn selected(&self) -> Result<Vec<u8>>;
    fn view(&self) -> Result<Option<View>>;
    fn has_stats(&self) -> Result<bool>;
    fn start_error(&self) -> Result<Vec<u8>>;
    fn expand(&self, text: &[u8]) -> Result<Vec<u8>>;
    fn effect(&self, effect: Effect) -> Result<()>;
    fn entries_empty(&self) -> Result<bool>;
}
fn joined(a: &[u8], b: &[u8]) -> Vec<u8> {
    let mut out = a.to_vec();
    out.extend_from_slice(b);
    out
}
fn append(io: &impl Io, label: impl AsRef<[u8]>, destination: i32, value: i32) -> Result<()> {
    io.effect(Effect::Append(Entry::new(label, destination, value)))
}
pub fn populate(flow: &Controller, io: &impl Io) -> Result<()> {
    io.effect(Effect::ClearDescription)?;
    let f = io.facts()?;
    if f.screen == 31 && f.context == 45 {
        io.effect(Effect::Title(b"INVITE TO PARTY".to_vec()))?;
        if flow.state().tab == 0 {
            io.effect(Effect::Description(f.friends_message))?;
            for (i, p) in f.friends.iter().enumerate() {
                if p.state == b"friend" {
                    let suffix = if p.presence == b"playing" {
                        b" - In match".as_slice()
                    } else if p.presence == b"offline" {
                        b" - Offline".as_slice()
                    } else {
                        b" - ONLINE".as_slice()
                    };
                    append(io, joined(&p.name, suffix), -36, i as u32 as i32)?;
                }
            }
            if io.entries_empty()? {
                append(io, b"No friends added yet", -82, 0)?;
            }
        } else {
            io.effect(Effect::Description(if f.clan.is_some() {
                b"Select an online clan member to invite.".to_vec()
            } else {
                b"Join a clan to invite its members.".to_vec()
            }))?;
            if let Some(clan) = f.clan {
                for (i, p) in clan.members.iter().enumerate() {
                    // Exact native OnlineClanPresence words: Online=2, Playing=3.
                    if !p.is_self && matches!(p.presence, 2 | 3) {
                        append(
                            io,
                            joined(&p.name, if p.presence == 3 { b" - In match" } else { b"" }),
                            -37,
                            i as u32 as i32,
                        )?;
                    }
                }
            }
            if io.entries_empty()? {
                append(io, b"No available online clan members", -82, 0)?;
            }
        }
        append(io, b"Invite by Friend Tag", 167, 9)?;
        return append(io, b"Back", -5, 0);
    }
    match f.screen {
        14 => {
            let layout = io.layout(14, 2, false, true)?;
            let title = io.read(layout.original, Read::Header, 0)?;
            io.effect(Effect::Title(title))?;
            if f.party.is_some() {
                if layout.buttons < 2 {
                    return io.effect(Effect::FormatError(
                        b"Matchmaking lobby has no authored Start/Game Setup buttons".to_vec(),
                    ));
                }
                let start = io.read(layout.original, Read::Button, 0)?;
                let setup = io.read(layout.original, Read::Button, 1)?;
                io.effect(Effect::Replace(vec![
                    Entry::new(start, -17, 0),
                    Entry::new(setup, 23, 0),
                ]))?;
            } else {
                io.effect(Effect::Replace(vec![
                    Entry::new(b"Create Party", -33, 0),
                    Entry::new(b"Party Invitations", 26, 2),
                ]))?;
            }
        }
        26 => {
            io.effect(Effect::Title(b"ONLINE".to_vec()))?;
            if let Some(party) = f.party {
                for (i, p) in io.members()?.iter().enumerate() {
                    append(
                        io,
                        &p.name,
                        if p.controller == -2 { -82 } else { 31 },
                        i as u32 as i32,
                    )?;
                }
                for (i, p) in party.outgoing.iter().enumerate() {
                    append(
                        io,
                        joined(b"Invited: ", &p.name),
                        31,
                        2000u32.wrapping_add(i as u32) as i32,
                    )?;
                }
            } else {
                append(io, b"Create Party", -33, 0)?;
            }
            for (i, p) in f.invitations.iter().enumerate() {
                append(
                    io,
                    joined(b"Invite: ", &p.name),
                    31,
                    1000u32.wrapping_add(i as u32) as i32,
                )?;
            }
        }
        216 => {
            io.effect(Effect::Title(b"PARTY OPTIONS".to_vec()))?;
            let Some(party) = f.party else {
                return io.effect(Effect::Replace(vec![
                    Entry::new(b"Create Party", -33, 0),
                    Entry::new(b"Party Invitations", 26, 0),
                ]));
            };
            append(io, b"Playlists", 219, 0)?;
            if party.leader {
                append(io, b"Invite Player", 31, 45)?;
                if f.clan.is_some_and(|c| c.role >= 1) {
                    append(io, b"Invite Clan To Party", -33, 1)?;
                }
            }
            append(io, b"Party Members", 26, 0)?;
            if f.voice_client {
                append(
                    io,
                    if f.voice_party && (f.voice_joined || f.voice_connecting) {
                        b"Leave Party Voice".as_slice()
                    } else {
                        b"Join Party Voice".as_slice()
                    },
                    -34,
                    0,
                )?;
                if f.voice_party && f.voice_joined {
                    append(
                        io,
                        if f.microphone_muted {
                            b"Unmute Microphone".as_slice()
                        } else {
                            b"Mute Microphone".as_slice()
                        },
                        -34,
                        1,
                    )?;
                }
            }
            append(io, b"Stop Searching", -33, 6)?;
            append(io, b"Leave Party", -33, 5)?;
        }
        31 => {
            io.effect(Effect::Title(b"PARTY".to_vec()))?;
            // Preserve the two native getter observations after the title write.
            let target = flow.state().target;
            let kind = flow.state().kind;
            if kind == 1 {
                if let Some(p) = f.invitations.iter().find(|p| p.id == target) {
                    io.effect(Effect::Title(p.name.clone()))?;
                    io.effect(Effect::Replace(vec![
                        Entry::new(b"Accept Party Invite", -33, 2),
                        Entry::new(b"Decline Party Invite", -33, 3),
                        Entry::new(b"Block Future Invites", -33, 4),
                    ]))?;
                }
            } else if kind == 2 && f.party.is_some() {
                if let Some(p) = f
                    .party
                    .as_ref()
                    .unwrap()
                    .outgoing
                    .iter()
                    .find(|p| p.id == target)
                {
                    io.effect(Effect::Title(p.name.clone()))?;
                    io.effect(Effect::Replace(vec![Entry::new(
                        b"Cancel Party Invite",
                        -33,
                        7,
                    )]))?;
                }
            } else if let Some(party) = f.party {
                if let Some(p) = party.members.iter().find(|p| p.handle == target) {
                    io.effect(Effect::Title(p.name.clone()))?;
                    if p.is_self {
                        io.effect(Effect::Replace(vec![
                            Entry::new(b"Playlists", 219, 0),
                            Entry::new(b"Leave Party", -33, 5),
                        ]))?;
                    } else if party.leader {
                        io.effect(Effect::Replace(vec![
                            Entry::new(b"Make Party Leader", -33, 8),
                            Entry::new(b"Remove From Party", -33, 9),
                        ]))?;
                    }
                    if !p.is_self && f.voice_party && f.voice_joined {
                        append(
                            io,
                            if f.muted.contains(&p.handle) {
                                b"Unmute Voice".as_slice()
                            } else {
                                b"Mute Voice".as_slice()
                            },
                            -34,
                            2,
                        )?;
                    }
                }
            }
            if io.entries_empty()? {
                append(io, b"Back", -5, 0)?;
            }
        }
        _ => {}
    }
    Ok(())
}
pub fn text(index: u32, io: &impl Io) -> Result<Vec<u8>> {
    let f = io.facts()?;
    if f.screen == 31 && f.context == 45 {
        return Ok(if index == 0 {
            f.description
        } else {
            Vec::new()
        });
    }
    if f.screen == 14 {
        let original = io.layout(14, 2, false, true)?.original;
        if index == 0 && io.seconds()? >= 0 {
            if let Some(view) = io.view()? {
                if let Some(id) = lobby::presentation(&view.input).status {
                    return io.expand(&io.read(original, Read::Localized, id)?);
                }
            }
        }
        if index == 0
            && io.matchmaking()?
            && f.party.as_ref().is_some_and(|p| !p.countdown_supported)
            && !io.selected()?.is_empty()
        {
            return io.start_error();
        }
        return match index {
            0 => Ok(f.party_message),
            1 | 2 | 3 | 48 => io.expand(&io.read(original, Read::Authored, index)?),
            10 => {
                if let Some(view) = io.view()? {
                    if let Some(id) = lobby::presentation(&view.input).privacy {
                        return io.read(original, Read::Localized, id);
                    }
                }
                Ok(Vec::new())
            }
            43 => {
                if let Some(p) = f
                    .party
                    .and_then(|p| p.members.into_iter().find(|p| p.leader))
                {
                    let mut text = io.read(original, Read::Authored, index)?;
                    if let Some(at) = text.windows(3).position(|w| w == [0xee, 0x90, 0x96]) {
                        text.splice(at..at + 3, p.name);
                    }
                    return Ok(text);
                }
                Ok(Vec::new())
            }
            44 if f.party.is_some() => io.expand(&io.read(original, Read::Authored, index)?),
            45 if io.has_stats()? => io.read(original, Read::Authored, 45),
            47 => {
                if let Some(view) = io.view()? {
                    let help = lobby::help(2, view.protocol, view.machines, view.input.privacy, 6);
                    if help.visible {
                        return io.read(original, Read::Localized, help.string);
                    }
                }
                Ok(Vec::new())
            }
            _ => Ok(Vec::new()),
        };
    }
    if f.screen == 26 {
        io.layout(26, 2, true, false)?;
        return match index {
            0 => Ok(b"PARTY".to_vec()),
            1 | 3 => io.read(false, Read::Authored, index),
            2 => Ok(if f.party.is_some() {
                b"Party members".to_vec()
            } else {
                b"Party invitations".to_vec()
            }),
            4 if f.party.is_some() => Ok(io.members()?.len().to_string().into_bytes()),
            6 if f.voice_party && f.voice_joined => Ok(if f.microphone_muted {
                b"MUTE".to_vec()
            } else {
                b"VOICE".to_vec()
            }),
            _ => Ok(Vec::new()),
        };
    }
    Ok(if index != 0 {
        Vec::new()
    } else if f.party_message.is_empty() {
        f.account_message
    } else {
        f.party_message
    })
}
