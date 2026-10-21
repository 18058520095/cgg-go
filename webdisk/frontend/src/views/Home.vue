<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import api from '../api'
import { useAuthStore } from '../stores/auth'

const router = useRouter()
const auth = useAuthStore()

const info = ref(null)
const files = ref([])
const error = ref('')
const loading = ref(true)

// 头像仍由后端静态目录提供，用绑定写法避免 vite 在构建期解析该绝对路径
const avatar = '/static/img/avatar.jpeg'

function formatSize(bytes) {
  const n = Number(bytes)
  if (!n) return '0 B'
  if (n < 1024) return n + ' B'
  if (n < 1024 * 1024) return (n / 1024).toFixed(2) + ' KB'
  return (n / (1024 * 1024)).toFixed(2) + ' MB'
}

async function loadInfo() {
  const res = await api.get('/user/info', {
    params: { username: auth.username, token: auth.token }
  })
  if (res.data && res.data.data) {
    info.value = res.data.data
  }
}

async function loadFiles() {
  const body = new URLSearchParams()
  body.append('limit', '10')
  const res = await api.post('/file/query', body, {
    params: { username: auth.username, token: auth.token }
  })
  files.value = Array.isArray(res.data) ? res.data : []
}

async function refresh() {
  error.value = ''
  loading.value = true
  try {
    await Promise.all([loadInfo(), loadFiles()])
  } catch (e) {
    error.value = '加载失败：' + e.message
  } finally {
    loading.value = false
  }
}

// 下载：先问后端要 nginx 上的真实下载地址，再跳转过去
// （不用 window.open / target=_blank：异步请求返回后浏览器会当作弹窗拦截）
async function onDownload(name) {
  try {
    const res = await api.get('/file/downloadurl', { params: { filename: name } })
    if (typeof res.data === 'string' && res.data.startsWith('http')) {
      window.location.href = res.data
    } else {
      error.value = '获取下载地址失败'
    }
  } catch (e) {
    error.value = '下载失败：' + e.message
  }
}

function onLogout() {
  auth.logout()
  router.push('/login')
}

onMounted(refresh)
</script>

<template>
  <div class="page">
    <div class="card wide">
      <header class="topbar">
        <img class="avatar" :src="avatar" alt="avatar" />
        <div class="who">
          <div class="name">{{ info ? info.Username : auth.username }}</div>
          <div class="meta">注册时间：{{ info ? info.SignupAt : '-' }}</div>
        </div>
        <div class="actions">
          <router-link class="btn" to="/upload">上传文件</router-link>
          <button class="btn ghost" @click="onLogout">退出登录</button>
        </div>
      </header>

      <div class="section-title">
        我的文件
        <button class="btn small ghost" @click="refresh">刷新</button>
      </div>

      <p v-if="error" class="error">{{ error }}</p>
      <p v-if="loading" class="tip">加载中…</p>
      <p v-else-if="files.length === 0" class="tip">暂无文件，去上传一个吧。</p>

      <table v-else class="files">
        <thead>
          <tr>
            <th>文件名</th>
            <th>大小</th>
            <th>上传时间</th>
            <th>最近更新</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="f in files" :key="f.FileHash">
            <td>{{ f.FileName }}</td>
            <td>{{ formatSize(f.FileSize) }}</td>
            <td>{{ f.UploadAt }}</td>
            <td>{{ f.LastUpdated }}</td>
            <td>
              <button class="btn small" @click="onDownload(f.FileName)">下载</button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>
