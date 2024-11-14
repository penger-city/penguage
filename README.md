# Penguage

The official language of Penger.

## Building from source

Penguage is written in the Hare programming language but is built with a `nob.c`.
This allows the project to bootstrap its own Hare development environment first.

```
$ cc -o nob nob.c
$ ./nob
$ ./pengc
```

NOTE: For the time being, you may need to run `./nob` twice when bootstrapping the Hare development environment.
The final step in its build process requires `scdoc`, which should be optional but will cause the build to fail if you don't have it at the ready.
When `./nob` reports this failure, simply run it again.
