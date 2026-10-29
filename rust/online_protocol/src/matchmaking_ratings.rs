//! Raw/effective gathering ratings and the original rank search envelope.
//!
//! Retail owners: 7E210, 73530/1931A0, 90880/90B30 and the 1936E0
//! executable-default table. This module does not supply identity, grouping,
//! hopper epoch, persistent XP, query scheduling or result authority.

pub const NONE: i32 = -1;
pub const MAX_SEATS: usize = 16;
pub const RANK_COUNT: usize = 128;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct RankMap([u8; RANK_COUNT]);

impl RankMap {
    /// Original 193560 validates each signed byte against its own index. It
    /// does not require monotonicity; adding that restriction changes inputs.
    pub fn from_slice(values: &[u8]) -> Option<Self> {
        let values: [u8; RANK_COUNT] = values.try_into().ok()?;
        if values
            .iter()
            .enumerate()
            .any(|(i, &value)| value > 127 || value < i as u8)
        {
            return None;
        }
        Some(Self(values))
    }

    /// Extracts the version-specific rank bytes from an already admitted
    /// header. This alone does not authenticate a complete hopper/profile.
    pub fn from_header(header: &[u8]) -> Option<Self> {
        if header.len() != 0x614 {
            return None;
        }
        Self::from_slice(&header[0x51c..0x59c])
    }

    pub fn retail_launch() -> Self {
        Self(std::array::from_fn(|i| {
            let step = match i {
                0..=5 => 5,
                6..=15 => 6,
                16..=25 => 7,
                26..=30 => 8,
                31..=35 => 9,
                _ => 10,
            };
            (i + step).min(127) as u8
        }))
    }

    pub fn values(&self) -> &[u8; RANK_COUNT] {
        &self.0
    }

    /// 1931A0's backwards walk, preserving the NONE sentinel before indexing.
    pub fn inverse(&self, rank: i32) -> Option<i32> {
        if !(NONE..=127).contains(&rank) {
            return None;
        }
        let mut index = rank;
        while index > 0 && i32::from(self.0[(index - 1) as usize]) >= rank {
            index -= 1;
        }
        Some(index)
    }

    pub fn advertised_minimum(&self, minimum: i32, maximum: i32) -> Option<i32> {
        if !(NONE..=127).contains(&minimum) || !(NONE..=127).contains(&maximum) {
            return None;
        }
        if minimum == NONE {
            return Some(NONE);
        }
        if minimum > maximum {
            return None;
        }
        Some(minimum.max(self.inverse(maximum)?))
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct RawStatistics {
    pub average: i32,
    pub maximum: i32,
    pub minimum: i32,
    pub rated_count: usize,
}

/// The raw 7E210 aggregates exclude identity flag bits 1/2. The separately
/// derived effective array never overwrites these operands or their divisor.
/// Missing nonguest ranks are still an upstream readiness responsibility.
pub fn raw_statistics(raw: &[i32], identity_flags: &[u32]) -> Option<RawStatistics> {
    if raw.len() > MAX_SEATS
        || raw.len() != identity_flags.len()
        || raw.iter().any(|r| !(NONE..=127).contains(r))
    {
        return None;
    }
    let (mut sum, mut maximum, mut minimum, mut count) = (0, 0, 127, 0);
    for (&rank, &flags) in raw.iter().zip(identity_flags) {
        if flags & 3 != 0 {
            continue;
        }
        sum += rank;
        maximum = maximum.max(rank);
        minimum = minimum.min(rank);
        count += 1;
    }
    if count == 0 {
        return Some(RawStatistics {
            average: NONE,
            maximum: NONE,
            minimum: NONE,
            rated_count: 0,
        });
    }
    Some(RawStatistics {
        average: sum / count as i32,
        maximum,
        minimum,
        rated_count: count,
    })
}

/// Separate adjusted-player producer in 7E210. Group extrema include all raw
/// entries; identity flags affect the raw aggregates, not this highest lookup.
/// The original calls F(H), not the decompilation's erroneous F(F(H)).
pub fn adjusted_ranks(
    raw: &[i32],
    groups: &[i32],
    catalog_index: usize,
    map: &RankMap,
) -> Option<Vec<i32>> {
    if raw.len() > MAX_SEATS
        || raw.len() != groups.len()
        || catalog_index >= 16
        || raw.iter().any(|r| !(NONE..=127).contains(r))
        || groups.iter().any(|g| !(0..16).contains(g))
    {
        return None;
    }
    if catalog_index == 0 {
        return Some(raw.to_vec());
    }
    let mut highest = [NONE; MAX_SEATS];
    for (&rank, &group) in raw.iter().zip(groups) {
        highest[group as usize] = highest[group as usize].max(rank);
    }
    raw.iter()
        .zip(groups)
        .map(|(&rank, &group)| {
            if rank == NONE {
                Some(NONE)
            } else {
                Some(rank.max(map.inverse(highest[group as usize])?))
            }
        })
        .collect()
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SearchEnvelope {
    pub inverse_maximum: i32,
    pub mapped_minimum: i32,
    pub ceiling: i32,
    pub maximum_radius: i32,
    pub raw_maximum: i32,
    pub initial_radius: i32,
}

impl SearchEnvelope {
    /// Post-allocation 90880 arithmetic. No average or capped display level is
    /// an operand. Missing ranks must be resolved before this valid-rank owner.
    pub fn new(minimum: i32, maximum: i32, map: &RankMap) -> Option<Self> {
        if minimum < 0 || maximum > 127 || minimum > maximum {
            return None;
        }
        let inverse_maximum = map.inverse(maximum)?;
        let mapped_minimum = minimum.max(inverse_maximum);
        let ceiling = i32::from(map.0[mapped_minimum as usize]);
        Some(Self {
            inverse_maximum,
            mapped_minimum,
            ceiling,
            maximum_radius: (mapped_minimum - inverse_maximum).max(ceiling - maximum),
            raw_maximum: maximum,
            initial_radius: 0,
        })
    }

    /// Original bounded 90B30 rank filters. Casual unbounded-query scheduling
    /// belongs to its distinct completion-driven search owner.
    pub fn bounds(self, radius: i32) -> Option<(i32, i32)> {
        if !(0..=self.maximum_radius).contains(&radius) {
            return None;
        }
        Some((
            (self.mapped_minimum - radius).max(self.inverse_maximum),
            (self.raw_maximum + radius).min(self.ceiling),
        ))
    }
}
