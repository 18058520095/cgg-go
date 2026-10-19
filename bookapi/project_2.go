package main

import (
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"sync"
)

type Book struct {
	ID     int    `json:"id"`
	Title  string `json:"title"`
	Author string `json:"author"`
}

type BookStore struct {
	mu    sync.RWMutex
	books map[int]Book
	next  int
}

func (s *BookStore) list(w http.ResponseWriter, r *http.Request) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	books := make([]Book, 0, len(s.books))
	for _, b := range s.books {
		books = append(books, b)
	}
	json.NewEncoder(w).Encode(books)
}

func (s *BookStore) create(w http.ResponseWriter, r *http.Request) {
	var b Book
	if err := json.NewDecoder(r.Body).Decode(&b); err != nil {
		fmt.Println("Error decoding JSON:", err)
		return
	}
	s.mu.Lock()
	s.next++
	b.ID = s.next
	s.books[b.ID] = b
	s.mu.Unlock()
	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(b)
}

func (s *BookStore) get(w http.ResponseWriter, r *http.Request) {
	id, _ := strconv.Atoi(r.PathValue("id"))
	s.mu.RLock()
	b, ok := s.books[id]
	s.mu.RUnlock()
	if !ok {
		w.WriteHeader(http.StatusNotFound)
		return
	}
	json.NewEncoder(w).Encode(b)
}

func main() {

	store := &BookStore{books: make(map[int]Book), next: 0}
	mux := http.NewServeMux()
	mux.HandleFunc("GET /books", store.list)
	mux.HandleFunc("POST /books", store.create)
	mux.HandleFunc("GET /books/{id}", store.get)
	http.ListenAndServe(":8080", mux)
}
