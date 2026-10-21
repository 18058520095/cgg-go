<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import api from '../api'

const router = useRouter()

const username = ref('')
const password = ref('')
const confirm = ref('')
const error = ref('')
const loading = ref(false)

async function onSubmit() {
  error.value = ''
  if (!username.value || !password.value) {
    error.value = '请输入用户名和密码'
    return
  }
  if (password.value !== confirm.value) {
    error.value = '两次输入的密码不一致'
    return
  }
  loading.value = true
  try {
    const body = new URLSearchParams()
    body.append('username', username.value)
    body.append('password', password.value)
    const res = await api.post('/user/signup', body)
    if (res.data === 'SUCCESS') {
      router.push('/login')
    } else {
      error.value = typeof res.data === 'string' ? res.data : '注册失败'
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
      <h1>Cloudisk 注册</h1>
      <form @submit.prevent="onSubmit">
        <label>用户名</label>
        <input v-model="username" type="text" placeholder="请输入用户名" autocomplete="username" />

        <label>密码</label>
        <input
          v-model="password"
          type="password"
          placeholder="请输入密码"
          autocomplete="new-password"
        />

        <label>确认密码</label>
        <input
          v-model="confirm"
          type="password"
          placeholder="请再次输入密码"
          autocomplete="new-password"
        />

        <button type="submit" :disabled="loading">
          {{ loading ? '注册中…' : '注册' }}
        </button>
      </form>
      <p v-if="error" class="error">{{ error }}</p>
      <p class="tip">已有账号？<router-link to="/login">返回登录</router-link></p>
    </div>
  </div>
</template>
