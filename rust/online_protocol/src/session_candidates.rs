//! Original retail candidate admission, scoring, sort and selection.
//!
//! The fixed 50 x 0x204 bank is an owned internal representation, not a public
//! network packet. Inputs must come from the admitted search/description/QoS
//! producers. Live service ordering and query/task lifecycles are separate.
//! Original owners: 90D90, 91000, 90F10, 13DA70/13DCD0, 91400 and 90B30.

use serde_json::{Value, json};

pub const COUNT: usize = 50;
pub const STRIDE: usize = 0x204;
pub const BANK_SIZE: usize = COUNT * STRIDE;
pub const REQUEST_LIMIT: usize = 16 * 1024;
pub type Record = [u8; STRIDE];

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    Search,
    Configuration,
    Bank,
    Record,
}

#[derive(Clone, Debug, PartialEq)]
pub struct Configuration {
    pub rank_radius: i32,
    pub rank_high: i32,
    pub rank_low: i32,
    pub exact_space_bonus: i32,
    pub maximum_age_seconds: i32,
    pub age_coefficient: f64,
    pub first_quality_threshold: i32,
    pub second_quality_threshold: i32,
    pub quality_adjustment: i32,
    pub latency_curve: [i32; 10],
    pub language: [[i32; 9]; 9],
    pub partial_qos_delay_ms: i32,
    pub threshold_initial: i32,
    pub threshold_final: i32,
}

impl Default for Configuration {
    fn default() -> Self {
        let language = std::array::from_fn(|a| {
            std::array::from_fn(|b| {
                let special = |n| matches!(n, 1 | 6 | 7);
                if a == b {
                    if special(a) { 100 } else { 0 }
                } else if special(a) || special(b) {
                    -100
                } else {
                    -50
                }
            })
        });
        Self {
            rank_radius: 4,
            rank_high: 100,
            rank_low: 25,
            exact_space_bonus: 50,
            maximum_age_seconds: 180,
            age_coefficient: 2.0,
            first_quality_threshold: 262144,
            second_quality_threshold: 262144,
            quality_adjustment: 100,
            latency_curve: [100, 80, 50, 0, -100, -200, -200, -200, -200, -200],
            language,
            partial_qos_delay_ms: 5000,
            threshold_initial: 200,
            threshold_final: 100,
        }
    }
}

impl Configuration {
    pub fn validate(&self) -> Result<(), Error> {
        if self.rank_radius < 0
            || self.rank_radius == i32::MAX
            || self.maximum_age_seconds < 0
            || self.partial_qos_delay_ms < 0
            || !self.age_coefficient.is_finite()
        {
            return Err(Error::Configuration);
        }
        Ok(())
    }

    pub fn from_json(value: &Value) -> Result<Self, Error> {
        let parse = || -> Option<Self> {
            exact_fields(
                value,
                &[
                    "rank_radius",
                    "rank_high",
                    "rank_low",
                    "exact_space_bonus",
                    "maximum_age_seconds",
                    "age_coefficient",
                    "first_quality_threshold",
                    "second_quality_threshold",
                    "quality_adjustment",
                    "latency_curve",
                    "language",
                    "partial_qos_delay_ms",
                    "threshold_initial",
                    "threshold_final",
                ],
            )?;
            let rows = value["language"].as_array()?;
            if rows.len() != 9 || !value["age_coefficient"].is_f64() {
                return None;
            }
            let mut language = [[0; 9]; 9];
            for (target, row) in language.iter_mut().zip(rows) {
                *target = words(row)?;
            }
            Some(Self {
                rank_radius: integer(&value["rank_radius"])?,
                rank_high: integer(&value["rank_high"])?,
                rank_low: integer(&value["rank_low"])?,
                exact_space_bonus: integer(&value["exact_space_bonus"])?,
                maximum_age_seconds: integer(&value["maximum_age_seconds"])?,
                age_coefficient: value["age_coefficient"].as_f64()?,
                first_quality_threshold: integer(&value["first_quality_threshold"])?,
                second_quality_threshold: integer(&value["second_quality_threshold"])?,
                quality_adjustment: integer(&value["quality_adjustment"])?,
                latency_curve: words(&value["latency_curve"])?,
                language,
                partial_qos_delay_ms: integer(&value["partial_qos_delay_ms"])?,
                threshold_initial: integer(&value["threshold_initial"])?,
                threshold_final: integer(&value["threshold_final"])?,
            })
        };
        let config = parse().ok_or(Error::Configuration)?;
        config.validate()?;
        Ok(config)
    }

