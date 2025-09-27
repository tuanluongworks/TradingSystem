
# Implementation Plan: C++ Trading System

**Branch**: `001-c-trading-system` | **Date**: 2025-09-27 | **Spec**: spec.md
**Input**: Feature specification from `/Users/tuanluong/Documents/GithubProjects/TradingSystem/specs/001-c-trading-system/spec.md`

## Execution Flow (/plan command scope)
```
1. Load feature spec from Input path
   → If not found: ERROR "No feature spec at {path}"
2. Fill Technical Context (scan for NEEDS CLARIFICATION)
   → Detect Project Type from file system structure or context (web=frontend+backend, mobile=app+api)
   → Set Structure Decision based on project type
3. Fill the Constitution Check section based on the content of the constitution document.
4. Evaluate Constitution Check section below
   → If violations exist: Document in Complexity Tracking
   → If no justification possible: ERROR "Simplify approach first"
   → Update Progress Tracking: Initial Constitution Check
5. Execute Phase 0 → research.md
   → If NEEDS CLARIFICATION remain: ERROR "Resolve unknowns"
6. Execute Phase 1 → contracts, data-model.md, quickstart.md, agent-specific template file (e.g., `CLAUDE.md` for Claude Code, `.github/copilot-instructions.md` for GitHub Copilot, `GEMINI.md` for Gemini CLI, `QWEN.md` for Qwen Code or `AGENTS.md` for opencode).
7. Re-evaluate Constitution Check section
   → If new violations: Refactor design, return to Phase 1
   → Update Progress Tracking: Post-Design Constitution Check
8. Plan Phase 2 → Describe task generation approach (DO NOT create tasks.md)
9. STOP - Ready for /tasks command
```

**IMPORTANT**: The /plan command STOPS at step 7. Phases 2-4 are executed by other commands:
- Phase 2: /tasks command creates tasks.md
- Phase 3-4: Implementation execution (manual or via tools)

## Summary
Build a desktop application for manual trading featuring a real-time UI connected to a high-performance C++ backend that manages market data, order execution, and position tracking. The system will provide real-time market data visualization, order entry capabilities, position tracking, and trade history with comprehensive risk management and persistence.

## Technical Context
**Language/Version**: C++20 (modern features for performance and safety)
**Primary Dependencies**: Dear ImGui (UI), OpenGL (rendering), Boost.Beast (WebSocket), sqlite_orm (database), GoogleTest (testing)
**Storage**: SQLite3 with backup file logging for trade persistence and audit trail
**Testing**: GoogleTest framework with comprehensive coverage for all trading logic and risk management
**Target Platform**: Cross-platform desktop (Windows, macOS, Linux)
**Project Type**: Single desktop application with three-tier modular architecture
**Performance Goals**: Low-latency order execution (<1ms), real-time UI updates (60 fps), high-throughput market data processing
**Constraints**: Thread-safe operations, graceful error handling, position limit enforcement, data integrity requirements
**Scale/Scope**: Single-user desktop application, moderate codebase (~10k LOC), multi-threaded architecture with async message queues

## Constitution Check
*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**Performance First**: ✅ PASS
- C++20 with modern features for optimal performance
- Low-latency design (<1ms order execution target)
- Minimal memory allocations via smart pointers and efficient algorithms

**Robustness and Reliability**: ✅ PASS
- Exception handling throughout trading logic
- Data integrity via SQLite with backup logging
- Graceful handling of network failures and reconnection

**Modularity and Separation of Concerns**: ✅ PASS
- Three-tier architecture: UI Layer, Core Logic Layer, Infrastructure Layer
- Clear interfaces between components via thread-safe message queues
- Independent testing and maintenance capabilities

**Testing Excellence**: ✅ PASS
- GoogleTest framework mandated for all backend logic
- Comprehensive test coverage required for trading logic and risk management
- TDD practices to be followed throughout implementation

**Code Quality and Standards**: ✅ PASS
- Google C++ Style Guide compliance required
- C++20 standard enforced
- Smart pointers mandated for concurrent code
- CMake build system with proper dependency management

## Project Structure

### Documentation (this feature)
```
specs/[###-feature]/
├── plan.md              # This file (/plan command output)
├── research.md          # Phase 0 output (/plan command)
├── data-model.md        # Phase 1 output (/plan command)
├── quickstart.md        # Phase 1 output (/plan command)
├── contracts/           # Phase 1 output (/plan command)
└── tasks.md             # Phase 2 output (/tasks command - NOT created by /plan)
```

### Source Code (repository root)
```
src/
├── core/                    # Core trading logic and entities
│   ├── models/             # Data models (Order, Position, Trade, Instrument)
│   ├── engine/             # Trading engine and order matching
│   ├── risk/               # Risk management and validation
│   └── messaging/          # Thread-safe message queues
├── infrastructure/         # External integrations and services
│   ├── market_data/        # WebSocket market data connection
│   ├── persistence/        # Database and file logging
│   └── networking/         # Network utilities and connection management
├── ui/                     # User interface layer
│   ├── components/         # ImGui UI components
│   ├── managers/           # UI state management
│   └── rendering/          # OpenGL rendering utilities
└── utils/                  # Common utilities and helpers

tests/
├── unit/                   # Unit tests for individual components
│   ├── core/              # Core logic tests
│   ├── infrastructure/    # Infrastructure layer tests
│   └── ui/                # UI component tests
├── integration/           # Integration tests between components
└── performance/           # Performance and latency benchmarks

include/                   # Public headers
external/                  # Third-party dependencies
cmake/                     # CMake configuration files
```

