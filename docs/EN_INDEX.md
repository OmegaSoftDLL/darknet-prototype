# English Documentation Index

The main design, story, and planning documents are currently available in Portuguese. This index provides English summaries and pointers to the full documents. Translations are welcome as contributions.

## Core Documents

### [`GAME_DESIGN.md`](../GAME_DESIGN.md) — Game Design Document (Portuguese)

A comprehensive design document covering:

- Core loop: explore zones, fight enemies, collect loot, craft/upgrade, advance phases.
- Classes and character progression.
- Enemy factions, bosses, and AI behaviors.
- Crafting, building, and economy systems.
- World structure: open-world phases, biomes, safe zones, portals, and chunk streaming.
- Multiplayer and monetization architecture (backend groundwork in `server/`).

### [`DARKNET_STORY.md`](../DARKNET_STORY.md) — Story Bible (Portuguese)

The narrative bible for the Darknet universe:

- Setting: a dystopian Earth after the KRONOS AI uprising.
- Main characters: VANCE RIOS, ZARA, KANE, LUNA, DR. CHEN, STEEL, and others.
- Act structure, major plot beats, and faction lore.
- Side quests, secret endings, and New Game+ content.

### [`ROADMAP.md`](../ROADMAP.md) — Development Roadmap (Portuguese)

Milestone planning from prototype to release, including:

- Engine and rendering milestones.
- Gameplay systems (combat, AI, world generation).
- Backend, multiplayer, and store integration.
- Polish, localization, and distribution targets.

### [`DISTRIBUTION.md`](../DISTRIBUTION.md) — Distribution & Marketing Plan (Portuguese)

Strategic notes on:

- Target platforms (Steam, itch.io).
- Monetization model (premium + optional cosmetics).
- Community building and content marketing.

## Engineering & Audit

### [`auditoria/2026-09-06-auditoria-completa.md`](../auditoria/2026-09-06-auditoria-completa.md) — Full Technical Audit (Portuguese)

A multi-domain audit covering architecture, gameplay, rendering, networking, save system, and audio. It lists P0/P1/P2/P3 issues and recommended fixes. This is the best starting point for new contributors looking for high-impact work.

## Contributing Translations

If you want to translate any of these documents to English:

1. Open an issue to claim the document.
2. Create a PR with the translated file named `EN_<original>.md` in the `docs/` folder.
3. Update this index to link to the new translation.
