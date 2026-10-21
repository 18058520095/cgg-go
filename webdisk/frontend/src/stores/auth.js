import { defineStore } from 'pinia'

const USERNAME_KEY = 'cloudisk_username'
const TOKEN_KEY = 'cloudisk_token'

// 登录态集中管理，并持久化到 localStorage，刷新页面不掉线
export const useAuthStore = defineStore('auth', {
  state: () => ({
    username: localStorage.getItem(USERNAME_KEY) || '',
    token: localStorage.getItem(TOKEN_KEY) || ''
  }),
  getters: {
    isLoggedIn: (state) => !!(state.username && state.token)
  },
  actions: {
    login(username, token) {
      this.username = username
      this.token = token
      localStorage.setItem(USERNAME_KEY, username)
      localStorage.setItem(TOKEN_KEY, token)
    },
    logout() {
      this.username = ''
      this.token = ''
      localStorage.removeItem(USERNAME_KEY)
      localStorage.removeItem(TOKEN_KEY)
    }
  }
})
