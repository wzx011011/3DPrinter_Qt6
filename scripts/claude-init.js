#!/usr/bin/env node
/**
 * claude-init.js — Auto-detect project stack and initialize .claude/ architecture
 *
 * Usage:
 *   node claude-init.js <target-directory>
 *   node claude-init.js .                  # current directory
 *   node claude-init.js . --force           # overwrite existing files
 *
 * Detects: framework, package manager, TypeScript, testing, linting, build tool,
 *          monorepo setup, task management approach.
 *
 * Generates: CLAUDE.md, rules/, agents/, skills/ with progressive disclosure.
 *
 * Architecture pattern (migrated from 3DPrinter_Qt6):
 *   rules/   = global constraints (what NOT to do) — auto-loaded every session
 *   agents/  = sub-agent runtime constraints (tools, isolation, model, turns)
 *   skills/  = user-invocable workflows (slash commands with $ARGUMENTS)
 *   reference/ = progressive disclosure (loaded on demand, not every session)
 */

'use strict';

const fs = require('fs');
const path = require('path');

// ─── CLI ──────────────────────────────────────────────────────────────────

const args = process.argv.slice(2);
let targetDir = null;
let force = false;

for (const arg of args) {
  if (arg === '--force') force = true;
  else if (!arg.startsWith('-')) targetDir = arg;
}

if (!targetDir) targetDir = '.';
targetDir = path.resolve(targetDir);

if (!fs.existsSync(targetDir)) {
  console.error(`Error: directory not found: ${targetDir}`);
  process.exit(1);
}

// ─── Detection ────────────────────────────────────────────────────────────

function exists(file) {
  return fs.existsSync(path.join(targetDir, file));
}

function readJson(file) {
  try {
    return JSON.parse(fs.readFileSync(path.join(targetDir, file), 'utf8'));
  } catch { return null; }
}

function hasDep(name) {
  const pkg = readJson('package.json');
  if (!pkg) return false;
  return !!(pkg.dependencies?.[name] || pkg.devDependencies?.[name]);
}

function detectPackageManager() {
  if (exists('pnpm-lock.yaml')) return 'pnpm';
  if (exists('yarn.lock')) return 'yarn';
  return 'npm';
}

function detectFramework() {
  if (hasDep('next')) return 'next';
  if (hasDep('nuxt')) return 'nuxt';
  if (hasDep('@angular/core')) return 'angular';
  if (hasDep('svelte')) return 'svelte';
  if (hasDep('vue')) return 'vue';
  if (hasDep('react')) return 'react';
  return 'vanilla';
}

function detectTypeScript() {
  return exists('tsconfig.json');
}

function detectBuildTool() {
  if (exists('next.config.js') || exists('next.config.mjs') || exists('next.config.ts')) return 'next';
  if (exists('nuxt.config.ts') || exists('nuxt.config.js')) return 'nuxt';
  if (exists('vite.config.ts') || exists('vite.config.js')) return 'vite';
  if (exists('webpack.config.js') || exists('webpack.config.ts')) return 'webpack';
  return detectPackageManager(); // fallback to package manager scripts
}

function detectTesting() {
  if (hasDep('vitest')) return 'vitest';
  if (hasDep('@playwright/test')) return 'playwright';
  if (hasDep('cypress')) return 'cypress';
  if (hasDep('jest')) return 'jest';
  return 'none';
}

function detectLinting() {
  if (exists('biome.json') || exists('biome.jsonc')) return 'biome';
  if (exists('.eslintrc.js') || exists('.eslintrc.cjs') ||
      exists('.eslintrc.json') || exists('.eslintrc.yml') ||
      exists('eslint.config.js') || exists('eslint.config.mjs') ||
      exists('eslint.config.ts')) return 'eslint';
  return 'none';
}

function detectMonorepo() {
  if (exists('nx.json')) return 'nx';
  if (exists('turbo.json')) return 'turbo';
  if (exists('lerna.json')) return 'lerna';
  if (exists('pnpm-workspace.yaml')) return 'pnpm-workspace';
  return 'none';
}

