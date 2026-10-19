todo
综合运用flag，结构体，文件读写，实现一个可持续化的代办管理 
CLi：支持 add list done 命令
command：
./todo add "写周报"        # 新增待办
./todo add "buy milk"     # 新增第二条（ID 自动为 2）
./todo list               # 列出全部，已完成的带 √
./todo done 1             # 把 ID=1 标记为完成

bookapi
用标准库实现一个内存版的图书管理系统 Api:GET /books POST /books GET /books/{id}
command：
go run ./bookapi              # 启动服务，监听 :8080
# 另开终端测试：
curl -X POST -d '{"title":"Go","author":"张三"}' http://localhost:8080/books
curl http://localhost:8080/books
curl http://localhost:8080/books/1


download
用标准库实现一个并发下载工具，支持从多个URL下载文件
command：
go run ./downloader