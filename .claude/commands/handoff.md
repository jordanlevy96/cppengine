---
allowed-tools: Bash(date:*), Bash(cat:*), Bash(pbcopy:*), Bash(tee:*)
description: Generate a comprehensive handoff document for another developer
---

# Developer Handoff Generator

Analyze the ENTIRE current conversation context and generate a comprehensive handoff document for another developer taking over this work. The handoff should enable them to continue seamlessly without needing to re-discover context.

**Important**: Review the full conversation history to capture all context, decisions, and progress made.

## Required Sections

### 1. Problem Statement

- What problem is being solved?
- Why does it matter?
- What triggered this work?

### 2. Original Requirements

- List all user requests from first principles
- Include any clarifications or refinements made during the conversation
- Note any implicit requirements that were discovered

### 3. Current Status

- What has been completed?
- What is in progress?
- What hasn't been started?
- Are there any failing tests or known issues?

### 4. Technical Context

- **Relevant Files**: List file paths that are central to this work
- **Key Classes/Functions**: Name the main code constructs involved (no code snippets)
- **Architecture Notes**: Any important patterns or design decisions made
- **Dependencies**: External services, libraries, or modules involved

### 5. Validation Criteria

- How do we know when this is done?
- What tests need to pass?
- What manual verification is needed?
- Any edge cases to watch for?

### 6. Constraints & Considerations

- Technical constraints discovered
- Business rules to follow
- Performance requirements
- Security considerations
- Compatibility requirements

### 7. Next Steps

- Ordered list of remaining work
- Any blockers or dependencies
- Suggested approach for each step

### 8. Open Questions

- Unresolved decisions
- Areas needing clarification
- Risks or uncertainties

### 9. Useful Commands

- Any commands used during development (test commands, build commands, etc.)

## Output Instructions

1. Generate the handoff document in clean Markdown format
2. Get the current timestamp: `date +%Y%m%d-%H%M%S`
3. Save the content to `./docs/handoff.<timestamp>.md` AND copy to clipboard in one command using:
   ```bash
   cat <<'EOF' | tee ./docs/handoff.<timestamp>.md | pbcopy
   <handoff content here>
   EOF
   ```
4. Confirm to the user that the handoff has been saved to Desktop and copied to clipboard

## Format Guidelines

- Be concise but complete
- Use bullet points for scannability
- Include file paths as `path/to/file.py:line_number` where relevant
- Reference function/class names without including actual code
- Focus on enabling continuation, not documenting everything
