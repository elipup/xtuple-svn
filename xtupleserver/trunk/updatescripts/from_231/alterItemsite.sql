BEGIN;

ALTER TABLE itemsite ADD COLUMN itemsite_warrpurc BOOLEAN NOT NULL DEFAULT FALSE;
ALTER TABLE itemsite ADD COLUMN itemsite_warrsell BOOLEAN NOT NULL DEFAULT FALSE;
ALTER TABLE itemsite ADD COLUMN itemsite_warrperiod INTEGER NOT NULL DEFAULT 0;
ALTER TABLE itemsite ADD COLUMN itemsite_warrship BOOLEAN NOT NULL DEFAULT FALSE;
ALTER TABLE itemsite ADD COLUMN itemsite_warrreg BOOLEAN NOT NULL DEFAULT FALSE;
ALTER TABLE itemsite ADD CONSTRAINT itemsite_warrperiod_check CHECK ((itemsite_warrperiod >= 0));

COMMIT;