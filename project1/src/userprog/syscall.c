
#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/vaddr.h"
#include "threads/interrupt.h"
#include "threads/thread.h"

typedef int pid_t;

static void syscall_handler (struct intr_frame *);
//mycode
void check_address (void *addr, int cnt);
void halt (void);
void exit (int status);
pid_t exec (const char* cmd_line);
int wait (pid_t pid);
int read (int fd, void* buffer, unsigned size);
int write (int fd, const void* buffer, unsigned size);
int fibonacci(int n);
int max_of_four_int(int a, int b, int c, int d);
//mycode

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/*
mycode
check_address
*/
void check_address(void* addr, int cnt) 
{
  uint32_t *pd = thread_current()->pagedir;
  for(int i = 0; i <= cnt; i++) {
    void* nowaddr = addr + i * 4;
    if(nowaddr == NULL) exit(-1);                //NULL
    if(!is_user_vaddr(nowaddr)) exit(-1);        //not user
    if(is_kernel_vaddr(nowaddr)) exit(-1);       //is kernel
    if(!pagedir_get_page(pd, nowaddr)) exit(-1); //unmapped
  }
  return;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  //printf ("system call!\n");
  //my code start
  //========================================================================
  check_address(f->esp, 0);
  switch (*(int32_t*)(f->esp)) {
    //cf) src/lib/syscall-nr.h
                       //args cnt
    case SYS_HALT:     //0
      check_address(f->esp, 0);
      halt();
      break;
    case SYS_EXIT:     //1
      check_address(f->esp, 1);
      exit(*(int*)((f->esp) + 4));
      break;
    case SYS_EXEC:     //1
      check_address(f->esp, 1);
      f->eax = exec((const char *) *(uint32_t *)(f->esp + 4));
      break;
    case SYS_WAIT:     //1
      check_address(f->esp, 1);
      f->eax = wait(*(uint32_t*)(f->esp + 4));
      break;
    case SYS_CREATE:   //2
      break;
    case SYS_REMOVE:   //1
      break;
    case SYS_OPEN:     //1
      break;
    case SYS_FILESIZE: //1
      break;
    case SYS_READ:     //3
      check_address(f->esp, 3);
      f->eax = read((int)*(uint32_t*)(f->esp + 4), (void*)*(uint32_t*)(f->esp + 8), (unsigned)*(uint32_t*)(f->esp + 12));
      break;
    case SYS_WRITE:    //3
      check_address(f->esp, 3);
      f->eax = write((int)*(uint32_t*)(f->esp + 4), (const void*)*(uint32_t*)(f->esp + 8), (unsigned)*(uint32_t*)(f->esp + 12));
      break;
    case SYS_SEEK:     //2
      break;
    case SYS_TELL:     //1
      break;
    case SYS_CLOSE:    //1
      break;
    case SYS_FIBONACCI://1
      check_address(f->esp, 1);
      f->eax = fibonacci(*(uint32_t *)(f->esp + 4));
      break;
    case SYS_MAXOFFOURINT://4
      check_address(f->esp, 4); 
      f->eax = max_of_four_int(*(uint32_t *)(f->esp + 4), *(uint32_t *)(f->esp + 8), *(uint32_t *)(f->esp + 12), *(uint32_t *)(f->esp + 16));
      break;
  }
  //my code end
  //========================================================================
  //thread_exit();
}


void halt (void) 
{
  shutdown_power_off();
  return;
}

void exit (int status) 
{
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;
  thread_exit();
  return;
}

pid_t exec (const char* cmd_line) 
{
  return process_execute(cmd_line);
}

int wait (pid_t pid) 
{
  return process_wait(pid);
}

int read (int fd, void* buffer, unsigned size) 
{
  //only stdin in this project (proj1)
  if(fd != 0) return -1; //not stdin

  int ret = 0;
  for(ret = 0; ret < size; ret++) {
    if((((char *)buffer)[ret] = input_getc()) == '\0') return ret;
  }
  return -1;
}

int write (int fd, const void* buffer, unsigned size) 
{
  //only stdout in this project (proj1)
  if(fd != 1) return -1; //not stdout
  putbuf(buffer, size);
  return size;
}


int fibonacci(int n) 
{
  if(n < 0) return -1;
  if(n == 0) return 0;
  int i_1 = 1, i_2 = 1, now;
  //a[i-1]     a[i-2]
  if(n == 1 || n == 2) return 1;
  for(int i = 3; i <= n; i++) {
    now = i_2 + i_1;
    i_2 = i_1;
    i_1 = now;
  }
  return now;
}

int max_of_four_int(int a, int b, int c, int d)
{
  int ret = a;
  if(ret < b) ret = b;
  if(ret < c) ret = c;
  if(ret < d) ret = d;
  return ret;
}