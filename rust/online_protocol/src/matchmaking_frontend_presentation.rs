//! Matchmaking text selection over actual native locale/asset observations.
//! These service-facing choices preserve the captured modern caller; original
//! UI/network producers and missing historical strings are separate evidence.
use crate::matchmaking_frontend_progress as progress;

pub type Result<T> = std::result::Result<T, String>;
#[derive(Clone, Debug, Default)]
pub struct Playlist {
    pub id: Vec<u8>,
    pub name: Vec<u8>,
    pub description: Vec<u8>,
    pub eligible: bool,
    pub ranked: bool,
    pub clan: bool,
    pub minimum: u32,
    pub maximum: u32,
    pub level: Option<u32>,
}
#[derive(Clone, Debug, Default)]
pub struct Facts {
    pub screen: u32,
    pub selected_row: u32,
    pub selected_id: Vec<u8>,
    pub playlists: Vec<Playlist>,
    pub status: u32,
    pub ui: i32,
    pub pane: u32,
    pub title: i32,
    pub countdown: i32,
    pub pc_progress: bool,
    pub matched: Option<u64>,
    pub gathering: Option<u64>,
    pub minimum: u32,
    pub queued: u32,
    pub variant: Vec<u8>,
    pub map: Vec<u8>,
    pub message: Vec<u8>,
    pub party: bool,
    pub leader: bool,
    pub party_message: Vec<u8>,
}
/// Locale/asset/profile readers, not alternative presentation policy owners.
/// Native callbacks execute without any mutable Rust controller state borrow.
pub trait Io {
    fn original_text(&self, screen: u32, pane: u32, index: u32) -> Result<Vec<u8>>;
    fn original_string(&self, screen: u32, id: u32) -> Result<Vec<u8>>;
    fn pc_string(&self, id: u32) -> Result<Vec<u8>>;
    fn pc_authored(&self, pane: u32) -> Result<Vec<u8>>;
    fn pc_match_line(&self) -> Result<Vec<u8>>;
    fn map_name(&self) -> Result<Vec<u8>>;
    fn quick_option(&self, index: u32) -> Result<Vec<u8>>;
    fn party_count(&self) -> Result<u64>;
    fn expand(&self, value: &[u8]) -> Result<Vec<u8>>;
}
fn append(mut a: Vec<u8>, b: &[u8]) -> Vec<u8> {
    a.extend_from_slice(b);
    a
}
fn replace(mut text: Vec<u8>, token: &[u8], value: &[u8]) -> Vec<u8> {
    if let Some(at) = text.windows(token.len()).position(|w| w == token) {
        text.splice(at..at + token.len(), value.iter().copied());
    }
    text
}
fn decimal(value: u64) -> Vec<u8> {
    value.to_string().into_bytes()
}
fn countdown(value: i32) -> Vec<u8> {
    if value == -1 {
        b":--".to_vec()
    } else {
        format!(":{value:02}").into_bytes()
    }
}
fn selected(f: &Facts) -> Option<&Playlist> {
    let id = if matches!(f.screen, 219 | 237) {
        f.playlists
            .get(f.selected_row as usize)
            .map(|p| &p.id)
            .unwrap_or(&f.selected_id)
    } else {
        &f.selected_id
    };
    f.playlists.iter().find(|p| &p.id == id)
}
pub fn original_text(f: &Facts, pane: u32, index: u32, io: &impl Io) -> Result<Vec<u8>> {
    // Preserve the caller's initial read/count even when a later branch
    // replaces the text, including its native error/exception precedence.
    let value = io.original_text(210, pane, index)?;
    let count = match f.matched.or(f.gathering) {
        Some(n) => n,
        None => io.party_count()?,
    };
    let value = replace(value, b"\xee\x90\xa5", &decimal(count));
    if index == 0 && pane == 3 {
        return Ok(if f.countdown == -1 {
            Vec::new()
        } else {
            replace(value, b"\xee\x91\x9b", &countdown(f.countdown))
        });
    }
    if index == 0 && matches!(pane, 0 | 1) {
        if let Some(title) = progress::title(f.title) {
            return io.expand(&io.original_string(210, title)?);
        }
    }
    if pane == 1 && index == 3 {
        match progress::needed(f.gathering.is_some(), count as i32, f.minimum as i32) {
            0 => return Ok(Vec::new()),
            2 => return io.original_string(210, 0x11000762),
            _ => {
                return Ok(replace(
                    io.original_string(210, 0x0e000761)?,
                    b"%d",
                    &decimal(u64::from(f.minimum).wrapping_sub(count)),
                ));
            }
        }
    }
    if pane == 3 && f.matched.is_some() {
        match index {
            2 => return Ok(replace(value, b"\xee\x90\x8a", &f.variant)),
            3 => return Ok(replace(value, b"\xee\x90\x88", &io.map_name()?)),
            4..=9 => return io.quick_option(index - 4),
            //257FBF needs actual source account/clan strings absent from the
            //replacement payload. Do not fabricate team-color clan names.
            11 => return Ok(Vec::new()),
            _ => (),
        }
    }
    io.expand(&value)
}
pub fn text(f: &Facts, index: u32, io: &impl Io) -> Result<Vec<u8>> {
    let p = selected(f);
    if matches!(f.screen, 219 | 237) {
        return match index {
            0 => io.original_text(f.screen, 0, 0),
            1 => Ok(p.map(|p| p.name.clone()).unwrap_or_default()),
            2 => Ok(p
                .map(|p| {
                    append(
                        p.description.clone(),
                        if p.eligible {
                            b""
                        } else if p.clan {
                            b"\n\nClan membership required; unavailable for this party."
                        } else {
                            b"\n\nUnavailable for this ONLINE client, party, or installed maps."
                        },
                    )
                })
                .unwrap_or_else(|| f.message.clone())),
            _ => Ok(Vec::new()),
        };
    }
    if f.screen == 210 {
        if index == 0 && f.pc_progress {
            if f.status == 5 || f.matched.is_some() {
                if f.pane == 3 && f.matched.is_some() {
                    let mut lines = Vec::new();
                    if f.countdown != -1 {
                        lines.push(replace(
                            io.pc_string(0x0f001afc)?,
                            b"\xee\x91\xb8",
                            &countdown(f.countdown),
                        ));
                    } else if let Some(title) = progress::title(f.ui) {
                        lines.push(io.pc_string(title - 2)?);
                    }
                    // This is one native rendering expression, retaining the
                    // actual selected locale/map readers and their ordering.
                    lines.push(io.pc_match_line()?);
                    let mut value = Vec::new();
                    for line in lines {
                        if !value.is_empty() {
                            value.push(b'\n');
                        }
                        value.extend(line);
                    }
                    return Ok(value);
                }
                let pane = progress::pc_pane(f.ui, f.countdown);
                if pane == 2 {
                    if let Some(title) = progress::title(f.ui) {
                        return io.pc_string(title - 2);
                    }
                }
                return io.pc_authored(pane);
            }
            return Ok(if f.message.is_empty() {
                b"Connecting to matchmaking...".to_vec()
            } else {
                f.message.clone()
            });
        }
        return Ok(match index {
            0 => {
                if f.status == 5 {
                    b"Searching for players...".to_vec()
                } else if f.matched.is_some() {
                    b"Players found".to_vec()
                } else {
                    b"ONLINE matchmaking".to_vec()
                }
            }
            1 => b"\xee\x84\x81 CANCEL".to_vec(),
            2 => append(
                decimal(u64::from(f.queued)),
                if f.queued == 1 {
                    b" player in queue"
                } else {
                    b" players in queue"
                },
            ),
            3 => p
                .map(|p| p.name.clone())
                .unwrap_or_else(|| b"Quickmatch".to_vec()),
            4 => {
                if f.matched.is_some() {
                    b"Match details".to_vec()
                } else {
                    b"Search status".to_vec()
                }
            }
            5 => {
                let mut value = if f.party && !f.leader && f.status == 3 {
                    if f.party_message.is_empty() {
                        b"Waiting for the party leader.".to_vec()
                    } else {
                        f.party_message.clone()
                    }
                } else {
                    f.message.clone()
                };
                if f.matched.is_some() {
                    value.extend_from_slice(b"\n\nMap: ");
                    value.extend_from_slice(&f.map);
                    value.extend_from_slice(b"\nGame: ");
                    value.extend_from_slice(&f.variant);
                }
                append(
                    value,
                    b"\n\nVoice: F6 on/off, F7 mic mute. Tap N for team radio.",
                )
            }
            _ => Vec::new(),
        });
    }
    Ok(match index {
        0 => {
            if let Some(p) = p {
                let mut value = if p.clan {
                    b"Clan Ranked".to_vec()
                } else if p.ranked {
                    b"Ranked".to_vec()
                } else {
                    b"Casual".to_vec()
                };
                value.extend_from_slice(
                    format!("\n\n{} - {} players", p.minimum, p.maximum).as_bytes(),
                );
                if let Some(level) = p.level {
                    value.extend_from_slice(if p.clan {
                        b"\n\nYour clan level: "
                    } else {
                        b"\n\nYour level: "
                    });
                    value.extend(decimal(u64::from(level)));
                } else if p.clan {
                    value.extend_from_slice(b"\n\nClan membership required");
                }
                value
            } else {
                b"ONLINE matchmaking".to_vec()
            }
        }
        1 => p
            .map(|p| p.name.clone())
            .unwrap_or_else(|| b"Playlists".to_vec()),
        2 => p
            .map(|p| {
                append(
                    p.description.clone(),
                    if p.eligible {
                        b""
                    } else {
                        b"\n\nUnavailable for this ONLINE client, party, or installed maps."
                    },
                )
            })
            .unwrap_or_else(|| f.message.clone()),
        _ => Vec::new(),
    })
}
