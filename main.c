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

/*!
 * \def CREATE_PARTITION_FAILED
 * \brief Create partition error code
 */
#define CREATE_PARTITION_FAILED	-1

/*!
 * \def MAP_CHILD_PAGE_FAILED
 * \brief Map child page error code
 */
#define MAP_CHILD_PAGE_FAILED	-2

/*!
 * \def MAP_STACK_PAGE_FAILED
 * \brief Map stack page error code
 */
#define MAP_STACK_PAGE_FAILED	-3

/*!
 * \def MAP_VIDT_PAGE_FAILED
 * \brief Map VIDT page error code
 */
#define MAP_VIDT_PAGE_FAILED	-4

/*
 * The minimal partition descriptor
 */
uint32_t pd;

/*
 * External symbols from link.ld file
 */
extern void *_minimal_addr_start;
extern void *_minimal_addr_end;

/*!
 * \fn static void printBootInfo(pip_fpinfo* bootinfo)
 * \brief Print the boot informations to the serial port
 * \param bootinfo A pointer to a pip_fpinfo structure
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
 * \brief Bootstraping a new child partition at a specific address
 * \param base The start address of the first memory page
 * \param size The size of the partition to map
 * \param laddr The address where to load the partition
 * \return 1 in the case of a success, less than 0 in the case of an error
 */
static int32_t bootstrapPartition(uint32_t base, uint32_t size, uint32_t laddr)
{
	uint32_t rtospd, rtossh1, rtossh2, rtossh3, stackPage, offset;
	user_ctx_t *context, **vidtPage;

	// Allocation of 5 memory pages for the creation of the child partition
	pd      = (uint32_t) Pip_AllocPage();
	rtospd  = (uint32_t) Pip_AllocPage();
	rtossh1 = (uint32_t) Pip_AllocPage();
	rtossh2 = (uint32_t) Pip_AllocPage();
	rtossh3 = (uint32_t) Pip_AllocPage();

	// Creation of the partition
	if (!Pip_CreatePartition(pd, rtospd, rtossh1, rtossh2, rtossh3))
		return CREATE_PARTITION_FAILED;

	// Map each page of the child partition to the newly created partition
	for (offset = 0; offset < size; offset += PAGE_SIZE)
	{
		if (!Pip_MapPageWrapper(base + offset, pd, laddr + offset))
			return MAP_CHILD_PAGE_FAILED;
	}

	// Allocation of a memory page for the child partition stack
	stackPage = (uint32_t) Pip_AllocPage();

	// Store the child's context at the top of the stack
	context = (user_ctx_t *) (stackPage + PAGE_SIZE - sizeof(user_ctx_t));

	// Creation of the child's context
	context->valid    = 0;
	context->eip      = laddr;
	context->pipflags = 0;
	context->eflags   = 0x202;
	context->regs.ebp = STACK_TOP_ADDR + PAGE_SIZE;
	context->regs.esp = context->regs.ebp - sizeof(user_ctx_t);
	context->valid    = 1;

	// Map the stack page to the newly created partition
	if (!Pip_MapPageWrapper(stackPage, pd, STACK_TOP_ADDR))
		return MAP_STACK_PAGE_FAILED;

	// Allocation of a memory page for the child's VIDT
	vidtPage = (user_ctx_t **) Pip_AllocPage();

	// Store the pointer of the child context into the child's VIDT at index 0
	vidtPage[0] = (user_ctx_t *) context->regs.esp;

	// Map the VIDT page to the newly created partition
	if (!Pip_MapPageWrapper((uint32_t) vidtPage, pd, VIDT_ADDR))
		return MAP_VIDT_PAGE_FAILED;

	return 1;
}

/*!
 * \brief The entry point of the root partition
 * \param bootinfo The boot information address from the %ebx register
 */
void main(pip_fpinfo* bootinfo)
{
	uint32_t minimal_start, minimal_end;
	user_ctx_t *setup_context;
	int32_t ret;

	printf("The root partition is booting...\n");

	// Retrieve the context from the stack top
	setup_context = (user_ctx_t *) (STACK_TOP_ADDR - sizeof(user_ctx_t));

	// Store the pointer of the setup context into the VIDT
	VIDT[32] = setup_context;
	VIDT[48] = setup_context;
	VIDT[49] = setup_context;

	// Retrieve the start and end address of the partition
	minimal_start = (uint32_t) &_minimal_addr_start;
	minimal_end   = (uint32_t) &_minimal_addr_end;

	printf("Checking the boot information integrity... ");
	if (bootinfo->magic != FPINFO_MAGIC)
	{
		printf("Failed.\n");
		PANIC();
	}
	printf("Done.\n");

	printf("");printf("");

	printBootInfo(bootinfo);

	printf("Initializing the memory pages... ");
	if (!Pip_InitPaging((void *) bootinfo->membegin, (void *) bootinfo->memend))
	{
		printf("Failed.\n");
		PANIC();
	}
	printf("Done.\n");

	printf("Bootstraping the minimal partition... ");
	if ((ret = bootstrapPartition(minimal_start, minimal_end - minimal_start, 0x700000)) < 1)
	{
		switch (ret)
		{
			case CREATE_PARTITION_FAILED:
				printf("Failed to create a new partition.\n");
				break;
			case MAP_CHILD_PAGE_FAILED:
				printf("Failed to map child page.\n");
				break;
			case MAP_STACK_PAGE_FAILED:
				printf("Failed to map stack page.\n");
				break;
			case MAP_VIDT_PAGE_FAILED:
				printf("Failed to map VIDT page.\n");
				break;
			default:
				printf("Unknown error code...\n");
		}
		PANIC();
	}
	printf("Done.\n");

	printf("It's all good. Now switching to the minimal partition...\n");
	for (;;)
	{
		Pip_Yield(pd, 0, 49, 0, 0);
	}

	// Should never be reached.

	PANIC();
}
