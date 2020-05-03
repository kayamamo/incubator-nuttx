/****************************************************************************
 * arch/arm/src/common/arm_createstack.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/kmalloc.h>
#include <nuttx/arch.h>
#include <nuttx/tls.h>
#include <nuttx/board.h>
#include <arch/board/board.h>

#include "arm_arch.h"
#include "arm_internal.h"

/****************************************************************************
 * Pre-processor Macros
 ****************************************************************************/

/* Configuration */

/* For use with EABI and floating point, the stack must be aligned to 8-byte
 * addresses.
 */

#define CONFIG_STACK_ALIGNMENT 8

/* Stack alignment macros */

#define STACK_ALIGN_MASK    (CONFIG_STACK_ALIGNMENT - 1)
#define STACK_ALIGN_DOWN(a) ((a) & ~STACK_ALIGN_MASK)
#define STACK_ALIGN_UP(a)   (((a) + STACK_ALIGN_MASK) & ~STACK_ALIGN_MASK)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_create_stack
 *
 * Description:
 *   Allocate a stack for a new thread and setup up stack-related information
 *   in the TCB.
 *
 *   The following TCB fields must be initialized by this function:
 *
 *   - adj_stack_size: Stack size after adjustment for hardware, processor,
 *     etc.  This value is retained only for debug purposes.
 *   - stack_alloc_ptr: Pointer to allocated stack
 *   - adj_stack_ptr: Adjusted stack_alloc_ptr for HW.  The initial value of
 *     the stack pointer.
 *
 * Input Parameters:
 *   - tcb: The TCB of new task
 *   - stack_size:  The requested stack size.  At least this much
 *     must be allocated.
 *   - ttype:  The thread type.  This may be one of following (defined in
 *     include/nuttx/sched.h):
 *
 *       TCB_FLAG_TTYPE_TASK     Normal user task
 *       TCB_FLAG_TTYPE_PTHREAD  User pthread
 *       TCB_FLAG_TTYPE_KERNEL   Kernel thread
 *
 *     This thread type is normally available in the flags field of the TCB,
 *     however, there are certain contexts where the TCB may not be fully
 *     initialized when up_create_stack is called.
 *
 *     If either CONFIG_BUILD_PROTECTED or CONFIG_BUILD_KERNEL are defined,
 *     then this thread type may affect how the stack is allocated.  For
 *     example, kernel thread stacks should be allocated from protected
 *     kernel memory.  Stacks for user tasks and threads must come from
 *     memory that is accessible to user code.
 *
 ****************************************************************************/

