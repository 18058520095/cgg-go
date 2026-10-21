package webui

import _ "embed"

// IndexHTML 内嵌的前端单页（无需额外静态文件，随二进制分发）
//
//go:embed index.html
var IndexHTML []byte