function detectTaskTracker() {
  if (exists('.github') && fs.statSync(path.join(targetDir, '.github')).isDirectory()) return 'github';
  const docsDir = path.join(targetDir, 'docs');
  if (fs.existsSync(docsDir) && fs.statSync(docsDir).isDirectory()) {
    const files = fs.readdirSync(docsDir).filter(f => f.endsWith('.md'));
    if (files.some(f => /task|todo|issue|进度|任务|roadmap/i.test(f))) return 'markdown-docs';
  }
  if (exists('TODO.md') || exists('TASKS.md') || exists('ROADMAP.md')) return 'markdown-root';
  return 'ad-hoc';
}

function detect() {
  const profile = {
    packageManager: detectPackageManager(),
    framework: detectFramework(),
    typescript: detectTypeScript(),
    buildTool: detectBuildTool(),
    testing: detectTesting(),
    linting: detectLinting(),
    monorepo: detectMonorepo(),
    taskTracker: detectTaskTracker(),
  };
  // Some frameworks imply a build tool
  if (profile.framework === 'next' && profile.buildTool !== 'next') profile.buildTool = 'next';
  if (profile.framework === 'nuxt' && profile.buildTool !== 'nuxt') profile.buildTool = 'nuxt';
  return profile;
}

// ─── Helpers ─────────────────────────────────────────────────────────────

const PM = {
  npm:   { install: 'npm install', build: 'npm run build', dev: 'npm run dev', test: 'npm test', lint: 'npm run lint', ci: 'npm ci' },
  pnpm:  { install: 'pnpm install', build: 'pnpm build', dev: 'pnpm dev', test: 'pnpm test', lint: 'pnpm lint', ci: 'pnpm install --frozen-lockfile' },
  yarn:  { install: 'yarn', build: 'yarn build', dev: 'yarn dev', test: 'yarn test', lint: 'yarn lint', ci: 'yarn install --frozen-lockfile' },
};

const frameworkLabel = {
  react: 'React', vue: 'Vue 3', next: 'Next.js', nuxt: 'Nuxt.js',
  angular: 'Angular', svelte: 'Svelte', vanilla: 'Vanilla JS/TS', unknown: 'Unknown',
};

const testingLabel = {
  vitest: 'Vitest', jest: 'Jest', playwright: 'Playwright', cypress: 'Cypress', none: 'None detected',
};

const lintingLabel = {
  eslint: 'ESLint', biome: 'Biome', none: 'None detected',
};

// ─── Template Generators ─────────────────────────────────────────────────

function genClaudeMd(p) {
  const pm = PM[p.packageManager];
  let lines = [
    `# ${path.basename(targetDir)} Claude Code Instructions`,
    '',
    'This repository uses Claude Code for assisted development.',
    '',
    '## Project Skills',
    '',
    '- Use `/implementing-task` to work on the next task or a specific task by ID.',
    '  Append `all` for continuous batch mode.',
    '- Use `/reviewing-code` to perform a read-only code quality analysis.',
    '',
    '## Build',
    '',
    `**Build:** \`${pm.build}\``,
    `**Dev:**   \`${pm.dev}\``,
    `**Test:**  \`${pm.test}\``,
  ];
  if (p.linting !== 'none') {
    lines.push(`**Lint:**  \`${pm.lint}\``);
  }
  lines.push('', 'See @.claude/rules/build-rules.md for detailed build rules.');
  lines.push('See @.claude/rules/code-quality.md for code style conventions.');

  if (p.monorepo !== 'none') {
    lines.push('');
    lines.push('## Monorepo');
    lines.push(`This is a **${p.monorepo}** monorepo. Use package-scoped commands when working on specific packages.`);
    lines.push(`Example: \`${pm.build} --filter=<package-name>\``);
  }

  return lines.join('\n') + '\n';
}

