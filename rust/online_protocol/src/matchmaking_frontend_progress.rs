//! Matchmaking progress presentation policy. Original locators are TU1.5;
//! service observations remain explicit inputs, not recovered Xbox producers.
use std::cmp::Ordering;

pub const NONE: i32 = -1;
pub fn start_countdown(elapsed: u32) -> (i32, bool) {
    let remaining = 10_000u32.saturating_sub(elapsed);
    (remaining.div_ceil(1000) as i32, remaining == 0)
}
pub fn published_countdown(remaining: Option<u32>) -> (i32, bool) {
    match remaining {
        Some(n @ 0..=10_000) => (n.div_ceil(1000) as i32, n == 0),
        _ => (NONE, false),
    }
}
pub fn pane(ui: i32, countdown: i32) -> u32 {
    match ui {
        4 => 1,
        6 => 2,
        7 => {
            if countdown != NONE {
                3
            } else {
                2
            }
        }
        8 => 3,
        _ => 0,
    }
}
pub fn pc_pane(ui: i32, countdown: i32) -> u32 {
    if ui == 5 {
        2
    } else {
        pane(ui, countdown).min(2)
    }
}
pub fn pane_sound(previous: u32, current: u32) -> Option<u32> {
    (previous != current && current == 3).then_some(12)
}
pub fn title(ui: i32) -> Option<u32> {
    const TITLES: [u32; 9] = [
        0x1400043a, 0x1200043b, 0x1300043c, 0x1300043d, 0x0e00043e, 0x0c00043f, 0x0d000440,
        0x0d000440, 0x16000441,
    ];
    usize::try_from(ui)
        .ok()
        .and_then(|i| TITLES.get(i))
        .copied()
}
pub fn cancel_allowed(pane: u32, elapsed: u32) -> bool {
    pane < 2 || (elapsed as i32) >= 60_000
}
pub fn consumes_y(pane: u32) -> bool {
    pane >= 2
}
pub fn leave_dialog(party_players: usize, local_players: usize) -> u32 {
    if party_players <= local_players {
        0x8d
    } else {
        0x8c
    }
}
pub fn team_colors(countdown: i32, teams: bool) -> bool {
    countdown != NONE && teams
}
pub fn uses_party(pane: u32, ranked: bool) -> bool {
    pane != 3 && (pane < 1 || ranked)
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum RowKind {
    Player,
    Matched,
    Empty,
}
pub fn rows(
    pane: u32,
    ranked: bool,
    party_players: u32,
    game_players: u32,
    maximum: Option<u32>,
) -> Vec<RowKind> {
    let party = uses_party(pane, ranked);
    let known = if party { party_players } else { game_players };
    let count = if party || pane == 1 {
        known.max(maximum.unwrap_or(0))
    } else {
        known
    };
    (0..count)
        .map(|i| {
            if i < known {
                RowKind::Player
            } else if i < game_players {
                RowKind::Matched
            } else {
                RowKind::Empty
            }
        })
        .collect()
}

#[derive(Clone, Copy, Debug, Default)]
pub struct Player {
    pub team: i32,
    pub value: i32,
}
/// 25790E caller -> 32B560 qsort -> 257692/2576D7/25770A comparators.
/// Both keys are source signed bytes. The admitted service slot sequence is
/// an explicit boundary. Equal keys deliberately follow the original swaps.
pub fn order(players: &[Player], ranked: bool, teams: bool) -> Vec<u32> {
    let mut indices: Vec<u32> = (0..players.len() as u32).collect();
    if !ranked && !teams {
        return indices;
    }
    let compare = |a: u32, b: u32| {
        let a = players[a as usize];
        let b = players[b as usize];
        let team = if teams {
            (a.team as i8).cmp(&(b.team as i8))
        } else {
            Ordering::Equal
        };
        if team != Ordering::Equal {
            team
        } else if ranked {
            (b.value as i8).cmp(&(a.value as i8))
        } else {
            Ordering::Equal
        }
    };
    original_qsort(&mut indices, &compare);
    indices
}
pub fn pane_order(players: &[Player], pane: u32, ranked: bool, teams: bool) -> Vec<u32> {
    order(players, pane == 3 && ranked, pane == 3 && teams)
}

// Exact element-index translation of TU1.5 CRT 32B560 and its 32B4F0
// small-sort helper. Retain pivot movement, equal-key scans, cutoff and
// smaller-partition-first stack order; std sorting has different tie order.
fn original_qsort(a: &mut [u32], compare: &impl Fn(u32, u32) -> Ordering) {
    if a.len() < 2 {
        return;
    }
    let mut stack = vec![(0isize, a.len() as isize - 1)];
    let cmp = |a: &[u32], x: isize, y: isize| compare(a[x as usize], a[y as usize]);
    while let Some((lo, hi)) = stack.pop() {
        if hi - lo < 8 {
            let mut high = hi;
            while high > lo {
                let mut maximum = lo;
                for p in lo + 1..=high {
                    if cmp(a, p, maximum) == Ordering::Greater {
                        maximum = p;
                    }
                }
                a.swap(maximum as usize, high as usize);
                high -= 1;
            }
            continue;
        }
        let mut mid = lo + (hi - lo + 1) / 2;
        if cmp(a, lo, mid) == Ordering::Greater {
            a.swap(lo as usize, mid as usize);
        }
        if cmp(a, lo, hi) == Ordering::Greater {
            a.swap(lo as usize, hi as usize);
        }
        if cmp(a, mid, hi) == Ordering::Greater {
            a.swap(mid as usize, hi as usize);
        }
        let mut low = lo;
        let mut high = hi;
        loop {
            // The two original scans avoid comparing the pivot to itself.
            if mid > low {
                loop {
                    low += 1;
                    if low >= mid || cmp(a, low, mid) == Ordering::Greater {
                        break;
                    }
                }
            }
            if mid <= low {
                loop {
                    low += 1;
                    if low > hi || cmp(a, low, mid) == Ordering::Greater {
                        break;
                    }
                }
            }
            loop {
                high -= 1;
                if high <= mid || cmp(a, high, mid) != Ordering::Greater {
                    break;
                }
            }
            if low > high {
                break;
            }
            a.swap(low as usize, high as usize);
            if mid == high {
                mid = low;
            }
        }
        high += 1;
        if mid < high {
            loop {
                high -= 1;
                if high <= mid || cmp(a, high, mid) != Ordering::Equal {
                    break;
                }
            }
        }
        if mid >= high {
            loop {
                high -= 1;
                if high <= lo || cmp(a, high, mid) != Ordering::Equal {
                    break;
                }
            }
        }
        let left = (lo < high).then_some((lo, high));
        let right = (low < hi).then_some((low, hi));
        if high - lo >= hi - low {
            if let Some(p) = left {
                stack.push(p);
            }
            if let Some(p) = right {
                stack.push(p);
            }
        } else {
            if let Some(p) = right {
                stack.push(p);
            }
            if let Some(p) = left {
                stack.push(p);
            }
        }
    }
}

pub fn option_category(variant_type: i32) -> Option<u32> {
    match variant_type {
        1 => Some(22),
        2 => Some(23),
        3 => Some(24),
        4 => Some(25),
        7 => Some(28),
        8 => Some(29),
        9 => Some(30),
        _ => None,
    }
}
pub fn game_icon(variant_type: i32) -> u32 {
    match variant_type {
        1 => 6,
        2 => 0,
        3 => 3,
        4 => 1,
        7 => 4,
        8 => 8,
        9 => 7,
        _ => 10,
    }
}
pub fn needed(available: bool, count: i32, required: i32) -> u8 {
    if !available {
        0
    } else if count < required {
        1
    } else {
        2
    }
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct TipClock {
    last: u32,
    initialized: bool,
}
impl TipClock {
    pub fn new(last: u32, initialized: bool) -> Self {
        Self { last, initialized }
    }
    pub fn update(&mut self, now: u32) -> bool {
        if (now.wrapping_sub(self.last) as i32) > 15_000 {
            self.initialized = false;
            self.last = now;
        }
        if self.initialized {
            false
        } else {
            self.initialized = true;
            true
        }
    }
}
pub fn tip_index(seed: &mut u32, count: u32) -> Option<u32> {
    if count > 32 {
        return None;
    }
    *seed = seed.wrapping_mul(0x19660d).wrapping_add(0x3c6ef35f);
    Some(((*seed >> 16) * count) >> 16)
}

#[derive(Clone, Debug, Default)]
pub struct Member {
    pub key: Player,
    pub level: Option<u32>,
    pub self_player: bool,
    pub local_controller: i32,
    pub handle: String,
}
#[derive(Clone, Debug, Default)]
pub struct Facts {
    pub pane: u32,
    pub ui: i32,
    pub status: u32,
    pub ranked: bool,
    pub teams: bool,
    pub party_present: bool,
    pub match_present: bool,
    pub gathering_present: bool,
    pub party: Vec<Member>,
    pub local: Vec<Member>,
    pub matched: Vec<Member>,
    pub gathering: Vec<Member>,
    pub maximum: u32,
    pub viewer_level: Option<u32>,
    pub party_voice: bool,
    pub voice_joined: bool,
    pub microphone_muted: bool,
    pub transmitting: bool,
    pub talking: Vec<String>,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Source {
    None,
    Party,
    Local,
    Match,
    Gathering,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Row {
    pub kind: RowKind,
    pub source: Source,
    pub index: u32,
    pub rank: Option<u32>,
    pub speaker: Option<u32>,
}
/// Select actual observations without revealing ranked opponents before pane3
/// or assigning identity/rank/voice from a display name.
pub fn project(f: &Facts) -> Vec<Row> {
    let party = uses_party(f.pane, f.ranked);
    let (source, members) = if party {
        if f.party_present {
            (Source::Party, &f.party)
        } else {
            (Source::Local, &f.local)
        }
    } else if f.match_present {
        (Source::Match, &f.matched)
    } else if f.gathering_present {
        (Source::Gathering, &f.gathering)
    } else {
        (Source::None, &f.local)
    };
    let known = if source == Source::None {
        0
    } else {
        members.len() as u32
    };
    let game = if f.match_present {
        f.matched.len()
    } else if f.gathering_present {
        f.gathering.len()
    } else {
        0
    } as u32;
    let max = (f.gathering_present && matches!(f.ui, 4 | 6)).then_some(f.maximum);
    let indices = if source == Source::Match && f.pane == 3 {
        order(
            &members.iter().map(|m| m.key).collect::<Vec<_>>(),
            f.ranked,
            f.teams,
        )
    } else {
        (0..known).collect()
    };
    rows(f.pane, f.ranked, if party { known } else { 0 }, game, max)
        .into_iter()
        .enumerate()
        .map(|(row, kind)| {
            let index = indices.get(row).copied().unwrap_or(row as u32);
            let member = (kind == RowKind::Player)
                .then(|| members.get(index as usize))
                .flatten();
            let level = if f.ranked && (4..=8).contains(&f.status) {
                match source {
                    Source::Party => member
                        .filter(|m| m.self_player && m.local_controller == -1)
                        .and(f.viewer_level),
                    Source::Local => (row == 0).then_some(f.viewer_level).flatten(),
                    Source::Match | Source::Gathering => member.and_then(|m| m.level),
                    Source::None => (row == 0 && matches!(f.status, 4 | 5))
                        .then_some(f.viewer_level)
                        .flatten(),
                }
            } else {
                None
            };
            let rank = level.filter(|n| (1..=50).contains(n)).map(|n| n - 1);
            let speaker = member
                .filter(|_| source == Source::Party && f.party_voice && f.voice_joined)
                .filter(|m| m.local_controller == -1)
                .and_then(|m| {
                    if m.self_player {
                        Some(if f.microphone_muted {
                            2
                        } else if f.transmitting {
                            0
                        } else {
                            1
                        })
                    } else {
                        f.talking.iter().any(|h| h == &m.handle).then_some(0)
                    }
                });
            Row {
                kind,
                source: if kind == RowKind::Player {
                    source
                } else {
                    Source::None
                },
                index,
                rank: if kind == RowKind::Player { rank } else { None },
                speaker,
            }
        })
        .collect()
}
pub fn visible_row(screen: i32, entries: &[i32], row: u32) -> bool {
    screen == 210
        && entries
            .get(row as usize)
            .is_some_and(|v| *v != -2 && *v != -3)
}
/// Queries also honor the currently installed native frame. A stale frame may
/// still contain row0 while a refresh has no roster; retain its existing
/// viewer-rank binding without granting an appearance or microphone identity.
pub fn query(f: &Facts, screen: i32, entries: &[i32], row: u32) -> Option<Row> {
    if !visible_row(screen, entries, row) {
        return None;
    }
    if let Some(r) = project(f).get(row as usize) {
        return Some(r.clone());
    }
    let party = uses_party(f.pane, f.ranked);
    let fallback = row == 0
        && f.ranked
        && (4..=8).contains(&f.status)
        && ((party && !f.party_present)
            || (!party && !f.match_present && !f.gathering_present && matches!(f.status, 4 | 5)));
    Some(Row {
        kind: RowKind::Player,
        source: Source::None,
        index: row,
        rank: fallback
            .then_some(f.viewer_level)
            .flatten()
            .filter(|n| (1..=50).contains(n))
            .map(|n| n - 1),
        speaker: None,
    })
}
