//! Native account observation, party transitions and retained panel focus.
//! Existing replacement-client policy, not recovered original Live services.
use crate::matchmaking_frontend_party as party;
use crate::matchmaking_frontend_party_flow::{Controller, screen as party_screen};
pub type Result<T> = std::result::Result<T, String>;

#[derive(Clone, Debug, Default)]
pub struct Account {
    pub revision: u64,
    pub notification: party::Notification,
    pub friends: Vec<Vec<u8>>,
    pub clan: Option<Vec<Vec<u8>>>,
    pub friend_search: bool,
    pub command_serial: u64,
    pub successful: Option<bool>,
    pub party_message: Vec<u8>,
}
#[derive(Clone, Debug, Default)]
pub struct Entry {
    pub label: Vec<u8>,
    pub destination: i32,
    pub value: i32,
}
#[derive(Clone, Debug, Default)]
pub struct Frame {
    pub screen: u32,
    pub context: u32,
    pub selected: u32,
    pub pcr: bool,
    pub entries: Vec<Entry>,
}
#[derive(Clone, Debug, Default)]
pub struct Facts {
    pub account_client: bool,
    pub login_client: bool,
    pub voice_client: bool,
    pub queue_client: bool,
    pub signed_in: Vec<u8>,
    pub tags: Vec<(Vec<u8>, Vec<u8>)>,
    pub account: Account,
    pub voice_revision: u64,
    pub frames: Vec<Frame>,
    pub launch: bool,
    pub mode: i32,
    pub sounds: u64,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Effect {
    Sound(u32),
    CommitAccount,
    CommitVoice,
    CancelQueue,
    StopSearch,
    ReturnOnline,
    Open(u32, u32),
    VoiceLeave,
    VoiceDrop,
    VoiceCreateParty,
    VoiceJoin,
    VoiceRefresh,
    EraseFrom(usize),
    ClearEntries(usize),
    Populate(usize, u32),
    Select(usize, u32),
    Revision,
    Unavailable(Vec<u8>),
}
pub trait Io {
    fn facts(&self) -> Result<Facts>;
    fn identity(&self) -> Result<Option<Vec<u8>>>;
    fn profile(&self, name: &[u8], tag: &[u8]) -> Result<()>;
    fn connect(&self, name: &[u8]) -> Result<()>;
    fn sign_out(&self) -> Result<()>;
    fn snapshot(&self) -> Result<Account>;
    fn stage_voice(&self, read: bool) -> Result<u64>;
    fn native_friends(&self) -> Result<bool>;
    fn effect(&self, effect: Effect) -> Result<()>;
}
fn top(f: &Facts) -> Result<&Frame> {
    f.frames
        .last()
        .ok_or_else(|| "Native account menu has no frame".into())
}
pub fn clan_screen(screen: u32, context: u32) -> bool {
    (screen == 26 && context == 1)
        || matches!(screen, 217 | 236)
        || (matches!(screen, 31 | 211 | 33) && context == 41)
}
pub fn friends_screen(screen: u32, context: u32) -> bool {
    (screen == 26 && context == 0)
        || (matches!(screen, 216 | 31) && context == 43)
        || (screen == 31 && matches!(context, 44 | 46))
}
pub fn friend_join(flow: &Controller, io: &impl Io) -> Result<()> {
    let serial = flow.state().friend_join_serial;
    if serial == 0 {
        return Ok(());
    }
    let f = io.facts()?;
    if !f.account.notification.authenticated
        || top(&f)?.screen != 31
        || top(&f)?.context != 43
        || f.account.command_serial != serial
    {
        flow.set_friend_join_serial(0);
        return Ok(());
    }
    if f.account.notification.busy || f.account.successful.is_none() {
        return Ok(());
    }
    let accepted = f.account.successful == Some(true) && f.account.notification.party.is_some();
    flow.set_friend_join_serial(0);
    if !accepted {
        io.effect(Effect::Unavailable(if f.account.party_message.is_empty() {
            b"The party join was not accepted. Refresh Friends and try again.".to_vec()
        } else {
            f.account.party_message
        }))?;
    }
    Ok(())
}
pub fn update(flow: &Controller, io: &impl Io) -> Result<()> {
    if !io.facts()?.account_client {
        return Ok(());
    }
    let identity = if io.facts()?.login_client {
        io.identity()?
    } else {
        None
    };
    if let Some(username) = identity {
        let f = io.facts()?;
        let name = if f.signed_in.is_empty() {
            username
        } else {
            let tag = f
                .tags
                .iter()
                .find(|(key, _)| key == &f.signed_in)
                .map_or(&[][..], |(_, value)| value.as_slice());
            io.profile(&f.signed_in, tag)?;
            f.signed_in
        };
        io.connect(&name)?;
        flow.set_account_name(name);
    } else {
        io.sign_out()?;
        flow.set_account_name(vec![]);
    }
    let next = io.snapshot()?;
    let voice_revision = io.stage_voice(io.facts()?.voice_client)?;
    let f = io.facts()?;
    if next.revision == f.account.revision && voice_revision == f.voice_revision {
        return Ok(());
    }
    let cues = party::sounds(&f.account.notification, &next.notification);
    let mut sounds = f.sounds;
    for cue in cues {
        if sounds < 16 {
            io.effect(Effect::Sound(cue))?;
            sounds += 1;
        }
    }
    let departed = f.account.notification.party.is_some()
        && next.notification.party.is_none()
        && next.notification.authenticated;
    let joined = f.account.notification.authenticated
        && f.account.notification.party.is_none()
        && next.notification.party.is_some()
        && next.notification.authenticated;
    let previous = f.account;
    io.effect(Effect::CommitAccount)?;
    flow.prune_dismissed(&next.notification.invitations);
    io.effect(Effect::CommitVoice)?;
    if joined {
        let f = io.facts()?;
        if !top(&f)?.pcr && !f.launch && !f.frames.iter().any(|f| f.screen == 14) {
            if f.queue_client {
                io.effect(Effect::CancelQueue)?;
            }
            io.effect(Effect::StopSearch)?;
            io.effect(Effect::ReturnOnline)?;
            io.effect(Effect::Open(14, 0))?;
        }
    }
    let state = flow.state();
    let f = io.facts()?;
    if state.voice_wanted
        && f.account.notification.party.is_some()
        && f.account.notification.authenticated
        && !state.voice_party
        && !top(&f)?.pcr
    {
        if f.voice_client {
            io.effect(Effect::VoiceLeave)?;
        }
        io.effect(Effect::VoiceDrop)?;
        io.effect(Effect::VoiceCreateParty)?;
        flow.set_voice_party(true);
        io.effect(Effect::VoiceJoin)?;
        io.effect(Effect::VoiceRefresh)?;
    }
    if departed && flow.state().voice_party {
        io.effect(Effect::VoiceLeave)?;
        flow.set_voice_wanted(false);
    }
    if departed {
        let f = io.facts()?;
        if let Some(index) = f.frames.iter().position(|f| f.screen == 14) {
            io.effect(Effect::EraseFrom(index))?;
            io.effect(Effect::Open(26, 2))?;
        }
    }
    friend_join(flow, io)?;
    let count = io.facts()?.frames.len();
    for index in 0..count {
        let facts = io.facts()?;
        let frame = &facts.frames[index];
        let clan = clan_screen(frame.screen, frame.context);
        let party = party_screen(facts.mode, frame.screen, frame.context);
        if (!clan
            && !party
            && !(io.native_friends()? && friends_screen(frame.screen, frame.context)))
            || frame.screen == 33
        {
            continue;
        }
        let selected = frame.selected;
        let selected_entry = frame.entries.get(selected as usize);
        let focus = selected_entry.map_or_else(Vec::new, |e| e.label.clone());
        let mut member_focus = vec![];
        let friend_roster = io.native_friends()?
            && friends_screen(frame.screen, frame.context)
            && matches!(frame.screen, 26 | 216);
        if friend_roster {
            if let Some(entry) = selected_entry.filter(|e| e.destination == 31) {
                if let Some(id) = previous.friends.get(entry.value as u32 as usize) {
                    member_focus = id.clone();
                }
            }
        }
        if frame.screen == 26 && frame.context == 1 {
            if let (Some(clan), Some(entry)) = (&previous.clan, selected_entry) {
                if let Some(id) = clan.get(entry.value as u32 as usize) {
                    member_focus = id.clone();
                }
            }
        }
        io.effect(Effect::ClearEntries(index))?;
        let kind = if friends_screen(frame.screen, frame.context) {
            0
        } else if party {
            1
        } else {
            2
        };
        io.effect(Effect::Populate(index, kind))?;
        let current = io.facts()?;
        let frame = &current.frames[index];
        io.effect(Effect::Select(
            index,
            if frame.entries.is_empty() {
                0
            } else {
                selected.min((frame.entries.len() - 1) as u32)
            },
        ))?;
        if frame.screen == 31
            && frame.context == 46
            && !previous.friend_search
            && current.account.friend_search
        {
            io.effect(Effect::Select(index, 0))?;
        } else if !member_focus.is_empty() && friend_roster {
            for (i, entry) in frame.entries.iter().enumerate() {
                if entry.destination == 31
                    && current.account.friends.get(entry.value as u32 as usize)
                        == Some(&member_focus)
                {
                    io.effect(Effect::Select(index, i as u32))?;
                    break;
                }
            }
        } else if !member_focus.is_empty() && current.account.clan.is_some() {
            for (i, entry) in frame.entries.iter().enumerate() {
                if current
                    .account
                    .clan
                    .as_ref()
                    .unwrap()
                    .get(entry.value as u32 as usize)
                    == Some(&member_focus)
                {
                    io.effect(Effect::Select(index, i as u32))?;
                    break;
                }
            }
        } else if !party_screen(current.mode, frame.screen, frame.context) {
            if let Some(i) = frame.entries.iter().position(|e| e.label == focus) {
                io.effect(Effect::Select(index, i as u32))?;
            }
        }
    }
    io.effect(Effect::Revision)
}
