//! Complete modern native friends caller, including matching-related joins.
//! Provider observations and primitive UI effects remain native IO. This does
//! not recover original Live service procedures or historical SDK behavior.
use crate::matchmaking_frontend_account_flow::{Entry, Frame};
use crate::matchmaking_frontend_party_flow::{Command, Controller};
pub type Result<T> = std::result::Result<T, String>;
#[derive(Clone, Debug, Default)]
pub struct Friend {
    pub id: Vec<u8>,
    pub name: Vec<u8>,
    pub tag: Vec<u8>,
    pub state: Vec<u8>,
    pub presence: Vec<u8>,
    pub joinable: Option<bool>,
    pub context: Vec<u8>,
}
#[derive(Clone, Debug, Default)]
pub struct Account {
    pub authenticated: bool,
    pub busy: bool,
    pub party: bool,
    pub leader: bool,
    pub available: bool,
    pub tag: Vec<u8>,
    pub message: Vec<u8>,
    pub friends: Vec<Friend>,
    pub search: Option<Friend>,
}
#[derive(Clone, Debug, Default)]
pub struct SocialFriend {
    pub handle: Vec<u8>,
    pub name: Vec<u8>,
    pub kind: i32,
    pub presence: i32,
}
#[derive(Clone, Debug, Default)]
pub struct Invite {
    pub handle: Vec<u8>,
    pub name: Vec<u8>,
    pub status: Vec<u8>,
}
#[derive(Clone, Debug, Default)]
pub struct Social {
    pub available: bool,
    pub enabled: bool,
    pub connecting: bool,
    pub ready: bool,
    pub busy: bool,
    pub message: Vec<u8>,
    pub revision: u64,
    pub friends: Vec<SocialFriend>,
    pub invitations: Vec<Invite>,
}
#[derive(Clone, Debug, Default)]
pub struct Facts {
    pub account_client: bool,
    pub social_client: bool,
    pub login_client: bool,
    pub account: Account,
    pub social: Social,
    pub frame: Frame,
    pub frames: Vec<Frame>,
    pub launch: bool,
    pub status: u32,
}
#[derive(Clone, Debug)]
pub enum Effect {
    Title(Vec<u8>),
    Description(Vec<u8>),
    Append(Entry),
    Replace(Vec<Entry>),
    ClearKeyboard,
    Open(u32, u32),
    Back,
    Pop,
    Revision,
    Unavailable(Vec<u8>),
    StageEmptySocial,
    CommitSocial,
    BindAccount,
    Enable,
    Disconnect,
    Pump,
    Presence(Vec<u8>),
    ClearIndex(usize),
    PopulateIndex(usize),
    SelectIndex(usize, u32),
}
pub trait Io {
    fn facts(&self) -> Result<Facts>;
    fn uses_steam(&self) -> Result<bool>;
    fn identity(&self) -> Result<bool>;
    fn pane(&self) -> Result<()>;
    fn localized(&self, index: u32) -> Result<Vec<u8>>;
    fn profile_name(&self) -> Result<Vec<u8>>;
    fn social_snapshot(&self) -> Result<Social>;
    fn friend_command(&self, action: &[u8], target: &[u8]) -> Result<bool>;
    fn party_command(&self, command: &Command) -> Result<bool>;
    fn wrapped_party_command(&self, command: &Command) -> Result<()>;
    fn command_serial(&self) -> Result<u64>;
    fn update_account(&self) -> Result<()>;
    /// 0 friend command, 1 party invitation, 2 activity accept, 3 activity dismiss.
    fn social_command(&self, kind: u32, target: &[u8], action: &[u8]) -> Result<bool>;
    fn effect(&self, effect: Effect) -> Result<()>;
}
pub fn screen(screen: u32, context: u32) -> bool {
    crate::matchmaking_frontend_account_flow::friends_screen(screen, context)
}
pub fn native(io: &impl Io) -> Result<bool> {
    let f = io.facts()?;
    Ok(f.account.available || (f.login_client && io.uses_steam()?))
}
pub fn available(io: &impl Io) -> Result<bool> {
    if !io.identity()? {
        return Ok(false);
    }
    let f = io.facts()?;
    Ok((f.account.authenticated && f.account.available)
        || (f.social.available && f.social.enabled && f.social.ready))
}
pub fn bind(flow: &Controller, io: &impl Io) -> Result<()> {
    if io.facts()?.social_client {
        let _ = io.social_snapshot()?;
    } else {
        io.effect(Effect::StageEmptySocial)?;
    }
    io.effect(Effect::CommitSocial)?;
    flow.set_friend_target(vec![]);
    flow.set_activity_target(vec![]);
    if io.facts()?.social_client {
        io.effect(Effect::BindAccount)?;
    }
    Ok(())
}
fn join_blocked(status: u32) -> bool {
    matches!(status, 4..=8)
}
fn presence(value: i32) -> &'static [u8] {
    match value {
        4 => b"Playing Halo 2",
        1 => b"ONLINE",
        2 => b"Away",
        3 => b"Do not disturb",
        _ => b"Offline",
    }
}
fn joined(parts: &[&[u8]]) -> Vec<u8> {
    parts.concat()
}
fn row(label: impl Into<Vec<u8>>, destination: i32, value: i32) -> Entry {
    Entry {
        label: label.into(),
        destination,
        value,
    }
}
fn append(io: &impl Io, label: impl Into<Vec<u8>>, destination: i32, value: i32) -> Result<()> {
    io.effect(Effect::Append(row(label, destination, value)))
}
fn native_row<'a>(flow: &Controller, f: &'a Facts) -> Option<&'a Friend> {
    if f.frame.context == 46 && f.account.search.is_some() {
        f.account.search.as_ref()
    } else {
        let target = flow.state().friend_target;
        f.account.friends.iter().find(|p| p.id == target)
    }
}
pub fn populate_native(flow: &Controller, io: &impl Io) -> Result<()> {
    let f = io.facts()?;
    let frame = &f.frame;
    let a = &f.account;
    io.effect(Effect::Title(if frame.screen == 26 {
        b"ONLINE".to_vec()
    } else if frame.screen == 31 {
        b"FRIEND OPTIONS".to_vec()
    } else {
        b"FRIENDS OPTIONS".to_vec()
    }))?;
    let mut description = if a.tag.is_empty() {
        a.message.clone()
    } else {
        joined(&[b"Your friend tag: ", &a.tag])
    };
    if !a.message.is_empty() && !a.tag.is_empty() {
        description.extend_from_slice(b"\n");
        description.extend_from_slice(&a.message);
    }
    io.effect(Effect::Description(description))?;
    if matches!(frame.screen, 26 | 216) {
        append(io, b"Find Player by Friend Tag", 167, 11)?;
        for (i, p) in a.friends.iter().enumerate() {
            if (p.state == b"blocked") != (frame.screen == 216) {
                continue;
            }
            let prefix = match p.state.as_slice() {
                b"incoming" => &b"Request: "[..],
                b"outgoing" => b"Pending: ",
                b"blocked" => b"Blocked: ",
                _ => b"",
            };
            append(io, joined(&[prefix, &p.name]), 31, i as i32)?;
        }
        if frame.screen == 26 && io.facts()?.frame.entries.len() == 1 {
            append(
                io,
                if a.available {
                    &b"No friends added yet"[..]
                } else {
                    b"Friends are unavailable"
                },
                -82,
                0,
            )?;
        }
        if frame.screen == 216 {
            append(io, b"Party Members", 26, 2)?;
            append(io, b"Back", -5, 0)?;
        }
        return Ok(());
    }
    if let Some(p) = native_row(flow, &f) {
        let mut description = joined(&[
            &p.tag,
            b"\n",
            match p.presence.as_slice() {
                b"playing" => b"Playing Halo 2",
                b"online" => b"ONLINE",
                _ => b"Offline",
            },
        ]);
        if !a.message.is_empty() {
            description.extend_from_slice(b"\n");
            description.extend_from_slice(&a.message);
        }
        io.effect(Effect::Description(description))?;
        let state = a
            .friends
            .iter()
            .find(|r| r.id == p.id)
            .map_or(&[][..], |r| r.state.as_slice());
        if state.is_empty() {
            append(io, b"Send Friend Request", -83, 0)?;
        }
        if state == b"incoming" {
            append(io, b"Accept Friend Request", -83, 1)?;
            append(io, b"Decline Friend Request", -83, 2)?;
        }
        if state == b"outgoing" {
            append(io, b"Cancel Friend Request", -83, 3)?;
        }
        if state == b"friend" {
            if a.party && a.leader {
                append(io, b"Invite To Party", -83, 7)?;
            }
            if frame.context == 43
                && !a.busy
                && !a.party
                && !f.launch
                && !join_blocked(f.status)
                && p.presence != b"offline"
                && p.joinable == Some(true)
                && !p.context.is_empty()
            {
                append(io, b"Join Party", -83, 8)?;
            }
            append(io, b"Remove Friend", -83, 4)?;
        }
        append(
            io,
            if state == b"blocked" {
                &b"Unblock Player"[..]
            } else {
                b"Block Player"
            },
            -83,
            if state == b"blocked" { 6 } else { 5 },
        )?;
    }
    append(io, b"Back", -5, 0)
}
pub fn populate(flow: &Controller, io: &impl Io) -> Result<()> {
    if native(io)? && io.facts()?.frame.context != 44 {
        return populate_native(flow, io);
    }
    let f = io.facts()?;
    let frame = &f.frame;
    let s = &f.social;
    io.effect(Effect::Title(if frame.screen == 26 {
        b"ONLINE".to_vec()
    } else if frame.screen == 31 {
        b"FRIEND OPTIONS".to_vec()
    } else {
        b"FRIENDS OPTIONS".to_vec()
    }))?;
    io.effect(Effect::Description(s.message.clone()))?;
    let enable = if f.login_client && io.uses_steam()? {
        &b"Link Discord Friends"[..]
    } else {
        b"Enable Discord Friends"
    };
    let f = io.facts()?;
    let frame = &f.frame;
    let s = &f.social;
    if frame.screen == 31 && frame.context == 44 {
        io.effect(Effect::Title(b"GAME INVITATION".to_vec()))?;
        let target = flow.state().activity_target;
        if let Some(p) = s.invitations.iter().find(|p| p.handle == target) {
            io.effect(Effect::Description(joined(&[
                &p.name,
                b"\nHalo 2 party invitation",
            ])))?;
            io.effect(Effect::Replace(vec![
                row(
                    if s.busy {
                        &b"Accepting Invitation..."[..]
                    } else {
                        b"Accept Game Invite"
                    },
                    -35,
                    if s.busy { 0 } else { 7 },
                ),
                row(b"Dismiss Invitation", -35, 8),
            ]))?;
        }
        append(io, b"Back", -5, 0)?;
        return Ok(());
    }
    if frame.screen == 26 {
        let replacement = if !s.available {
            Some(row(b"Discord Friends unavailable", -35, 0))
        } else if !s.enabled {
            Some(row(enable, -35, 1))
        } else if !s.ready {
            Some(row(
                if s.connecting {
                    &b"Connecting Discord Friends..."[..]
                } else {
                    b"Reconnect Discord Friends"
                },
                -35,
                if s.connecting { 0 } else { 1 },
            ))
        } else {
            None
        };
        if let Some(entry) = replacement {
            io.effect(Effect::Replace(vec![entry]))?;
        } else {
            for (i, p) in s.invitations.iter().enumerate() {
                append(
                    io,
                    p.name.clone(),
                    31,
                    (i as i32).wrapping_neg().wrapping_sub(1),
                )?;
            }
            for (i, p) in s.friends.iter().enumerate() {
                if p.kind == 3 {
                    continue;
                }
                let prefix = match p.kind {
                    1 => &b"Request: "[..],
                    2 => b"Pending: ",
                    _ => b"",
                };
                append(io, joined(&[prefix, &p.name]), 31, i as i32)?;
            }
            if io.facts()?.frame.entries.is_empty() {
                io.effect(Effect::Replace(vec![row(b"No Discord friends", -35, 0)]))?;
            }
        }
    } else if frame.screen == 216 {
        if s.available {
            append(
                io,
                if s.enabled {
                    &b"Reconnect Discord Friends"[..]
                } else {
                    enable
                },
                -35,
                1,
            )?;
        }
        if s.enabled {
            append(io, b"Disconnect Discord Friends", -35, 2)?;
        }
        append(io, b"Party Members", 26, 2)?;
        append(io, b"Back", -5, 0)?;
    } else {
        let target = flow.state().friend_target;
        let Some(p) = s.friends.iter().find(|p| p.handle == target) else {
            return io.effect(Effect::Replace(vec![row(b"Back", -5, 0)]));
        };
        io.effect(Effect::Description(joined(&[
            &p.name,
            b"\n",
            presence(p.presence),
        ])))?;
        if p.kind == 0 && f.account.party && f.account.leader {
            append(io, b"Invite To Party", -35, 3)?;
        }
        if p.kind == 1 {
            append(io, b"Accept Discord Friend Request", -35, 4)?;
            append(io, b"Decline Discord Friend Request", -35, 5)?;
        }
        if p.kind == 2 {
            append(io, b"Cancel Discord Friend Request", -35, 6)?;
        }
        append(io, b"Back", -5, 0)?;
    }
    Ok(())
}
pub fn accept_native(flow: &Controller, io: &impl Io) -> Result<bool> {
    let f = io.facts()?;
    let frame = &f.frame;
    let Some(e) = frame.entries.get(frame.selected as usize) else {
        return Ok(true);
    };
    match e.destination {
        167 => {
            io.effect(Effect::ClearKeyboard)?;
            io.effect(Effect::Open(167, 11))?;
            return Ok(true);
        }
        -5 => {
            io.effect(Effect::Back)?;
            return Ok(true);
        }
        26 => {
            io.effect(Effect::Open(26, e.value as u32))?;
            return Ok(true);
        }
        31 => {
            if let Some(p) = f.account.friends.get(e.value as u32 as usize) {
                flow.set_friend_target(p.id.clone());
                io.effect(Effect::Open(31, 43))?;
            }
            return Ok(true);
        }
        _ => (),
    }
    if e.destination != -83 || !f.account_client || f.account.busy {
        return Ok(true);
    }
    let target = if frame.context == 46 && f.account.search.is_some() {
        f.account.search.as_ref().unwrap().id.clone()
    } else {
        flow.state().friend_target
    };
    if target.is_empty() {
        return Ok(true);
    }
    if e.value == 7 {
        io.wrapped_party_command(&Command {
            action: b"invite_friend".to_vec(),
            value: target,
            ..Command::default()
        })?;
        return Ok(true);
    }
    if e.value == 8 {
        if let Some(p) = f
            .account
            .friends
            .iter()
            .find(|p| p.id == target)
            .filter(|p| {
                frame.context == 43
                    && !f.account.party
                    && !f.launch
                    && !join_blocked(f.status)
                    && p.presence != b"offline"
                    && p.joinable == Some(true)
            })
        {
            if io.party_command(&Command {
                action: b"join_friend".to_vec(),
                value: target,
                join_context: p.context.clone(),
                ..Command::default()
            })? {
                flow.set_friend_join_serial(io.command_serial()?);
                io.update_account()?;
                io.effect(Effect::Revision)?;
            } else {
                io.effect(Effect::Unavailable(
                    b"Refresh Friends before joining this party.".to_vec(),
                ))?;
            }
        }
        return Ok(true);
    }
    let actions = [
        &b"request"[..],
        b"accept",
        b"decline",
        b"cancel",
        b"remove",
        b"block",
        b"unblock",
    ];
    if (0..=6).contains(&e.value) && io.friend_command(actions[e.value as usize], &target)? {
        io.effect(Effect::Back)?;
        io.update_account()?;
        io.effect(Effect::Revision)?;
    }
    Ok(true)
}
// Negative UI values encode invitation indices as -(index+1). The bitwise
// form preserves admitted values without C++'s undefined -INT_MIN operation.
fn invitation(value: i32) -> usize {
    (!value as u32) as usize
}
pub fn update(_flow: &Controller, io: &impl Io) -> Result<()> {
    if !io.facts()?.social_client {
        return Ok(());
    }
    let name = io.profile_name()?;
    io.effect(Effect::Presence(name))?;
    io.effect(Effect::Pump)?;
    if native(io)? {
        return Ok(());
    }
    let next = io.social_snapshot()?;
    let previous = io.facts()?.social;
    if next.revision == previous.revision {
        return Ok(());
    }
    io.effect(Effect::CommitSocial)?;
    let count = io.facts()?.frames.len();
    for index in 0..count {
        let facts = io.facts()?;
        let frame = &facts.frames[index];
        if !screen(frame.screen, frame.context) {
            continue;
        }
        let mut selected = vec![];
        if frame.screen == 26 {
            if let Some(e) = frame
                .entries
                .get(frame.selected as usize)
                .filter(|e| e.destination == 31)
            {
                selected = if e.value < 0 {
                    previous
                        .invitations
                        .get(invitation(e.value))
                        .map(|p| p.handle.clone())
                } else {
                    previous
                        .friends
                        .get(e.value as u32 as usize)
                        .map(|p| p.handle.clone())
                }
                .unwrap_or_default();
            }
        }
        let old = frame.selected;
        io.effect(Effect::ClearIndex(index))?;
        io.effect(Effect::PopulateIndex(index))?;
        let facts = io.facts()?;
        let frame = &facts.frames[index];
        io.effect(Effect::SelectIndex(
            index,
            if frame.entries.is_empty() {
                0
            } else {
                old.min((frame.entries.len() - 1) as u32)
            },
        ))?;
        if !selected.is_empty() {
            for (i, e) in frame.entries.iter().enumerate() {
                if e.destination != 31 {
                    continue;
                }
                let handle = if e.value < 0 {
                    facts
                        .social
                        .invitations
                        .get(invitation(e.value))
                        .map(|p| &p.handle)
                } else {
                    facts
                        .social
                        .friends
                        .get(e.value as u32 as usize)
                        .map(|p| &p.handle)
                };
                if handle == Some(&selected) {
                    io.effect(Effect::SelectIndex(index, i as u32))?;
                }
            }
        }
    }
    io.effect(Effect::Revision)
}
pub fn accept(flow: &Controller, io: &impl Io) -> Result<bool> {
    let f = io.facts()?;
    let frame = f.frame;
    if !screen(frame.screen, frame.context) {
        return Ok(false);
    }
    if native(io)? && frame.context != 44 {
        return accept_native(flow, io);
    }
    let f = io.facts()?;
    let Some(e) = frame.entries.get(frame.selected as usize) else {
        return Ok(true);
    };
    match e.destination {
        -5 => {
            io.effect(Effect::Back)?;
            return Ok(true);
        }
        26 => {
            io.effect(Effect::Open(26, e.value as u32))?;
            return Ok(true);
        }
        31 => {
            if e.value < 0 {
                if let Some(p) = f.social.invitations.get(invitation(e.value)) {
                    flow.set_activity_target(p.handle.clone());
                    io.effect(Effect::Open(31, 44))?;
                }
            } else if let Some(p) = f.social.friends.get(e.value as u32 as usize) {
                flow.set_friend_target(p.handle.clone());
                io.effect(Effect::Open(31, 43))?;
            }
            return Ok(true);
        }
        _ => (),
    }
    if e.destination != -35 || !f.social_client {
        return Ok(true);
    }
    if frame.screen == 31 && frame.context == 44 {
        if e.value == 7 {
            let _ = io.social_command(2, &flow.state().activity_target, b"")?;
        } else if e.value == 8 {
            let _ = io.social_command(3, &flow.state().activity_target, b"")?;
            io.effect(Effect::Back)?;
        }
        update(flow, io)?;
        return Ok(true);
    }
    if e.value == 1 {
        io.effect(Effect::Enable)?;
    } else if e.value == 2 {
        io.effect(Effect::Disconnect)?;
    } else if e.value >= 3 {
        let target = flow.state().friend_target;
        let Some(p) = f.social.friends.iter().find(|p| p.handle == target) else {
            return Ok(true);
        };
        if e.value == 3 && p.kind == 0 && f.account.party && f.account.leader {
            if io.social_command(1, &p.handle, b"")? {
                io.update_account()?;
                loop {
                    let f = io.facts()?;
                    let top = f.frames.last().ok_or("Native friends menu has no frame")?;
                    if f.frames.len() <= 1
                        || (top.screen == 26 && top.context == 2)
                        || matches!(top.screen, 14 | 186)
                    {
                        break;
                    }
                    io.effect(Effect::Pop)?;
                }
                if io
                    .facts()?
                    .frames
                    .last()
                    .ok_or("Native friends menu has no frame")?
                    .screen
                    != 26
                {
                    io.effect(Effect::Open(26, 2))?;
                }
                io.effect(Effect::Revision)?;
            } else {
                io.effect(Effect::Unavailable(
                    b"Refresh Discord Friends and your ONLINE party before inviting this player."
                        .to_vec(),
                ))?;
            }
            return Ok(true);
        }
        if (4..=6).contains(&e.value) {
            let actions = [&b"accept"[..], b"decline", b"cancel"];
            let _ = io.social_command(0, &p.handle, actions[(e.value - 4) as usize])?;
        }
        io.effect(Effect::Back)?;
    }
    update(flow, io)?;
    Ok(true)
}
pub fn text(flow: &Controller, io: &impl Io, index: u32) -> Result<Vec<u8>> {
    let native = native(io)?;
    let f = io.facts()?;
    let frame = &f.frame;
    if native && frame.context != 44 {
        if frame.screen == 26 {
            io.pane()?;
            if matches!(index, 0 | 1 | 3) {
                return io.localized(index);
            }
            return Ok(if index == 2 {
                if f.account.tag.is_empty() {
                    b"Halo friends".to_vec()
                } else {
                    f.account.tag
                }
            } else {
                vec![]
            });
        }
        return Ok(if let Some(p) = native_row(flow, &f) {
            match index {
                0 => p.name.clone(),
                1 => p.tag.clone(),
                _ => vec![],
            }
        } else if index == 0 {
            f.account.message
        } else {
            vec![]
        });
    }
    if frame.screen == 26 {
        io.pane()?;
        if matches!(index, 0 | 1 | 3) {
            return io.localized(index);
        }
        return Ok(if index == 2 {
            b"Discord friends".to_vec()
        } else {
            vec![]
        });
    }
    if frame.screen == 31 {
        if frame.context == 44 {
            let target = flow.state().activity_target;
            let Some(p) = f.social.invitations.iter().find(|p| p.handle == target) else {
                return Ok(if index == 0 {
                    if f.social.busy {
                        b"Joining ONLINE party...".to_vec()
                    } else {
                        b"Invitation unavailable".to_vec()
                    }
                } else {
                    vec![]
                });
            };
            if index == 2 && f.account.party {
                return Ok(b"Leave current party first.".to_vec());
            }
            return Ok(match index {
                0 => p.name.clone(),
                1 => p.status.clone(),
                _ => vec![],
            });
        }
        let target = flow.state().friend_target;
        let Some(p) = f.social.friends.iter().find(|p| p.handle == target) else {
            return Ok(if index == 0 {
                b"Friend unavailable".to_vec()
            } else {
                vec![]
            });
        };
        return Ok(match index {
            0 => p.name.clone(),
            1 => presence(p.presence).to_vec(),
            _ => vec![],
        });
    }
    Ok(if index == 0 { f.social.message } else { vec![] })
}
pub fn column(io: &impl Io, row: u32, column: u32) -> Result<Vec<u8>> {
    let f = io.facts()?;
    let frame = &f.frame;
    let Some(_e) = frame
        .entries
        .get(row as usize)
        .filter(|e| frame.screen == 26 && e.destination == 31 && column == 1)
    else {
        return Ok(vec![]);
    };
    let native = native(io)?;
    let f = io.facts()?;
    let frame = &f.frame;
    let Some(e) = frame.entries.get(row as usize) else {
        return Ok(vec![]);
    };
    if native {
        return Ok(f.account.friends.get(e.value as u32 as usize).map_or_else(
            Vec::new,
            |p| match p.state.as_slice() {
                b"incoming" => b"Friend request".to_vec(),
                b"outgoing" => b"Pending".to_vec(),
                _ => match p.presence.as_slice() {
                    b"playing" => b"Playing Halo 2".to_vec(),
                    b"online" => b"ONLINE".to_vec(),
                    _ => b"Offline".to_vec(),
                },
            },
        ));
    }
    Ok(if e.value < 0 {
        f.social
            .invitations
            .get(invitation(e.value))
            .map_or_else(Vec::new, |p| p.status.clone())
    } else {
        f.social
            .friends
            .get(e.value as u32 as usize)
            .map_or_else(Vec::new, |p| presence(p.presence).to_vec())
    })
}
