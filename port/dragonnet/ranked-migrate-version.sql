-- Dragon Net: additive migration for databases made by the DBT-NX Worker. Keeps profiles, ratings and past results;
-- searches made before it count as version 40001 (DBT-NX's). Run once before deploying this Worker.
ALTER TABLE ranked_matches ADD COLUMN version INTEGER NOT NULL DEFAULT 40001;
CREATE INDEX IF NOT EXISTS ranked_version_waiting ON ranked_matches(state,rule_set,version,made);
