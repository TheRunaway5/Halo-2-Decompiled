//! Public PCR history read policy. Modern website API, not Xbox Live recovery.
//! Persisted source indices/coverage remain explicit; guest/private identities
//! never become public player/opponent IDs. The shared current-profile totals
//! cache remains a typed provider boundary, separate from result authority.
use super::*;

fn columns(row: &Value, count: usize) -> Flow<&[Value]> {
    let cells = row
        .as_array()
        .ok_or(Error(503, "History database row required"))?;
    need(cells.len() == count, 503, "History database columns differ")?;
    Ok(cells)
}
fn object(row: &[Value], names: &[&str]) -> Value {
    Value::Object(
        names
            .iter()
            .zip(row)
            .map(|(k, v)| ((*k).to_owned(), v.clone()))
            .collect(),
    )
}
fn queries(value: &Value, allowed: &[&str]) -> Flow<BTreeMap<String, String>> {
    let pairs = value
        .as_array()
        .ok_or(Error(400, "History query pairs required"))?;
    need(pairs.len() <= allowed.len(), 400, "Unexpected query fields")?;
    let mut result = BTreeMap::new();
    for pair in pairs {
        let pair = columns(pair, 2)?;
        let key = m::text(&pair[0], 32)?;
        let value = pair[1]
            .as_str()
            .ok_or(Error(400, "History query text required"))?;
        need(
            value.len() <= 128
                && allowed.contains(&key)
                && result.insert(key.to_owned(), value.to_owned()).is_none(),
            400,
            "Unexpected query fields",
        )?;
    }
    Ok(result)
}
const BASE64: &[u8] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
fn encode(bytes: &[u8]) -> String {
    let mut output = String::new();
    let mut bits = 0u32;
    let mut count = 0;
    for byte in bytes {
        bits = (bits << 8) | u32::from(*byte);
        count += 8;
        while count >= 6 {
            count -= 6;
            output.push(BASE64[((bits >> count) & 63) as usize] as char);
        }
    }
    if count > 0 {
        output.push(BASE64[((bits << (6 - count)) & 63) as usize] as char);
    }
    output
}
fn cutoff(value: &str) -> Flow<(Value, String)> {
    need(
        !value.is_empty() && value.len() <= 128 && value.len() % 4 != 1,
        400,
        "Invalid history cursor",
    )?;
    let mut output = Vec::new();
    let mut bits = 0u32;
    let mut count = 0;
    for byte in value.bytes() {
        let index = BASE64
            .iter()
            .position(|b| *b == byte)
            .ok_or(Error(400, "Invalid history cursor"))?;
        bits = (bits << 6) | index as u32;
        count += 6;
        if count >= 8 {
            count -= 8;
            output.push((bits >> count) as u8);
        }
    }
    let parsed = crate::matchmaking_coordinator::decode(&output)
        .map_err(|_| Error(400, "Invalid history cursor"))?;
    let pair = columns(&parsed, 2).map_err(|_| Error(400, "Invalid history cursor"))?;
    m::integer(&pair[0], 0, 9007199254740991)?;
    let id = uuid(&pair[1])?.to_owned();
    Ok((pair[0].clone(), id))
}
fn blob(value: &Value, length: usize) -> Flow<Option<Vec<u8>>> {
    if value.is_null() {
        return Ok(None);
    }
    m::fields(value, &["blob"])?;
    Ok(Some(m::hex(&value["blob"], length)?))
}
fn blob_text(value: &Value, length: usize) -> Flow<Value> {
    Ok(match blob(value, length)? {
        Some(bytes) => json!(bytes.iter().map(|b| format!("{b:02x}")).collect::<String>()),
        None => Value::Null,
    })
}
fn current_profile(c: &mut Cursor<'_>, who: &str) -> Flow<Value> {
    profiles::view(c, who)
}
fn player(c: &mut Cursor<'_>, public: &str) -> Flow<Value> {
    c.one("SELECT h.player,p.discord_username,p.screen_name FROM history_players h JOIN player_profiles p ON p.player=h.player WHERE h.public_id=?",json!([public]))?
        .ok_or(Error(404,"Player history not found").into())
}
fn search(c: &mut Cursor<'_>, query: &Value) -> Flow<Value> {
    let query = queries(query, &["discord_username"])?;
    let username = query
        .get("discord_username")
        .map(String::as_str)
        .unwrap_or("")
        .to_ascii_lowercase();
    need(
        (2..=32).contains(&username.len())
            && username
                .bytes()
                .all(|b| b.is_ascii_lowercase() || b.is_ascii_digit() || b == b'_' || b == b'.'),
        400,
        "Use an exact Discord username",
    )?;
    //128 rows is the native continuation bound. A larger ambiguous stale-name
    //bank rejects rather than silently omitting matches or expanding memory.
    let rows=c.query("SELECT h.public_id,p.discord_username,p.screen_name FROM history_players h JOIN player_profiles p ON p.player=h.player WHERE p.discord_username!='' AND p.discord_username=? LIMIT 129",json!([username]))?;
    let mut output = Vec::new();
    for row in rows {
        output.push(object(
            columns(&row, 3)?,
            &["id", "discord_username", "screen_name"],
        ));
    }
    Ok(json!({"players":output}))
}
fn matches(c: &mut Cursor<'_>, public: &str, query: &Value) -> Flow<Value> {
    let query = queries(query, &["limit", "before"])?;
    let identity = player(c, public)?;
    let identity = columns(&identity, 3)?;
    let who = txt(&identity[0])?;
    let limit = query.get("limit").map(String::as_str).unwrap_or("20");
    need(
        (1..=2).contains(&limit.len()) && limit.bytes().all(|b| b.is_ascii_digit()),
        400,
        "Invalid page size",
    )?;
    let limit = limit
        .parse::<usize>()
        .map_err(|_| Error(400, "Invalid page size"))?;
    need((1..=50).contains(&limit), 400, "Invalid page size")?;
    let mut params = vec![json!(who)];
    let mut predicate = String::new();
    if let Some(value) = query.get("before") {
        let (when, id) = cutoff(value)?;
        params.extend([when.clone(), when, json!(id)]);
        predicate.push_str(" AND (m.completed<? OR (m.completed=? AND h.public_id<?))");
    }
    params.push(json!(limit + 1));
    let rows=c.query(&format!("SELECT h.public_id,m.completed,m.playlist,m.revision,m.map,m.variant,m.ranked,r.placement,r.score,r.kills,r.deaths,r.assists,r.suicides,h.format FROM match_result_players r JOIN match_results m ON m.id=r.match JOIN carnage_reports h ON h.match=m.id WHERE r.player=?{predicate} ORDER BY m.completed DESC,h.public_id DESC LIMIT ?"),json!(params))?;
    let names = [
        "id",
        "completed",
        "playlist",
        "revision",
        "map",
        "variant",
        "ranked",
        "placement",
        "score",
        "kills",
        "deaths",
        "assists",
        "suicides",
        "format",
    ];
    let mut output = Vec::new();
    for row in rows.iter().take(limit) {
        output.push(object(columns(row, 14)?, &names));
    }
    let next = if rows.len() > limit {
        let row = columns(&rows[limit - 1], 14)?;
        json!(encode(json!([row[1], row[0]]).to_string().as_bytes()))
    } else {
        Value::Null
    };
    let mut profile = current_profile(c, who)?;
    profile["id"] = json!(public);
    profile["discord_username"] = identity[1].clone();
    Ok(json!({"player":profile,"matches":output,"next":next}))
}
fn report(c: &mut Cursor<'_>, public: &str, query: &Value) -> Flow<Value> {
    queries(query, &[])?;
    let row=c.one("SELECT h.match,m.completed,m.playlist,m.revision,m.map,m.variant,m.ranked,m.validation,h.format,h.admitted_at,h.admitted_players,h.published_players,h.variant_bytes FROM carnage_reports h JOIN match_results m ON m.id=h.match WHERE h.public_id=?",json!([public]))?
        .ok_or(Error(404,"Carnage report not found"))?;
    let row = columns(&row, 13)?;
    let key = txt(&row[0])?;
    let mut output = object(
        &row[1..12],
        &[
            "completed",
            "playlist",
            "revision",
            "map",
            "variant",
            "ranked",
            "validation",
            "format",
            "admitted_at",
            "admitted_players",
            "published_players",
        ],
    );
    output["id"] = json!(public);
    output["variant_bytes"] = blob_text(&row[12], 304)?;
    output["guest_details"] = json!("excluded");
    let rows=c.query("SELECT a.player,h.public_id,p.discord_username,p.screen_name,coalesce(c.screen_name,s.screen_name),coalesce(c.discord_username_at_match,s.discord_username_at_match),coalesce(c.slot,s.slot),c.appearance,a.placement,a.score,a.kills,a.deaths,a.assists,a.suicides,t.team,t.individual_placement FROM match_result_players a JOIN history_players h ON h.player=a.player JOIN player_profiles p ON p.player=a.player LEFT JOIN carnage_players c ON c.match=a.match AND c.player=a.player LEFT JOIN carnage_player_summaries s ON s.match=a.match AND s.player=a.player LEFT JOIN match_result_memberships t ON t.match=a.match AND t.player=a.player WHERE a.match=? ORDER BY coalesce(c.slot,s.slot),h.public_id",json!([key]))?;
    need(
        rows.len() <= 16,
        503,
        "History roster exceeds original slots",
    )?;
    let mut players = Vec::new();
    for row in rows {
        let row = columns(&row, 16)?;
        let who = txt(&row[0])?;
        let mut player = object(
            &row[1..7],
            &[
                "id",
                "discord_username",
                "screen_name",
                "screen_name_at_match",
                "discord_username_at_match",
                "slot",
            ],
        );
        player["appearance"] = blob_text(&row[7], 16)?;
        player["details_available"] = json!(!row[7].is_null());
        player.as_object_mut().unwrap().extend(
            object(
                &row[8..],
                &[
                    "placement",
                    "score",
                    "kills",
                    "deaths",
                    "assists",
                    "suicides",
                    "team",
                    "individual_placement",
                ],
            )
            .as_object()
            .unwrap()
            .clone(),
        );
        for (name, table, columns, max) in [
            ("general", "carnage_general_stats", "stat,value", 45),
            ("medals", "carnage_medals", "medal,count", 32),
            ("weapons", "carnage_weapon_stats", "weapon,field,value", 360),
        ] {
            let mut details = Vec::new();
            loop {
                let page=c.query(&format!("SELECT {columns} FROM {table} WHERE match=? AND player=? ORDER BY {columns} LIMIT 128 OFFSET ?"),json!([key,who,details.len()]))?;
                need(
                    details.len() + page.len() <= max,
                    503,
                    "History detail bank exceeds source indices",
                )?;
                let count = page.len();
                details.extend(page);
                if count < 128 {
                    break;
                }
            }
            player[name] = json!(details);
        }
        let opponents=c.query("SELECT h.public_id,o.field0,o.field1 FROM carnage_opponents o JOIN history_players h ON h.player=o.opponent WHERE o.match=? AND o.player=? ORDER BY h.public_id",json!([key,who]))?;
        need(
            opponents.len() <= 16,
            503,
            "History opponent bank exceeds original slots",
        )?;
        let mut public_opponents = Vec::new();
        for row in opponents {
            let row = columns(&row, 3)?;
            public_opponents.push(json!({"player":row[0],"values":[row[1],row[2]]}));
        }
        player["opponents"] = json!(public_opponents);
        players.push(player);
    }
    output["players"] = json!(players);
    let rows = c.query(
        "SELECT team,placement,score FROM match_result_teams WHERE match=? ORDER BY team",
        json!([key]),
    )?;
    need(rows.len() <= 16, 503, "History team bank exceeds bounds")?;
    let mut teams = Vec::new();
    for row in rows {
        let row = columns(&row, 3)?;
        let mut team = object(row, &["team", "placement", "score"]);
        let general = c.query(
            "SELECT stat,value FROM carnage_team_stats WHERE match=? AND team=? ORDER BY stat",
            json!([key, row[0]]),
        )?;
        need(
            general.len() <= 45,
            503,
            "History team bank exceeds source indices",
        )?;
        team["general"] = json!(general);
        teams.push(team);
    }
    output["teams"] = json!(teams);
    Ok(output)
}
pub(super) fn operation(r: &Value, c: &mut Cursor<'_>) -> Flow<Value> {
    match txt(&r["mode"])? {
        "search" => search(c, &r["features"]),
        "matches" => matches(c, uuid(&r["target"])?, &r["features"]),
        "report" => report(c, uuid(&r["target"])?, &r["features"]),
        "profile" => {
            queries(&r["features"], &[])?;
            let public = uuid(&r["target"])?;
            let identity = player(c, public)?;
            let identity = columns(&identity, 3)?;
            let mut result = current_profile(c, txt(&identity[0])?)?;
            result["id"] = json!(public);
            Ok(result)
        }
        _ => Err(Error(400, "Unknown history read").into()),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn cursor_and_query_contracts_are_bounded_and_preserve_public_ordering() {
        let id = "00000000-0000-0000-0000-000000000001";
        for when in [0, 100, 9007199254740991i64] {
            let input = json!([when, id]);
            let text = encode(input.to_string().as_bytes());
            let (value, public) = cutoff(&text).ok().expect("Valid public cursor");
            assert_eq!(value, input[0]);
            assert_eq!(public, id);
        }
        for bad in ["a", "abcde", "____", "e30", "bad!", ""] {
            assert!(cutoff(bad).is_err(), "{bad}");
        }
        for value in [
            json!([["limit", "1"], ["limit", "2"]]),
            json!([["extra", "1"]]),
            json!({"limit":"1"}),
        ] {
            assert!(queries(&value, &["limit"]).is_err());
        }
        assert_eq!(
            queries(&json!([["limit", "50"]]), &["limit"])
                .ok()
                .expect("Valid query")["limit"],
            "50"
        );
    }
}
