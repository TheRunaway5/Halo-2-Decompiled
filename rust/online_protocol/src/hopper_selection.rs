//! TU1.5 ordered map/variant draw (198EDB after successful block admission).
//!
//! Rows are not deduplicated. Availability is independently admitted map
//! ownership, not a packet field. The service retains the entire selected
//! row, including its option buffer. Filesystem/catalog admission is separate.

pub const MAX_ROWS: usize = 100;
// The original narrows total+1 to a signed short. Reject that overflow domain
// at authored catalog admission rather than selecting an unavailable row.
pub const MAX_TOTAL: u32 = 32_766;

#[derive(Clone, Copy, Debug)]
pub struct Row {
    pub weight: i32,
    pub available: bool,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Draw {
    pub index: usize,
    pub seed: u32,
    pub total: u32,
    pub choice: u32,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct InvalidPool;

pub fn effective_weight(row: Row) -> u32 {
    if row.available {
        row.weight.clamp(1, 1000) as u32
    } else {
        0
    }
}

/// A failed/empty draw spends no RNG state. One successful draw advances the
/// original 32-bit LCG exactly once, even for a single available row.
pub fn choose(seed: u32, rows: &[Row]) -> Result<Option<Draw>, InvalidPool> {
    if rows.len() > MAX_ROWS {
        return Err(InvalidPool);
    }
    let total: u32 = rows.iter().copied().map(effective_weight).sum();
    if total > MAX_TOTAL {
        return Err(InvalidPool);
    }
    if total == 0 {
        return Ok(None);
    }
    let seed = seed.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
    let choice = 1 + (((seed >> 16) * total) >> 16);
    let mut cumulative = 0;
    for (index, row) in rows.iter().copied().enumerate() {
        cumulative += effective_weight(row);
        if cumulative >= choice {
            return Ok(Some(Draw {
                index,
                seed,
                total,
                choice,
            }));
        }
    }
    Err(InvalidPool)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn unchanged_original_selector_corpus() {
        let fixture: serde_json::Value = serde_json::from_str(include_str!(
            "../../../tests/fixtures/hopper_selection_tu15_original.json"
        ))
        .unwrap();
        let mut accepted = 0;
        let mut overflow = 0;
        for case in fixture["cases"].as_array().unwrap() {
            let installed = case["installed"].as_array().unwrap();
            let rows: Vec<Row> = case["rows"]
                .as_array()
                .unwrap()
                .iter()
                .map(|row| Row {
                    weight: row["weight"].as_i64().unwrap() as i32,
                    available: installed.contains(&row["map_id"]),
                })
                .collect();
            let seed = case["seed"].as_u64().unwrap() as u32;
            if !case["safe_total"].as_bool().unwrap() {
                assert_eq!(choose(seed, &rows), Err(InvalidPool));
                overflow += 1;
                continue;
            }
            let result = if case["block_read"].as_bool().unwrap() {
                choose(seed, &rows).unwrap()
            } else {
                None
            };
            assert_eq!(
                result.map(|draw| draw.index as u64),
                case["selected"].as_u64()
            );
            assert_eq!(
                result.map_or(seed, |draw| draw.seed) as u64,
                case["updated_seed"].as_u64().unwrap()
            );
            if let Some(draw) = result {
                assert_eq!(draw.total as u64, case["total"].as_u64().unwrap());
                assert_eq!(case["rows"][draw.index]["map_id"], case["selected_map"]);
            }
            accepted += 1;
        }
        assert_eq!((accepted, overflow), (420, 36));
    }

    #[test]
    fn exact_ticket_frequency_preserves_order_and_duplicates() {
        let rows = [
            Row {
                weight: 3,
                available: true,
            },
            Row {
                weight: 1,
                available: false,
            },
            Row {
                weight: 1,
                available: true,
            },
            Row {
                weight: 4,
                available: true,
            },
        ];
        // The LCG multiplier is odd, hence a bijection of u32. Invert it to
        // exercise each high-word value once, rather than a flaky Monte Carlo.
        let inverse = 4_276_115_653_u32;
        let mut counts = [0; 4];
        for high in 0..65_536_u32 {
            let seed = (high << 16)
                .wrapping_sub(1_013_904_223)
                .wrapping_mul(inverse);
            let draw = choose(seed, &rows).unwrap().unwrap();
            assert_eq!(draw.seed >> 16, high);
            counts[draw.index] += 1;
        }
        assert_eq!(counts, [24_576, 0, 8192, 32_768]);
        assert_eq!(
            choose(
                0,
                &[Row {
                    weight: 1,
                    available: true
                }; 101]
            ),
            Err(InvalidPool)
        );
        assert_eq!(
            choose(
                9,
                &[Row {
                    weight: 1,
                    available: false
                }]
            ),
            Ok(None)
        );
    }

    #[test]
    fn every_public_hopper_matches_its_complete_integer_frequency_manifest() {
        let catalog: serde_json::Value = serde_json::from_str(include_str!(
            "../../../multiplayer/master_server/examples/matchmaking-online.json"
        ))
        .unwrap();
        let manifest: serde_json::Value = serde_json::from_str(include_str!(
            "../../../multiplayer/master_server/data/online-hopper-frequencies-v1.json"
        ))
        .unwrap();
        let playlists = catalog["playlists"].as_array().unwrap();
        assert_eq!(playlists.len(), 14);
        for (playlist, expected) in playlists
            .iter()
            .zip(manifest["playlists"].as_array().unwrap())
        {
            assert_eq!(playlist["id"], expected["id"]);
            let rows: Vec<_> = playlist["games"]
                .as_array()
                .unwrap()
                .iter()
                .map(|g| Row {
                    weight: g["weight"].as_i64().unwrap() as i32,
                    available: true,
                })
                .collect();
            let mut counts = vec![0_u32; rows.len()];
            for high in 0..65_536_u32 {
                let seed = (high << 16)
                    .wrapping_sub(1_013_904_223)
                    .wrapping_mul(4_276_115_653);
                let result = choose(seed, &rows).unwrap().unwrap();
                counts[result.index] += 1;
            }
            for (count, row) in counts.iter().zip(expected["rows"].as_array().unwrap()) {
                assert_eq!(
                    *count as u64,
                    row["draws_of_65536"].as_u64().unwrap(),
                    "{} {}",
                    playlist["id"],
                    row["id"]
                );
            }
        }
    }
}
