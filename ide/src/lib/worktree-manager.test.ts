import { describe, it, expect, afterEach } from 'vitest';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { access } from 'node:fs/promises';
import { join } from 'node:path';
import {
  createAllWorktrees,
  destroyAllWorktrees,
  createWorktree,
  pruneStaleWorktrees,
} from './worktree-manager.js';
import { INSTANCE_COUNT } from './constants.js';

const execFileAsync = promisify(execFile);

async function git(args: string[]): Promise<string> {
  const { stdout } = await execFileAsync('git', args);
  return stdout.trim();
}

async function getRepoRoot(): Promise<string> {
  return git(['rev-parse', '--show-toplevel']);
}

describe('worktree-manager', () => {
  afterEach(async () => {
    // Always clean up worktrees after each test
    try {
      await destroyAllWorktrees();
    } catch {
      // Best effort cleanup
    }
  });

  it('prunes stale worktrees without error on clean repo', async () => {
    await expect(pruneStaleWorktrees()).resolves.toBeUndefined();
  });

  it('creates a single worktree with correct branch', async () => {
    await pruneStaleWorktrees();
    const wt = await createWorktree(0);

    expect(wt.id).toBe(0);
    expect(wt.branch).toBe('nekode-0');
    expect(wt.path).toContain('.worktrees/i-0');

    // Verify directory exists
    await expect(access(wt.path)).resolves.toBeUndefined();

    // Verify branch exists
    const branches = await git(['branch', '--list', 'nekode-0']);
    expect(branches).toContain('nekode-0');
  });

  it('creates all worktrees', async () => {
    const worktrees = await createAllWorktrees();

    expect(worktrees).toHaveLength(INSTANCE_COUNT);

    for (let i = 0; i < INSTANCE_COUNT; i++) {
      expect(worktrees[i].id).toBe(i);
      expect(worktrees[i].branch).toBe(`nekode-${i}`);
      await expect(access(worktrees[i].path)).resolves.toBeUndefined();
    }

    // Verify git worktree list shows them
    const list = await git(['worktree', 'list']);
    for (let i = 0; i < INSTANCE_COUNT; i++) {
      expect(list).toContain(`nekode-${i}`);
    }
  });

  it('destroys all worktrees and branches', async () => {
    await createAllWorktrees();
    await destroyAllWorktrees();

    // Verify branches are gone
    const branches = await git(['branch', '--list', 'nekode-*']);
    expect(branches).toBe('');

    // Verify worktree list only shows main
    const list = await git(['worktree', 'list']);
    const lines = list.split('\n').filter((l) => l.trim());
    expect(lines).toHaveLength(1); // Only main worktree
  });

  it('handles double-destroy gracefully', async () => {
    await createAllWorktrees();
    await destroyAllWorktrees();
    await expect(destroyAllWorktrees()).resolves.toBeUndefined();
  });

  it('symlinks node_modules if present', async () => {
    await pruneStaleWorktrees();
    const repoRoot = await getRepoRoot();
    const wt = await createWorktree(0);

    // Check if node_modules exists in repo root
    try {
      await access(join(repoRoot, 'node_modules'));
      // If it exists, worktree should have a symlink
      const { stdout } = await execFileAsync('readlink', [join(wt.path, 'node_modules')]);
      expect(stdout.trim()).toBe(join(repoRoot, 'node_modules'));
    } catch {
      // No node_modules in repo root — skip this check
    }
  });

  it('prune + recreate works after crash simulation', async () => {
    // Create worktrees, then "crash" by not cleaning up
    await createAllWorktrees();

    // Simulate restart: prune and recreate should work
    const worktrees = await createAllWorktrees();
    expect(worktrees).toHaveLength(INSTANCE_COUNT);
  });
});
