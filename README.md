# Launcher partition

The purpose of this project is to illustrate, using a simple example, how to
transfer the execution flow from the root partition to a child partition.

## Project structure

```
.
├── 0boot.S
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

## Useful commands for debugging

### Disassembling a flat binary

You can use the following command in order to disassemble a flat binary file :

```console
$ objdump -b binary -m i386 --adjust-vma=0x700000 -D binary.bin | less
```

### GDB

You can stepped into a flat binary file using the following command:

```
(gdb) stepi
or
(gdb) nexti
```