    pub fn as_json(&self) -> Value {
        json!({
            "rank_radius": self.rank_radius, "rank_high": self.rank_high,
            "rank_low": self.rank_low, "exact_space_bonus": self.exact_space_bonus,
            "maximum_age_seconds": self.maximum_age_seconds, "age_coefficient": self.age_coefficient,
            "first_quality_threshold": self.first_quality_threshold,
            "second_quality_threshold": self.second_quality_threshold,
            "quality_adjustment": self.quality_adjustment, "latency_curve": self.latency_curve,
            "language": self.language, "partial_qos_delay_ms": self.partial_qos_delay_ms,
            "threshold_initial": self.threshold_initial, "threshold_final": self.threshold_final,
        })
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Search {
    pub player_count: i32,
    pub average_rank: i32,
    pub language: i32,
    pub quality_first: i32,
    pub quality_second: i32,
    pub minimum_rank: Option<i32>,
    pub maximum_rank: Option<i32>,
    pub own_address: Option<[u8; 36]>,
    pub clan: Option<[u8; 12]>,
}

impl Search {
    pub fn validate(&self) -> Result<(), Error> {
        if !(1..=16).contains(&self.player_count)
            || !(0..=127).contains(&self.average_rank)
            || !(0..=8).contains(&self.language)
            || [self.minimum_rank, self.maximum_rank]
                .into_iter()
                .flatten()
                .any(|n| !(0..=127).contains(&n))
        {
            return Err(Error::Search);
        }
        Ok(())
    }

    pub fn from_json(value: &Value) -> Result<Self, Error> {
        let parse = || -> Option<Self> {
            exact_fields(
                value,
                &[
                    "player_count",
                    "average_rank",
                    "language",
                    "quality_first",
                    "quality_second",
                    "minimum_rank",
                    "maximum_rank",
                    "own_address",
                    "clan",
                ],
            )?;
            Some(Self {
                player_count: integer(&value["player_count"])?,
                average_rank: integer(&value["average_rank"])?,
                language: integer(&value["language"])?,
                quality_first: integer(&value["quality_first"])?,
                quality_second: integer(&value["quality_second"])?,
                minimum_rank: optional_integer(&value["minimum_rank"])?,
                maximum_rank: optional_integer(&value["maximum_rank"])?,
                own_address: optional_hex(&value["own_address"])?,
                clan: optional_hex(&value["clan"])?,
            })
        };
        let search = parse().ok_or(Error::Search)?;
        search.validate()?;
        Ok(search)
    }
}

fn exact_fields(value: &Value, fields: &[&str]) -> Option<()> {
    let object = value.as_object()?;
    (object.len() == fields.len() && fields.iter().all(|f| object.contains_key(*f))).then_some(())
}

fn integer(value: &Value) -> Option<i32> {
    i32::try_from(value.as_i64()?).ok()
}

fn optional_integer(value: &Value) -> Option<Option<i32>> {
    if value.is_null() {
        Some(None)
    } else {
        Some(Some(integer(value)?))
    }
}

fn words<const N: usize>(value: &Value) -> Option<[i32; N]> {
    let values = value.as_array()?;
    if values.len() != N {
        return None;
    }
    let mut result = [0; N];
    for (output, input) in result.iter_mut().zip(values) {
        *output = integer(input)?;
    }
    Some(result)
}

fn optional_hex<const N: usize>(value: &Value) -> Option<Option<[u8; N]>> {
    if value.is_null() {
        return Some(None);
    }
    let source = value.as_str()?.as_bytes();
    if source.len() != N * 2 {
        return None;
    }
    let nibble = |c: u8| match c {
        b'0'..=b'9' => Some(c - b'0'),
        b'a'..=b'f' => Some(c - b'a' + 10),
        b'A'..=b'F' => Some(c - b'A' + 10),
        _ => None,
    };
    let mut output = [0; N];
    for (byte, pair) in output.iter_mut().zip(source.chunks_exact(2)) {
        *byte = nibble(pair[0])? * 16 + nibble(pair[1])?;
    }
    Some(Some(output))
}

fn json_carrier(data: &[u8]) -> Result<Value, Error> {
    if data.len() > REQUEST_LIMIT {
        return Err(Error::Search);
    }
    serde_json::from_slice(data).map_err(|_| Error::Search)
}

pub fn decode_configuration(data: &[u8]) -> Result<Configuration, Error> {
    Configuration::from_json(&json_carrier(data)?)
}

pub fn decode_search(data: &[u8]) -> Result<Search, Error> {
    Search::from_json(&json_carrier(data)?)
}

pub fn request(data: &[u8]) -> Result<(Search, Configuration), Error> {
    let value = json_carrier(data)?;
    exact_fields(&value, &["search", "config"]).ok_or(Error::Search)?;
    Ok((
        Search::from_json(&value["search"])?,
        Configuration::from_json(&value["config"])?,
    ))
}

fn read(row: &Record, at: usize) -> i32 {
    i32::from_le_bytes(
        row[at..at + 4]
            .try_into()
            .expect("fixed original word offset"),
    )
}

fn write(row: &mut Record, at: usize, value: i32) {
    row[at..at + 4].copy_from_slice(&value.to_le_bytes());
}

fn allowed(search: &Search, row: &Record) -> Result<bool, Error> {
    let detailed = row[0xa4] != 0;
    if detailed && (read(row, 0xac) != 4 || read(row, 0xb0) < 0x2651 || read(row, 0xb4) > 0x2651) {
        return Ok(false);
    }
    let (mask, minimum, maximum) = if detailed {
        (0x110, 0x12c, 0x124)
    } else {
        (0x70, 0x64, 0x68)
    };
    if search.own_address.is_some_and(|own| row[0x24..0x48] == own) {
        return Ok(false);
    }
    if detailed && let Some(clan) = search.clan {
        let count = read(row, 0x130);
        if !(0..=16).contains(&count) {
            return Err(Error::Record);
        }
        if (0..count as usize).any(|i| row[0x134 + i * 12..0x140 + i * 12] == clan) {
            return Ok(false);
        }
    }
    Ok(read(row, mask) & (1 << (search.player_count - 1)) != 0
        && search.minimum_rank.is_none_or(|n| read(row, minimum) >= n)
        && search.maximum_rank.is_none_or(|n| read(row, maximum) <= n))
}

/// Truncating original conversion, including its integer-indefinite overflow.
fn convert(value: f64) -> i32 {
    if value.is_finite() && (-2147483648.0..2147483648.0).contains(&value) {
        value.trunc() as i32
    } else {
        i32::MIN
    }
}

fn latency_score(milliseconds: i32, config: &Configuration) -> i32 {
    // Each operation is a separate original SSE single-precision operation.
    let t = (milliseconds as f32 * 0.01_f32).clamp(0.0, 9.0);
    let index = t.trunc() as usize;
    if index == 9 {
        return config.latency_curve[9];
    }
    let lower = config.latency_curve[index] as f32;
    let upper = config.latency_curve[index + 1] as f32;
    let delta = upper - lower;
    let fraction = t - index as f32;
    let adjustment = fraction * delta;
    convert(f64::from(lower + adjustment))
}

fn score(search: &Search, row: &mut Record, config: &Configuration) -> Result<(), Error> {
    write(row, 0x1f8, 0);
    if row[0] != 0 && row[1] != 0 && !allowed(search, row)? {
        row[1] = 0;
    }
    if row[0] == 0 || row[1] == 0 {
        return Ok(());
    }
    let detailed = row[0xa4] != 0;
    let (space, rank, age) = if detailed {
        (0x10c, 0x120, 0x11c)
    } else {
        (0x60, 0x5c, 0x6c)
    };
    let rank = read(row, rank);
    if !(0..=127).contains(&rank) {
        return Err(Error::Record);
    }
    let difference = (rank - search.average_rank).abs();
    write(row, 0x1fc, difference);
    let mut score = 0;
    if difference <= config.rank_radius {
        let step = config.rank_high.wrapping_sub(config.rank_low) / (config.rank_radius + 1);
        score = config.rank_high.wrapping_sub(step.wrapping_mul(difference));
    }
    if read(row, space) == search.player_count {
        score = score.wrapping_add(config.exact_space_bonus);
    }
    let age = read(row, age).clamp(0, config.maximum_age_seconds);
    write(row, 0x200, age);
    // Original x87 controls use precision53 and nearest rounding, then the
    // truncating 322B0C helper. Preserve conversion before later additions.
    let age_term = f64::from(age) * config.age_coefficient;
    score = convert(f64::from(score) + age_term);
    if detailed {
        if (read(row, 0xf8) < config.first_quality_threshold
            || read(row, 0xfc) < config.second_quality_threshold)
            && search.quality_first >= config.first_quality_threshold
            && search.quality_second >= config.second_quality_threshold
        {
            score = score.wrapping_add(config.quality_adjustment);
        }
        let language = read(row, 0xb8);
        if !(0..=8).contains(&language) {
            return Err(Error::Record);
        }
        score = score.wrapping_add(config.language[search.language as usize][language as usize]);
    }
    if row[0x74] != 0 {
        score = score.wrapping_add(latency_score(read(row, 0x90), config));
    }
    write(row, 0x1f8, score);
    Ok(())
}

fn precedes(first: &Record, second: &Record) -> bool {
    // Original predicate means "worse". Its asymmetric first-record ready
    // check forbids substituting a tuple key or standard total-order sort.
    let (a, b) = (read(first, 0x1f8), read(second, 0x1f8));
    if a != b {
        return a < b;
    }
    if first[0x74] != 0 {
        let (a, b) = (read(first, 0x90), read(second, 0x90));
        if a != b {
            return a > b;
        }
    }
    let (a, b) = (read(first, 0x1fc), read(second, 0x1fc));
    if a != b {
        return a > b;
    }
    read(first, 0x10c) > read(second, 0x10c)
}

fn sort(rows: &mut [Record; COUNT]) {
    let mut stack = vec![(0, COUNT - 1)];
    while let Some((mut lo, mut hi)) = stack.pop() {
        loop {
            let size = hi - lo + 1;
            if size <= 8 {
                while hi > lo {
                    let mut worst = lo;
                    for p in lo + 1..=hi {
                        if precedes(&rows[p], &rows[worst]) {
                            worst = p;
                        }
                    }
                    rows.swap(worst, hi);
                    hi -= 1;
                }
                break;
            }
            rows.swap(lo + size / 2, lo);
            let (mut low, mut high) = (lo, hi + 1);
            loop {
                low += 1;
                while low <= hi && !precedes(&rows[low], &rows[lo]) {
                    low += 1;
                }
                high -= 1;
                while high > lo && !precedes(&rows[lo], &rows[high]) {
                    high -= 1;
                }
                if high < low {
                    break;
                }
                rows.swap(low, high);
            }
            rows.swap(lo, high);
            // Original subtracts one BYTE from the left span, not one row.
            let left_bytes = (high as isize - lo as isize) * STRIDE as isize - 1;
            let right_bytes = (hi as isize - low as isize) * STRIDE as isize;
            if left_bytes >= right_bytes {
                if lo + 1 < high {
                    stack.push((lo, high - 1));
                }
                if low < hi {
                    lo = low;
                    continue;
                }
            } else {
                if low < hi {
                    stack.push((low, hi));
                }
                if lo + 1 < high {
                    hi = high - 1;
                    continue;
                }
            }
            break;
        }
    }
}

pub fn threshold(radius: i32, maximum: i32, config: &Configuration) -> Result<i32, Error> {
    config.validate()?;
    if !(-1..=127).contains(&radius) || !(0..=127).contains(&maximum) {
        return Err(Error::Search);
    }
    if radius == -1 || maximum == 0 {
        return Ok(config.threshold_final);
    }
    let ratio = radius as f32 / maximum as f32;
    let difference = config.threshold_final as f32 - config.threshold_initial as f32;
    let adjustment = ratio * difference;
    Ok(convert(f64::from(
        config.threshold_initial as f32 + adjustment,
    )))
}

/// Owns the search operands, selected configuration and exact sorted byte cache.
pub struct Candidates {
    search: Search,
    config: Configuration,
    bank: Box<[u8; BANK_SIZE]>,
}

impl Candidates {
    pub fn new(search: Search, config: Configuration) -> Result<Self, Error> {
        search.validate()?;
        config.validate()?;
        Ok(Self {
            search,
            config,
            bank: Box::new([0; BANK_SIZE]),
        })
    }

    /// Configuration alone does not invalidate the original memcmp cache.
    pub fn configure(&mut self, search: Search, config: Configuration) -> Result<(), Error> {
        search.validate()?;
        config.validate()?;
        (self.search, self.config) = (search, config);
        Ok(())
    }

    pub fn bank(&self) -> &[u8; BANK_SIZE] {
        &self.bank
    }

    /// Restores a trusted internal cache projection. This is not admission or
    /// a network-bank parser; callers must retain its original ownership.
    pub fn restore_cache(&mut self, bank: &[u8]) -> Result<(), Error> {
        if bank.len() != BANK_SIZE {
            return Err(Error::Bank);
        }
        self.bank.copy_from_slice(bank);
        Ok(())
    }

    pub fn update(&mut self, bank: &[u8]) -> Result<&[u8; BANK_SIZE], Error> {
        if bank.len() != BANK_SIZE {
            return Err(Error::Bank);
        }
        if bank == self.bank.as_slice() {
            return Ok(&self.bank);
        }
        let mut rows = [[0; STRIDE]; COUNT];
        for (row, source) in rows.iter_mut().zip(bank.chunks_exact(STRIDE)) {
            row.copy_from_slice(source);
            score(&self.search, row, &self.config)?;
        }
        sort(&mut rows);
        for (output, row) in self.bank.chunks_exact_mut(STRIDE).zip(&rows) {
            output.copy_from_slice(row);
        }
        Ok(&self.bank)
    }

    pub fn select(&self, minimum: i32, elapsed_query_ms: i32, active: bool) -> Option<Record> {
        if !active {
            return None;
        }
        let (mut eligible, mut ready, mut selected) = (0, 0, None);
        for row in self.bank.chunks_exact(STRIDE) {
            if row[0] != 0 && row[1] != 0 {
                eligible += 1;
                if row[0x74] != 0 {
                    ready += 1;
                    let row: &Record = row.try_into().expect("fixed bank stride");
                    if selected.is_none() && read(row, 0x1f8) >= minimum {
                        selected = Some(*row);
                    }
                }
            }
        }
        if ready == eligible
            || (ready >= eligible / 4 && elapsed_query_ms > self.config.partial_qos_delay_ms)
        {
            selected
        } else {
            None
        }
    }
}