function genBuildRules(p) {
  const pm = PM[p.packageManager];
  let lines = [
    '# Build Rules',
    '',
    '## Build Commands',
    '',
    `| Command | Script |`,
    `|---------|--------|`,
    `| Install | \`${pm.install}\` |`,
    `| Build   | \`${pm.build}\` |`,
    `| Dev     | \`${pm.dev}\` |`,
    `| Test    | \`${pm.test}\` |`,
  ];
  if (p.linting !== 'none') {
    lines.push(`| Lint    | \`${pm.lint}\` |`);
  }
  lines.push(`| CI Install | \`${pm.ci}\` |`);

  lines.push('', '## Prohibited');
  lines.push('- Do not create alternative build scripts.');
  lines.push('- Do not use `--force` or skip dependency checks.');
  lines.push('- Do not commit lock file changes without a corresponding package.json change.');

  if (p.testing !== 'none') {
    lines.push('', '## Testing');
    lines.push(`Use \`${pm.test}\` as the authoritative test command.`);
    if (p.testing === 'vitest') {
      lines.push('For single file: `npx vitest run path/to/file.test.ts`');
    } else if (p.testing === 'jest') {
      lines.push('For single file: `npx jest path/to/file.test.ts`');
    } else if (p.testing === 'playwright') {
      lines.push('For e2e: `npx playwright test`');
      lines.push('For single spec: `npx playwright test path/to/spec.ts`');
    } else if (p.testing === 'cypress') {
      lines.push('For e2e: `npx cypress run`');
      lines.push('For single spec: `npx cypress run --spec path/to/spec.ts`');
    }
  }

  if (p.monorepo !== 'none') {
    lines.push('', '## Monorepo');
    lines.push('Scope commands to specific packages:');
    if (p.monorepo === 'turbo') {
      lines.push(`  ${pm.build} --filter=<package>`);
      lines.push(`  ${pm.test} --filter=<package>`);
    } else if (p.monorepo === 'nx') {
      lines.push('  npx nx run <project>:build');
      lines.push('  npx nx run <project>:test');
    } else {
      lines.push(`  ${pm.build} --filter=<package>`);
    }
    lines.push('Always verify the affected package builds and tests pass.');
  }

  return lines.join('\n') + '\n';
}

function genCodeQuality(p) {
  let lines = [
    '# Code Quality Rules',
    '',
  ];

  // TypeScript
  if (p.typescript) {
    lines.push('## TypeScript');
    lines.push('- Prefer strict mode. Do not add `@ts-ignore` or `// @ts-nocheck` without a comment explaining why.');
    lines.push('- Use explicit return types for exported functions.');
    lines.push('- Prefer `interface` over `type` for object shapes.');
    lines.push('- No implicit `any` — use proper types or `unknown` with narrowing.');
    lines.push('');
  }

  // Framework conventions
  const fw = p.framework;
  if (fw === 'react' || fw === 'next') {
    lines.push('## React Conventions');
    lines.push('- Components: PascalCase file names (e.g., `UserProfile.tsx`).');
    lines.push('- Hooks: camelCase with `use` prefix (e.g., `useAuth.ts`).');
    lines.push('- Utilities: camelCase (e.g., `formatDate.ts`).');
    lines.push('- Prefer functional components with hooks over class components.');
    lines.push('- Colocate component styles, tests, and types in the same directory.');
    if (fw === 'next') {
      lines.push('');
      lines.push('## Next.js Conventions');
      lines.push('- Use App Router (`app/` directory) for new pages.');
      lines.push('- Use Server Components by default; add `"use client"` only when needed.');
      lines.push('- API routes go in `app/api/`.');
      lines.push('- Shared UI components go in `components/`, not in `app/`.');
    }
  } else if (fw === 'vue' || fw === 'nuxt') {
    lines.push('## Vue 3 Conventions');
    lines.push('- Components: PascalCase file names (e.g., `UserProfile.vue`).');
    lines.push('- Composables: camelCase with `use` prefix (e.g., `useAuth.ts`).');
    lines.push('- Prefer `<script setup lang="ts">` for single-file components.');
    lines.push('- Use `defineProps` / `defineEmits` with typed interfaces.');
    lines.push('- Colocate component styles and tests in the same directory.');
    if (fw === 'nuxt') {
      lines.push('');
      lines.push('## Nuxt.js Conventions');
      lines.push('- Use auto-imports; do not manually import composables from `#imports`.');
      lines.push('- Pages go in `pages/`, layouts in `layouts/`, middleware in `middleware/`.');
      lines.push('- Server API routes go in `server/api/`.');
    }
  } else if (fw === 'angular') {
    lines.push('## Angular Conventions');
    lines.push('- Use standalone components.');
    lines.push('- Services: `*.service.ts`, Guards: `*.guard.ts`, Resolvers: `*.resolver.ts`.');
    lines.push('- Prefer signals over RxJS for simple state; keep RxJS for complex async flows.');
  } else if (fw === 'svelte') {
    lines.push('## Svelte Conventions');
    lines.push('- Components: PascalCase file names (e.g., `UserProfile.svelte`).');
    lines.push('- Stores go in `src/lib/stores/`.');
    lines.push('- Use `$props()` rune for typed props.');
  } else {
    lines.push('## General JavaScript/TypeScript');
    lines.push('- Use ES modules (`import`/`export`).');
    lines.push('- Prefer `const`/`let` over `var`.');
    lines.push('- Use template literals over string concatenation.');
  }

  lines.push('');
  lines.push('## Anti-Patterns');
  lines.push('- No `console.log` in production code — use a proper logger.');
  lines.push('- No commented-out code blocks — delete them.');
  lines.push('- No magic numbers — extract to named constants.');
  lines.push('- No deeply nested ternaries — use early returns or extract to variables.');

  if (p.linting !== 'none') {
    lines.push('');
    lines.push(`## Linting (${lintingLabel[p.linting]})`);
    lines.push(`Run lint before committing. Fix all errors, do not suppress warnings.`);
    if (p.linting === 'biome') {
      lines.push('Use `biome check --write .` for auto-fixable issues.');
    }
  }

  return lines.join('\n') + '\n';
}

