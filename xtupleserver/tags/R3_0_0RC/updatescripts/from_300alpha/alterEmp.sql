BEGIN;

ALTER TABLE emp ADD COLUMN emp_image_id INTEGER REFERENCES image(image_id);

COMMIT;
