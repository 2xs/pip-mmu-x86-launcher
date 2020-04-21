#ifndef __DEF_LAUNCHER_H__
#define __DEF_LAUNCHER_H__

#define INITIAL_STACK_TOP 	0xffffe000
#define BOOTINFO_ADDR		0xffffc000
#define VIDT_ADDR		0xfffff000
#define PAGE_SIZE		4096
#define NULL			((void *) 0)

#define VIDT	((user_ctx_t **) VIDT_ADDR)

#define PANIC() 			\
	do				\
	{				\
		printf("PANIC\n");	\
		for (;;);		\
	} while (0)

typedef struct pushad_regs_s
{
	uint32_t edi; //!< General register EDI
	uint32_t esi; //!< General register ESI
	uint32_t ebp; //!< EBP
	uint32_t esp; //!< Stack pointer
	uint32_t ebx; //!< General register EBX
	uint32_t edx; //!< General register EDX
	uint32_t ecx; //!< General register ECX
	uint32_t eax; //!< General register EAX
} pushad_regs_t;

typedef struct user_ctx_s
{
	uint32_t eip;
	uint32_t pipflags;
	uint32_t eflags;
	pushad_regs_t regs;

	uint32_t valid;
	uint32_t nfu[4];
} user_ctx_t;

#endif /* __DEF_LAUNCHER_H__ */
