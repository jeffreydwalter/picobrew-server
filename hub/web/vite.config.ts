import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/api': { target: process.env.HUB_API_URL || 'http://127.0.0.1:8080', changeOrigin: false },
      '/openapi.json': { target: process.env.HUB_API_URL || 'http://127.0.0.1:8080' },
    },
  },
  build: { outDir: 'dist', sourcemap: false },
})
