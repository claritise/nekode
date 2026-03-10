import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { symlink, mkdir, copyFile, access, readlink, unlink } from 'node:fs/promises';
import { join, resolve } from 'node:path';
import { INSTANCE_COUNT } from './constants.js';

const execFileAsync = promisify(execFile);

const WORKTREE_DIR = '.worktrees';
const BRANCH_PREFIX = 'nekode';

export interface WorktreeInfo {
  id: number;
  path: string;
  branch: string;
}

/** Get the git repo root directory. */
async function getRepoRoot(): Promise<string> {
  const { stdout } = await execFileAsync('git', ['rev-parse', '--show-toplevel']);
  return stdout.trim();
}

/** Run a git command in the repo root. */
async function git(args: string[], cwd?: string): Promise<string> {
  const { stdout } = await execFileAsync('git', args, { cwd });
  return stdout.trim();
}

/** Get the branch name for an instance. */
function branchName(id: number): string {
  return `${BRANCH_PREFIX}-${id}`;
}

/** Get the worktree directory name for an instance. */
function worktreeDirName(id: number): string {
  return `i-${id}`;
}

/**
 * Clean up stale worktrees and branches from previous crashes.
 * Always call this before creating new worktrees.
 */
export async function pruneStaleWorktrees(): Promise<void> {
  const repoRoot = await getRepoRoot();

  // Prune stale worktree entries
  await git(['worktree', 'prune'], repoRoot);

  // Remove any leftover worktree directories first (must happen before branch deletion)
  const worktreeBase = join(repoRoot, WORKTREE_DIR);
  for (let i = 0; i < INSTANCE_COUNT; i++) {
    const wtPath = join(worktreeBase, worktreeDirName(i));
    try {
      await git(['worktree', 'remove', '--force', wtPath], repoRoot);
    } catch {
      // Already gone — fine
    }
  }

  // Prune again after removals to clear any dangling entries
  await git(['worktree', 'prune'], repoRoot);

  // Force-delete any leftover nekode branches (safe now that worktrees are gone)
  for (let i = 0; i < INSTANCE_COUNT; i++) {
    const branch = branchName(i);
    try {
      await git(['branch', '-D', branch], repoRoot);
    } catch {
      // Branch doesn't exist — that's fine
    }
  }
}

/**
 * Create a single worktree for an instance.
 * Returns the absolute path to the worktree directory.
 */
export async function createWorktree(id: number): Promise<WorktreeInfo> {
  const repoRoot = await getRepoRoot();
  const branch = branchName(id);
  const wtRelative = join(WORKTREE_DIR, worktreeDirName(id));
  const wtAbsolute = resolve(repoRoot, wtRelative);

  // Create worktree branching from HEAD
  await git(['worktree', 'add', wtRelative, '-b', branch, 'HEAD'], repoRoot);

  // Symlink node_modules from main repo if it exists
  await symlinkNodeModules(repoRoot, wtAbsolute);

  // Copy .env if it exists in repo root
  await copyIfExists(join(repoRoot, '.env'), join(wtAbsolute, '.env'));

  return { id, path: wtAbsolute, branch };
}

/**
 * Create all worktrees. Prunes stale ones first.
 */
export async function createAllWorktrees(): Promise<WorktreeInfo[]> {
  await pruneStaleWorktrees();

  const worktrees: WorktreeInfo[] = [];
  for (let i = 0; i < INSTANCE_COUNT; i++) {
    worktrees.push(await createWorktree(i));
  }
  return worktrees;
}

/**
 * Destroy all worktrees and their branches.
 */
export async function destroyAllWorktrees(): Promise<void> {
  const repoRoot = await getRepoRoot();

  for (let i = 0; i < INSTANCE_COUNT; i++) {
    const wtPath = join(repoRoot, WORKTREE_DIR, worktreeDirName(i));
    try {
      await git(['worktree', 'remove', '--force', wtPath], repoRoot);
    } catch {
      // Already gone
    }
    try {
      await git(['branch', '-D', branchName(i)], repoRoot);
    } catch {
      // Branch already gone
    }
  }

  // Final prune
  await git(['worktree', 'prune'], repoRoot);
}

/**
 * Symlink node_modules from the main repo into a worktree.
 * If a symlink already exists and points to the right place, skip it.
 */
async function symlinkNodeModules(repoRoot: string, wtPath: string): Promise<void> {
  const source = join(repoRoot, 'node_modules');
  const target = join(wtPath, 'node_modules');

  try {
    await access(source);
  } catch {
    return; // No node_modules in repo root
  }

  try {
    const existing = await readlink(target);
    if (existing === source) return; // Already correct
    await unlink(target);
  } catch {
    // Doesn't exist yet
  }

  await symlink(source, target, 'dir');
}

/**
 * Copy a file if it exists in the source location.
 */
async function copyIfExists(src: string, dest: string): Promise<void> {
  try {
    await access(src);
    await mkdir(join(dest, '..'), { recursive: true });
    await copyFile(src, dest);
  } catch {
    // Source doesn't exist — skip
  }
}
