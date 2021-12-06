package main

import (
	"bufio"
	"fmt"
	"log"
	"math/rand"
	"net"
	"strings"
	"time"
)

type connState int

const (
	NotAuthenticated connState = iota
	Authenticated    connState = iota
	Waiting          connState = iota
	Playing          connState = iota
)

var stateToString = map[connState]string{
	NotAuthenticated: "NotAuthenticated",
	Authenticated: "Authenticated",
	Waiting:       "Waiting",
	Playing:       "Playing",
}

type userConn struct {
	conn    net.Conn
	scanner *bufio.Scanner
	state   connState
	user    string
	field   []int
	room    string
}

func (c *userConn) Write(s ...string) (int, error) {
	return c.conn.Write([]byte(strings.Join(s, " ")))
}

func (c *userConn) Writeln(s ...string) (int, error) {
	return c.conn.Write([]byte(strings.Join(s, " ") + "\n"))
}

var (
	serverHello                = []byte("100 xtetris server\r\n")
	serverBye                  = []byte("221 Bye\r\n")
	serverOK                   = []byte("250 OK\r\n")
	serverCommandNotRecognized = []byte("500 NOT OK\r\n")
	serverBadArguments         = []byte("501 NOT OK\r\n")
	serverInvalidState         = []byte("502 NOT OK\r\n")
	serverAuthSuccessful       = []byte("235 Authentication successful\r\n")
	serverAuthFailed           = []byte("535 Authentication failed\r\n")
)

type room struct {
	A *userConn
	B *userConn
}

var (
	rooms = make(map[string]*room, 0)
	users = make(map[string]*userConn, 0)
)

func memset(a []int, v int) {
	for i := range a {
		a[i] = v
	}
}

func handleAUTH(c *userConn, args []string) (err error) {
	if _, ok := users[args[0]]; ok {
		c.conn.Write(serverAuthFailed)
		return nil
	}

	c.state = Authenticated
	c.user = args[0]
	users[c.user] = c

	c.conn.Write(serverAuthSuccessful)
	return nil
}

func handleJOIN(c *userConn, args []string) (err error) {
	r, ok := rooms[args[0]]

	if !ok {
		r = &room{}
		rooms[args[0]] = r
	}

	if r.A == nil {
		r.A = c
	} else if r.B == nil {
		r.B = c
	} else {
		r.A.state = Playing
		r.B.state = Playing
	}

	c.state = Waiting
	c.room = args[0]
	return nil
}

// List all players and their state.
func handleLISTP(c *userConn, args []string) (err error) {
	for _, user := range users {
		if user.state != NotAuthenticated {
			c.Write(user.user, stateToString[user.state])
			if user.room != "" {
				c.Writeln("", user.room)
			} else {
				c.Writeln("", "<none>")
			}
		}
	}

	return nil
}

// List all games and their state
func handleLISTG(c *userConn, args []string) (err error) {
	for name, r := range rooms {
		c.Write(name)
		if r.A != nil {
			c.Write("", r.A.user)
		} else {
			c.Write("", "<none>")
		}
		if r.B != nil {
			c.Writeln("", r.B.user)
		} else {
			c.Writeln("", "<none>")
		}
	}

	return nil
}

type handlerFunc = func(c *userConn, args []string) (err error)

var (
	cmdAuth  = "AUTH"
	cmdJoin  = "JOIN"
	cmdListP = "LISTP"
	cmdListG = "LISTG"
)

type cmdRequirements struct {
	argc   int
	states []connState
}

var allowedCmd = map[string]cmdRequirements{
	cmdAuth:  {argc: 1, states: []connState{NotAuthenticated}},
	cmdJoin:  {argc: 1, states: []connState{Authenticated}},
	cmdListP: {argc: 0, states: []connState{Authenticated, Waiting, Playing}},
	cmdListG: {argc: 0, states: []connState{Authenticated, Waiting, Playing}},
}

var handlers = map[string]handlerFunc{
	cmdAuth:  handleAUTH,
	cmdJoin:  handleJOIN,
	cmdListP: handleLISTP,
	cmdListG: handleLISTG,
}

func handleConnection(conn net.Conn) {
	defer conn.Close()
	_, err := conn.Write(serverHello)
	if err != nil {
		log.Println(err)
		return
	}

	c := userConn{
		conn:  conn,
		state: NotAuthenticated,
		field: make([]int, 150),
	}

	scanner := bufio.NewScanner(conn)
	c.scanner = scanner

LOOP:
	for scanner.Scan() {
		if scanner.Err() != nil {
			log.Println(scanner.Err())
			return
		}
		line := scanner.Text()

		parts := strings.Fields(string(line))
		command := strings.ToUpper(parts[0])
		args := parts[1:]

		requirements, ok := allowedCmd[command]
		if !ok {
			c.conn.Write(serverCommandNotRecognized)
			continue LOOP
		}

		allowed := requirements.argc == len(args)
		if !allowed {
			c.conn.Write(serverBadArguments)
			continue LOOP
		}

		allowed = false
		for _, state := range requirements.states {
			if c.state == state {
				allowed = true
				break
			}
		}
		if !allowed {
			c.conn.Write(serverInvalidState)
			continue LOOP
		}

		handler, ok := handlers[command]
		if !ok {
			c.conn.Write(serverCommandNotRecognized)
			continue LOOP
		}

		if err := handler(&c, args); err != nil {
			log.Printf("[handle%v] %v\n", command, err)
			return
		}
	}
}

func main() {
	listener, err := net.Listen("tcp4", "127.0.0.1:3000")
	if err != nil {
		fmt.Println(err)
		return
	}
	defer listener.Close()

	rand.Seed(time.Now().Unix())

	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println(err)
			return
		}
		go handleConnection(conn)
	}
}
