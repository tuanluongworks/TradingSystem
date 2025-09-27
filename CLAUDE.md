# Claude Agent Playbook — TradingSystem

Audience: Claude Code working in this repo. Source of truth is specs/001-c-trading-system. Keep responses action-oriented and under 150 lines.

## Mission
- Deliver a single-user desktop trading workstation with low-latency order handling, reliable risk controls, and durable audit trails.
- Maintain clean separation between core logic, infrastructure integrations, and Dear ImGui/OpenGL UI so each layer testable independently.
- Favour deterministic, reproducible behaviour; randomness only behind explicit simulation flags.

## Current Snapshot
- Language/tooling: C++20, CMake, vcpkg for dependencies (Boost.Beast, sqlite_orm, sqlite3, Dear ImGui, GoogleTest, spdlog, nlohmann-json).
- Threads: core runs background order processing worker + market data feed; UI thread must stay responsive.
- Persistence: SQLite primary store, CSV backup in `data/backups/` when DB unavailable.
- Config: JSON via `src/utils/config.hpp`; default config lives in `config/` (generated on first run).

## Architecture Map
- `src/core/`: order engine, risk manager, models, message queues. Zero UI includes.
- `src/infrastructure/`: market data adapters, networking utilities, persistence services.
- `src/ui/`: Dear ImGui panels, layout managers, render helpers.
- `src/utils/`: config, logging helpers, timing utilities shared across layers.
- `include/contracts/`: stable interfaces consumed by UI/tests; treat as public API.
- `tests/`: unit (per layer), integration (order lifecycle), performance (queue throughput/latency).

## Critical Systems & Priorities
- **TradingEngine**: single writer; work is queued via `MessageQueue<std::function<void()>>`. Never block callbacks while holding `engine_mutex_`.
- **RiskManager**: synchronous pre-trade checks; maintain per-symbol exposure and daily P&L. Update limits atomically.
- **Market Data Provider**: supports real feed + simulator. Losing connection must flip trading disabled flag and emit UI status.
- **Persistence**: `SQLiteService` writes orders/trades/positions transactionally; backup writer mirrors to CSV when DB down and resyncs later.
- **UI Panels**: consume events only; all mutations go through engine/risk APIs.

## Quality Bars (non-negotiable)
- Performance: avoid heap churn on hot paths; reuse buffers; prefer move semantics.
- Concurrency: respect mutex ownership, avoid nested locks, use atomics for counters, respect stop flags before joining threads.
- Reliability: every rejection must carry reason strings; reconnect loops exponential backoff with max cap; no silent failures.
- Separation: UI must never touch core containers directly; use message/event adapters.
- Observability: log via spdlog (no `std::cout` in production paths); ensure log level configurable.

## Build & Run
- Configure: `cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake`.
- Build: `cmake --build build --config Debug` (use Release for latency measurements).
- Test: `ctest --test-dir build --output-on-failure` or target specific suites (`ctest -R core` etc.).
- Binary outputs land in `build/bin/`; runtime assets (DB, csv) write under `data/`.

## Testing Expectations
- Update/extend unit tests alongside code; mirroring folder structure under `tests/unit/...`.
- Integration tests must cover end-to-end order flow (submission → fills → persistence). Keep deterministic by stubbing market data when needed.
- When touching concurrency or performance code, add/adjust benchmarks under `tests/performance/` and document assumptions.
- Never downgrade coverage: if existing test becomes obsolete, replace it with one that proves the new behaviour.

## Workflow & Collaboration
- Start from spec + plan under `specs/001-c-trading-system/`; do not contradict requirements without tech lead approval.
- Keep `include/contracts/` signatures stable; if change unavoidable, note breaking impact and update all dependents (UI/tests/docs).
- Prefer incremental PRs: isolate risk/engine/UI changes; keep commits buildable.
- Run full test matrix before handing work back, especially after touching threading/persistence.
- Document TODOs inline only when paired with owner + follow-up task reference.

## Local Quirks & Gotchas
- `MessageQueue` supports shutdown semantics; always call `shutdown()` before destroying producer threads.
- Simulator randomness must use seeded RNG from config for reproducible tests.
- Database paths are relative; ensure directories exist before writing (utilities in `src/utils/filesystem.hpp`).
- ImGui state is reset on hot reload; keep persistent UI prefs in config manager rather than static globals.
- Windows builds need `/permissive-` and `/Zc:preprocessor`; CMake toolchain already sets but avoid compiler-specific hacks in code.

## When Uncertain, Ask
- Position/risk limit semantics or pricing rules unclear? Confirm with risk manager doc in `specs/.../data-model.md` before coding.
- Market data/engine integration questions? Check `src/infrastructure/market_data` strategy and raise clarifications before modifying core.
- Any change impacting storage schema requires migration plan + backup compatibility notes.
- If requirements conflict with performance or safety bars, escalate instead of compromising silently.

Stay disciplined: keep latency low, state consistent, and tests green.
