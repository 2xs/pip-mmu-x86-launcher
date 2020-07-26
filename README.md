# Launcher partition

The purpose of this project is to illustrate, using a simple example, how to
transfer the execution flow from the root partition to a child partition.

## Project structure

The project structure is the following:

```
.
├── 0boot.S
├── doc
├── Doxyfile
├── include
│   └── launcher.h
├── link.ld
├── main.c
├── Makefile
├── minimal
│   ├── boot.S
│   ├── link.ld
│   ├── main.c
│   └── Makefile
├── partitions.S
└── README.md
```

The root partition code can be found at the root of the project in the `0boot.S`
and `main.c` files.

The child partition code can be found in the `boot.S` and `main.c` files in the
`minimal` directory.

## Documentation

You can generate the project documentation with the following command:

```console
$ make doc
```

You'll find it in the `doc` directory.

## Useful commands for debugging

### Disassembling a flat binary file

You can use the following command in order to disassemble a flat binary file :

```console
$ objdump -b binary -m i386 --adjust-vma=0x700000 -D binary.bin | less
```

### Step into a flat binary file with GDB

You can step into a flat binary file with GDB using the following command:

```
(gdb) stepi
or
(gdb) nexti
```
