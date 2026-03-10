# Schema-Driven Development

Notes on nekode's approach to document-driven development: a hyperlinked set of structured documents that serve as both codebase documentation and build specifications.

## Core idea

The primary development workflow is refining documents, not writing code. By the time an agent builds a feature, the documentation is detailed enough that implementation is obvious. Code is a side effect of sufficiently refined documentation.

Documents are **dual-purpose** — they describe both the current state of the codebase and planned features. Inline tags disambiguate what's real vs aspirational.

## Status tags

HTML comments for tagging. Invisible in rendered markdown, parseable by agents.

### Block-level

```markdown
<!-- status:implemented, since:v0.1 -->
### Hook system
Hooks fire at lifecycle events...
<!-- /status -->

<!-- status:planned, phase:2, depends:phase-1e -->
### Hook shimming
Each worktree gets a `.claude/settings.local.json`...
<!-- /status -->
```

### Inline

```markdown
| `led:<0-3>:<state>` | Set LED state | <!-- status:implemented -->
| `oled:<state>` | Set OLED animation | <!-- status:planned, phase:4 -->
```

### Tag dimensions

| Tag | Values | Purpose |
|-----|--------|---------|
| `status` | `implemented`, `planned`, `in-progress`, `deprecated` | What's real vs aspirational |
| `phase` | `1a`–`6` | When it gets built |
| `depends` | phase/section refs | Build ordering |
| `confidence` | `high`, `low`, `speculative` | How settled the design is |
| `validated` | date or commit hash | Last confirmed match with reality |

## Why dual-purpose documents

- One place to look, one place to update — no drift between spec and docs
- Agents see both what exists and what's coming, in context
- Implementing a feature = updating a tag in the same paragraph, not syncing two files
- Cross-references between planned and implemented sections are regular markdown links
- `confidence:low` tells agents "don't build to this interface yet"

## Prior art

The pattern is emerging under the name **Spec-Driven Development (SDD)**. Research conducted March 2026.

### GitHub Spec Kit

