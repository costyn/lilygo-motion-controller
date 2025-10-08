import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import viteCompression from 'vite-plugin-compression';
import fs from 'fs';
import path from 'path';

export default defineConfig({
  define: {
    __BUILD_TIME__: JSON.stringify(new Date().toISOString()),
  },
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
      closeBundle: {
        order: 'post',
        async handler() {
          // Wait a moment for compression plugin to finish
          await new Promise(resolve => setTimeout(resolve, 100));

          console.log('🔄 Starting copy to SPIFFS data directory...');

          // Skip if build failed
          if (!fs.existsSync('dist') || fs.readdirSync('dist').length === 0) {
            console.warn('❌ No dist output found – skipping copy.');
            return;
          }

        // Ensure data directory exists
        const dataDir = path.resolve(__dirname, '../data');
        if (!fs.existsSync(dataDir)) {
          fs.mkdirSync(dataDir, { recursive: true });
          console.log('📁 Created data directory');
        }

        // Remove old files
        const existingFiles = fs.readdirSync(dataDir);
        for (const file of existingFiles) {
          fs.unlinkSync(path.join(dataDir, file));
        }
        console.log(`🗑️  Removed ${existingFiles.length} old files`);

        // Copy only compressed files and HTML to data directory
        try {
          const distFiles = fs.readdirSync('dist');
          let copiedCount = 0;

          for (const file of distFiles) {
            const sourcePath = path.join('dist', file);
            const destPath = path.join(dataDir, file);

            // Only copy .gz files and .html files
            if (fs.statSync(sourcePath).isFile() &&
                (file.endsWith('.gz') || file.endsWith('.html'))) {
              fs.copyFileSync(sourcePath, destPath);
              copiedCount++;
            }
          }

          console.log(`✅ Successfully copied ${copiedCount} files to ${dataDir}/`);

          // List what was copied
          const finalFiles = fs.readdirSync(dataDir);
          console.log('📋 Files in data directory:');
          finalFiles.forEach(file => {
            const stats = fs.statSync(path.join(dataDir, file));
            const sizeKB = Math.round(stats.size / 1024);
            console.log(`   📄 ${file} (${sizeKB} KB)`);
          });
        }
        catch (error) {
          console.error('❌ Copy failed:', error);
        }
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