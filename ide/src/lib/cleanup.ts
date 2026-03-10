import { destroyAllWorktrees } from './worktree-manager.js';
import type { Instance } from './types.js';
import { destroyInstance } from './instance-manager.js';

let cleanupRegistered = false;
let cleanupRan = false;
let instances: Instance[] = [];

/**
 * Set the active instances for cleanup.
 * Call this whenever instances are created or changed.
 */
export function setCleanupInstances(inst: Instance[]): void {
  instances = inst;
}

/**
 * Run cleanup: kill all PTY processes, then remove all worktrees.
 * Safe to call multiple times — only runs once.
 */
async function runCleanup(): Promise<void> {
  if (cleanupRan) return;
  cleanupRan = true;

  // Kill all PTY processes first
  await Promise.allSettled(instances.map((inst) => destroyInstance(inst)));

  // Remove worktrees and branches
  try {
    await destroyAllWorktrees();
  } catch {
    // Best effort — don't crash during shutdown
  }
}

/**
 * Register signal handlers for graceful shutdown.
 * Safe to call multiple times — only registers once.
 */
export function registerCleanupHandlers(): void {
  if (cleanupRegistered) return;
  cleanupRegistered = true;

  const handleSignal = (signal: string) => {
    void runCleanup().finally(() => {
      process.exit(signal === 'SIGINT' ? 130 : 143);
    });
  };

  process.on('SIGINT', () => handleSignal('SIGINT'));
  process.on('SIGTERM', () => handleSignal('SIGTERM'));
  process.on('SIGHUP', () => handleSignal('SIGHUP'));

  process.on('beforeExit', () => {
    void runCleanup();
  });
}

export { runCleanup };
