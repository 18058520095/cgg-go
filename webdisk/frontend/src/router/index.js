import { createRouter, createWebHistory } from 'vue-router'
import { useAuthStore } from '../stores/auth'
import Login from '../views/Login.vue'
import Register from '../views/Register.vue'
import Home from '../views/Home.vue'
import Upload from '../views/Upload.vue'

const routes = [
  { path: '/', redirect: '/login' },
  {
    path: '/login',
    name: 'login',
    component: Login,
    // alias 兼容课程文档中的旧页面路径
    alias: ['/user/signin', '/user/sigin', '/static/view/signin.html']
  },
  {
    path: '/register',
    name: 'register',
    component: Register,
    alias: ['/user/signup', '/static/view/signup.html']
  },
  {
    path: '/home',
    name: 'home',
    component: Home,
    alias: ['/static/view/home.html']
  },
  {
    path: '/upload',
    name: 'upload',
    component: Upload,
    alias: ['/file/upload', '/static/view/index.html']
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

const publicPages = ['/login', '/register']

// 未登录访问业务页面 -> 跳登录；已登录访问登录/注册 -> 跳主页
router.beforeEach((to) => {
  const auth = useAuthStore()
  if (!publicPages.includes(to.path) && !auth.isLoggedIn) {
    return { path: '/login' }
  }
  if (publicPages.includes(to.path) && auth.isLoggedIn) {
    return { path: '/home' }
  }
  return true
})

export default router
