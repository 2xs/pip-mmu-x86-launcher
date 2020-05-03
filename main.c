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

#include <stdint.h>

#include <pip/stdio.h>
#include <pip/fpinfo.h>
#include <pip/paging.h>
#include <pip/api.h>

#include "launcher.h"

/*
 * The minimal partition descriptor.
 */
uint32_t pd;

/*
 * External symbols from link.ld file.
 */
extern void *_minimal_addr_start;
extern void *_minimal_addr_end;

/*!
 * \brief Print boot informations.
 * \param bootinfo A pointer to a pip_fpinfo structure.
 */
static void printBootInfo(pip_fpinfo* bootinfo)
{
	printf("Magic number... 0x%x\n", bootinfo->magic);
	printf("Memory start... 0x%x\n", bootinfo->membegin);
	printf("Memory end... 0x%x\n", bootinfo->memend);
	printf("Pip revision... %s\n",   bootinfo->revision);
	printf("Child address start... 0x%x\n", (uint32_t) &_minimal_addr_start);
	printf("Child address end... 0x%x\n", (uint32_t) &_minimal_addr_end);
}

/*!
 * \fn static void bootstrapPartition(uint32_t base, uint32_t size, uint32_t laddr)
 * \brief Bootstraping a new partition.
 * \param base The start address of the partition.
 * \param size The size of the partition.
 * \param laddr The address where to load the partition.
 */
static uint32_t bootstrapPartition(uint32_t base, uint32_t size, uint32_t laddr)
{
	uint32_t rtospd, rtossh1, rtossh2, rtossh3, stackPage, offset;
	user_ctx_t *context, **vidtPage;
	void *stack;

	printf("Creating a new partition... ");
	pd      = (uint32_t) Pip_AllocPage();
	rtospd  = (uint32_t) Pip_AllocPage();
	rtossh1 = (uint32_t) Pip_AllocPage();
	rtossh2 = (uint32_t) Pip_AllocPage();
	rtossh3 = (uint32_t) Pip_AllocPage();

	if (!Pip_CreatePartition(pd, rtospd, rtossh1, rtossh2, rtossh3))
	{
		printf("Failed.\n");
		return 0;
	}
	printf("Done.\n");

	printf("Mapping each partition page... ");
	for (offset = 0; offset < size; offset += PAGE_SIZE)
	{
		if (!Pip_MapPageWrapper(base + offset, pd, laddr + offset))
		{
			printf("Failed.\n");
			return 0;
		}
	}
	printf("%d page(s) mapped.\n", offset / PAGE_SIZE);

	printf("Allocating a new page for the stack... ");
	stackPage = (uint32_t) Pip_AllocPage();
	printf("Done.\n");

	printf("Creating a new context for the partition... ");
	stack = context   = (user_ctx_t *) (stackPage + PAGE_SIZE - sizeof(user_ctx_t));
	context->valid    = 0;
	context->eip      = laddr;
	context->pipflags = 0;
	context->eflags   = 0x202;
	context->regs.ebp = STACK_TOP_ADDR + PAGE_SIZE;
	context->regs.esp = context->regs.ebp - sizeof(user_ctx_t);
	context->valid    = 1;
	printf("Done.\n");

	printf("Mapping the stack page... ");
	if (!Pip_MapPageWrapper(stackPage, pd, STACK_TOP_ADDR))
	{
		printf("Failed.\n");
		return 0;
	}
	printf("Done.\n");

	printf("Allocating a new page for the VIDT... ");
	vidtPage = (user_ctx_t **) Pip_AllocPage();
	printf("Done.\n");

	printf("Saving the context in the VIDT... ");
	vidtPage[0] = (user_ctx_t *) context->regs.esp;
	printf("Done.\n");

	printf("Mapping the VIDT page... ");
	if (!Pip_MapPageWrapper((uint32_t) vidtPage, pd, VIDT_ADDR))
	{
		printf("Failed.\n");
		return 0;
	}
	printf("Done.\n");

	return 1;
}

/*!
 * \brief The entry point of the root partition.
 * \param bootinfo The boot information address from the %ebx register.
 */
void main(pip_fpinfo* bootinfo)
{
	uint32_t minimal_start, minimal_end;
	user_ctx_t *setup_context;

	printf("The root partition is booting...\n");

	setup_context = (user_ctx_t *) (STACK_TOP_ADDR - sizeof(user_ctx_t));
	VIDT[32] = setup_context;
	VIDT[48] = setup_context;
	VIDT[49] = setup_context;

	minimal_start = (uint32_t) &_minimal_addr_start;
	minimal_end   = (uint32_t) &_minimal_addr_end;

	printf("Checking the boot information integrity... ");
	if (bootinfo->magic != FPINFO_MAGIC)
	{
		printf("Failed.\n");
		PANIC();
	}
	printf("Done.\n");

	printBootInfo(bootinfo);

	printf("Initializing the memory pages... ");
	if (!Pip_InitPaging((void *) bootinfo->membegin, (void *) bootinfo->memend))
	{
		printf("Failed.\n");
		PANIC();
	}
	printf("Done.\n");

	printf("Bootstraping the minimal partition...\n");
	if (!bootstrapPartition(minimal_start, minimal_end - minimal_start, 0x700000))
	{
		PANIC();
	}
	printf("Partition successfully bootstraped...\n");

	printf("It's all good. Now switching to the minimal partition...\n");
	for (;;)
	{
		Pip_Yield(pd, 0, 49, 0, 0);
	}

	// Should never be reached.

	PANIC();
}
