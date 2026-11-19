import { defineConfig } from 'vitest/config'
import react from '@vitejs/plugin-react'
export default defineConfig({
  plugins: [react()],
  test: {
    include: ['tests/dom.integration.tsx'],
    environment: 'jsdom',
    environmentOptions: { jsdom: { url: 'http://127.0.0.1:18081/' } },
    testTimeout: 20000,
    hookTimeout: 20000,
    fileParallelism: false,
  },
})