function genGitWorkflow() {
  const lines = [
    '# Git Workflow Rules',
    '',
    '## Branch Strategy',
    '- `main` — stable, deployable branch.',
    '- `feat/<description>` — new features.',
    '- `fix/<description>` — bug fixes.',
    '- `refactor/<description>` — code refactoring.',
    '',
    '## Commit Conventions',
    '- Use conventional commits: `feat:`, `fix:`, `refactor:`, `docs:`, `test:`, `chore:`.',
    '- Keep commits small and focused — one logical change per commit.',
    '- Do not commit generated files, lock file changes without package.json changes, or secrets.',
    '',
    '## Pull Requests',
    '- PR title should summarize the change in one line.',
    '- PR description should explain *why*, not just *what*.',
    '- Do not force-push to main.',
    '',
    '## Prohibited',
    '- No `git push --force` to shared branches.',
    '- No committing `.env`, credentials, or API keys.',
    '- No large binary files without Git LFS.',
  ];
  return lines.join('\n') + '\n';
}

function genTaskWorker() {
  const lines = [
    '---',
    'name: task-worker',
    'description: "Proactively use for development tasks that involve reading code, implementing changes, running tests, and verifying builds. Use when the task involves modifying source files, running npm commands, or creating PRs."',
    'tools:',
    '  - Read',
    '  - Write',
    '  - Edit',
    '  - Glob',
    '  - Grep',
    '  - Bash',
    'disallowedTools:',
    '  - AskUserQuestion',
    'maxTurns: 30',
    'model: inherit',
    'memory: project',
    'isolation: worktree',
    '---',
    '',
    '# Task Worker Sub-Agent',
    '',
    'You are a development worker agent that implements bounded tasks in isolation.',
    '',
    '## Context Strategy',
    '',
    '- Read the task tracker (GitHub Issues, docs/, or task description) to understand what to do.',
    '- Focus on a single bounded task — do not chain unrelated changes.',
    '- Report completion with: files changed, test results, remaining gaps.',
    '',
    '## Workflow',
    '',
    '1. Identify the task and its acceptance criteria.',
    '2. Read relevant source files to understand the existing code.',
    '3. Implement the minimal change that satisfies the task.',
    '4. Run tests and verify the build.',
    '5. Report results — do not wait for further instructions.',
    '',
    '## Constraints',
    '',
    '- Follow `.claude/rules/build-rules.md` and `.claude/rules/code-quality.md`.',
    '- Do not introduce breaking changes without explicit task description.',
    '- Do not skip tests or lint checks.',
    '',
    '## Prohibited',
    '',
    '- Do not ask the user questions — escalate by reporting the blocker.',
    '- Do not make changes outside the task scope.',
    '- Do not commit secrets or generated files.',
    '',
    '## Output Format',
    '',
    'After completing:',
    '- Task ID and description',
    '- Files modified',
    '- Test/build verification results',
    '- Remaining gaps or blockers',
  ];
  return lines.join('\n') + '\n';
}

