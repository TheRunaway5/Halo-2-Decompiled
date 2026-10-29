//! Original hopper getters and the assignment corridor of retail 0x71730.
//!
//! The caller's session/readiness/summary producers remain separate owners.
//! No clocks, RNG, identities, downloaded settings or adjusted ranks originate
//! here. `assign` consumes the raw +0xD4 weights and +0x194 group indices.

use crate::team_balance::{self, Options};

pub const HOPPER_BYTES: usize = 0x614;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum HopperKind {
    CasualFfa = 1,
    RankedFfa = 2,
    CasualTeams = 3,
    RankedTeams = 4,
    Clan = 5,
}

impl TryFrom<i32> for HopperKind {
    type Error = ();
    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            1 => Ok(Self::CasualFfa),
            2 => Ok(Self::RankedFfa),
            3 => Ok(Self::CasualTeams),
            4 => Ok(Self::RankedTeams),
            5 => Ok(Self::Clan),
            _ => Err(()),
        }
    }
}

/// These are literal header fields, not independently inferred defaults.
/// In FFA width/height are player limits; in team hoppers width is team count,
/// height is minimum team size and depth is maximum team size.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Hopper {
    pub kind: HopperKind,
    pub width: i32,
    pub height: i32,
    pub depth: i32,
    pub imbalance: i32,
    pub flag_enabled: bool,
    pub flag_minimum: i32,
    pub flag_maximum: i32,
}

impl Hopper {
    /// Version-specific retail header parsing. Exact size and valid type are
    /// required; decoding does not establish historical provenance or validity
    /// of the complete hopper file.
    pub fn from_header(bytes: &[u8]) -> Option<Self> {
        if bytes.len() != HOPPER_BYTES {
            return None;
        }
        let word = |offset| i32::from_le_bytes(bytes[offset..offset + 4].try_into().unwrap());
        Some(Self {
            kind: word(0).try_into().ok()?,
            width: word(0x5ec),
            height: word(0x5f0),
            depth: word(0x5f4),
            imbalance: word(0x5f8),
            flag_enabled: bytes[0x604] != 0,
            flag_minimum: word(0x608),
            flag_maximum: word(0x60c),
        })
    }

    pub fn is_team(self) -> bool {
        matches!(
            self.kind,
            HopperKind::CasualTeams | HopperKind::RankedTeams | HopperKind::Clan
        )
    }

    /// Retail 193470/193490. Visibility is distinct from skill persistence.
    pub fn rank_visible(self) -> bool {
        matches!(
            self.kind,
            HopperKind::RankedFfa | HopperKind::RankedTeams | HopperKind::Clan
        )
    }

    pub fn team_count(self) -> i32 {
        if self.is_team() { self.width } else { -1 }
    }

    pub fn maximum_team_size(self) -> i32 {
        if self.is_team() { self.depth } else { 1 }
    }

    /// Retail 193400 reads only the low byte for type 3. A nonzero full word
    /// whose low byte is zero must still return zero.
    pub fn allowed_imbalance(self) -> i32 {
        match self.kind {
            HopperKind::CasualTeams => i32::from(self.imbalance as u8 != 0),
            HopperKind::RankedTeams | HopperKind::Clan => self.imbalance,
            _ => 0,
        }
    }

    pub fn flags(self) -> (bool, i32, i32) {
        match self.kind {
            HopperKind::Clan => (true, self.height, self.depth),
            HopperKind::RankedTeams if self.flag_enabled => {
                (true, self.flag_minimum, self.flag_maximum)
            }
            _ => (false, 0, 0),
        }
    }

