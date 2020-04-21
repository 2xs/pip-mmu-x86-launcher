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
 * External symbols from link.ld file.
 */

extern void *_minimal_addr_start;
extern void *_minimal_addr_end;

/*!
 * \brief Print the boot information.
 * \param bootinfo A pointer to a pip_fpinfo structure.
 */

void printBootInfo(pip_fpinfo* bootinfo)
{
	printf("\nRoot partition is booting...\n");

	printf("BOOTINFO:\n");

	if (bootinfo->magic != FPINFO_MAGIC)
	{
		printf("\tBad magic number = 0x%x...\n", bootinfo->magic);
		PANIC();
	}

	printf("\tMagic number        = 0x%x\n", bootinfo->magic);
	printf("\tMemory start        = 0x%x\n", bootinfo->membegin);
	printf("\tMemory end          = 0x%x\n", bootinfo->memend);
	printf("\tPip revision        = %s\n",   bootinfo->revision);
	printf("\tChild address start = 0x%x\n", (uint32_t) &_minimal_addr_start);
	printf("\tChild address end   = 0x%x\n", (uint32_t) &_minimal_addr_end);
}

/*!
 * \brief Print the VIDT entries containning a pointer to a context.
 */

void printVIDT(void)
{
	for (uint32_t i = 0; i < 256; i++)
	{
		if (VIDT[i] != NULL)
		{
			printf("VIDT[%d]:\n", i);
			printf("\tctxaddr  = 0x%x\n", VIDT[i]);
			printf("\tvalid    = 0x%x\n", VIDT[i]->valid);
			printf("\teip      = 0x%x\n", VIDT[i]->eip);
			printf("\tpipflags = 0x%x\n", VIDT[i]->pipflags);
			printf("\teflags   = 0x%x\n", VIDT[i]->eflags);
			printf("\tesp      = 0x%x\n", VIDT[i]->regs.esp);
			printf("\tebx      = 0x%x\n", VIDT[i]->regs.ebx);
		}
	}
}

/*!
 * \brief The entry point of the root partition.
 * \param bootinfo The boot information address from the %ebx register.
 */

void main(pip_fpinfo* bootinfo)
{
	uint32_t rtospd, rtossh1, rtossh2, rtossh3, stackPage;
	uint32_t minimal_start, minimal_end, offset, length;
	user_ctx_t **vidtPage;
	uint32_t pd;

	user_ctx_t *setup_context;
	void *stack;

	// Retrieve the setup context from the stack.
	setup_context = (user_ctx_t *) (INITIAL_STACK_TOP - sizeof(user_ctx_t));

	// Save the setup context to index 48 and 49 of the VIDT.
	VIDT[48] = setup_context;
	VIDT[49] = setup_context;

	// Retrieve the start and end address of the minimal partition.
	minimal_start = (uint32_t) &_minimal_addr_start;
	minimal_end   = (uint32_t) &_minimal_addr_end;

	// Print PIP boot information.
	printBootInfo(bootinfo);

	/*printf("");
	printf("");*/

	// Initialize the paging from the beginning to the end of the available memory.
	if (!Pip_InitPaging((void *) bootinfo->membegin, (void *) bootinfo->memend))
	{
		printf("Failed to initialize paging...\n");
		PANIC();
	}

	pd      = (uint32_t) Pip_AllocPage();
	rtospd  = (uint32_t) Pip_AllocPage();
	rtossh1 = (uint32_t) Pip_AllocPage();
	rtossh2 = (uint32_t) Pip_AllocPage();
	rtossh3 = (uint32_t) Pip_AllocPage();

	// Create a new partition for the minimal partition.
	if (!Pip_CreatePartition(pd, rtospd, rtossh1, rtossh2, rtossh3))
	{
		printf("Failed to create a new partition...\n");
		PANIC();
	}

	// Map all pages between start and end to the child partition.
	length = minimal_end - minimal_start;
	for (offset = 0; offset < length; offset += PAGE_SIZE)
	{
		// Map the current page to the child partition.
		if (!Pip_MapPageWrapper(minimal_start + offset, pd, 0x700000 + offset))
		{
			printf("Failed to map the current page to the child partition...\n");
			PANIC();
		}
	}

	// Allocate a new page for the child's stack.
	stackPage = (uint32_t) Pip_AllocPage();

	// Map the stack page into the child.
	if (!Pip_MapPageWrapper(stackPage, pd, INITIAL_STACK_TOP))
	{
		printf("Failed to map a page for the child's stack...\n");
		PANIC();
	}

	// Allocate a new page for the child's VIDT.
	vidtPage = (user_ctx_t **) Pip_AllocPage();

	// Write the child's context in entry number 0.
	stack = vidtPage[0] = (user_ctx_t *) (INITIAL_STACK_TOP + PAGE_SIZE - sizeof(user_ctx_t));
	vidtPage[0]->valid    = 0;
	vidtPage[0]->eip      = (uint32_t) 0x700000;
	vidtPage[0]->pipflags = 0;
	vidtPage[0]->eflags   = 0x2;
	vidtPage[0]->regs.esp = (uint32_t) stack;
	vidtPage[0]->regs.ebx = BOOTINFO_ADDR;
	vidtPage[0]->valid    = 1;

	// Map the VIDT page into the child partition.
	if (!Pip_MapPageWrapper((uint32_t) vidtPage, pd, VIDT_ADDR))
	{
		printf("Failed to map a page for the child's VIDT...\n");
		PANIC();
	}

	// Switch to child context.
	Pip_Yield(pd, 0, 49, 0, 0);

	// Should never be reached.
	PANIC();
}
