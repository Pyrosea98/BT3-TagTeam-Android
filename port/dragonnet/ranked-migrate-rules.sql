-- Additive migration for databases created before net25rules. Keep profiles,
-- ratings and existing matches. Run once before deploying the updated Worker.
ALTER TABLE ranked_matches ADD COLUMN rule_set TEXT NOT NULL DEFAULT 'legacy';
CREATE INDEX IF NOT EXISTS ranked_rules_waiting ON ranked_matches(state,rule_set,made);