    pub fn options(self, use_parties: bool) -> Option<Options> {
        let (flag_parties, minimum, maximum) = self.flags();
        if !self.is_team()
            || !(1..=4).contains(&self.width)
            || !(1..=16).contains(&self.depth)
            || self.allowed_imbalance() < 0
            || (flag_parties && (minimum < 0 || minimum > maximum || maximum > 16))
        {
            return None;
        }
        Some(Options {
            teams: self.width as usize,
            maximum_team_size: self.depth as usize,
            flag_parties,
            flag_minimum: minimum as usize,
            flag_maximum: maximum as usize,
            use_parties,
            optimize: true,
        })
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum AssignmentPath {
    FreeForAll = 0,
    PreservedParties = 1,
    IndividualRetry = 2,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Assignment {
    pub teams: Vec<i32>,
    pub path: AssignmentPath,
}

/// The original ordered final-assignment decisions, after the caller's genuine
/// summary/readiness gates. Capacity is checked before weighted optimization.
/// Failure returns no output; no arbitrary partition or random tie is tried.
pub fn assign(hopper: Hopper, parties: &[i32], raw_weights: &[i32]) -> Option<Assignment> {
    if parties.len() > 16
        || parties.len() != raw_weights.len()
        || parties.iter().any(|p| !(0..16).contains(p))
    {
        return None;
    }
    if !hopper.is_team() {
        return Some(Assignment {
            teams: (0..parties.len() as i32).collect(),
            path: AssignmentPath::FreeForAll,
        });
    }
    for preserve in [true, false] {
        if !preserve && hopper.kind != HopperKind::CasualTeams {
            break;
        }
        let options = hopper.options(preserve)?;
        if let Some(spread) = team_balance::by_count(options, parties)
            && spread <= hopper.allowed_imbalance()
            && let Some(teams) = team_balance::balance(options, parties, raw_weights)
        {
            return Some(Assignment {
                teams,
                path: if preserve {
                    AssignmentPath::PreservedParties
                } else {
                    AssignmentPath::IndividualRetry
                },
            });
        }
    }
    None
}

/// Explicit adapter for the existing native catalog. This is a service input
/// translation, not an authenticated historical hopper. Significant-party
/// spread is a separate admission rule, never a count-imbalance coefficient.
#[derive(Clone, Copy, Debug)]
pub struct NativePolicy {
    pub ranked: bool,
    pub team_count: u32,
    pub team_size: u32,
    pub clan_minimum: u32,
    pub clan_imbalance: u32,
    pub significant_minimum: u32,
    pub significant_maximum: u32,
    pub significant_spread: u32,
}

impl NativePolicy {
    pub fn hopper(self) -> Option<Hopper> {
        if !(2..=4).contains(&self.team_count)
            || self.team_size > 8
            || self.clan_minimum > self.team_size
            || self.clan_imbalance > 15
            || self.significant_minimum > self.significant_maximum
            || self.significant_maximum > 16
            || self.significant_spread > 15
        {
            return None;
        }
        let kind = if self.clan_minimum != 0 {
            if !self.ranked || self.team_count != 2 {
                return None;
            }
            HopperKind::Clan
        } else if self.team_size == 0 {
            if self.ranked {
                HopperKind::RankedFfa
            } else {
                HopperKind::CasualFfa
            }
        } else if self.ranked {
            HopperKind::RankedTeams
        } else {
            HopperKind::CasualTeams
        };
        Some(Hopper {
            kind,
            width: self.team_count as i32,
            height: if self.clan_minimum != 0 {
                self.clan_minimum
            } else {
                self.team_size
            } as i32,
            depth: self.team_size as i32,
            imbalance: self.clan_imbalance as i32,
            flag_enabled: self.significant_minimum != 0,
            flag_minimum: self.significant_minimum as i32,
            flag_maximum: self.significant_maximum as i32,
        })
    }

    /// Preserve the existing explicitly native published-party admission rule
    /// while its original summary producer is qualified separately. This never
    /// changes raw assignment weights or substitutes for original count checks.
    pub fn assign(self, parties: &[i32], raw_weights: &[i32]) -> Option<Assignment> {
        let hopper = self.hopper()?;
        if parties.len() > 16 || parties.iter().any(|p| !(0..16).contains(p)) {
            return None;
        }
        let mut counts = [0u32; 16];
        for &party in parties {
            counts[party as usize] += 1;
        }
        if self.clan_minimum != 0 {
            let groups: Vec<_> = counts.into_iter().filter(|&n| n != 0).collect();
            if groups.len() != 2
                || groups
                    .iter()
                    .any(|&n| n < self.clan_minimum || n > self.team_size)
                || groups[0].abs_diff(groups[1]) > self.clan_imbalance
            {
                return None;
            }
        } else if self.significant_minimum != 0 && hopper.is_team() {
            let groups: Vec<_> = counts
                .into_iter()
                .filter(|&n| n >= self.significant_minimum && n <= self.significant_maximum)
                .collect();
            if !groups.is_empty()
                && (groups.len() != self.team_count as usize
                    || groups.iter().max()?.abs_diff(*groups.iter().min()?)
                        > self.significant_spread)
            {
                return None;
            }
        }
        assign(hopper, parties, raw_weights)
    }
}
