import axios from 'axios'

// baseURL 留空：开发态走 vite proxy，生产态与后端同源
const api = axios.create({
  baseURL: '',
  timeout: 20000
})

export default api
