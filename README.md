# mock-shell
 - C Program that supports IO redirection, piping, command substitution and process management mimicing a terminal/shell (currently only for use on Linux systems/WSL due to libraries)
- Build all files using `make`
- Run the program using `./monitor myshell` (monitor prevents fork from being called [indefinitely](https://en.wikipedia.org/wiki/Fork_bomb) which may result in resource starvation.)
