//! Complete modern native clan caller and account/voice binding policy.
//! Original Live providers and full Xbox callers remain separate evidence gaps.
use crate::matchmaking_frontend_account_flow::Entry;
use crate::matchmaking_frontend_party_flow::{Command as PartyCommand, Controller};
pub type Result<T> = std::result::Result<T, String>;
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Command {
    pub action: Vec<u8>,
    pub value: Vec<u8>,
    pub role: u32,
    pub confirmed: bool,
}
#[derive(Clone, Debug, Default)]
pub struct Member {
    pub id: Vec<u8>,
    pub name: Vec<u8>,
    pub role: u32,
    pub is_self: bool,
    pub presence: i32,
}
#[derive(Clone, Debug, Default)]
pub struct Invite {
    pub id: Vec<u8>,
    pub name: Vec<u8>,
}
#[derive(Clone, Debug, Default)]
pub struct Message {
    pub name: Vec<u8>,
    pub body: Vec<u8>,
}
#[derive(Clone, Debug, Default)]
pub struct Clan {
    pub name: Vec<u8>,
    pub role: u32,
    pub members: Vec<Member>,
    pub outgoing: Vec<Invite>,
    pub messages: Vec<Message>,
}
#[derive(Clone, Debug, Default)]
pub struct Account {
    pub message: Vec<u8>,
    pub clan: Option<Clan>,
    pub invitations: Vec<Invite>,
    pub party: Option<bool>,
}
#[derive(Clone, Debug, Default)]
pub struct Voice {
    pub joined: bool,
    pub connecting: bool,
    pub microphone_muted: bool,
    pub transmitting: bool,
    pub muted: Vec<Vec<u8>>,
    pub talking: Vec<Vec<u8>>,
    pub message: Vec<u8>,
    pub revision: u64,
}
#[derive(Clone, Debug, Default)]
pub struct Frame {
    pub screen: u32,
    pub context: u32,
    pub selected: u32,
    pub description: Vec<u8>,
    pub entries: Vec<Entry>,
}
#[derive(Clone, Debug, Default)]
pub struct Facts {
    pub account_client: bool,
    pub voice_client: bool,
    pub social_client: bool,
    pub same_account: bool,
    pub candidate_account: bool,
    pub local_accounts: Vec<bool>,
    pub account: Account,
    pub voice: Voice,
    pub frame: Option<Frame>,
    pub frames: Vec<Frame>,
}
#[derive(Clone, Copy, Debug)]
pub enum Target {
    View,
    Top,
}
#[derive(Clone, Debug)]
pub enum Field {
    Title(Vec<u8>),
    Description(Vec<u8>),
    Append(Entry),
    Replace(Vec<Entry>),
    Select(u32),
}
#[derive(Clone, Debug)]
pub enum Effect {
    Frame(Target, Field),
    Open(u32, u32),
    Pop,
    Back,
    ClearKeyboard,
    Revision,
    Unavailable(Vec<u8>),
    SignOutLocal(usize),
    ClearLocalAccounts,
    VoiceLeave,
    AccountSignOut,
    InstallAccount,
    ResetAccount,
    ClearSounds,
    BindSocial,
    CreateVoice(bool),
    DropVoice,
    ResetVoice(u64),
    InstallVoice,
    PushToTalk(bool),
    VoiceJoin,
    Microphone(bool),
    Mute(Vec<u8>, bool),
}
pub trait Io {
    fn facts(&self) -> Result<Facts>;
    fn effect(&self, effect: Effect) -> Result<()>;
    fn command(&self, command: &Command) -> Result<bool>;
    fn party_command(&self, command: &PartyCommand) -> Result<()>;
    fn update_account(&self) -> Result<()>;
    fn select(&self, index: u32) -> Result<()>;
    fn prepare_pane(&self, screen: u32, pane: u32) -> Result<()>;
    fn localized(&self, index: u32) -> Result<Vec<u8>>;
    fn localized_direct(&self, screen: u32, pane: u32, index: u32) -> Result<Vec<u8>>;
}
pub fn screen(screen: u32, context: u32) -> bool {
    crate::matchmaking_frontend_account_flow::clan_screen(screen, context)
}
fn view(f: &Facts) -> Result<&Frame> {
    f.frame
        .as_ref()
        .ok_or_else(|| "Native clan frame owner absent".into())
}
fn top(f: &Facts) -> Result<&Frame> {
    f.frames
        .last()
        .ok_or_else(|| "Native clan stack owner absent".into())
}
fn joined(p: &[&[u8]]) -> Vec<u8> {
    p.concat()
}
fn row(label: impl Into<Vec<u8>>, destination: i32, value: i32) -> Entry {
    Entry {
        label: label.into(),
        destination,
        value,
    }
}
fn field(io: &impl Io, target: Target, value: Field) -> Result<()> {
    io.effect(Effect::Frame(target, value))
}
fn append(io: &impl Io, label: impl Into<Vec<u8>>, destination: i32, value: i32) -> Result<()> {
    field(
        io,
        Target::View,
        Field::Append(row(label, destination, value)),
    )
}
fn role(value: u32) -> Result<&'static [u8]> {
    [&b"Peon"[..], b"Member", b"Staff", b"Overlord"]
        .get(value as usize)
        .copied()
        .ok_or_else(|| "Native clan role is outside the produced role table".into())
}
fn member<'a>(account: &'a Account, target: &[u8]) -> Option<&'a Member> {
    account
        .clan
        .as_ref()?
        .members
        .iter()
        .find(|p| p.id == target)
}
pub fn bind_account(flow: &Controller, io: &impl Io) -> Result<()> {
    let f = io.facts()?;
    if f.same_account {
        if f.candidate_account && !f.voice_client {
            io.effect(Effect::CreateVoice(true))?;
            io.effect(Effect::ResetVoice(u64::MAX))?;
        }
        return Ok(());
    }
    let count = io.facts()?.local_accounts.len();
    for i in 0..count {
        if io.facts()?.local_accounts[i] {
            io.effect(Effect::SignOutLocal(i))?;
        }
    }
    io.effect(Effect::ClearLocalAccounts)?;
    if io.facts()?.voice_client {
        io.effect(Effect::VoiceLeave)?;
    }
    if io.facts()?.account_client {
        io.effect(Effect::AccountSignOut)?;
    }
    io.effect(Effect::InstallAccount)?;
    io.effect(Effect::ResetAccount)?;
    flow.set_account_name(vec![]);
    flow.set_friend_join_serial(0);
    io.effect(Effect::ClearSounds)?;
    flow.clear_dismissed();
    if io.facts()?.social_client {
        io.effect(Effect::BindSocial)?;
    }
    if io.facts()?.account_client {
        io.effect(Effect::CreateVoice(false))?;
    } else {
        io.effect(Effect::DropVoice)?;
    }
    flow.set_voice_party(false);
    io.effect(Effect::ResetVoice(0))
}
pub fn bind_voice(flow: &Controller, io: &impl Io) -> Result<()> {
    if io.facts()?.voice_client {
        io.effect(Effect::VoiceLeave)?;
    }
    io.effect(Effect::InstallVoice)?;
    flow.set_voice_party(false);
    io.effect(Effect::ResetVoice(0))
}
pub fn push_to_talk(io: &impl Io, held: bool) -> Result<()> {
    if !io.facts()?.voice_client {
        return Ok(());
    }
    let held = held && view(&io.facts()?)?.screen != 167;
    io.effect(Effect::PushToTalk(held))
}
pub fn populate(flow: &Controller, io: &impl Io) -> Result<()> {
    let f = io.facts()?;
    let frame = view(&f)?;
    let state = flow.state();
    let target = member(&f.account, &state.clan_target);
    match frame.screen {
        26 => {
            field(io, Target::View, Field::Title(b"ONLINE".to_vec()))?;
            field(io, Target::View, Field::Description(vec![]))?;
            if let Some(clan) = &f.account.clan {
                for p in &clan.members {
                    let label = joined(&[&p.name, if p.is_self { b" (you)" } else { b"" }]);
                    let index = view(&io.facts()?)?.entries.len() as i32;
                    append(io, label, 31, index)?;
                }
                for (i, p) in clan.outgoing.iter().enumerate() {
                    append(
                        io,
                        joined(&[b"Invited: ", &p.name]),
                        31,
                        (2000u32.wrapping_add(i as u32)) as i32,
                    )?;
                }
            } else {
                append(io, b"Create Clan", 167, 4)?;
                for (i, p) in f.account.invitations.iter().enumerate() {
                    append(
                        io,
                        joined(&[b"Invitation: ", &p.name]),
                        31,
                        (1000u32.wrapping_add(i as u32)) as i32,
                    )?;
                }
            }
        }
        217 => {
            field(io, Target::View, Field::Title(b"CLAN OPTIONS".to_vec()))?;
            field(io, Target::View, Field::Description(vec![]))?;
            let Some(clan) = &f.account.clan else {
                return append(io, b"Create Clan", 167, 4);
            };
            if clan.role >= 1 {
                append(io, b"Send Clan Invite", 167, 7)?;
            }
            if clan.role >= 1 && f.account.party == Some(true) {
                append(io, b"Invite Clan To Party", -33, 1)?;
            }
            append(io, b"Read Clan Messages", 211, 41)?;
            if clan.role >= 1 {
                append(io, b"Message Clan", 167, 8)?;
            }
            if f.voice_client {
                append(
                    io,
                    if !flow.state().voice_party && (f.voice.joined || f.voice.connecting) {
                        &b"Leave Clan Voice"[..]
                    } else {
                        b"Join Clan Voice"
                    },
                    -32,
                    0,
                )?;
                if !flow.state().voice_party && f.voice.joined {
                    append(
                        io,
                        if f.voice.microphone_muted {
                            &b"Unmute Microphone"[..]
                        } else {
                            b"Mute Microphone"
                        },
                        -32,
                        1,
                    )?;
                }
            }
            append(io, b"Leave Clan", -30, 3)?;
        }
        31 => {
            field(io, Target::View, Field::Title(b"CLAN".to_vec()))?;
            field(io, Target::View, Field::Description(vec![]))?;
            if state.clan_target_kind == 1 {
                if let Some(p) = f
                    .account
                    .invitations
                    .iter()
                    .find(|p| p.id == state.clan_target)
                {
                    field(io, Target::View, Field::Title(p.name.clone()))?;
                    field(
                        io,
                        Target::View,
                        Field::Replace(vec![
                            row(b"Accept Clan Invite", -30, 0),
                            row(b"Decline Clan Invite", -30, 1),
                            row(b"Block Future Invites", -30, 2),
                        ]),
                    )?;
                }
            } else if state.clan_target_kind == 2 {
                if let Some(p) = f
                    .account
                    .clan
                    .as_ref()
                    .and_then(|c| c.outgoing.iter().find(|p| p.id == state.clan_target))
                {
                    field(io, Target::View, Field::Title(p.name.clone()))?;
                    field(
                        io,
                        Target::View,
                        Field::Replace(vec![row(b"Cancel Clan Invite", -30, 4)]),
                    )?;
                }
            } else if let (Some(p), Some(clan)) = (target, &f.account.clan) {
                field(io, Target::View, Field::Title(p.name.clone()))?;
                if p.is_self {
                    append(io, b"Leave Clan", -30, 3)?;
                } else if clan.role >= 2 && clan.role > p.role {
                    append(io, b"Change Clan Role", 236, 0)?;
                    append(io, b"Remove From Clan", -30, 5)?;
                }
                if !p.is_self && !flow.state().voice_party && f.voice.joined {
                    let muted = f.voice.muted.contains(&p.id);
                    append(
                        io,
                        if muted {
                            &b"Unmute Voice"[..]
                        } else {
                            b"Mute Voice"
                        },
                        -32,
                        2,
                    )?;
                }
            }
            if view(&io.facts()?)?.entries.is_empty() {
                append(io, b"Back", -5, 0)?;
            }
        }
        236 => {
            field(io, Target::View, Field::Title(b"CHANGE CLAN ROLE".to_vec()))?;
            field(io, Target::View, Field::Description(vec![]))?;
            for i in 0..4 {
                append(io, role(i)?, -30, 10 + i as i32)?;
            }
        }
        211 => {
            field(io, Target::View, Field::Title(b"CLAN MESSAGES".to_vec()))?;
            field(io, Target::View, Field::Description(vec![]))?;
            if let Some(clan) = &f.account.clan {
                for (i, p) in clan.messages.iter().enumerate() {
                    append(io, p.name.clone(), -1, i as i32)?;
                }
            }
            if view(&io.facts()?)?.entries.is_empty() {
                append(io, b"No clan messages", -1, 0)?;
            }
        }
        _ => (),
    }
    Ok(())
}
pub fn column(flow: &Controller, io: &impl Io, row: u32, column: u32) -> Result<Vec<u8>> {
    let f = io.facts()?;
    let frame = view(&f)?;
    if frame.screen != 26 || frame.context != 1 || f.account.clan.is_none() {
        return Ok(vec![]);
    }
    let clan = f.account.clan.as_ref().unwrap();
    let Some(p) = clan.members.get(row as usize) else {
        return Ok(if column == 1 {
            b"Invited".to_vec()
        } else {
            vec![]
        });
    };
    if column != 1 {
        return Ok(vec![]);
    }
    let mut label = role(p.role)?.to_vec();
    label.extend_from_slice(match p.presence {
        2 => b" - ONLINE",
        1 => b" - Offline",
        3 => b" - Playing Halo 2",
        _ => b"",
    });
    if !flow.state().voice_party && f.voice.joined {
        if f.voice.muted.contains(&p.id) {
            label.extend_from_slice(b" - muted");
        } else if (p.is_self && f.voice.transmitting) || f.voice.talking.contains(&p.id) {
            label.extend_from_slice(b" - talking");
        }
    }
    Ok(label)
}
fn pages(body: &[u8]) -> Result<Vec<&[u8]>> {
    let mut pages = vec![];
    let mut begin = 0;
    while begin < body.len() {
        let mut end = (begin + 64).min(body.len());
        while end < body.len() && (body[end] & 0xc0) == 0x80 {
            if end == 0 {
                return Err("Native clan message page has no progressing boundary".into());
            }
            end -= 1;
        }
        if end <= begin {
            return Err("Native clan message page has no progressing boundary".into());
        }
        pages.push(&body[begin..end]);
        begin = end;
    }
    Ok(pages)
}
pub fn text(flow: &Controller, io: &impl Io, index: u32) -> Result<Vec<u8>> {
    let f = io.facts()?;
    let frame = view(&f)?;
    match frame.screen {
        33 => Ok(if index == 0 {
            frame.description.clone()
        } else {
            vec![]
        }),
        26 => {
            io.prepare_pane(26, 1)?;
            let f = io.facts()?;
            Ok(match index {
                0 => b"CLAN".to_vec(),
                1 | 3 => return io.localized(index),
                2 => f
                    .account
                    .clan
                    .map_or_else(|| b"No clan".to_vec(), |c| c.name),
                4 => f.account.clan.map_or_else(Vec::new, |c| {
                    let count = c.members.len();
                    format!("{count} {}", if count == 1 { "member" } else { "members" })
                        .into_bytes()
                }),
                6 => {
                    if f.account.clan.is_none() {
                        vec![]
                    } else if !flow.state().voice_party && f.voice.joined {
                        if f.voice.microphone_muted {
                            b"MIC MUTED".to_vec()
                        } else {
                            b"MIC ON".to_vec()
                        }
                    } else {
                        b"VOICE OFF".to_vec()
                    }
                }
                8 => {
                    if !flow.state().voice_party && f.voice_client && f.voice.revision > 1 {
                        if f.voice.joined && f.voice.microphone_muted {
                            b"Clan voice - microphone muted.".to_vec()
                        } else {
                            f.voice.message
                        }
                    } else {
                        f.account.message
                    }
                }
                _ => vec![],
            })
        }
        236 => {
            let target = flow.state().clan_target;
            let p = member(&f.account, &target);
            match index {
                0 => Ok(if let Some(p) = p {
                    joined(&[&p.name, b"\nCurrent role: ", role(p.role)?])
                } else {
                    b"Member unavailable".to_vec()
                }),
                1 => Ok(role(frame.selected.min(3))?.to_vec()),
                2 => io.localized_direct(236, frame.selected.min(3), index),
                _ => Ok(vec![]),
            }
        }
        211 => {
            let Some(p) = f
                .account
                .clan
                .as_ref()
                .and_then(|c| c.messages.get(frame.selected as usize))
            else {
                return Ok(if index == 1 {
                    b"No clan messages.".to_vec()
                } else {
                    vec![]
                });
            };
            let pages = pages(&p.body)?;
            match index {
                1 => {
                    if pages.is_empty() {
                        return Err("Native clan message body produces no text page".into());
                    }
                    Ok(pages[flow.state().clan_message_page as usize % pages.len()].to_vec())
                }
                2 => Ok(if pages.len() > 1 {
                    format!(
                        "Message {}/{} - X next",
                        flow.state().clan_message_page as usize % pages.len() + 1,
                        pages.len()
                    )
                    .into_bytes()
                } else {
                    b"Clan message".to_vec()
                }),
                _ => Ok(vec![]),
            }
        }
        _ => Ok(if index == 0 {
            f.account.message
        } else {
            vec![]
        }),
    }
}
pub fn command(flow: &Controller, command: &Command, io: &impl Io) -> Result<()> {
    if command.action == b"leave" && !flow.state().voice_party && io.facts()?.voice_client {
        io.effect(Effect::VoiceLeave)?;
    }
    if !io.facts()?.account_client || !io.command(command)? {
        io.effect(Effect::Unavailable(
            b"Wait for the ONLINE account connection or current clan operation to finish.".to_vec(),
        ))?;
        return Ok(());
    }
    loop {
        let f = io.facts()?;
        if f.frames.len() <= 1 {
            break;
        }
        let frame = top(&f)?;
        if frame.screen == 26 && frame.context == 1 {
            break;
        }
        io.effect(Effect::Pop)?;
    }
    if top(&io.facts()?)?.screen != 26 {
        io.effect(Effect::Open(26, 1))?;
    }
    io.update_account()?;
    io.effect(Effect::Revision)
}
pub fn confirm(flow: &Controller, command: Command, message: Vec<u8>, io: &impl Io) -> Result<()> {
    flow.set_clan_confirmation(Some(command));
    io.effect(Effect::Open(33, 41))?;
    field(io, Target::Top, Field::Title(b"CLAN CONFIRMATION".to_vec()))?;
    field(io, Target::Top, Field::Description(message))?;
    field(
        io,
        Target::Top,
        Field::Replace(vec![row(b"Confirm", -31, 0), row(b"Cancel", -5, 0)]),
    )?;
    field(io, Target::Top, Field::Select(1))
}
pub fn accept(flow: &Controller, io: &impl Io) -> Result<bool> {
    let f = io.facts()?;
    let frame = view(&f)?.clone();
    if !screen(frame.screen, frame.context) {
        return Ok(false);
    }
    if frame.screen == 33 {
        if frame.selected == 0 && flow.state().clan_confirmation.is_some() {
            let c = flow.take_clan_confirmation().unwrap();
            command(flow, &c, io)?;
        } else {
            flow.set_clan_confirmation(None);
            io.effect(Effect::Back)?;
        }
        return Ok(true);
    }
    let Some(entry) = frame.entries.get(frame.selected as usize) else {
        return Ok(true);
    };
    if entry.destination == -33 {
        io.party_command(&PartyCommand {
            action: b"invite_clan".to_vec(),
            ..PartyCommand::default()
        })?;
        return Ok(true);
    }
    if frame.screen == 26 {
        if entry.destination == 167 {
            io.effect(Effect::ClearKeyboard)?;
            io.effect(Effect::Open(167, 4))?;
            return Ok(true);
        }
        let index = entry.value as u32;
        let target = if index >= 2000 {
            f.account
                .clan
                .as_ref()
                .and_then(|c| c.outgoing.get((index - 2000) as usize))
                .map(|p| (p.id.clone(), 2))
        } else {
            None
        }
        .or_else(|| {
            if index >= 1000 {
                f.account
                    .invitations
                    .get((index - 1000) as usize)
                    .map(|p| (p.id.clone(), 1))
            } else {
                None
            }
        })
        .or_else(|| {
            f.account
                .clan
                .as_ref()
                .and_then(|c| c.members.get(index as usize))
                .map(|p| (p.id.clone(), 0))
        });
        if let Some((id, kind)) = target {
            flow.set_clan_target(id, kind);
            io.effect(Effect::Open(31, 41))?;
        }
        return Ok(true);
    }
    match entry.destination {
        167 => {
            io.effect(Effect::ClearKeyboard)?;
            io.effect(Effect::Open(167, entry.value as u32))?;
            return Ok(true);
        }
        211 => {
            io.effect(Effect::Open(211, 41))?;
            return Ok(true);
        }
        236 => {
            io.effect(Effect::Open(236, 0))?;
            let f = io.facts()?;
            if let Some(p) = member(&f.account, &flow.state().clan_target) {
                io.select(p.role)?;
            }
            return Ok(true);
        }
        -5 => {
            io.effect(Effect::Back)?;
            return Ok(true);
        }
        _ => (),
    }
    if entry.destination == -32 && f.voice_client {
        match entry.value {
            0 => {
                if flow.state().voice_party {
                    io.effect(Effect::VoiceLeave)?;
                    io.effect(Effect::DropVoice)?;
                    flow.set_voice_wanted(false);
                    flow.set_voice_party(false);
                    io.effect(Effect::CreateVoice(false))?;
                    io.effect(Effect::VoiceJoin)?;
                } else if f.voice.joined || f.voice.connecting {
                    io.effect(Effect::VoiceLeave)?;
                } else {
                    io.effect(Effect::VoiceJoin)?;
                }
            }
            1 => io.effect(Effect::Microphone(!f.voice.microphone_muted))?,
            2 => {
                let id = flow.state().clan_target;
                let muted = !f.voice.muted.contains(&id);
                io.effect(Effect::Mute(id, muted))?;
            }
            _ => (),
        }
        io.effect(Effect::Back)?;
        io.update_account()?;
        return Ok(true);
    }
    if entry.destination != -30 {
        return Ok(true);
    }
    let target = flow.state().clan_target;
    if entry.value >= 10 {
        let rank = (entry.value - 10) as u32;
        let c = Command {
            action: b"role".to_vec(),
            value: target,
            role: rank,
            confirmed: true,
        };
        if rank == 3 {
            confirm(
                flow,
                c,
                b"Promote this member to Overlord? You won't be able to undo this.".to_vec(),
                io,
            )?;
        } else {
            command(flow, &c, io)?;
        }
    } else {
        let actions = [
            &b"accept"[..],
            b"decline",
            b"block",
            b"leave",
            b"cancel",
            b"remove",
        ];
        let action = actions
            .get(entry.value as u32 as usize)
            .ok_or("Native clan command is outside the produced action table")?;
        let c = Command {
            action: action.to_vec(),
            value: target,
            ..Command::default()
        };
        if entry.value == 3 {
            let single = f
                .account
                .clan
                .as_ref()
                .is_some_and(|c| c.members.len() == 1);
            confirm(
                flow,
                c,
                if single {
                    b"Leave and permanently delete your clan?".to_vec()
                } else {
                    b"Leave your clan?".to_vec()
                },
                io,
            )?;
        } else if entry.value == 5 {
            confirm(flow, c, b"Remove this member from your clan?".to_vec(), io)?;
        } else {
            command(flow, &c, io)?;
        }
    }
    Ok(true)
}
pub fn advance_message(flow: &Controller, io: &impl Io) -> Result<bool> {
    let f = io.facts()?;
    let frame = view(&f)?;
    if frame.screen != 211 || frame.context != 41 {
        return Ok(false);
    }
    flow.advance_clan_message();
    io.effect(Effect::Revision)?;
    Ok(true)
}