**Structure Decision**: Single desktop application with modular three-tier architecture separating UI, core trading logic, and infrastructure concerns. Each layer communicates via thread-safe message queues to enable independent testing and maintenance.

## Phase 0: Outline & Research
1. **Extract unknowns from Technical Context** above:
   - For each NEEDS CLARIFICATION → research task
   - For each dependency → best practices task
   - For each integration → patterns task

2. **Generate and dispatch research agents**:
   ```
   For each unknown in Technical Context:
     Task: "Research {unknown} for {feature context}"
   For each technology choice:
     Task: "Find best practices for {tech} in {domain}"
   ```

3. **Consolidate findings** in `research.md` using format:
   - Decision: [what was chosen]
   - Rationale: [why chosen]
   - Alternatives considered: [what else evaluated]

**Output**: research.md with all NEEDS CLARIFICATION resolved

## Phase 1: Design & Contracts
*Prerequisites: research.md complete*

1. **Extract entities from feature spec** → `data-model.md`:
   - Entity name, fields, relationships
   - Validation rules from requirements
   - State transitions if applicable

2. **Generate API contracts** from functional requirements:
   - For each user action → endpoint
   - Use standard REST/GraphQL patterns
   - Output OpenAPI/GraphQL schema to `/contracts/`

3. **Generate contract tests** from contracts:
   - One test file per endpoint
   - Assert request/response schemas
   - Tests must fail (no implementation yet)

4. **Extract test scenarios** from user stories:
   - Each story → integration test scenario
   - Quickstart test = story validation steps

5. **Update agent file incrementally** (O(1) operation):
   - Run `.specify/scripts/bash/update-agent-context.sh claude`
     **IMPORTANT**: Execute it exactly as specified above. Do not add or remove any arguments.
   - If exists: Add only NEW tech from current plan
   - Preserve manual additions between markers
   - Update recent changes (keep last 3)
   - Keep under 150 lines for token efficiency
   - Output to repository root

**Output**: data-model.md, /contracts/*, failing tests, quickstart.md, agent-specific file

## Phase 2: Task Planning Approach
*This section describes what the /tasks command will do - DO NOT execute during /plan*

**Task Generation Strategy**:
- Load `.specify/templates/tasks-template.md` as base template
- Generate foundational tasks from data model entities (Instrument, Order, Position, Trade, MarketTick, RiskLimit)
- Generate contract implementation tasks from `/contracts/` API interfaces
- Generate integration tests from quickstart validation scenarios
- Generate infrastructure tasks for threading, messaging, and persistence
- Generate UI component tasks from UI interface contracts

**Specific Task Categories**:

1. **Foundation Tasks [P]** (Parallel execution):
   - Core entity model classes (6 tasks)
   - Exception handling and logging utilities
   - Thread-safe message queue implementation
   - Configuration management system

2. **Infrastructure Tasks** (Sequential dependencies):
   - Market data connector (WebSocket + JSON parsing)
   - Database persistence service with SQLite
   - Risk manager implementation
   - Trading engine simulator

3. **UI Tasks** (Dependent on core models):
   - ImGui panel implementations (4 main panels)
   - UI state management and event handling
   - OpenGL rendering context setup
   - UI-to-backend message integration

4. **Integration Tasks** (Final validation):
   - End-to-end test scenarios from quickstart.md
   - Performance benchmarking tests
   - Error condition handling tests
   - Configuration validation tests

**Ordering Strategy**:
- **Phase 1**: Foundation and core models (can run in parallel)
- **Phase 2**: Infrastructure services (sequential, depends on models)
- **Phase 3**: UI components (depends on infrastructure APIs)
- **Phase 4**: Integration and validation (depends on all components)
- Mark [P] for tasks that can execute in parallel within each phase

**Dependency Management**:
- Each task specifies prerequisites from previous phases
- Test tasks always precede implementation tasks (TDD approach)
- Interface contracts must be implemented before dependent components
- Database schema creation before persistence service
- Message queue implementation before cross-layer communication

**Estimated Output**: 35-40 numbered, dependency-ordered tasks in tasks.md

**Testing Strategy Integration**:
- Every core component gets unit test task
- Contract interface compliance tests for all implementations
- Integration tests for multi-component workflows
- Performance tests for latency-critical paths

**IMPORTANT**: This phase is executed by the /tasks command, NOT by /plan

## Phase 3+: Future Implementation
*These phases are beyond the scope of the /plan command*

**Phase 3**: Task execution (/tasks command creates tasks.md)  
**Phase 4**: Implementation (execute tasks.md following constitutional principles)  
**Phase 5**: Validation (run tests, execute quickstart.md, performance validation)

## Complexity Tracking
*Fill ONLY if Constitution Check has violations that must be justified*

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| [e.g., 4th project] | [current need] | [why 3 projects insufficient] |
| [e.g., Repository pattern] | [specific problem] | [why direct DB access insufficient] |


## Progress Tracking
*This checklist is updated during execution flow*

**Phase Status**:
- [x] Phase 0: Research complete (/plan command)
- [x] Phase 1: Design complete (/plan command)
- [x] Phase 2: Task planning complete (/plan command - describe approach only)
- [ ] Phase 3: Tasks generated (/tasks command)
- [ ] Phase 4: Implementation complete
- [ ] Phase 5: Validation passed

**Gate Status**:
- [x] Initial Constitution Check: PASS
- [x] Post-Design Constitution Check: PASS
- [x] All NEEDS CLARIFICATION resolved
- [x] Complexity deviations documented (none required)

---
*Based on Constitution v2.1.1 - See `/memory/constitution.md`*
