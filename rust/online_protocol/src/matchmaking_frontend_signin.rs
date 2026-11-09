//! Replacement-client sign-in/navigation caller and saved profile namespaces.
//! Credentials, secure randomness, actual workers and retained frames are IO.
//! This preserves the modern native caller, not original Xbox/Live procedures.
use crate::matchmaking_frontend_account_flow::Entry;
use crate::matchmaking_frontend_party_flow::Controller;
use std::collections::BTreeMap;
pub type Result<T> = std::result::Result<T, String>;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct State {
    pub keys: BTreeMap<Vec<u8>, Vec<u8>>,
    pub bound: Vec<u8>,
    pub suppressed: bool,
    pub awaiting: bool,
    pub pending_screen: u32,
    pub pending_context: u32,
}
impl Default for State {
    fn default() -> Self {
        Self {
            keys: BTreeMap::new(),
            bound: vec![],
            suppressed: false,
            awaiting: false,
            pending_screen: 186,
            pending_context: 0,
        }
    }
}
#[derive(Clone, Debug, Default)]
pub struct Frame {
    pub screen: u32,
    pub context: u32,
    pub selected: u32,
    pub instance: u64,
    pub pane_clock: u32,
    pub description: Vec<u8>,
    pub entries: Vec<Entry>,
}
#[derive(Clone, Debug, Default)]
pub struct Facts {
    pub login: bool,
    pub account: bool,
    pub queue: bool,
    pub voice: bool,
    pub social: bool,
    pub directory: bool,
    pub mode: i32,
    pub signed_in: Vec<u8>,
    pub keyboard: Vec<u8>,
    pub frames: Vec<Frame>,
}
impl Facts {
    fn top(&self) -> Result<&Frame> {
        self.frames
            .last()
            .ok_or_else(|| "Native sign-in frame owner absent".into())
    }
}
#[derive(Clone, Copy, Debug, Default)]
pub struct Snapshot {
    pub status: i32,
    pub identity: bool,
}
#[derive(Clone, Debug)]
pub enum Effect {
    Save,
    Bind(Vec<u8>),
    ResetPrior,
    SignOutAccount,
    ClearAccount,
    SignOutQueue,
    StopSearch,
    LeaveVoice,
    DisconnectSocial,
    UpdateCouch,
    UpdateFriends,
    UpdateAccount,
    UpdatePrivacy,
    UpdateLeave,
    UpdatePostgame,
    Restore,
    Start,
    Cancel,
    SignOutLogin,
    ConnectAccount,
    ClearCouch,
    CancelDirectory,
    ClearDirectory,
    CancelQueue,
    ClearNavigation,
    QuickOff,
    ResetProgress,
    EnterLobby,
    Open(u32, u32),
    Pop,
    Unavailable(Vec<u8>),
    KeyboardProfile,
    KeyboardTag,
    CommitBuilt {
        index: u32,
        instance: u64,
        pane_clock: u32,
        selected: u32,
    },
    DiscardBuilt,
    Revision,
}
pub trait Io {
    /// Cached raw native fields; provider/worker observations are separate.
    fn facts(&self) -> Result<Facts>;
    fn effect(&self, effect: Effect) -> Result<()>;
    fn random(&self) -> Result<Vec<u8>>;
    fn steam(&self) -> Result<bool>;
    fn authorized(&self) -> Result<bool>;
    fn snapshot(&self) -> Result<Snapshot>;
    fn identity(&self) -> Result<bool>;
    fn profile_name(&self) -> Result<Vec<u8>>;
    fn build(&self, screen: u32, context: u32) -> Result<Frame>;
}
pub fn online_screen(screen: u32) -> bool {
    matches!(
        screen,
        21 | 26
            | 27
            | 31
            | 34
            | 35
            | 36
            | 182
            | 186
            | 210
            | 211
            | 215
            | 216
            | 217
            | 219
            | 236
            | 237
    )
}
pub fn load_key(c: &Controller, name: Vec<u8>, key: Vec<u8>) {
    if key.len() == 32
        && key
            .iter()
            .all(|v| v.is_ascii_digit() || (b'a'..=b'f').contains(v))
    {
        c.edit_signin(|s| {
            s.keys.insert(name, key);
        });
    }
}
pub fn take_key(c: &Controller, name: &[u8]) -> Option<Vec<u8>> {
    c.take_signin_key(name)
}
pub fn restore_key(c: &Controller, io: &impl Io, key: Option<Vec<u8>>) -> Result<()> {
    if let Some(key) = key {
        let name = io.profile_name()?;
        c.edit_signin(|s| {
            s.keys.insert(name, key);
        });
    }
    Ok(())
}
pub fn key(c: &Controller, io: &impl Io, name: &[u8]) -> Result<Vec<u8>> {
    if name.is_empty() {
        return Ok(vec![]);
    }
    if let Some(key) = c.signin_state().keys.get(name) {
        return Ok(key.clone());
    }
    let bytes = io.random()?;
    if bytes.len() != 16 {
        return Err("Native saved sign-in randomness requires sixteen bytes".into());
    }
    let hex = b"0123456789abcdef";
    let value: Vec<_> = bytes
        .into_iter()
        .flat_map(|v| [hex[(v >> 4) as usize], hex[(v & 15) as usize]])
        .collect();
    c.edit_signin(|s| {
        s.keys.insert(name.to_vec(), value.clone());
    });
    io.effect(Effect::Save)?;
    Ok(value)
}
pub fn bind(c: &Controller, io: &impl Io) -> Result<()> {
    if !io.facts()?.login || !io.steam()? {
        return Ok(());
    }
    let name = io.facts()?.signed_in;
    let key = key(c, io, &name)?;
    if c.signin_state().bound == key {
        return Ok(());
    }
    io.effect(Effect::Bind(key.clone()))?;
    c.edit_signin(|s| {
        s.bound = key;
        s.suppressed = false;
    });
    io.effect(Effect::ResetPrior)?;
    if io.facts()?.account {
        io.effect(Effect::SignOutAccount)?;
    }
    io.effect(Effect::ClearAccount)?;
    if io.facts()?.queue {
        io.effect(Effect::SignOutQueue)?;
    }
    io.effect(Effect::StopSearch)?;
    if io.facts()?.voice {
        io.effect(Effect::LeaveVoice)?;
    }
    if io.facts()?.social {
        io.effect(Effect::DisconnectSocial)?;
    }
    c.clear_dismissed();
    Ok(())
}
pub fn prepare_open(
    c: &Controller,
    io: &impl Io,
    mut screen: u32,
    mut context: u32,
) -> Result<(u32, u32)> {
    if screen == 21 {
        screen = 20;
    }
    if io.facts()?.login
        && io.steam()?
        && (screen == 20 || online_screen(screen) || (io.facts()?.mode == 3 && screen == 14))
    {
        bind(c, io)?;
        if !c.signin_state().suppressed && !io.authorized()? {
            io.effect(Effect::Restore)?;
            c.edit_signin(|s| s.awaiting = true);
        }
    }
    if (online_screen(screen) || (io.facts()?.mode == 3 && matches!(screen, 14 | 204 | 231)))
        && !io.identity()?
    {
        c.edit_signin(|s| {
            s.pending_screen = screen;
            s.pending_context = context;
        });
        screen = 20;
        context = 0;
    } else if screen == 20 {
        c.edit_signin(|s| {
            s.pending_screen = 186;
            s.pending_context = 0;
        });
    }
    Ok((screen, context))
}
fn pop_to_main(io: &impl Io) -> Result<()> {
    loop {
        let f = io.facts()?;
        if f.frames.len() <= 1 || f.top()?.screen == 6 {
            return Ok(());
        }
        io.effect(Effect::Pop)?;
    }
}
pub fn update(c: &Controller, io: &impl Io) -> Result<()> {
    bind(c, io)?;
    for e in [
        Effect::UpdateCouch,
        Effect::UpdateFriends,
        Effect::UpdateAccount,
        Effect::UpdatePrivacy,
        Effect::UpdateLeave,
        Effect::UpdatePostgame,
    ] {
        io.effect(e)?;
    }
    if !io.facts()?.login {
        return Ok(());
    }
    let status = io.snapshot()?;
    if io.facts()?.top()?.screen == 20 {
        if c.signin_state().awaiting && io.identity()? {
            c.edit_signin(|s| s.awaiting = false);
            let s = c.signin_state();
            io.effect(Effect::Pop)?;
            io.effect(Effect::Open(s.pending_screen, s.pending_context))?;
            return Ok(());
        }
        if status.status == 4 {
            c.edit_signin(|s| s.awaiting = false);
        }
    } else if !status.identity && io.facts()?.frames.iter().any(|f| online_screen(f.screen)) {
        if io.facts()?.queue {
            io.effect(Effect::SignOutQueue)?;
        }
        io.effect(Effect::StopSearch)?;
        pop_to_main(io)?;
        io.effect(Effect::Open(20, 0))?;
    }
    let count = io.facts()?.frames.len();
    for index in 0..count {
        let f = io.facts()?;
        let old = f
            .frames
            .get(index)
            .ok_or("Native sign-in retained frame absent")?;
        if !matches!(old.screen, 20 | 186) {
            continue;
        }
        let updated = io.build(old.screen, old.context)?;
        let f = io.facts()?;
        let retained = f
            .frames
            .get(index)
            .ok_or("Native sign-in retained frame absent")?;
        if retained.description == updated.description {
            let a = retained
                .entries
                .first()
                .ok_or("Native sign-in retained label absent")?;
            let b = updated
                .entries
                .first()
                .ok_or("Native sign-in rebuilt label absent")?;
            if a.label == b.label {
                io.effect(Effect::DiscardBuilt)?;
                continue;
            }
        }
        io.effect(Effect::CommitBuilt {
            index: index as u32,
            instance: retained.instance,
            pane_clock: retained.pane_clock,
            selected: retained
                .selected
                .min((updated.entries.len() as u32).wrapping_sub(1)),
        })?;
        io.effect(Effect::Revision)?;
    }
    Ok(())
}
pub fn accept(c: &Controller, io: &impl Io) -> Result<bool> {
    if io.facts()?.top()?.screen != 20 {
        return Ok(false);
    }
    if io.identity()? {
        let f = io.facts()?;
        let top = f.top()?;
        if top.selected == 0 {
            io.effect(Effect::Open(186, 0))?;
        } else if top.selected == 1 {
            io.effect(Effect::KeyboardProfile)?;
            let empty = io.facts()?.keyboard.is_empty();
            io.effect(Effect::Open(167, if empty { 0 } else { 1 }))?;
        } else {
            let e = top
                .entries
                .get(top.selected as usize)
                .ok_or("Native sign-in selected entry absent")?;
            if e.destination == 167 && e.value == 10 {
                io.effect(Effect::KeyboardTag)?;
                io.effect(Effect::Open(167, 10))?;
            } else {
                if io.facts()?.queue {
                    io.effect(Effect::SignOutQueue)?;
                }
                io.effect(Effect::StopSearch)?;
                if io.facts()?.account {
                    io.effect(Effect::SignOutAccount)?;
                }
                if io.facts()?.voice {
                    io.effect(Effect::LeaveVoice)?;
                }
                c.set_voice_wanted(false);
                if io.facts()?.social {
                    io.effect(Effect::DisconnectSocial)?;
                }
                c.edit_signin(|s| s.suppressed = true);
                io.effect(Effect::SignOutLogin)?;
                io.effect(Effect::ResetPrior)?;
                io.effect(Effect::ClearCouch)?;
                if io.facts()?.directory {
                    io.effect(Effect::CancelDirectory)?;
                }
                io.effect(Effect::ClearDirectory)?;
                pop_to_main(io)?;
                io.effect(Effect::Open(20, 0))?;
            }
        }
    } else if io.facts()?.login {
        if io.snapshot()?.identity && io.facts()?.account {
            io.effect(Effect::ConnectAccount)?;
            c.edit_signin(|s| s.awaiting = true);
            update(c, io)?;
            return Ok(true);
        }
        let status = io.snapshot()?.status;
        if !matches!(status, 1 | 2) {
            io.effect(Effect::Start)?;
            c.edit_signin(|s| s.awaiting = true);
            update(c, io)?;
        }
    } else {
        io.effect(Effect::Unavailable(
            b"Account sign-in is unavailable in this preview. Start the game normally to sign in."
                .to_vec(),
        ))?;
    }
    Ok(true)
}
pub fn back(c: &Controller, io: &impl Io) -> Result<()> {
    if io.facts()?.top()?.screen == 20 && c.signin_state().awaiting {
        if io.facts()?.login {
            io.effect(Effect::Cancel)?;
        }
        c.edit_signin(|s| s.awaiting = false);
    }
    Ok(())
}
pub fn return_online(c: &Controller, io: &impl Io, message: &[u8]) -> Result<()> {
    io.effect(Effect::ClearNavigation)?;
    io.effect(Effect::Open(6, 0))?;
    let awaiting = !io.identity()?;
    c.edit_signin(|s| s.awaiting = awaiting);
    io.effect(Effect::Open(186, 0))?;
    if !message.is_empty() {
        io.effect(Effect::Unavailable(message.to_vec()))?;
    }
    Ok(())
}
pub fn return_lobby(c: &Controller, io: &impl Io, message: &[u8]) -> Result<()> {
    if io.facts()?.queue {
        io.effect(Effect::CancelQueue)?;
    }
    io.effect(Effect::QuickOff)?;
    io.effect(Effect::ResetProgress)?;
    return_online(c, io, &[])?;
    if io.facts()?.top()?.screen == 20 {
        c.edit_signin(|s| {
            s.pending_screen = 14;
            s.pending_context = 0;
        });
    } else {
        io.effect(Effect::Open(14, 0))?;
    }
    io.effect(Effect::EnterLobby)?;
    if !message.is_empty() {
        io.effect(Effect::Unavailable(message.to_vec()))?;
    }
    Ok(())
}
