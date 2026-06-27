-- DARKNET — esquema inicial do banco (Cyber Station)
CREATE TABLE IF NOT EXISTS accounts (
  id           TEXT PRIMARY KEY,
  name         TEXT NOT NULL,
  email        TEXT UNIQUE,
  pass_hash    TEXT,
  gems         INTEGER NOT NULL DEFAULT 0,    -- moeda premium (dinheiro real)
  inv_slots    INTEGER NOT NULL DEFAULT 40,
  created_at   TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS inventory (
  id        BIGSERIAL PRIMARY KEY,
  account   TEXT REFERENCES accounts(id),
  item_id   TEXT NOT NULL,
  qty       INTEGER NOT NULL DEFAULT 1
);

-- Transações de dinheiro real — fonte de verdade são os webhooks do provedor
CREATE TABLE IF NOT EXISTS transactions (
  id            BIGSERIAL PRIMARY KEY,
  account       TEXT REFERENCES accounts(id),
  provider      TEXT NOT NULL,            -- ex.: 'stripe'
  provider_ref  TEXT NOT NULL,            -- id do PaymentIntent
  pack_id       TEXT NOT NULL,
  amount_cents  INTEGER NOT NULL,
  currency      TEXT NOT NULL DEFAULT 'BRL',
  status        TEXT NOT NULL DEFAULT 'pending',  -- pending|paid|refunded
  created_at    TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS progress (
  account     TEXT PRIMARY KEY REFERENCES accounts(id),
  level       INTEGER NOT NULL DEFAULT 1,
  credits     INTEGER NOT NULL DEFAULT 0,
  char_class  INTEGER NOT NULL DEFAULT 0,
  save_json   JSONB
);