**Repo:** [github/spec-kit](https://github.com/github/spec-kit) | Python CLI (`pip install specify-cli`) | v0.1.4 | MIT

Five phases: Constitution → Specify → Plan → Tasks → Implement. Creates `specs/###-feature-name/` per feature with auto-numbered branches.

**Document schemas:**

- **`constitution.md`** — project-level architectural principles that gate all work. Example articles: Library-First, Test-First Imperative, Anti-Abstraction.
- **`spec.md`** — user scenarios (P1/P2/P3 priority, Given/When/Then acceptance), functional requirements (`FR-001` numbering, "System MUST..." format), success criteria (`SC-001`). Max 3 `[NEEDS CLARIFICATION]` markers.
- **`plan.md`** — technical context (language, deps, storage, testing, platform, perf goals, constraints, scale), constitution compliance check, project structure, complexity tracking table with deviation justification.
- **`tasks.md`** — `- [ ] T001 [P] [US1] Description with exact file path`. `[P]` = parallelizable. Phased: Setup → Foundational → User Stories by priority → Polish.
- **`research.md`** — findings on `[NEEDS CLARIFICATION]` items.

**Integration:** `specify init --ai claude-code` installs slash commands. Templates are elaborate system prompts that constrain LLM behavior — forced clarification markers, self-review checklists, constitutional gates.

**Criticisms:** "Reinvented waterfall." Heavy upfront documentation before any code. AI agents don't reliably follow all spec instructions. Mostly unusable for large existing codebases. Spec rot over time.

### AWS Kiro

**Site:** [kiro.dev](https://kiro.dev) | IDE (VS Code fork) | Closed source

Three phases: Requirements → Design → Tasks. Creates `.kiro/specs/{feature-name}/` per feature. Also has `.kiro/steering/` files (`product.md`, `structure.md`, `tech.md`) as persistent project context (like CLAUDE.md).

**Document schemas:**

- **`requirements.md`** — user stories ("As a [role], I want [feature], so that [benefit]") with **EARS acceptance criteria** (originally from Rolls-Royce jet engine control systems): `GIVEN [precondition] WHEN [trigger] THEN [expected behavior]`. Patterns: Event-driven, State-driven, Conditional, Optional feature.
- **`design.md`** — technical overview, component architecture, Mermaid diagrams (flowchart, sequenceDiagram, erDiagram), TypeScript interfaces for data models, API endpoint tables, error handling strategy.
- **`tasks.md`** — `- [ ] 1. Description` with indented implementation steps and `_Requirements: 1.1, 1.2_` traceability lines. Kiro's agent executes tasks one-by-one, writing code and running tests.

**Workflow variants:** Requirements-First (default), Design-First (for existing apps), Bugfix (reproduction steps, current/expected/unchanged behavior with regression tests).

**Criticisms (Martin Fowler/Birgitta Boeckeler):** "Sledgehammer to crack a nut" — a small bug generated 4 user stories with 16 acceptance criteria. Specs don't prevent AI hallucination. Design artifacts don't auto-update when implementation diverges. Throttling errors and stuck tasks in practice.

### Dialectic-Driven Development (DDD, formerly DocDD)

**Site:** [dialectician.ai](https://dialectician.ai) | **Book:** [ddd-book](https://dialecticianai.github.io/ddd-book/) | **CLI:** [hegel-cli](https://github.com/dialecticianai/hegel-cli) (Rust)

The closest philosophical match to nekode's approach. Three principles: AI as Generator / Human as Editor, Disposable Artifacts / Durable Insight, Parsimony Over Extensibility.

**Three operating modes:**

| Mode | Goal | Artifacts |
|------|------|-----------|
| **Research** | Knowledge capture | Cached docs, learning documents, open questions tracker |
| **Discovery** | Learning via toy models | SPEC.md, PLAN.md, README.md, LEARNINGS.md |
| **Execution** | Production features | KICKOFF.md, SPEC.md, PLAN.md, CODE_MAP.md |

**Document schemas:**

- **`SPEC.md`** — behavioral contract (NOT implementation). Overview, data model with JSON examples, core operations (syntax, params, examples, validation), test scenarios (simple/complex/error), falsifiable success criteria checkboxes.
- **`PLAN.md`** — strategic TDD roadmap. Each step: Goal → Step N.a Write Tests → Step N.b Implement → Success Criteria. No literal code — illustrative patterns only.
- **`README.md`** — 100-200 word context-refresh for agents. Purpose, 3-5 key API signatures, core concepts, gotchas, quick test command.
- **`LEARNINGS.md`** — dual role as roadmap AND artifact. Status markers: `Validated`, `Challenged`, `Failed`, `Uncertain`. Pivots documented. One page max.
- **`KICKOFF.md`** (Execution mode) — Napkin Physics (problem in 1 sentence, 3-5 assumptions, one invariant, ≤5 mechanism bullets) + Binary-Weave Plan (each stage introduces exactly one primitive, then integrates: A, B, A+B=C, D, C+D=E...).
- **`CODE_MAP.md`** — one per directory, non-recursive. Architecture overview, component docs, integration patterns, known issues. Updated every commit that changes structure.

**Hegel CLI:** State machine orchestrator. `hegel start discovery`, `hegel next '{"spec_complete": true}'`, `hegel status`, `hegel analyze` (per-phase metrics). Integrates with Claude Code hooks. Detects repeated commands (>6 in 180s) and injects warnings.

**Criticisms:** CLI+JSON bias — explicitly not suited for GUIs, embedded, real-time. Solo-oriented. The book is "written for AI agents, not humans." Small community (7 stars). The "human never directly edits files" principle is extreme.

### Tessl

**Site:** [tessl.io](https://tessl.io) | Closed beta (framework) + open beta (registry) | Sept 2025

Specs as "long-term memory" committed alongside code and tests. The Spec Registry (10k+ pre-built specs for npm/PyPI packages) prevents API hallucinations by providing version-specific API surfaces.

**Spec format (`.spec.md`):**

```markdown
---
name: calculator
description: Basic arithmetic operations
targets:
  - src/calculator.ts
---

## Capabilities
- It adds two numbers together [@test](../tests/add.test.ts)
- It subtracts two numbers [@test](../tests/subtract.test.ts)

## API Surface
```typescript {.api}
export function add(a: number, b: number): number
export function subtract(a: number, b: number): number
```
```

**Special annotations:** `[@test](path)` links capability to test, `[@use](spec-ref)` imports another spec, `[@generate]` directs code generation. Generated code marked `// GENERATED FROM SPEC - DO NOT EDIT`.

**Key insight:** Specs stay synchronized because they are the mechanism through which changes are applied, not a separate documentation concern.

### BMAD-METHOD

**Repo:** [bmad-code-org/BMAD-METHOD](https://github.com/bmad-code-org/BMAD-METHOD) | v6.0.4 | 39.9k stars | MIT

AI-driven agile framework with 9 named agent personas (Mary/Analyst, John/PM, Winston/Architect, Sally/UX, Bob/Scrum Master, Amelia/Developer, Quinn/QA, Barry/Quick Flow, Paige/Tech Writer). Installs via `npx bmad-method install`.

**4-phase workflow:**

1. **Analysis** (optional) — brainstorming, domain/market/technical research, product brief
2. **Planning** — PRD creation (12-step workflow + 13-step validation), UX spec
3. **Solutioning** — architecture with ADRs, epics & stories (Given/When/Then), implementation readiness gate (PASS/CONCERNS/FAIL)
4. **Implementation** — sprint planning via `sprint-status.yaml`, story prep, TDD execution, code review, retrospective

**Quick Flow** bypasses phases 1-3 for small changes: `bmad-quick-spec` → `bmad-quick-dev`.

**Agent schema (YAML):** Each agent has metadata, persona (role, identity, communication style, principles), critical actions (non-negotiable behaviors), and a menu of trigger → workflow mappings.

**Party Mode:** Multiple agent personas collaborate in a single session via team bundles.

**Key pattern:** Scale-adaptive — `domain-complexity.csv` and `project-types.csv` inform how much planning depth workflows apply. Quick Flow for trivial, full ceremony for enterprise.

### SDD-Pilot

**Repo:** [attilaszasz/sdd-pilot](https://github.com/attilaszasz/sdd-pilot) | GitHub template repo | Evolved from Spec Kit

8-phase pipeline with strict quality gates: Specify → Clarify → Plan → Checklist → Tasks → Analyze → Implement → QC. 16 specialized sub-agents in `.claude/agents/`.

**Notable schemas:**

- **`sad.md`** (System Architecture Document) — C4 diagrams in Mermaid (System Context, Container, Component views), solution strategy, runtime flows with sequence diagrams, cross-cutting concerns (security, reliability, observability), ADRs, risks/assumptions/constraints.
- **`qc-report.md`** — overall verdict (PASS/FAIL), test results, coverage, static analysis, security audit, requirements traceability table, checklist fulfillment spot-check.
- **`project-instructions.md`** — non-negotiable principles with MUST/SHOULD language, semantic versioned (MAJOR/MINOR/PATCH), checked at Plan, Analyze, and QC phases.

**Task format:** `- [ ] T### [P?] [US#?] {FR-###?} Description with file path` — adds `{FR-###}` requirement traceability on top of Spec Kit's format.

**Autopilot mode:** Runs 7 phases unattended. 6 halt conditions including CRITICAL compliance violations, implement-QC loop exhaustion (10 iterations), and document sufficiency failures.

### GraphMD

**Repo:** [graphmd-lpe/graphmd](https://github.com/graphmd-lpe/graphmd)

Literate Programming Environment where markdown is the primary executable artifact. Core abstraction: **Markdown-Based Executable Knowledge Graph** — documents are nodes, links are edges, front matter provides typing, fenced code blocks are executable.

**Six-phase workflow:** Research → Design → Roadmap → Planning → Development → Review. Planning and Development run on separate git branches. Conventional commits with phase prefixes (`research:`, `design:`, `plan:`, `dev:`).

**Three-layer context tracking:** Backlog (what to do), Changelog (what changed), Journal (detailed session notes). Append-only provenance for auditability.

**Philosophy:** Explicitly anti-AGI. AI is a "template generator" that needs human review at every step. All workflow prompts end with "Stand by for further instructions" to prevent autonomous execution.

**Status:** Very early. Minimal community (2 HN points, 1 comment).

### Supporting tools

- **Agent OS** ([buildermethods.com/agent-os](https://buildermethods.com/agent-os)) — shell-based system for discovering, documenting, and injecting coding standards. `/discover-standards` extracts patterns from existing code, `/inject-standards` feeds them to agents, `/shape-spec` enhances plan mode. Profile inheritance for different tech stacks.
- **AgDR** ([me2resh/agent-decision-record](https://github.com/me2resh/agent-decision-record)) — extends ADRs for AI agents. Y-Statement formula: "In the context of **[situation]**, facing **[concern]**, I decided **[decision]** to achieve **[goal]**, accepting **[tradeoff]**." YAML frontmatter with agent, model, trigger, status fields.
- **Archgate** ([archgate/cli](https://github.com/archgate/cli)) — turns ADRs into enforceable governance. Each ADR gets a companion `.rules.ts` with automated compliance checks. MCP server for AI agent integration. Works fully offline.

---

## Comparative analysis

### What works across all tools

- **Structured specs produce better agent output** than vague prompts — universally confirmed
- **Phase gating** prevents premature implementation — every tool has some form of "don't code until the spec is approved"
- **Requirement traceability** (`FR-###`, `SC-###`, `[@test]` links) — helps agents verify completeness
- **Given/When/Then acceptance criteria** — used by Spec Kit, Kiro, BMAD, SDD-Pilot. Agents handle this format well

### What fails or struggles

- **Specs don't prevent hallucination** — Fowler's team confirmed agents still generate unrequested features despite detailed specs
- **Overhead for small tasks** — every tool acknowledges this (Kiro's sledgehammer problem, BMAD's Quick Flow, DDD's Execution mode bypass)
- **Spec rot** — keeping specs in sync with code over time is governance burden. Only Tessl addresses this by making specs the mechanism of change
- **Brownfield weakness** — SDD works best greenfield. Existing codebases resist the upfront spec ceremony
- **Agent non-compliance** — agents don't reliably follow all spec instructions regardless of detail level

### What nekode can learn from each

| Tool | Steal this |
|------|-----------|
| **Spec Kit** | `[NEEDS CLARIFICATION]` markers, `[P]` parallelization flags on tasks, constitution concept |
| **Kiro** | EARS acceptance criteria format, steering files as persistent context, bugfix workflow variant |
| **DDD** | CODE_MAP.md per directory, Napkin Physics for pre-spec simplification, `Validated`/`Challenged`/`Failed`/`Uncertain` evidence markers, Binary-Weave integration pattern |
| **Tessl** | `[@test]` links from capabilities to tests, `{.api}` blocks for public interface, specs as mechanism of change (not separate docs) |
| **BMAD** | Scale-adaptive ceremony (quick flow vs full), implementation readiness gate, Quick Flow for small changes |
| **SDD-Pilot** | `{FR-###}` traceability tags on tasks, QC report with requirements traceability table, autopilot halt conditions |
| **GraphMD** | Phase-prefixed commits, three-layer context tracking (backlog/changelog/journal) |
| **Agent OS** | `/discover-standards` — extract patterns from existing code into documented standards |
| **AgDR** | Y-Statement formula for decision records |
| **Archgate** | Companion `.rules.ts` enforcement files for governance |

## What's distinct about nekode's approach

1. **Hyperlinked document graph** — not flat files, a navigable web of cross-references. No existing tool treats the document set as a graph.
2. **Dual-purpose docs** — same document is both current-state documentation and future-state spec. Every other tool separates these concerns into different files.
3. **Inline status tags** — `<!-- status:implemented -->` keeps implementation state in the same paragraph as the spec, not in a separate tracker.
4. **Refinement-as-development** — documents are the primary work product, code is a side effect. DDD comes closest but still has distinct implementation phases.
5. **Cross-subsystem monorepo scope** — docs span firmware and application code. No existing tool handles multi-subsystem projects.
6. **No ceremony scaling problem** — because the document IS both the spec and the docs, there's no "skip the spec for small changes" escape hatch needed. You're always editing the same document whether the change is small or large.

## Open questions

- Should tags be enforced by a lint step or hook?
- How granular should tagging be — per section, per table row, per sentence?
- How to handle sections that are partially implemented?
- Should `validated` tags be auto-updated by CI?
- Adopt `[NEEDS CLARIFICATION]` markers from Spec Kit?
- Adopt `[@test]` links from Tessl for connecting specs to test files?
- Should nekode have a CODE_MAP.md per directory (from DDD) or keep everything in the hyperlinked doc graph?
- How to handle the brownfield problem — nekode already has implemented firmware, so docs need to describe existing code accurately
- What's the right level of acceptance criteria formality — full EARS or simpler Given/When/Then?
