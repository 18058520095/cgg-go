<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import api from '../api'
import { useAuthStore } from '../stores/auth'

const router = useRouter()
const auth = useAuthStore()

const selected = ref(null)
const message = ref('')
const error = ref('')
const loading = ref(false)

function onPick(e) {
  selected.value = e.target.files[0] || null
  message.value = ''
  error.value = ''
}

async function onUpload() {
  if (!selected.value) {
    error.value = '请先选择文件'
    return
  }
  loading.value = true
  message.value = ''
  error.value = ''
  try {
    const form = new FormData()
    form.append('file', selected.value)
    const res = await api.post('/file/upload', form, {
      params: { username: auth.username, token: auth.token }
    })
    if (res.data === 'upload Success') {
      message.value = '上传成功'
      selected.value = null
    } else {
      error.value = typeof res.data === 'string' ? res.data : '上传失败'
    }
  } catch (e) {
    error.value = '上传失败：' + e.message
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <div class="page">
    <div class="card">
      <h1>上传文件</h1>
      <form @submit.prevent="onUpload">
        <input type="file" @change="onPick" />
        <button type="submit" :disabled="loading">
          {{ loading ? '上传中…' : '开始上传' }}
        </button>
      </form>
      <p v-if="message" class="success">{{ message }}</p>
      <p v-if="error" class="error">{{ error }}</p>
      <p class="tip"><router-link to="/home">返回文件列表</router-link></p>
    </div>
  </div>
</template>