function genImplementingSkill(p) {
  const pm = PM[p.packageManager];

  let taskSourceSection;
  if (p.taskTracker === 'github') {
    taskSourceSection = [
      '1. 使用 `gh issue list` 查看待办任务，选择优先级最高的开放 Issue。',
      '2. 如果 `$ARGUMENTS` 包含 Issue 编号（如 `123`），则直接处理该 Issue。',
      '3. 读取 Issue 详情理解需求。',
    ].join('\n');
  } else if (p.taskTracker === 'markdown-docs') {
    taskSourceSection = [
      '1. 读取 `docs/` 目录下的任务追踪文件，定位最高优先级 `[ ]` 或 `[-]` 任务。',
      '2. 如果 `$ARGUMENTS` 包含任务 ID（如 `P2.1`），则直接处理该任务。',
    ].join('\n');
  } else if (p.taskTracker === 'markdown-root') {
    taskSourceSection = [
      '1. 读取项目根目录的 `TODO.md` / `TASKS.md` / `ROADMAP.md`，定位下一个待办项。',
      '2. 如果 `$ARGUMENTS` 包含任务描述，直接处理。',
    ].join('\n');
  } else {
    taskSourceSection = [
      '1. 从 `$ARGUMENTS` 中获取任务描述。如果没有，请用户提供任务。',
      '2. 理解需求范围和验收标准。',
    ].join('\n');
  }

  const lines = [
    '---',
    'name: implementing-task',
    'description: "Implementing a development task by reading task tracker, making code changes, running tests, and verifying the build. Triggers when the user asks to work on a task, pick up the next task, or run implementation in batch mode."',
    '---',
    '',
    '# Task Implementation',
    '',
    '推进开发任务：读取任务源 → 理解需求 → 实现变更 → 验证构建。',
    '',
    '遵循 `.claude/rules/build-rules.md` 和 `.claude/rules/code-quality.md` 中的规范，不在此重复。',
    '',
    '## Parameters',
    '',
    '- `$ARGUMENTS` 为空：**单任务模式** — 自动选择下一个任务，完成 1 个后停止。',
    '- `$ARGUMENTS` 包含 `all`：**批量模式** — 连续推进直到遇到停止条件。',
    '- `$ARGUMENTS` 包含具体 ID 或描述：**定向模式** — 处理指定任务。',
    '',
    '## Workflow',
    '',
    taskSourceSection,
    '3. 阅读相关源码，理解现有实现。',
    '4. 实现最小、可验证的变更。',
    `5. 运行 \`${pm.test}\` 验证测试通过。`,
    `6. 运行 \`${pm.build}\` 验证构建通过。`,
    '7. 报告完成结果。',
    '',
    '## Batch Mode',
    '',
    '批量模式下，每完成一个任务后：',
    '- 如果测试或构建失败，停止并报告。',
    '- 如果上下文使用超过 50%，停止并建议开始新会话。',
    '- 否则继续下一个任务。',
    '',
    '停止条件详见 `reference/stop-conditions.md`。',
    '',
    '## Sub-Agent Strategy',
    '',
    '对于独立且边界清晰的任务，使用子 agent 委托以保持主上下文精简。',
    '详见 `reference/subagent-strategy.md`。',
  ];
  return lines.join('\n') + '\n';
}

function genImplementingOutputFormat(p) {
  const pm = PM[p.packageManager];
  const lines = [
    '# Output Format Templates',
    '',
    '## Task Queue Overview',
    '',
    '```',
    'Tasks: <total> total | <open> open | <in-progress> in progress | <done> done',
    '',
    '| # | ID | Title | Status |',
    '|---|-----|-------|--------|',
    '| 1 | ... | ... | [ ] |',
    '```',
    '',
    '## Current Task Progress',
    '',
    '```',
    'Task: <ID> - <title>',
    'Source: <GitHub Issue / docs/*.md / ad-hoc>',
    '',
    'Changes:',
    '  - <file1>: <brief change>',
    '  - <file2>: <brief change>',
    '',
    'Verification:',
    `  [PASS/FAIL] ${pm.test}`,
    `  [PASS/FAIL] ${pm.build}`,
    '```',
    '',
    '## Completion Report',
    '',
    '```',
    'Completed: <ID> - <title>',
    'Files: <count> changed',
    'Tests: <pass>/<total> passed',
    'Build: PASS/FAIL',
    'Gaps: <remaining items or "none">',
    '```',
    '',
    '## Batch Session Summary',
    '',
    '```',
    'Batch session complete',
    'Tasks completed: <n>',
    'Tasks failed: <n>',
    'Total files changed: <n>',
    'Session reason: <stop condition that ended the batch>',
    '```',
  ];
  return lines.join('\n') + '\n';
}