int up_create_stack(FAR struct tcb_s *tcb, size_t stack_size, uint8_t ttype)
{
#ifdef CONFIG_TLS
  /* Add the size of the TLS information structure */

  stack_size += sizeof(struct tls_info_s);

  /* The allocated stack size must not exceed the maximum possible for the
   * TLS feature.
   */

  DEBUGASSERT(stack_size <= TLS_MAXSTACK);
  if (stack_size >= TLS_MAXSTACK)
    {
      stack_size = TLS_MAXSTACK;
    }
#endif

  /* Is there already a stack allocated of a different size?  Because of
   * alignment issues, stack_size might erroneously appear to be of a
   * different size.  Fortunately, this is not a critical operation.
   */

  if (tcb->stack_alloc_ptr && tcb->adj_stack_size != stack_size)
    {
      /* Yes.. Release the old stack */

      up_release_stack(tcb, ttype);
    }

  /* Do we need to allocate a new stack? */

  if (!tcb->stack_alloc_ptr)
    {
      /* Allocate the stack.  If DEBUG is enabled (but not stack debug),
       * then create a zeroed stack to make stack dumps easier to trace.
       * If TLS is enabled, then we must allocate aligned stacks.
       */

#ifdef CONFIG_TLS
#ifdef CONFIG_MM_KERNEL_HEAP
      /* Use the kernel allocator if this is a kernel thread */

      if (ttype == TCB_FLAG_TTYPE_KERNEL)
        {
          tcb->stack_alloc_ptr =
            (uint32_t *)kmm_memalign(TLS_STACK_ALIGN, stack_size);
        }
      else
#endif
        {
          /* Use the user-space allocator if this is a task or pthread */

          tcb->stack_alloc_ptr =
            (uint32_t *)kumm_memalign(TLS_STACK_ALIGN, stack_size);
        }

#else /* CONFIG_TLS */
#ifdef CONFIG_MM_KERNEL_HEAP
      /* Use the kernel allocator if this is a kernel thread */

      if (ttype == TCB_FLAG_TTYPE_KERNEL)
        {
          tcb->stack_alloc_ptr = (uint32_t *)kmm_malloc(stack_size);
        }
      else
#endif
        {
          /* Use the user-space allocator if this is a task or pthread */

          tcb->stack_alloc_ptr = (uint32_t *)kumm_malloc(stack_size);
        }
#endif /* CONFIG_TLS */

#ifdef CONFIG_DEBUG_FEATURES
      /* Was the allocation successful? */

      if (!tcb->stack_alloc_ptr)
        {
          serr("ERROR: Failed to allocate stack, size %d\n", stack_size);
        }
#endif
    }

  /* Did we successfully allocate a stack? */

  if (tcb->stack_alloc_ptr)
    {
#if defined(CONFIG_TLS) && defined(CONFIG_STACK_COLORATION)
      uintptr_t stack_base;
#endif
      size_t top_of_stack;
      size_t size_of_stack;

      /* The ARM uses a push-down stack:  the stack grows toward lower
       * addresses in memory.  The stack pointer register, points to
       * the lowest, valid work address (the "top" of the stack).  Items
       * on the stack are referenced as positive word offsets from sp.
       */

      top_of_stack = (uint32_t)tcb->stack_alloc_ptr + stack_size - 4;

      /* The ARM stack must be aligned to 8-byte alignment for EABI.
       * If necessary top_of_stack must be rounded down to the next
       * boundary
       */

      top_of_stack = STACK_ALIGN_DOWN(top_of_stack);

      /* The size of the stack in bytes is then the difference between
       * the top and the bottom of the stack (+4 because if the top
       * is the same as the bottom, then the size is one 32-bit element).
       * The size need not be aligned.
       */

      size_of_stack = top_of_stack - (uint32_t)tcb->stack_alloc_ptr + 4;

      /* Save the adjusted stack values in the struct tcb_s */

      tcb->adj_stack_ptr  = (uint32_t *)top_of_stack;
      tcb->adj_stack_size = size_of_stack;

#ifdef CONFIG_TLS
      /* Initialize the TLS data structure */

      memset(tcb->stack_alloc_ptr, 0, sizeof(struct tls_info_s));

#ifdef CONFIG_STACK_COLORATION
      /* If stack debug is enabled, then fill the stack with a
       * recognizable value that we can use later to test for high
       * water marks.
       */

      stack_base = (uintptr_t)tcb->stack_alloc_ptr +
                   sizeof(struct tls_info_s);
      stack_size = tcb->adj_stack_size -
                   sizeof(struct tls_info_s);
      arm_stack_color((FAR void *)stack_base, stack_size);

#endif /* CONFIG_STACK_COLORATION */
#else /* CONFIG_TLS */
#ifdef CONFIG_STACK_COLORATION
      /* If stack debug is enabled, then fill the stack with a
       * recognizable value that we can use later to test for high
       * water marks.
       */

      arm_stack_color(tcb->stack_alloc_ptr, tcb->adj_stack_size);

#endif /* CONFIG_STACK_COLORATION */
#endif /* CONFIG_TLS */

      board_autoled_on(LED_STACKCREATED);
      return OK;
    }

  return ERROR;
}

/****************************************************************************
 * Name: arm_stack_color
 *
 * Description:
 *   Write a well know value into the stack
 *
 ****************************************************************************/

#ifdef CONFIG_STACK_COLORATION
void arm_stack_color(FAR void *stackbase, size_t nbytes)
{
  /* Take extra care that we do not write outsize the stack boundaries */

  uint32_t *stkptr = (uint32_t *)(((uintptr_t)stackbase + 3) & ~3);
  uintptr_t stkend = (((uintptr_t)stackbase + nbytes) & ~3);
  size_t    nwords = (stkend - (uintptr_t)stackbase) >> 2;

  /* Set the entire stack to the coloration value */

  while (nwords-- > 0)
    {
      *stkptr++ = STACK_COLOR;
    }
}
#endif