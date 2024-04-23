# Context switch
#
# void swtch(struct context **old, struct context *new);
#
# The reason for using a double pointer for the old context is to allow the function 
# to modify the value of the pointer itself. By passing a double pointer, the
# function can update the value of the pointer to point to a different memory
# location if needed. This is useful when switching between different contexts.


# The swtch function takes two arguments: the address of the old context structure 
# and the address of the new context structure. It saves the old values of the
# callee-saved registers (EBP, EBX, ESI, EDI) onto the stack, switches the stacks
# by setting the stack pointer to the new context structure, and then restores the
# saved values of the callee-saved registers from the stack. Finally, it returns to
# the caller.
#
# This function is typically used during a context switch between different threads 
# or processes to save and restore the state of the registers.

.globl swtch
swtch:
  # At offset 0 relative to the stack pointer %esp, you typically find the return
  # address of a function call. At offset 4, the first argument.
  movl 4(%esp), %eax # Load the address of the old context structure from the stack into EAX
  movl 8(%esp), %edx # Load the address of the new context structure from the stack into EDX


  # Save old callee-saved registers
  pushl %ebp # Save the old value of EBP onto the stack
  pushl %ebx # Save the old value of EBX onto the stack
  pushl %esi # Save the old value of ESI onto the stack
  pushl %edi # Save the old value of EDI onto the stack

  # Switch stacks
  movl %esp, (%eax) # Store the current stack pointer into the old context structure
  movl %edx, %esp # Set the stack pointer to the new context structure

  # Load new callee-saved registers
  popl %edi # Restore the saved value of EDI from the stack
  popl %esi # Restore the saved value of ESI from the stack
  popl %ebx # Restore the saved value of EBX from the stack
  popl %ebp # Restore the saved value of EBP from the stack
  ret # Return to the caller
