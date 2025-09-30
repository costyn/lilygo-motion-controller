import { type ClassValue, clsx } from "clsx"
import { twMerge } from "tailwind-merge"

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs))
}

// Utility functions for the motor controller app

export const CONTROLLER_HOSTNAME = 'lilygo-motioncontroller.local';

export function getWebSocketUrl(endpoint: string): string {
  // During development, use the dev server's proxy
  // if (import.meta.env.DEV) {
  //   return `ws://localhost:5173${endpoint}`;
  // }

  // In production, try .local hostname first, fallback to current host
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';

  // Try to use .local hostname first
  if (window.location.hostname !== CONTROLLER_HOSTNAME) {
    return `${protocol}//${CONTROLLER_HOSTNAME}${endpoint}`;
  }

  return `${protocol}//${window.location.host}${endpoint}`;
}

export function formatPosition(position: number): string {
  return position.toLocaleString();
}

export function validatePosition(position: number, minLimit: number, maxLimit: number): boolean {
  return position >= minLimit && position <= maxLimit;
}

export function clampPosition(position: number, minLimit: number, maxLimit: number): number {
  return Math.max(minLimit, Math.min(maxLimit, position));
}

// Debounce utility for button presses
export function debounce<T extends (...args: any[]) => any>(
  func: T,
  wait: number
): T {
  let timeout: NodeJS.Timeout | undefined;

  return ((...args: any[]) => {
    const later = () => {
      clearTimeout(timeout);
      func(...args);
    };

    clearTimeout(timeout);
    timeout = setTimeout(later, wait);
  }) as T;
}

// Generate unique ID for presets
export function generateId(): string {
  return Date.now().toString(36) + Math.random().toString(36).substr(2);
}

// Local storage helpers
export const storage = {
  get: <T>(key: string, defaultValue: T): T => {
    try {
      const item = localStorage.getItem(key);
      return item ? JSON.parse(item) : defaultValue;
    } catch {
      return defaultValue;
    }
  },

  set: (key: string, value: any): void => {
    try {
      localStorage.setItem(key, JSON.stringify(value));
    } catch {
      // Ignore storage errors
    }
  },

  remove: (key: string): void => {
    try {
      localStorage.removeItem(key);
    } catch {
      // Ignore storage errors
    }
  }
};

// Theme helpers
export const THEME_KEY = 'lilygo-motor-theme';
export const PRESETS_KEY = 'lilygo-motor-presets';

export type Theme = 'light' | 'dark' | 'system';

export function getSystemTheme(): 'light' | 'dark' {
  return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
}

export function applyTheme(theme: Theme) {
  const root = document.documentElement;

  let effectiveTheme: 'light' | 'dark';
  if (theme === 'system') {
    effectiveTheme = getSystemTheme();
  } else {
    effectiveTheme = theme;
  }

  if (effectiveTheme === 'dark') {
    root.classList.add('dark');
  } else {
    root.classList.remove('dark');
  }
}