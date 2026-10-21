<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import api from '../api'
import { useAuthStore } from '../stores/auth'

const router = useRouter()
const auth = useAuthStore()

const username = ref('')
const password = ref('')
const error = ref('')
const loading = ref(false)

async function onSubmit() {
  error.value = ''
  if (!username.value || !password.value) {
    error.value = '请输入用户名和密码'
    return
  }
  loading.value = true
  try {
    const body = new URLSearchParams()
    body.append('username', username.value)
    body.append('password', password.value)
    const res = await api.post('/user/signin', body)
    const data = res.data
    // 登录成功返回 JSON { data: { Token, Username } }，失败返回纯文本
    if (data && data.data && data.data.Token) {
      auth.login(data.data.Username, data.data.Token)
      router.push('/home')
    } else {
      error.value = typeof data === 'string' ? data : 'Login Failed'
    }
  } catch (e) {
    error.value = '请求失败：' + e.message
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <div class="page">
    <div class="card">
      <h1>Cloudisk 登录</h1>
      <form @submit.prevent="onSubmit">
        <label>用户名</label>
        <input v-model="username" type="text" placeholder="请输入用户名" autocomplete="username" />

        <label>密码</label>
        <input
          v-model="password"
          type="password"
          placeholder="请输入密码"
          autocomplete="current-password"
        />

        <button type="submit" :disabled="loading">
          {{ loading ? '登录中…' : '登录' }}
        </button>
      </form>
      <p v-if="error" class="error">{{ error }}</p>
      <p class="tip">
        还没有账号？<router-link to="/register">立即注册</router-link>
      </p>
    </div>
  </div>
</template>
