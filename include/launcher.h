#ifndef __DEF_LAUNCHER_H__
#define __DEF_LAUNCHER_H__

/*!
 * \def BOOTINFO_ADDR
 * \brief The boot informations address
 */
#define BOOTINFO_ADDR	0xffffc000

/*!
 * \def STACK_TOP_ADDR
 * \brief The stack top address
 */
#define STACK_TOP_ADDR 	0xffffe000

/*!
 * \def VIDT_ADDR
 * \define The VIDT address
 */
#define VIDT_ADDR	0xfffff000

/*!
 * \def PAGE_SIZE
 * \brief The page size
 */
#define PAGE_SIZE	4096

/*!
 * \def VIDT
 * \brief The VIDT of the current partition
 */
#define VIDT		((user_ctx_t **) VIDT_ADDR)

/*!
 * \def PANIC()
 * \brief The macro used for unexpected behavior
 */
#define PANIC()				\
	do {				\
		printf("Panic!\n");	\
		for (;;);		\
	} while (0)

/*!
 * \struct pushad_regs_t
 * \brief Registers structure for x86
 */
typedef struct pushad_regs_s
{
	uint32_t edi;       //!< General register EDI
	uint32_t esi;       //!< General register ESI
	uint32_t ebp;       //!< Base pointer
	uint32_t esp;       //!< Stack pointer
	uint32_t ebx;       //!< General register EBX
	uint32_t edx;       //!< General register EDX
	uint32_t ecx;       //!< General register ECX
	uint32_t eax;       //!< General register EAX
} pushad_regs_t;

/*!
 * \struct user_ctx_t
 * \brief User saved context
 */
typedef struct user_ctx_s
{
	uint32_t eip;       //!< Extended instruction pointer
	uint32_t pipflags;  //!< Flags used by PIP
	uint32_t eflags;    //!< Status register
	pushad_regs_t regs; //!< General-purpose registers
	uint32_t valid;     //!< Structure validity: 1 valid, 0 invalid
	uint32_t nfu[4];    //!< Unused
} user_ctx_t;

#endif /* __DEF_LAUNCHER_H__ */
