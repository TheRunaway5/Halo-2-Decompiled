CREATE TABLE IF NOT EXISTS match_result_consensus(
          match TEXT PRIMARY KEY REFERENCES match_results(id),
          policy TEXT NOT NULL CHECK(policy IN ('native-survivors-v1','native-unanimous-v1')),
          admitted_connections INTEGER NOT NULL, confirming_connections INTEGER NOT NULL,
          departed_slots INTEGER NOT NULL, authority_epoch INTEGER NOT NULL)
