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

## Knowledge graph panel

The nekode IDE is the 4-panel Claude Code multiplexer (see [nekode-tui-plan.md](nekode-tui-plan.md)). The knowledge graph is an additional panel on top of that — a reader, not a writer.

**Core principle:** Claude Code is the writing tool. The knowledge graph panel is a navigation and status tool. You refine documents by talking to Claude Code in the terminal panes. The graph panel shows you where you are in the knowledge base and what state everything is in.

### Evolution path

Aligns with the three major implementation phases in [nekode-tui-plan.md](nekode-tui-plan.md):

- **Phase 1** (nekode TUI): The IDE itself — 4 Claude Code instances, status tracking, git status panel. No knowledge base UI.
- **Phase 2** (Knowledge base): Plain markdown in `docs/`. Claude Code reads/writes them. Status tag tooling, cross-referencing conventions, completion flow. No special panel — the docs are just files, agents consume them naturally.
- **Phase 3** (Knowledge graph): Visual graph panel — renders the doc graph as a navigable view. Nodes = documents/sections, edges = markdown links, colour = status tags. Read-only. Filtered views (show me everything at `status:planned` for `phase:2`), dependency visualization, completeness dashboards, per-instance awareness.

### How it differs from Obsidian

Obsidian is a writing tool — you edit in it. nekode's graph panel is a **navigation and status** tool. The distinction matters because:
- The writing interface is Claude Code (natural language → structured document edits)
- The graph panel shows the shape of the knowledge base, not the content
- Filtered views are the primary interaction: "what's left in phase 2?", "what depends on this?", "what's at confidence:low?"
- With 4 instances potentially touching different parts of the graph, the panel gives a bird's-eye view of what each instance is working on

### What the panel shows

- Document/section nodes with status tag colouring (green = implemented, blue = in-progress, grey = planned, red = deprecated)
- Edges from markdown links between documents
- Per-instance indicators: which parts of the graph each Claude Code instance is currently touching
- Confidence overlay: dim nodes at `confidence:low`, bright nodes at `confidence:high`
- Phase filtering: collapse/expand by phase to focus on current work

## nekode SDD — how it actually works

### Philosophy

The knowledge graph replaces Jira, Linear, Confluence, GitHub Issues — all of it. Everything lives in `docs/` as markdown. The documents are a living organism that evolves with the codebase, covering:

- Technical architecture (what exists, how it works)
- Feature proposals (what's planned, why)
- Refinement from proposal → technical spec (getting precise enough for an agent to build)
- Research notes (best practices, prior art, technology decisions)
- Build plans with phases, steps, and success criteria

### v1 principles

1. **No bespoke schema.** Documents are CLAUDE.md-like — natural prose with whatever structure makes sense for the content. Agents generate and consume them the same way they would any markdown doc. No special parser, no required frontmatter, no enforced format. Structure emerges organically.

2. **Agents write, humans steer.** Agents generate and refine documents. Humans read, redirect, and approve. The existing docs in this repo ([nekode-tui-plan.md](nekode-tui-plan.md), [schema-driven-development.md](schema-driven-development.md)) are examples of the format — written collaboratively with Claude Code, not hand-authored to a template.

3. **Two-step completion.** Agents can mark a feature/phase/document as **tentatively complete** when all planned success criteria are met. The user then manually marks it as **done**. This mirrors real dev teams — the developer says "I think this is ready" and the reviewer signs off.

4. **Status tags are lightweight.** HTML comments (`<!-- status:implemented -->`) are the only structural convention. Everything else — section headings, table formats, diagram styles — is freeform. Tags can be added gradually as documents mature, not upfront.

5. **User chooses what to build.** The knowledge graph presents the full picture — what's implemented, what's proposed, what's in research. The user decides what to work on next. The IDE doesn't impose a workflow order.

### What the docs cover (by example)

The existing nekode docs already demonstrate the pattern:

| Document | Role | Equivalent in traditional tooling |
|----------|------|----------------------------------|
| [CLAUDE.md](../CLAUDE.md) | Project constitution, conventions, quick reference | README + contributing guide + architectural constraints |
| [nekode-tui-plan.md](nekode-tui-plan.md) | Technical spec + implementation plan + build phases with success criteria | Jira epic + tech design doc + sprint plan |
| [schema-driven-development.md](schema-driven-development.md) | Research notes + methodology + product vision + competitive analysis | Confluence pages + market research deck + ADRs |

Future docs might include:
- `hardware-architecture.md` — firmware modules, pin assignments, serial protocol (currently in CLAUDE.md, could be extracted)
- `product-vision.md` — user personas, use cases, positioning
- `customer-research.md` — user interviews, feedback, feature requests
- Per-feature docs as features get complex enough to warrant their own document

### How agents interact with the docs

Agents consume the docs as context, the same way they read CLAUDE.md today. No special integration needed for v1:

- **Researching**: "Read schema-driven-development.md and summarize what we can learn from Tessl's approach"
- **Refining**: "Update the serial protocol section of nekode-tui-plan.md to add the new `volume:` command"
- **Building**: "Implement Phase 1b from nekode-tui-plan.md. The success criteria are at the end of that section."
- **Reviewing**: "Check if the current implementation matches the spec in the PTY rendering section"

The agent reads the doc, understands the intent, and acts. If the doc is unclear, the agent asks or marks it `[NEEDS CLARIFICATION]`. If the doc is wrong, the agent updates it after implementation.

### Completion flow

```
status:planned → agent implements → agent marks status:tentative → user reviews → status:implemented
```

- **planned**: Described in docs but not yet built
- **in-progress**: An agent is actively working on it
- **tentative**: Agent believes it meets all success criteria, awaiting human review
- **implemented**: Human has confirmed it works
- **deprecated**: Superseded or removed

### What comes later (Phase 2 → 3)

Phase 2 (knowledge base) adds:
- Status tag tooling: scan, summarize, lint
- `[NEEDS CLARIFICATION]` markers with tracking
- Two-step completion flow (tentative → implemented)
- Document templates and conventions

Phase 3 (knowledge graph) adds:
- Visual graph panel in the IDE
- Completeness dashboards derived from status tags
- Per-instance awareness (which agent is touching what)
- Dependency visualization

Future (unscheduled):
- Structured frontmatter (YAML) for machine-parseable metadata
- `[@test]` links from specs to test files (from Tessl)
- Automated tag updates via Claude Code hooks (PostToolUse → check if section was modified → update `validated` tag)

## Open questions

- How to handle the brownfield problem — nekode already has implemented firmware, so docs need to describe existing code accurately
- When does a section get big enough to split into its own document?
- Should agents auto-update docs after implementation, or only when explicitly asked?
- How to prevent doc drift when multiple agents are editing the same document concurrently (worktree isolation helps but doesn't solve cross-doc references)
