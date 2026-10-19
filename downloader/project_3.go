package main

import (
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"sync"
)

func download(url string, wg *sync.WaitGroup) {
	defer wg.Done()
	resp, err := http.Get(url)
	if err != nil {
		fmt.Println("Error downloading:", url, err)
		return
	}
	defer resp.Body.Close()
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		fmt.Println("Error reading body:", err)
		return
	}
	name := filepath.Base(url)
	if err := os.WriteFile(name, body, 0644); err != nil {
		fmt.Println("写入文件失败:", name, err)
		return
	}
	fmt.Printf("已下载 %s (%d 字节)\n", name, len(body))
}

func main() {
	urls := []string{
		"https://go.dev/",
		"https://golang.google.cn/doc",
		"https://pkg.go.dev/",
	}
	var wg sync.WaitGroup
	for _, url := range urls {
		wg.Add(1)
		go download(url, &wg)
	}
	wg.Wait()
	fmt.Println("所有下载完成")
}
