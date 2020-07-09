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
#define CREATE_PARTITION_FAILED	 0

/*!
 * \def MAP_CHILD_PAGE_FAILED
 * \brief Map child page error code
 */
#define MAP_CHILD_PAGE_FAILED	-1

/*!
 * \def MAP_STACK_PAGE_FAILED
 * \brief Map stack page error code
 */
#define MAP_STACK_PAGE_FAILED	-2

/*!
 * \def MAP_VIDT_PAGE_FAILED
 * \brief Map VIDT page error code
 */
#define MAP_VIDT_PAGE_FAILED	-3

/*
 * External symbols from link.ld file
 */
extern void *_minimal_addr_start;
extern void *_minimal_addr_end;
extern void *__start_read_only;
extern void *__end_read_only;

/*!
 * \fn static void printBootInfo(pip_fpinfo* bootinfo)
 * \brief Print the boot informations to the serial port
 * \param bootinfo A pointer to a pip_fpinfo structure
 */
static void printBootInfo(pip_fpinfo* bootinfo)
{
	printf("Magic number... 0x%x\n", bootinfo->magic);
	printf("Free memory start... 0x%x\n", bootinfo->membegin);
	printf("Free memory end... 0x%x\n", bootinfo->memend);
	printf("Pip revision... %s\n",   bootinfo->revision);
	printf("Root partition start... 0x%x\n", &__start_read_only);
	printf("Root partition end... 0x%x\n", &__end_read_only);
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
static int32_t bootstrapPartition(uint32_t base, uint32_t size, uint32_t laddr, uint32_t *child_part_desc)
{
	// Allocation of 5 memory pages for the creation of the child partition
	*child_part_desc   = (uint32_t) Pip_AllocPage();
	printf("Child partition part desc : 0x%x\n", *child_part_desc);
	uint32_t child_page_dir    = (uint32_t) Pip_AllocPage();
	printf("Child partition page dir : 0x%x\n", child_page_dir);
	uint32_t child_shadow1     = (uint32_t) Pip_AllocPage();
	printf("Child partition shadow1 : 0x%x\n", child_shadow1);
	uint32_t child_shadow2     = (uint32_t) Pip_AllocPage();
	printf("Child partition shadow2 : 0x%x\n", child_shadow2);
	uint32_t child_linked_list = (uint32_t) Pip_AllocPage();
	printf("Child partition linked_list : 0x%x\n", child_linked_list);

	// Creation of the partition
	if (!Pip_CreatePartition(*child_part_desc, child_page_dir, child_shadow1, child_shadow2, child_linked_list))
		return CREATE_PARTITION_FAILED;

	// Map each page of the child partition to the newly created partition
	for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE)
	{
		if (!Pip_MapPageWrapper(base + offset, *child_part_desc, laddr + offset))
			return MAP_CHILD_PAGE_FAILED;
	}

	// Allocation of a memory page for the child partition stack
	uint32_t stackPage = (uint32_t) Pip_AllocPage();
	printf("Child stack : 0x%x\n", stackPage);

	// Store the child's context at the top of the stack
	user_ctx_t *context_paddr = (user_ctx_t *) (stackPage + PAGE_SIZE - sizeof(user_ctx_t));
	printf("Child context : 0x%x - 0x%x\n", context_paddr, (uint32_t) context_paddr + sizeof(user_ctx_t));

	// Creation of the child's context
	context_paddr->valid    = 0;
	context_paddr->eip      = laddr;
	context_paddr->pipflags = 0;
	context_paddr->eflags   = 0x202;
	context_paddr->regs.ebp = STACK_TOP_VADDR + PAGE_SIZE;
	context_paddr->regs.esp = context_paddr->regs.ebp - sizeof(user_ctx_t);
	context_paddr->valid    = 1;

	// Map the stack page to the newly created partition
	if (!Pip_MapPageWrapper(stackPage, *child_part_desc, STACK_TOP_VADDR))
		return MAP_STACK_PAGE_FAILED;

	// Allocation of a memory page for the child's VIDT
	user_ctx_t **vidtPage = (user_ctx_t **) Pip_AllocPage();
	printf("Child VIDT : 0x%x\n", vidtPage);

	// Store the pointer of the child context into the child's VIDT at index 0, 48, 49
	user_ctx_t *ctx_vaddr = (user_ctx_t *) (STACK_TOP_VADDR +
			PAGE_SIZE - sizeof(user_ctx_t));

	vidtPage[0]  = ctx_vaddr;
	vidtPage[48] = ctx_vaddr;
	vidtPage[49] = ctx_vaddr;

	printf("Child init context written at 0x%x\n", &vidtPage[0]);

	// Map the VIDT page to the newly created partition
	if (!Pip_MapPageWrapper((uint32_t) vidtPage, *child_part_desc, VIDT_VADDR))
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
	setup_context = (user_ctx_t *) (STACK_TOP_VADDR - sizeof(user_ctx_t));

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

	printBootInfo(bootinfo);

	printf("\nInitializing the memory pages... ");
	if (!Pip_InitPaging((void *) bootinfo->membegin, (void *) bootinfo->memend))
	{
		printf("Failed.\n");
		PANIC();
	}
	printf("Done.\n");

	printf("Bootstraping the minimal partition... ");
	uint32_t child_part_desc;
	if ((ret = bootstrapPartition(minimal_start, minimal_end - minimal_start, 0x700000, &child_part_desc)) < 1)
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
		unsigned rc = Pip_Yield(child_part_desc, 0, 49, 0, 0);
		switch(rc) {
			case 1:
				printf("Yield returned FAIL_INVALID_INT_LEVEL\n");
				break;
			case 2:
				printf("Yield returned FAIL_INVALID_CTX_SAVE_INDEX\n");
				break;
			case 3:
				printf("Yield returned FAIL_ROOT_CALLER\n");
				break;
			case 4:
				printf("Yield returned FAIL_INVALID_CHILD\n");
				break;
			case 5:
				printf("Yield returned FAIL_UNAVAILABLE_TARGET_VIDT\n");
				break;
			case 6:
				printf("Yield returned FAIL_UNAVAILABLE_CALLER_VIDT\n");
				break;
			case 7:
				printf("Yield returned FAIL_MASKED_INTERRUPT\n");
				break;
			case 8:
				printf("Yield returned FAIL_UNAVAILABLE_TARGET_CTX\n");
				break;
			case 9:
				printf("Yield returned FAIL_CALLER_CONTEXT_SAVE\n");
				break;
			case 0:
				printf("Yield succeeded !\n");
				break;
			default:
				printf("Yield returned an unexpected value : 0x%x\n", rc);
				break;
		}
	}

	// Should never be reached.

	PANIC();
}
