// TODO username/password authentication
// TODO more connection states
// TODO allow for the game to be paused
// TODO allow for >2 player games
// TODO allow users to exit and re-join rooms in case of a connection timeout
// TODO allow for variable sized fields
// TODO allow for custom tetraminos
// TODO implement game warmup
// TODO find mental energy to actually implement this
// TODO add logging and error handling to everything

package main

import (
	"bufio"
	"flag"
	"fmt"
	"log"
	"math/rand"
	"net"
	"strconv"
	"strings"
	"sync"
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
	Authenticated:    "Authenticated",
	Waiting:          "Waiting",
	Playing:          "Playing",
}

type userConn struct {
	conn    net.Conn
	scanner *bufio.Scanner
	state   connState

	// Tetramino held
	holding int
	// Tetramino Queue
	queue []int
	// Field (15x10 only)
	field []int

	// User stats
	score   int
	cleared int
	level   int

	// Metadata
	user string
	room string

	mu sync.Mutex
}

func (c *userConn) Write(s ...string) (int, error) {
	return c.conn.Write([]byte(strings.Join(s, " ")))
}

func (c *userConn) Writeln(s ...string) (int, error) {
	return c.conn.Write([]byte(strings.Join(s, " ") + "\r\n"))
}

var (
	serverHello = []byte("100 xtetris server\r\n")
	serverBye   = []byte("221 Bye\r\n")
	serverOK    = []byte("250 OK\r\n")

	serverUnknownError = []byte("599 Unknown Error\r\n")

	serverCommandNotRecognized = []byte("500 NOT OK\r\n")
	serverBadArguments         = []byte("501 NOT OK\r\n")
	serverInvalidState         = []byte("502 NOT OK\r\n")

	serverAuthSuccessful = []byte("235 Authentication successful\r\n")
	serverAuthFailed     = []byte("535 Authentication failed\r\n")

	serverJoinSuccessful = []byte("236 Join successful\r\n")
	serverJoinFailed     = []byte("536 Join failed\r\n")

	serverSetStateSuccessful = []byte("236 SetState successful\r\n")
	serverSetStateFailed     = []byte("536 SetState failed\r\n")
)

type room struct {
	A *userConn
	B *userConn
}

var (
	rooms = make(map[string]*room, 0)
	users = make(map[string]*userConn, 0)
)

func handleClose(c *userConn) {
	if c.room != "" {
		if rooms[c.room].A == c {
			rooms[c.room].A = nil
		} else if rooms[c.room].B == c {
			rooms[c.room].B = nil
		}
		if rooms[c.room].A == nil && rooms[c.room].B == nil {
			delete(rooms, c.room)
		}
	}
	delete(users, c.user)
}

// Authenticate player.
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

// Join room. When both slots in a room are filled. The game starts.
func handleJOIN(c *userConn, args []string) (err error) {
	r, ok := rooms[args[0]]

	if !ok {
		r = &room{}
		rooms[args[0]] = r
	}

	if r.A == nil {
		c.state = Waiting
		r.A = c
	} else if r.B == nil {
		c.state = Waiting
		r.B = c
	}

	if r.A != nil && r.B != nil {
		r.A.state = Playing
		r.B.state = Playing
	}

	c.room = args[0]

	c.conn.Write(serverJoinSuccessful)
	return nil
}