function genStopConditions() {
  const lines = [
    '# Stop Conditions',
    '',
    '## Batch Mode Auto-Stop',
    '',
    'Batch mode (`$ARGUMENTS` contains `all`) stops when:',
    '',
    '1. **Test failure** — Any test fails. Fix before continuing.',
    '2. **Build failure** — Build fails. Fix before continuing.',
    '3. **Context pressure** — Autocompact has fired 2+ times in this session.',
    '4. **No more tasks** — All open tasks in the tracker are completed.',
    '5. **External dependency** — Task requires a library, API, or resource not yet available.',
    '',
    '## Continuous Execution Constraints',
    '',
    '- Do not start a new task without confirming the previous one passed all checks.',
    '- Do not batch more than 5 tasks without a checkpoint report.',
    '- If a task takes more than 3 implementation iterations, stop and report for review.',
  ];
  return lines.join('\n') + '\n';
}

function genSubAgentStrategy() {
  const lines = [
    '# Sub-Agent Strategy',
    '',
    '## When to Delegate',
    '',
    '| Condition | Action | Agent |',
    '|-----------|--------|-------|',
    '| Independent file/module change | Delegate | `task-worker` |',
    '| Cross-module refactoring | Handle in main context | — |',
    '| Build verification only | Handle in main context | — |',
    '| Multiple independent tasks | Parallel delegates | `task-worker` x N |',
    '',
    '## Delegation Rules',
    '',
    '1. Each delegate gets a self-contained prompt with: task description, affected files, acceptance criteria.',
    '2. Each delegate runs in an isolated worktree — no shared state with siblings.',
    '3. After all delegates complete, review results in main context.',
    '4. If a delegate fails, investigate in main context before retrying.',
  ];
  return lines.join('\n') + '\n';
}

function genReviewingSkill(p) {
  const pm = PM[p.packageManager];
  const lines = [
    '---',
    'name: reviewing-code',
    'description: "Reviewing code quality, patterns, and potential issues without making changes. Triggers when the user asks for a code review, quality analysis, or architectural assessment."',
    '---',
    '',
    '# Code Review',
    '',
    '对代码进行只读质量分析，不修改任何文件。',
    '',
    '## Parameters',
    '',
    '- `$ARGUMENTS` 指定审查范围：文件路径、目录、组件名、或 "all"（全局审查）。',
    '- 为空时对最近变更的文件进行审查。',
    '',
    '## Workflow',
    '',
    '1. 确定审查范围（从 git diff、指定路径、或全项目扫描）。',
    '2. 阅读相关源码和规则文件。',
    '3. 对照 `.claude/rules/code-quality.md` 检查合规性。',
    '4. 识别问题并分类。',
    '5. 输出审查报告。',
    '',
    '## Review Checklist',
    '',
    `- [ ] ${p.typescript ? 'TypeScript 类型安全' : '类型正确性'}`,
    '- [ ] 命名规范（组件 PascalCase、工具 camelCase）',
    `- [ ] ${lintingLabel[p.linting]} 规则合规`,
    '- [ ] 无 magic numbers / 无 console.log / 无注释掉的代码',
    '- [ ] 错误处理完整（无 unhandled promise rejection）',
    '- [ ] 性能问题（不必要的重渲染、内存泄漏风险）',
    '- [ ] 安全问题（XSS、注入、敏感数据泄露）',
    '',
    '## Output',
    '',
    '输出格式详见 `reference/output-format.md`。',
  ];
  return lines.join('\n') + '\n';
}

function genReviewingOutputFormat() {
  const lines = [
    '# Review Output Format',
    '',
    '## Review Report Template',
    '',
    '```',
    'Code Review: <scope>',
    'Files reviewed: <n>',
    'Date: <date>',
    '',
    'Summary: <overall quality assessment>',
    '',
    '### Issues Found',
    '',
    '| # | Severity | File:Line | Issue | Suggestion |',
    '|---|----------|-----------|-------|------------|',
    '| 1 | <error/warn/info> | ... | ... | ... |',
    '',
    '### Patterns Observed',
    '',
    '- <positive pattern 1>',
    '- <concern pattern 1>',
    '',
    '### Recommendations',
    '',
    '1. <priority recommendation>',
    '2. <recommendation>',
    '```',
    '',
    '## Severity Levels',
    '',
    '- **error**: Must fix — bugs, security issues, broken functionality.',
    '- **warn**: Should fix — code quality, maintainability, performance.',
    '- **info**: Consider — style, convention, minor improvements.',
  ];
  return lines.join('\n') + '\n';
}

