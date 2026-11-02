//! Native accepted-party UI projection over actual profile/login/seat reads.
//! These are replacement-client producers, not recovered original Live rules.
pub type Result<T> = std::result::Result<T, String>;
pub type Appearance = [u8; 9];

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Local {
    pub name: Vec<u8>,
    pub guest: bool,
    pub appearance: Option<Appearance>,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Member {
    pub handle: Vec<u8>,
    pub name: Vec<u8>,
    pub ready: Vec<u8>,
    pub is_self: bool,
    pub leader: bool,
    pub online: bool,
    pub locals: Vec<Local>,
    pub controller: i32,
    pub guest: bool,
    pub appearance: Option<Appearance>,
    pub postgame: Option<bool>,
}
impl Default for Member {
    fn default() -> Self {
        Self {
            handle: Vec::new(),
            name: Vec::new(),
            ready: Vec::new(),
            is_self: false,
            leader: false,
            online: false,
            locals: Vec::new(),
            controller: -1,
            guest: false,
            appearance: None,
            postgame: None,
        }
    }
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Seat {
    pub controller: u32,
    pub name: Vec<u8>,
    pub appearance: Appearance,
}
pub trait Io {
    fn primary_appearance(&self) -> Result<Appearance>;
    fn local_players(&self) -> Result<Vec<Seat>>;
    /// Absent resource and present unauthenticated resource remain distinct IO.
    fn authorization(&self, controller: u32) -> Result<Option<bool>>;
}
pub fn members(party: Option<&[Member]>, io: &impl Io) -> Result<Vec<Member>> {
    let mut out = Vec::new();
    let Some(party) = party else {
        return Ok(out);
    };
    for member in party {
        let mut primary = member.clone();
        if member.is_self {
            primary.appearance = Some(io.primary_appearance()?);
        }
        out.push(primary);
        if member.is_self {
            for seat in io.local_players()? {
                if seat.controller == 0 {
                    continue;
                }
                let mut local = member.clone();
                local.name = seat.name;
                local.leader = false;
                local.locals.clear();
                local.controller = seat.controller as i32;
                local.appearance = Some(seat.appearance);
                local.guest = !io.authorization(seat.controller)?.unwrap_or(false);
                out.push(local);
            }
        } else {
            for seat in &member.locals {
                let mut local = member.clone();
                local.name = seat.name.clone();
                local.leader = false;
                local.locals.clear();
                local.guest = seat.guest;
                local.appearance = seat.appearance;
                // Remote seats retain the primary's connection/handle.
                local.controller = -2;
                out.push(local);
            }
        }
    }
    Ok(out)
}
#[derive(Clone, Debug, Default)]
pub struct Voice {
    pub party: bool,
    pub joined: bool,
    pub transmitting: bool,
    pub muted: Vec<Vec<u8>>,
    pub talking: Vec<Vec<u8>>,
}
pub fn column(players: &[Member], voice: &Voice, screen: u32, row: u32, column: u32) -> Vec<u8> {
    if screen != 26 || row as usize >= players.len() || column != 1 {
        return Vec::new();
    }
    let member = &players[row as usize];
    if member.guest {
        b"Guest".to_vec()
    } else if voice.party && voice.joined && voice.muted.contains(&member.handle) {
        b"Muted".to_vec()
    } else if voice.party
        && voice.joined
        && ((member.is_self && voice.transmitting) || voice.talking.contains(&member.handle))
    {
        b"Talking".to_vec()
    } else if !member.online {
        b"Offline".to_vec()
    } else if !member.ready.is_empty() {
        b"Ready".to_vec()
    } else if member.leader {
        b"Leader".to_vec()
    } else {
        b"ONLINE".to_vec()
    }
}
#[derive(Clone, Debug, Default)]
pub struct Membership {
    pub handle: Vec<u8>,
    pub local_count: u64,
}
#[derive(Clone, Debug, Default)]
pub struct Notification {
    pub authenticated: bool,
    pub busy: bool,
    pub party: Option<Vec<Membership>>,
    pub invitations: Vec<Vec<u8>>,
    pub message_empty: bool,
}
/// Literal native sound-table indices; the ABI adapter asserts their enum IDs.
pub fn sounds(before: &Notification, after: &Notification) -> Vec<u32> {
    if !before.authenticated || !after.authenticated {
        return Vec::new();
    }
    let mut out = Vec::new();
    let added = |a: &[Membership], b: &[Membership]| {
        b.iter().any(|member| {
            a.iter()
                .find(|old| old.handle == member.handle)
                .is_none_or(|old| old.local_count < member.local_count)
        })
    };
    match (&before.party, &after.party) {
        (None, Some(_)) => out.push(3),
        (Some(_), None) => out.push(4),
        (Some(before), Some(after)) => {
            if added(after, before) {
                out.push(4);
            }
            if added(before, after) {
                out.push(3);
            }
        }
        _ => (),
    }
    if after
        .invitations
        .iter()
        .any(|id| !before.invitations.contains(id))
    {
        out.push(8);
    }
    if before.busy && !after.busy && !after.message_empty {
        out.push(2);
    }
    out
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Invitation {
    pub id: Vec<u8>,
    pub name: Vec<u8>,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Notice {
    pub invitation: Invitation,
    pub busy: bool,
    pub can_accept: bool,
}
#[derive(Clone, Debug, Default)]
pub struct NoticeFacts {
    pub authenticated: bool,
    pub signed_in: Vec<u8>,
    pub invitations: Vec<Invitation>,
    pub dismissed: Vec<Vec<u8>>,
    pub busy: bool,
    pub party: bool,
    pub launch: bool,
    pub status: u32,
}
pub fn notice(f: &NoticeFacts) -> Option<Notice> {
    if !f.authenticated || f.signed_in.is_empty() {
        return None;
    }
    f.invitations
        .iter()
        .find(|invite| !f.dismissed.contains(&invite.id))
        .map(|invite| Notice {
            invitation: invite.clone(),
            busy: f.busy,
            can_accept: !f.busy && !f.party && !f.launch && !matches!(f.status, 6 | 7 | 8),
        })
}
