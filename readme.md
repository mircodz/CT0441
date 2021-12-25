# XTetris

**Disclamer**: The code is under constant change. Most of it should be re-written in a more elegant and safe way.

## TODO's
See `main.c` and other source/header files for features yet to be implemented.

## Client

### Building

```
$ git clone http://github.com/mircodezorzi/CT0441
$ git submodule update --init --remote --merge
$ cd include/libdz
$ make
$ cd $OLDPWD
$ make
```

### Running
```
$ ./build/{debug,release}/client
```

## Server

### Building

```
$ go build
```

### Running

```
$ ./server -port 5000
```
