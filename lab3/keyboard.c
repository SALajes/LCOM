#include <lcom/lcf.h>

#include <minix/sysutil.h>

// Any header files below this line should have been created by you
#include "keyboard.h"
#include "i8042.h"

int sys_inb_counter = 0; //global counter that saves the number of sys_inb calls 
uint32_t scan_code; //scan code of a keyboard key
int global_io_id; //saves the interrupt that will be used to subscribe and remove interrupts

//definition of function sys_inb_cnt
#ifdef LAB3 
int sys_inb_cnt(port_t port, uint32_t *byte) 
{
  sys_inb_counter++;
  
  if(sys_inb(port, byte) == SUCCESS)
  {
    return 0;
  }
  else return 1;
}
#else
#define sys_inb_cnt(p,q) sys_inb(p,q)
#endif


int kbd_subscribe_interrupt(uint8_t policy, uint8_t *io_id) //this function subscribes interruption calls to an I/O with name bit_no
{
  global_io_id = *io_id; //the global variable is given the identity of the I/O we want to deal with

  if(sys_irqsetpolicy(KBD_IRQ, policy, &global_io_id) == OK) //verifies validity of subscription
  { 
    return 0; 
  }

  return 1;
}

int kbd_unsubscribe_interrupt() //this function unsubscribes the interruption calls requested for the I/O identified by io_id
{
  if(sys_irqrmpolicy(&global_io_id) != OK) //verifies validity of unsubscription
  {
    return 1;
  }

  return 0;
}

void (kbc_ih)()
{
  uint32_t validity_mask = BIT(6) | BIT(7); //sets bit 6 and 7, meanwhile, all the other remain unset (0)
  uint32_t status; // will hold the status of the keyboard 
  uint32_t code_received; //will receive the scan code requested

  sys_inb_cnt(KBD_STAT_REG, &status); //asks for the status

  status = status & validity_mask; //masks it to compare
  
  sys_inb_cnt(KBD_OUT_BUFF, &code_received);

  if(status == BIT(6) || status == BIT(7)) //checks validity of the status
  {} //if it is invalid we don't use the code
  else
  {
    scan_code = code_received; //if valid, we use this code passing it to the global variable scan_code
  }  
}

int kbd_read_and_display(uint8_t *num_bytes, uint8_t scan_byte[], bool *end_loop)
{
  bool make; // if true, it's a make code, if false, is a break code

  if(scan_code == ESC_BREAK_CODE) //if the key pressed was ESC, then the loop will end (finalization condition)
  {
    *end_loop = true; //this happens by making end_loop true
  } // it doesn't break automatically because this code is supposed to be displayed too

  if(scan_code == LONG_SCANCODE1 || scan_code == LONG_SCANCODE2) //identifying if the code will have more than 1 byte
  {
    *num_bytes = 2; //we are prepared to deal only with 2 bytes

    scan_byte[0] = scan_code; //saving the scan code that will be read first (in this case will be 0xE0)
  }
  else //scan code only has 1 byte
  {
    if((scan_code & MSB_MASK) == MSB_SET) //verifies if it is a make or break code by check the most significant bit
    {
      make = false; //if it is set it's a break code
    }
    else
    {
      make = true; //if it is unset it's a make code
    }

    scan_byte[*num_bytes - 1] = scan_code; //having one or more bytes, it will save the last one (or only one) in the correct position to be printed

    if (kbd_print_scancode(make, *num_bytes, scan_byte) != SUCCESS) //checks validity of printing
    {
      return 1;
    }  

    *num_bytes = 1;//only has on byte
  } 

  return 0;
}

int reenable_interrupts()
{
  uint32_t kbd_command_state;

  sys_outb(KBD_IN_BUFF_COM, KBC_READ_COMMAND); //writes the command to read
  sys_inb(KBD_IN_BUFF_ARG, &kbd_command_state); //passes the variable kbd_command_state as argument

  kbd_command_state |= INTERRUPT_ENABLE_MASK; //change the state to enable interrupts
  
  sys_outb(KBD_IN_BUFF_COM, KBC_WRITE_COMMAND); //writes the command to write
  sys_outb(KBD_IN_BUFF_ARG, kbd_command_state); //passes the same variable as argument

  return 0;
}

int print_sys_inb_counter()
{
  if (kbd_print_no_sysinb(sys_inb_counter) == FAIL) //checks validity of printing
  {
    return 1;
  }

  return 0;
}
