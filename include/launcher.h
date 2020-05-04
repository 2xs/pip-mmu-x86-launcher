/*******************************************************************************/
/*  © Université Lille 1, The Pip Development Team (2015-2020)                 */
/*                                                                             */
/*  This software is a computer program whose purpose is to run a minimal,     */
/*  hypervisor relying on proven properties such as memory isolation.          */
/*                                                                             */
/*  This software is governed by the CeCILL license under French law and       */
/*  abiding by the rules of distribution of free software.  You can  use,      */
/*  modify and/ or redistribute the software under the terms of the CeCILL     */
/*  license as circulated by CEA, CNRS and INRIA at the following URL          */
/*  "http://www.cecill.info".                                                  */
/*                                                                             */
/*  As a counterpart to the access to the source code and  rights to copy,     */
/*  modify and redistribute granted by the license, users are provided only    */
/*  with a limited warranty  and the software's author,  the holder of the     */
/*  economic rights,  and the successive licensors  have only  limited         */
/*  liability.                                                                 */
/*                                                                             */
/*  In this respect, the user's attention is drawn to the risks associated     */
/*  with loading,  using,  modifying and/or developing or reproducing the      */
/*  software by the user in light of its specific status of free software,     */
/*  that may mean  that it is complicated to manipulate,  and  that  also      */
/*  therefore means  that it is reserved for developers  and  experienced      */
/*  professionals having in-depth computer knowledge. Users are therefore      */
/*  encouraged to load and test the software's suitability as regards their    */
/*  requirements in conditions enabling the security of their systems and/or   */
/*  data to be ensured and,  more generally, to use and operate it in the      */
/*  same conditions as regards security.                                       */
/*                                                                             */
/*  The fact that you are presently reading this means that you have had       */
/*  knowledge of the CeCILL license and that you accept its terms.             */
/*******************************************************************************/

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
 * \brief The VIDT address
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
