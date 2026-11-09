//! Complete modern native signed-local/guest caller and controller join flow.
//! Provider/device/profile/frame IO is native; no state guard spans a callback.
//! This does not recover original Xbox/Live identity or service procedures.
use crate::matchmaking_frontend_account_flow::Entry;
use crate::matchmaking_frontend_party_flow::Controller;
pub type Result<T> = std::result::Result<T, String>;
pub const CONTEXT: u32 = 0x434f5543;
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct State {
    pub subjects: [Vec<u8>; 4],
    pub options: Option<u32>,
    pub joining: Option<u32>,
    pub joining_online: bool,
    pub host_profile: u32,
    pub revision: u64,
}
#[derive(Clone, Debug, Default)]
pub struct Frame {
    pub screen: u32,
    pub context: u32,
    pub selected: u32,
    pub instance: u64,
    pub description: Vec<u8>,
    pub entries: Vec<Entry>,
}
#[derive(Clone, Debug, Default)]
pub struct Facts {
    pub status: i32,
    pub mode: i32,
    pub primary_login: bool,
    pub account_client: bool,
    pub authenticated: bool,
    pub factory: bool,
    pub local_logins: Vec<bool>,
    pub local_accounts: Vec<bool>,
    pub profiles: Vec<Vec<u8>>,
    pub inputs: Vec<i32>,
    pub profile: u32,
    pub signed_in: Vec<u8>,
    pub system_link_host: bool,
    pub countdown: Option<i32>,
    pub frame: Option<Frame>,
    pub frames: Vec<Frame>,
}
#[derive(Clone, Debug, Default)]
pub struct Snapshot {
    pub status: i32,
    pub identity: Option<Vec<u8>>,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Login {
    Primary,
    Local(u32),
    Held,
}
#[derive(Clone, Debug)]
pub enum Effect {
    Title(Vec<u8>),
    Description(Vec<u8>),
    Replace(Vec<Entry>),
    Append(Entry),
    Open(u32, u32),
    Back,
    Pop,
    Revision,
    Unavailable(Vec<u8>),
    LocalProfile(u32, Vec<u8>),
    LocalInput(u32, i32),
    ResetLocalTeam(u32),
    Profile(u32),
    SignOutAccount(u32),
    ResetAccount(u32),
    ClearAccounts,
    ResetLogin(u32),
    CreateLogin(u32),
    BindLogin(u32),
    StartLogin(u32),
    CreateAccount(u32),
    ProfileAccount(u32),
    ConnectAccount(u32),
    DescriptionIndex(u32, Vec<u8>),
    CommitBuilt(u64, u32),
    DiscardBuilt,
    EmitLocal(u32),
    ProfileFromCurrent(u32),
}
pub trait Io {
    fn facts(&self) -> Result<Facts>;
    fn effect(&self, effect: Effect) -> Result<()>;
    fn hold_login(&self, slot: u32) -> Result<bool>;
    fn release_login(&self) -> Result<()>;
    fn snapshot(&self, login: Login) -> Result<Snapshot>;
    fn authorized(&self, login: Login) -> Result<bool>;
    fn steam(&self, login: Login) -> Result<bool>;
    fn cancel(&self, login: Login) -> Result<()>;
    fn identity(&self) -> Result<bool>;
    fn description(&self) -> Result<Vec<u8>>;
    fn build(&self) -> Result<Frame>;
    fn players(&self) -> Result<Vec<u32>>;
}
fn frame(f: &Facts) -> Result<&Frame> {
    f.frame
        .as_ref()
        .ok_or_else(|| "Native couch frame owner absent".into())
}
fn slot<T>(rows: &[T], i: u32) -> Result<&T> {
    rows.get(i as usize)
        .ok_or_else(|| "Native couch slot outside the produced four-controller bank".into())
}
fn row(label: impl Into<Vec<u8>>, destination: i32, value: i32) -> Entry {
    Entry {
        label: label.into(),
        destination,
        value,
    }
}
fn pending(status: i32) -> bool {
    matches!(status, 1 | 2)
}
pub fn screen(s: u32, c: u32) -> bool {
    s == 31 && c == CONTEXT
}
pub fn selected_controller(flow: &Controller) -> Option<u32> {
    let s = flow.couch_state();
    s.joining.or(s.options)
}
pub fn guest_entry(flow: &Controller, s: u32) -> bool {
    s == 10 && flow.couch_state().joining_online
}
pub fn profile_entry(flow: &Controller, io: &impl Io, s: u32, name: &[u8]) -> Result<bool> {
    if s != 10 || flow.couch_state().joining.is_none() {
        return Ok(true);
    }
    let f = io.facts()?;
    Ok(name != f.signed_in && !f.profiles.iter().any(|p| p == name))
}
pub fn identity(io: &impl Io) -> Result<bool> {
    let f = io.facts()?;
    if f.account_client && !f.authenticated {
        return Ok(false);
    }
    if !f.primary_login {
        return Ok(false);
    }
    Ok(io.snapshot(Login::Primary)?.identity.is_some())
}
pub fn mutable(io: &impl Io) -> Result<bool> {
    if !io.identity()? {
        return Ok(false);
    }
    Ok(!matches!(io.facts()?.status, 4..=8))
}
pub fn open(flow: &Controller, io: &impl Io, c: u32) -> Result<()> {
    if c == 0 || c >= 4 {
        return Ok(());
    }
    if slot(&io.facts()?.profiles, c)?.is_empty() || !mutable(io)? {
        return Ok(());
    }
    flow.edit_couch(|s| s.options = Some(c));
    io.effect(Effect::Open(31, CONTEXT))
}
pub fn populate(flow: &Controller, io: &impl Io) -> Result<()> {
    io.effect(Effect::Description(vec![]))?;
    io.effect(Effect::Title(b"LOCAL PLAYER".to_vec()))?;
    let Some(c) = flow.couch_state().options else {
        return io.effect(Effect::Replace(vec![row(b"Back", -5, 0)]));
    };
    io.effect(Effect::Title(
        format!("LOCAL PLAYER {}", c.wrapping_add(1)).into_bytes(),
    ))?;
    let held = io.hold_login(c)?;
    let status = if held {
        io.snapshot(Login::Held)?.status
    } else {
        0
    };
    let busy = pending(status);
    let provider = if io.facts()?.primary_login && io.steam(Login::Primary)? {
        "Steam"
    } else {
        "Discord"
    };
    io.effect(Effect::Replace(vec![row(
        format!(
            "{}",
            if busy {
                format!("Cancel {provider} Sign-In")
            } else {
                "Play as Guest".into()
            }
        ),
        -80,
        0,
    )]))?;
    if !busy {
        let linked = held && io.authorized(Login::Held)?;
        io.effect(Effect::Append(row(
            format!(
                "{} {provider}",
                if linked {
                    "Continue with"
                } else {
                    "Sign in with"
                }
            ),
            -80,
            1,
        )))?;
    }
    io.effect(Effect::Append(row(b"Remove Local Player", -80, 2)))?;
    io.release_login()
}
pub fn text(flow: &Controller, io: &impl Io, index: u32) -> Result<Vec<u8>> {
    let Some(c) = flow.couch_state().options else {
        return Ok(vec![]);
    };
    if index > 1 {
        return Ok(vec![]);
    }
    if index == 0 {
        return Ok(slot(&io.facts()?.profiles, c)?.clone());
    }
    let mut result = b"Casual only; stats not saved.".to_vec();
    if io.hold_login(c)? {
        let status = io.snapshot(Login::Held)?;
        if pending(status.status) {
            result = if io.steam(Login::Held)? {
                b"Finish Steam in your browser.".to_vec()
            } else {
                b"Finish Discord in your browser.".to_vec()
            };
        } else if io.authorized(Login::Held)? {
            result = if io.steam(Login::Held)? {
                b"Steam linked; stats saved.".to_vec()
            } else {
                b"Discord linked; stats saved.".to_vec()
            };
        } else if status.status == 4 {
            result = b"Sign-in failed. Try again.".to_vec();
        }
    }
    io.release_login()?;
    Ok(result)
}
pub fn accept(flow: &Controller, io: &impl Io) -> Result<bool> {
    let f = io.facts()?;
    let view = frame(&f)?;
    if view.screen == 10
        && flow.couch_state().joining_online
        && flow.couch_state().joining.is_some()
        && !view.entries.is_empty()
    {
        let e = slot(&view.entries, view.selected)?;
        if e.destination == -80 {
            let c = flow.couch_state().joining.unwrap();
            io.effect(Effect::LocalProfile(
                c,
                format!("Guest {}", c.wrapping_add(1)).into_bytes(),
            ))?;
            flow.edit_couch(|s| {
                s.joining = None;
                s.joining_online = false;
            });
            io.effect(Effect::Profile(flow.couch_state().host_profile))?;
            io.effect(Effect::Pop)?;
            flow.couch_changed();
            io.effect(Effect::Revision)?;
            open(flow, io, c)?;
            return Ok(true);
        }
    }
    let f = io.facts()?;
    let view = frame(&f)?;
    if !screen(view.screen, view.context) {
        return Ok(false);
    }
    let Some(c) = flow.couch_state().options else {
        return Ok(true);
    };
    if view.entries.is_empty() {
        return Ok(true);
    }
    let action = slot(&view.entries, view.selected)?.value;
    if action == 0 {
        if *slot(&io.facts()?.local_accounts, c)? {
            io.effect(Effect::SignOutAccount(c))?;
        }
        io.effect(Effect::ResetAccount(c))?;
        if *slot(&io.facts()?.local_logins, c)? {
            io.cancel(Login::Local(c))?;
        }
        io.effect(Effect::ResetLogin(c))?;
        flow.edit_couch(|s| s.subjects[c as usize].clear());
        flow.couch_changed();
        io.effect(Effect::Back)?;
        return Ok(true);
    }
    if action == 2 {
        io.effect(Effect::Back)?;
        leave(flow, io, c)?;
        return Ok(true);
    }
    if *slot(&io.facts()?.local_logins, c)? && io.authorized(Login::Local(c))? {
        io.effect(Effect::Back)?;
        return Ok(true);
    }
    if !io.facts()?.factory {
        io.effect(Effect::Unavailable(
            b"Account sign-in is unavailable for this local player.".to_vec(),
        ))?;
        return Ok(true);
    }
    let mut busy = io.facts()?.primary_login && pending(io.snapshot(Login::Primary)?.status);
    if !busy {
        let count = io.facts()?.local_logins.len();
        for i in 0..count {
            if *slot(&io.facts()?.local_logins, i as u32)?
                && pending(io.snapshot(Login::Local(i as u32))?.status)
            {
                busy = true;
                break;
            }
        }
    }
    if busy {
        io.effect(Effect::Unavailable(
            b"Finish the current account sign-in first.".to_vec(),
        ))?;
        return Ok(true);
    }
    if !*slot(&io.facts()?.local_logins, c)? {
        io.effect(Effect::CreateLogin(c))?;
    }
    if !*slot(&io.facts()?.local_logins, c)? {
        return Err("Native local login factory produced no owner".into());
    }
    io.effect(Effect::BindLogin(c))?;
    io.effect(Effect::StartLogin(c))?;
    io.effect(Effect::Revision)?;
    Ok(true)
}
pub fn update(flow: &Controller, io: &impl Io) -> Result<()> {
    if io.identity()? {
        let count = io.facts()?.frames.len();
        for i in 0..count {
            let f = io.facts()?;
            if slot(&f.frames, i as u32)?.screen == 186 {
                let description = io.description()?;
                if slot(&io.facts()?.frames, i as u32)?.description != description {
                    io.effect(Effect::DescriptionIndex(i as u32, description))?;
                    io.effect(Effect::Revision)?;
                }
            }
        }
    }
    for i in 1..4 {
        let identity = if *slot(&io.facts()?.local_logins, i)? && io.authorized(Login::Local(i))? {
            io.snapshot(Login::Local(i))?.identity
        } else {
            None
        };
        let subject = identity.clone().unwrap_or_default();
        if subject != flow.couch_state().subjects[i as usize] {
            flow.edit_couch(|s| s.subjects[i as usize] = subject.clone());
            flow.couch_changed();
            io.effect(Effect::Revision)?;
        }
        let primary = if io.facts()?.primary_login {
            io.snapshot(Login::Primary)?.identity
        } else {
            None
        };
        let duplicate = identity.is_some()
            && (primary.as_ref() == identity.as_ref()
                || flow.couch_state().subjects[1..i as usize].contains(&subject));
        if !io.identity()?
            || !io.facts()?.account_client
            || identity.is_none()
            || duplicate
            || slot(&io.facts()?.profiles, i)?.is_empty()
        {
            if *slot(&io.facts()?.local_accounts, i)? {
                io.effect(Effect::SignOutAccount(i))?;
            }
            io.effect(Effect::ResetAccount(i))?;
        } else {
            if !*slot(&io.facts()?.local_accounts, i)? {
                io.effect(Effect::CreateAccount(i))?;
            }
            if !*slot(&io.facts()?.local_accounts, i)? {
                return Err("Native profile worker produced no owner".into());
            }
            io.effect(Effect::ProfileAccount(i))?;
            io.effect(Effect::ConnectAccount(i))?;
        }
    }
    let f = io.facts()?;
    let current = frame(&f)?;
    if screen(current.screen, current.context) {
        let updated = io.build()?;
        let f = io.facts()?;
        let current = frame(&f)?;
        let equal = current.entries.len() == updated.entries.len()
            && current.entries.iter().zip(&updated.entries).all(|(a, b)| {
                a.label == b.label && a.destination == b.destination && a.value == b.value
            });
        if !equal {
            let selected = current
                .selected
                .min((updated.entries.len() as u32).wrapping_sub(1));
            io.effect(Effect::CommitBuilt(current.instance, selected))?;
            io.effect(Effect::Revision)?;
        } else {
            io.effect(Effect::DiscardBuilt)?;
        }
    }
    Ok(())
}
pub fn clear(flow: &Controller, io: &impl Io) -> Result<()> {
    let count = io.facts()?.local_accounts.len();
    for i in 0..count {
        if *slot(&io.facts()?.local_accounts, i as u32)? {
            io.effect(Effect::SignOutAccount(i as u32))?;
        }
    }
    io.effect(Effect::ClearAccounts)?;
    let count = io.facts()?.local_logins.len();
    for i in 0..count {
        if *slot(&io.facts()?.local_logins, i as u32)? {
            io.cancel(Login::Local(i as u32))?;
        }
        io.effect(Effect::ResetLogin(i as u32))?;
    }
    flow.edit_couch(|s| {
        s.subjects = Default::default();
        s.options = None;
        s.joining_online = false;
    });
    flow.couch_changed();
    Ok(())
}
pub fn local_players(io: &impl Io) -> Result<()> {
    for (i, c) in io.players()?.into_iter().enumerate() {
        if c != 0 {
            if c >= 4 {
                return Err("Native local player has no produced controller login slot".into());
            }
            io.effect(Effect::EmitLocal(i as u32))?;
        }
    }
    Ok(())
}
pub fn controller_slot(
    flow: &Controller,
    io: &impl Io,
    instance: i32,
    join: bool,
) -> Result<Option<u32>> {
    if instance < 0 {
        return Ok(None);
    }
    let f = io.facts()?;
    for (i, id) in f.inputs.iter().enumerate() {
        if *id == instance {
            return Ok(Some(i as u32));
        }
    }
    let state = flow.couch_state();
    if state.joining.is_some() || state.options.is_some() {
        return Ok(None);
    }
    let online = if mutable(io)? {
        let f = io.facts()?;
        let view = frame(&f)?;
        view.screen == 186
            || (view.screen == 14 && f.mode == 3)
            || (view.screen == 26 && view.context == 2)
    } else {
        false
    };
    let f = io.facts()?;
    if join && *slot(&f.inputs, 0)? == -1 && !online {
        io.effect(Effect::LocalInput(0, instance))?;
        return Ok(Some(0));
    }
    let view = frame(&f)?;
    let local =
        matches!(view.screen, 30 | 14) && (f.mode == 1 || (f.mode == 2 && !f.system_link_host));
    if (local || online) && join && !f.signed_in.is_empty() {
        let count = f.profiles.len();
        for i in 1..count {
            if slot(&io.facts()?.profiles, i as u32)?.is_empty() {
                let c = i as u32;
                io.effect(Effect::LocalInput(c, instance))?;
                flow.edit_couch(|s| s.joining = Some(c));
                let profile = io.facts()?.profile;
                flow.edit_couch(|s| {
                    s.host_profile = profile;
                    s.joining_online = online;
                });
                if io.facts()?.countdown.is_some_and(|v| v >= 0) {
                    io.effect(Effect::Back)?;
                }
                io.effect(Effect::Open(10, 0))?;
                return Ok(Some(c));
            }
        }
    }
    Ok(None)
}
pub fn finish(flow: &Controller, io: &impl Io) -> Result<()> {
    let s = flow.couch_state();
    let Some(c) = s.joining else {
        return Ok(());
    };
    let online = s.joining_online;
    io.effect(Effect::ProfileFromCurrent(c))?;
    io.effect(Effect::ResetLocalTeam(c))?;
    flow.edit_couch(|s| s.joining = None);
    io.effect(Effect::Profile(flow.couch_state().host_profile))?;
    io.effect(Effect::Revision)?;
    flow.edit_couch(|s| s.joining_online = false);
    if online {
        flow.couch_changed();
        open(flow, io, c)?;
    }
    Ok(())
}
pub fn leave(flow: &Controller, io: &impl Io, c: u32) -> Result<()> {
    if c == 0 || c >= 4 || flow.couch_state().joining.is_some() {
        return Ok(());
    }
    if io.facts()?.mode == 3 && !mutable(io)? {
        return Ok(());
    }
    if io.facts()?.countdown.is_some_and(|v| v >= 0) {
        io.effect(Effect::Back)?;
    }
    io.effect(Effect::LocalProfile(c, vec![]))?;
    io.effect(Effect::LocalInput(c, -1))?;
    io.effect(Effect::ResetLocalTeam(c))?;
    io.effect(Effect::Revision)?;
    if *slot(&io.facts()?.local_logins, c)? {
        io.cancel(Login::Local(c))?;
    }
    io.effect(Effect::ResetLogin(c))?;
    flow.edit_couch(|s| s.subjects[c as usize].clear());
    flow.couch_changed();
    if *slot(&io.facts()?.local_accounts, c)? {
        io.effect(Effect::SignOutAccount(c))?;
    }
    io.effect(Effect::ResetAccount(c))
}
pub fn back(flow: &Controller, io: &impl Io) -> Result<bool> {
    let f = io.facts()?;
    let view = frame(&f)?;
    if !screen(view.screen, view.context) {
        return Ok(false);
    }
    if let Some(c) = flow.couch_state().options {
        if io.hold_login(c)? && !io.authorized(Login::Held)? {
            io.cancel(Login::Held)?;
            let index = flow
                .couch_state()
                .options
                .ok_or("Native couch options disappeared during the original reset expression")?;
            io.effect(Effect::ResetLogin(index))?;
        }
        io.release_login()?;
    }
    flow.edit_couch(|s| s.options = None);
    io.effect(Effect::Pop)?;
    flow.couch_changed();
    io.effect(Effect::Revision)?;
    Ok(true)
}
pub fn back_join(flow: &Controller, io: &impl Io) -> Result<bool> {
    let Some(c) = flow.couch_state().joining else {
        return Ok(false);
    };
    if frame(&io.facts()?)?.screen != 10 {
        return Ok(false);
    }
    io.effect(Effect::LocalInput(c, -1))?;
    flow.edit_couch(|s| s.joining = None);
    io.effect(Effect::Profile(flow.couch_state().host_profile))?;
    flow.edit_couch(|s| s.joining_online = false);
    io.effect(Effect::Pop)?;
    io.effect(Effect::Revision)?;
    Ok(true)
}
