# Imhotep Documentation Index

> Last Updated: January 16, 2026

## Quick Reference

| Document | Purpose | Status |
|----------|---------|--------|
| [CLAUDE.md](../CLAUDE.md) | AI assistant context, quick reference | Stable |
| [README.md](../README.md) | Build instructions, getting started | Stable |
| [CHANGELOG.md](../CHANGELOG.md) | Version history, decisions | Stable |
| [REFERENCES.md](../REFERENCES.md) | Library credits, licenses | Stable |

---

## Architecture Documentation

| Document | Purpose | Status |
|----------|---------|--------|
| [UI_SYSTEM.md](architecture/UI_SYSTEM.md) | HTML/CSS/Lua reactive UI, threading, events | Stable |
| [EDITOR_ARCHITECTURE.md](architecture/EDITOR_ARCHITECTURE.md) | Editor design, all implementation phases | Phase 1 Complete |
| [EDITOR_VIEWPORT.md](architecture/EDITOR_VIEWPORT.md) | Viewport rendering, texture transfer, click handling | Partial |
| [TRANSFORM_PIPELINE.md](architecture/TRANSFORM_PIPELINE.md) | Hierarchical transform refactor plan | Planned |
| [VULKAN_MIGRATION.md](architecture/VULKAN_MIGRATION.md) | OpenGL to Vulkan migration analysis | Research |

---

## Status Legend

- **Stable**: Documentation is current and accurate
- **Phase N Complete**: Implementation through phase N finished
- **Partial**: Some work complete, some planned
- **Planned**: Design documented, implementation not started
- **Research**: Analysis/learning resource, not immediate priority

---

## Document Summaries

### CLAUDE.md
Quick-reference for AI assistants and developers: project overview, build instructions, common workflows, file organization. For detailed system docs, see architecture documents.

### UI_SYSTEM.md
Canonical reference for the reactive UI system: template directives (v-if, v-for), multi-threaded rendering, event handling, Lua state management, thread safety.

### EDITOR_ARCHITECTURE.md
Full editor design and implementation roadmap:
- **Phase 1** (Complete): Foundation - window, panels, basic UI
- **Phase 2** (Planned): Scene viewport with camera controls
- **Phase 3** (Planned): Inspector and component editing
- **Phase 4** (Planned): Hot reload
- **Phase 5** (Planned): UI template editor
- **Phase 6** (Planned): Advanced features (undo/redo, save/load)

### EDITOR_VIEWPORT.md
Editor viewport system:
- **Texture Transfer** (Planned): Optimize PNG+base64 pipeline to direct texture sharing
- **Click Handling** (Complete): v-for handler expression substitution fix

### TRANSFORM_PIPELINE.md
Transform system refactor for proper hierarchy support:
- Phase 1-5 all planned (WorldTransform component, HierarchySystem, dirty flags)

### VULKAN_MIGRATION.md
Future graphics API migration research: current OpenGL analysis, Vulkan requirements, migration strategy.

---

## Updating Documentation

When modifying docs:
1. Update "Last Updated" date in document header
2. Update status in this index
3. Cross-reference related documents
