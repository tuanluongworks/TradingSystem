<!--
SYNC IMPACT REPORT:
Version change: N/A → 1.0.0 (Initial constitution creation)
Modified principles: N/A (new creation)
Added sections: All sections (new constitution)
Removed sections: N/A
Templates requiring updates:
  ✅ plan-template.md: Constitution Check section aligned with new principles
  ✅ spec-template.md: Compatible with requirements (no changes needed)
  ✅ tasks-template.md: Task categories align with development guidelines
Follow-up TODOs: None
-->

# C++ Trading System Core Constitution

## Core Principles

### I. Performance First
The system MUST be optimized for low latency and high throughput. All code MUST prioritize efficient algorithms, minimize memory allocations, and leverage modern C++ features for performance. Performance benchmarks MUST be established and maintained for critical paths.

### II. Robustness and Reliability
The backend MUST be stable, handle exceptions gracefully, and maintain data integrity. Code MUST be resilient to network failures and unexpected inputs. All error conditions MUST have defined recovery strategies.

### III. Modularity and Separation of Concerns
The system architecture MUST clearly separate the UI, core trading logic, and backend services (data connection, execution). Each module MUST have well-defined interfaces and minimal coupling.

## Development Standards

### Language and Build Requirements
- **Language Standard**: All new code MUST use C++20
- **Build System**: CMake MUST be used as the primary build system
- **Dependencies**: External libraries MUST be managed through vcpkg or Conan package managers
- **Concurrency**: Multi-threading MUST use C++ standard library (`std::thread`, `std::mutex`, `std::atomic`). Raw pointers MUST be avoided in concurrent code; smart pointers are required

### Quality Assurance
- **Testing**: Unit tests are MANDATORY for all backend logic using GoogleTest framework
- **Code Style**: All code MUST adhere to the Google C++ Style Guide
- **UI Technology**: UI MUST be lightweight and cross-platform, preferring Dear ImGui or minimal Qt Quick over heavy frameworks

## Governance

**Amendment Process**: Constitution changes require documentation, technical review, and migration plan. All code reviews MUST verify compliance with these principles.

**Complexity Justification**: Any deviation from simplicity principles MUST be documented with technical rationale and simpler alternatives considered.

**Compliance Review**: Regular audits MUST ensure adherence to performance, reliability, and modularity requirements.

**Version**: 1.0.0 | **Ratified**: 2025-09-27 | **Last Amended**: 2025-09-27