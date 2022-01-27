# XTetris

- https://github.com/mircodezorzi/libdz
- https://github.com/mircodezorzi/CT0441

Team members: Mirco De Zorzi (891275)

Disclamer: The code is under constant change. Most of it should be re-written in a more elegant and safe way.
           The project also contains a lot of commented code, that was abandoned mid-development.
	   Because of reasons I won't go into, work on this library has been halted for almost 2 months now.
	   Read `/report/main.pdf` for more information about the state of the project.

## Project Structure

+----------+----------------------------------+
| /build   | Output directory.                |
| /docs    | Doxygen generated documentation. |
| /report  | Official project report.         |
| /server  | Server source code (Golang)      |
+----------+----------------------------------+
| /src     |                                  |
| /include |                                  |
+----------+----------------------------------+

+---------------------------+-----------------------------------+
| /include/libdz            | Root directory.                   |
| /include/libdz/build      | Output directory.                 |
| /include/libdz/docs       | Doxygen generated documentation.  |
+---------------------------+-----------------------------------+
| /include/libdz/src        |                                   |
| /include/libdz/include/dz |                                   |
+---------------------------+-----------------------------------+

# Building (release version)

    $ git clone http://github.com/mircodezorzi/CT0441
    $ git submodule update --init --remote --merge
    $ cd include/libdz
    $ make DEBUG=0
    $ cd $OLDPWD
    $ make DEBUG=0

# Debugging

    $ make # by default, compile debug build
    $ make run
    ./build/debug/client > stderr

# Building (and running) the server

    $ go get
    $ go build
    $ ./server -port 5000

$ Running
                         ip        port name room
    ./build/debug/client 127.0.0.1 5000 foo  b
    ./build/debug/client 127.0.0.1 5000 bar  b
