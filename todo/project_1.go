package main

import (
	"encoding/json"
	"fmt"
	"os"
	"strconv"
	"time"
)

type Todo struct {
	ID        int       `json:"id"`
	Text      string    `json:"text"`
	Done      bool      `json:"done"`
	CreatedAt time.Time `json:"created_at"`
}

const dataFile = "todos.json"

func load() []Todo {
	data, err := os.ReadFile(dataFile)
	if err != nil {
		fmt.Println("Error reading file:", err)
		return nil
	}
	var todos []Todo
	json.Unmarshal(data, &todos)
	return todos
}

func save(todos []Todo) {
	data, _ := json.MarshalIndent(todos, "", " ")
	os.WriteFile(dataFile, data, 0644)
}

func add(text string) {
	todos := load()
	id := 1
	if len(todos) > 0 {
		id = todos[len(todos)-1].ID + 1
	}
	todos = append(todos, Todo{ID: id, Text: text, CreatedAt: time.Now()})
	save(todos)
	fmt.Printf("已添加 todo %d: %s\n", id, text)
}

func list() {
	todos := load()
	for _, t := range todos {
		mark := ""
		if t.Done {
			mark = "√"
		}
		fmt.Printf("%s#%d %s\n", mark, t.ID, t.Text)
	}
}

func done(id int) {
	todos := load()
	for i := range todos {
		if todos[i].ID == id {
			todos[i].Done = true
			save(todos)
			fmt.Printf("已标记 todo %d 为已完成\n", id)
			return
		}
	}
	fmt.Printf("未找到 todo %d\n", id)
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: todo add <text> | list | done <id>")
		return
	}
	switch os.Args[1] {
	case "add":
		if len(os.Args) < 3 {
			fmt.Println("Usage: todo add <text>")
			return
		}
		add(os.Args[2])
	case "list":
		list()
	case "done":
		if len(os.Args) < 3 {
			fmt.Println("Usage: todo done <id>")
			return
		}
		id, err := strconv.Atoi(os.Args[2])
		if err != nil {
			fmt.Println("id 必须是数字:", os.Args[2])
			return
		}
		done(id)
	default:
		fmt.Println("未知命令:", os.Args[1])
		fmt.Println("Usage: todo add <text> | list | done <id>")
	}
}
