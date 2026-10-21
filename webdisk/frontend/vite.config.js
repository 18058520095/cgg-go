import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// 开发态用 vite dev server，把 /user /file 请求代理到 C++ 后端(1234)
// 生产态执行 vite build 产出 dist/，由 wfrest 直接托管
export default defineConfig({
  plugins: [vue()],
  base: '/',
  build: {
    outDir: 'dist',
    emptyOutDir: true
  },
  server: {
    port: 5173,
    proxy: {
      '/user': 'http://127.0.0.1:1234',
      '/file': 'http://127.0.0.1:1234',
      '/static': 'http://127.0.0.1:1234'
    }
  }
})
