import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import viteCompression from 'vite-plugin-compression';
import { execSync } from 'node:child_process';
import fs from 'fs';
import path from 'path';

export default defineConfig({
  plugins: [
    react(),
    viteCompression({
      algorithm: 'gzip',
      ext: '.gz',
      filter: /\.(html|js|css|json)$/i,
      deleteOriginFile: false
    }),
    {
      name: 'copy-to-spiffs-data',
      closeBundle() {
        // skip if build failed
        if (!fs.existsSync('dist') || fs.readdirSync('dist').length === 0) {
          console.warn('No dist output found – skipping copy.');
          return;
        }

        // Ensure data directory exists
        const dataDir = path.resolve(__dirname, '../data');
        if (!fs.existsSync(dataDir)) {
          fs.mkdirSync(dataDir, { recursive: true });
        }

        // remove old files
        execSync(`rm -rf ${dataDir}/*`);

        // copy new hashed files & gz
        try {
          execSync(`cp dist/*.* dist/*.gz ${dataDir}/`);
          console.log(`✓ Build artifacts copied to ${dataDir}/`);
        }
        catch (error) {
          console.warn('Copy failed:', error);
        }
      }
    }
  ],
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
    },
  },
  build: {
    outDir: 'dist',
    assetsDir: '',
    rollupOptions: {
      output: {
        entryFileNames: '[name]-[hash].js',
        chunkFileNames: '[name]-[hash].js',
        assetFileNames: '[name]-[hash].[ext]'
      }
    }
  },
  server: {
    host: true,
    port: 5173,
    // Proxy WebSocket connections to the controller during development
    proxy: {
      '/ws': {
        target: 'ws://lilygo-motioncontroller.local',
        ws: true,
        changeOrigin: true
      },
      '/debug': {
        target: 'ws://lilygo-motioncontroller.local',
        ws: true,
        changeOrigin: true
      },
      '/update': {
        target: 'http://lilygo-motioncontroller.local',
        changeOrigin: true
      }
    }
  }
});