// ─── File Manifest ────────────────────────────────────────────────────────

function getFileManifest(p) {
  return [
    { path: 'CLAUDE.md', content: genClaudeMd(p), desc: 'Project-level Claude Code instructions' },
    { path: '.claude/rules/build-rules.md', content: genBuildRules(p), desc: 'Build commands and constraints' },
    { path: '.claude/rules/code-quality.md', content: genCodeQuality(p), desc: 'Code style and conventions' },
    { path: '.claude/rules/git-workflow.md', content: genGitWorkflow(), desc: 'Git conventions' },
    { path: '.claude/agents/task-worker.md', content: genTaskWorker(), desc: 'Sub-agent for isolated task execution' },
    { path: '.claude/skills/implementing-task/SKILL.md', content: genImplementingSkill(p), desc: 'Main implementation workflow skill' },
    { path: '.claude/skills/implementing-task/reference/output-format.md', content: genImplementingOutputFormat(p), desc: 'Output format templates' },
    { path: '.claude/skills/implementing-task/reference/stop-conditions.md', content: genStopConditions(), desc: 'Batch mode stop conditions' },
    { path: '.claude/skills/implementing-task/reference/subagent-strategy.md', content: genSubAgentStrategy(), desc: 'Sub-agent delegation strategy' },
    { path: '.claude/skills/reviewing-code/SKILL.md', content: genReviewingSkill(p), desc: 'Code review workflow skill' },
    { path: '.claude/skills/reviewing-code/reference/output-format.md', content: genReviewingOutputFormat(), desc: 'Review report template' },
  ];
}

// ─── Writer ──────────────────────────────────────────────────────────────

function writeFile(filePath, content) {
  const fullPath = path.join(targetDir, filePath);
  const dir = path.dirname(fullPath);
  fs.mkdirSync(dir, { recursive: true });

  if (fs.existsSync(fullPath) && !force) {
    console.log(`  [SKIP] ${filePath} (already exists, use --force to overwrite)`);
    return false;
  }

  fs.writeFileSync(fullPath, content, 'utf8');
  console.log(`  [OK]   ${filePath}`);
  return true;
}

// ─── Main ────────────────────────────────────────────────────────────────

function main() {
  console.log('');
  console.log('========================================');
  console.log(' claude-init — .claude Architecture Setup');
  console.log('========================================');
  console.log('');
  console.log(`Target: ${targetDir}`);
  console.log('');

  // Detect
  const profile = detect();

  console.log('Detection Results:');
  console.log(`  Package Manager: ${profile.packageManager}`);
  console.log(`  Framework:       ${frameworkLabel[profile.framework]}`);
  console.log(`  TypeScript:      ${profile.typescript ? 'Yes' : 'No'}`);
  console.log(`  Build Tool:      ${profile.buildTool}`);
  console.log(`  Testing:         ${testingLabel[profile.testing]}`);
  console.log(`  Linting:         ${lintingLabel[profile.linting]}`);
  console.log(`  Monorepo:        ${profile.monorepo === 'none' ? 'No' : profile.monorepo}`);
  console.log(`  Task Tracker:    ${profile.taskTracker}`);
  console.log('');

  // Generate
  const files = getFileManifest(profile);

  console.log('Generating files:');
  let created = 0;
  let skipped = 0;
  for (const file of files) {
    if (writeFile(file.path, file.content)) {
      created++;
    } else {
      skipped++;
    }
  }

  console.log('');
  console.log('========================================');
  console.log(` Done: ${created} created, ${skipped} skipped`);
  console.log('========================================');
  console.log('');

  if (created > 0) {
    console.log('Next steps:');
    console.log('  1. Review the generated files and adapt to your project needs.');
    console.log('  2. Start a Claude Code session and verify skills appear with /skills.');
    console.log('  3. Run /implementing-task to start working on your first task.');
    if (profile.taskTracker === 'ad-hoc') {
      console.log('');
      console.log('  Note: No task tracker detected. Create a TODO.md or set up GitHub Issues');
      console.log('  for /implementing-task to automatically pick up tasks.');
    }
    console.log('');
  }
}

main();