// List all players and their state.
func handleLISTP(c *userConn, args []string) (err error) {
	c.Writeln(strconv.Itoa(len(users)))
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

// List all games and their state.
func handleLISTG(c *userConn, args []string) (err error) {
	c.Writeln(strconv.Itoa(len(rooms)))
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

func readStats(state *userConn, args []string) {
	i := 0

	state.holding, _ = strconv.Atoi(args[0])
	n, _ := strconv.Atoi(args[1])

	temp_queue := make([]int, 0)
	for ; i < n; i++ {
		tmp, _ := strconv.Atoi(args[2+i])
		temp_queue = append(temp_queue, tmp)
	}
	state.queue = temp_queue

	state.score, _ = strconv.Atoi(args[2+i])
	state.cleared, _ = strconv.Atoi(args[3+i])
	state.level, _ = strconv.Atoi(args[4+i])

	for ; i < len(args)-9; i++ {
		n, _ := strconv.Atoi(args[8+i])
		state.field[i] = n
	}
}

// Set field state for player.
func handleSETSTATE(c *userConn, args []string) (err error) {
	fmt.Println(args)
	if rooms[c.room].A == c || rooms[c.room].B == c {
		readStats(c, args)
	} else {
		c.conn.Write(serverUnknownError)
	}

	c.conn.Write(serverSetStateSuccessful)
	return nil
}

func sendStats(conn net.Conn, state *userConn) (int, error) {
	//fmt.Println(state)
	return conn.Write([]byte(fmt.Sprintf(
		"%d %d %s %d %d %d %s\r\n",
		state.holding,
		len(state.queue),
		strings.Trim(fmt.Sprint(state.queue), "[]"),
		state.score,
		state.cleared,
		state.level,
		strings.Trim(fmt.Sprint(state.field), "[]"),
	)))
}

// Get field state for other player.
func handleGETSTATE(c *userConn, args []string) (err error) {
	if rooms[c.room] != nil {
		// No ternary operator :(
		if rooms[c.room].A == c {
			sendStats(c.conn, rooms[c.room].B)
		} else if rooms[c.room].B == c {
			sendStats(c.conn, rooms[c.room].A)
		} else {
			c.conn.Write(serverUnknownError)
		}
	}
	return nil
}

func handleISREADY(c *userConn, args []string) (err error) {
	if c.state == Playing {
		c.conn.Write([]byte{'1', '\r', '\n'})
	} else {
		c.conn.Write([]byte{'0', '\r', '\n'})
	}
	return nil
}

func handleGAMEOVER(c *userConn, args []string) (err error) {
	c.state = Authenticated
	for i := 0; i < 150; i++ {
		c.field[i] = 5
	}
	c.conn.Write(serverOK)
	return nil
}

type handlerFunc = func(c *userConn, args []string) (err error)

var (
	cmdAuth     = "AUTH"
	cmdJoin     = "JOIN"
	cmdListP    = "LISTP"
	cmdListG    = "LISTG"
	cmdSetState = "SETSTATE"
	cmdGetState = "GETSTATE"
	cmdIsReady  = "ISREADY"
	cmdGameOver = "GAMEOVER"
)

type cmdRequirements struct {
	argc   int
	states []connState
}

var ANY = -1

var allowedCmd = map[string]cmdRequirements{
	cmdAuth:     {argc: 1, states: []connState{NotAuthenticated}},
	cmdJoin:     {argc: 1, states: []connState{Authenticated}},
	cmdListP:    {argc: 0, states: []connState{Authenticated, Waiting, Playing}},
	cmdListG:    {argc: 0, states: []connState{Authenticated, Waiting, Playing}},
	cmdSetState: {argc: ANY, states: []connState{Playing}},
	cmdGetState: {argc: 0, states: []connState{Playing}},
	cmdIsReady:  {argc: 0, states: []connState{Authenticated, Waiting, Playing}},
	cmdGameOver: {argc: 0, states: []connState{Playing}},
}

var handlers = map[string]handlerFunc{
	cmdAuth:     handleAUTH,
	cmdJoin:     handleJOIN,
	cmdListP:    handleLISTP,
	cmdListG:    handleLISTG,
	cmdSetState: handleSETSTATE,
	cmdGetState: handleGETSTATE,
	cmdIsReady:  handleISREADY,
	cmdGameOver: handleGAMEOVER,
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
		queue: make([]int, 3),
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

		if _, ok := users[c.user]; !ok {
			handleClose(&c)
		}

		parts := strings.Fields(string(line))
		command := strings.ToUpper(parts[0])
		args := parts[1:]

		if command[0] == 0 {
			command = command[1:]
		}

		requirements, ok := allowedCmd[command]
		if !ok {
			c.conn.Write(serverCommandNotRecognized)
			continue LOOP
		}

		allowed := requirements.argc == len(args) || requirements.argc == ANY
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

		c.mu.Lock()
		if &c != nil {
			if err := handler(&c, args); err != nil {
				log.Printf("[handle%v] %v\n", command, err)
				c.mu.Unlock()
				return
			}
		}
		c.mu.Unlock()
	}
	handleClose(&c)
}

func main() {
	var port int
	flag.IntVar(&port, "port", 5000, "listen port")
	flag.Parse()

	listener, err := net.Listen("tcp4", fmt.Sprintf("127.0.0.1:%d", port))
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
