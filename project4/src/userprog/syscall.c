
#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/vaddr.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/off_t.h"
#include "threads/synch.h"

typedef int pid_t;


static void syscall_handler (struct intr_frame *);
//mycode

struct file {
  struct inode *inode;
  off_t pos;
  bool deny_write;
};

void check_address (void *addr, int cnt);
void halt (void);
void exit (int status);
pid_t exec (const char* cmd_line);
int wait (pid_t pid);
int read (int fd, void* buffer, unsigned size);
int write (int fd, const void* buffer, unsigned size);
int fibonacci(int n);
int max_of_four_int(int a, int b, int c, int d);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void* buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
//mycode

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
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

void check_buffer(void *buffer, unsigned len, bool need_check) { //proj 4
  for(int i = 0; i < len; i++) {
    if(!is_user_vaddr(buffer + i)) exit(-1);
    if(!need_check) continue;
    struct vm_entry *vme = get_vme(buffer + i);
    if(vme && !vme->can_write) exit(-1); 
  }
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
      check_address(f->esp, 2);
      f->eax = create((const char *) *(uint32_t *)(f->esp + 4), (unsigned) *(uint32_t *)(f->esp + 8));
      break;
    case SYS_REMOVE:   //1
      check_address(f->esp, 1);
      f->eax = remove((const char *) *(uint32_t*)(f->esp + 4));
      break;
    case SYS_OPEN:     //1
      check_address(f->esp, 1);
      f->eax = open((const char *) *(uint32_t*)(f->esp + 4));
      break;
    case SYS_FILESIZE: //1
      check_address(f->esp, 1);
      f->eax = filesize((int)*(uint32_t*)(f->esp + 4));
      break;
    case SYS_READ:     //3
      check_address(f->esp, 3);

      //to pass write-code2
      check_buffer((void *)*(uint32_t *)(f->esp+8), (unsigned)*((uint32_t *)(f->esp+12)), 1);

      f->eax = read((int)*(uint32_t*)(f->esp + 4), (void*)*(uint32_t*)(f->esp + 8), (unsigned)*(uint32_t*)(f->esp + 12));
      break;
    case SYS_WRITE:    //3
      check_address(f->esp, 3);

      //to pass write-code2
      check_buffer((void *)*(uint32_t *)(f->esp+8), (unsigned)*((uint32_t *)(f->esp+12)), 0);
      
      f->eax = write((int)*(uint32_t*)(f->esp + 4), (const void*)*(uint32_t*)(f->esp + 8), (unsigned)*(uint32_t*)(f->esp + 12));
      break;
    case SYS_SEEK:     //2, void
      check_address(f->esp, 2);
      seek((int)*(uint32_t *)(f->esp + 4), (unsigned) *(uint32_t *)(f->esp + 8));
      break;
    case SYS_TELL:     //1
      check_address(f->esp, 1);
      f->eax = tell((int)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_CLOSE:    //1, void
      check_address(f->esp, 1);
      close((int)*(uint32_t *)(f->esp + 4));
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
  for(int f = 3; f <= 130; f++) if(thread_current()->fd[f]) close(f);
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
  check_address(buffer, 1);
  lock_acquire(&filesys_lock);
  int ret = -1;
  if(fd != 0) {
    if(fd < 0 || fd > 127) {
      lock_release(&filesys_lock);
      exit(-1); 
    }
    struct thread* t = thread_current();
    if(t->fd[fd] == NULL) {
      lock_release(&filesys_lock);
      exit(-1);
    }
    ret = file_read(t->fd[fd], buffer, size);
  }
  else {
    for(int i = 0; i < size; i++) {
      if((((char *)buffer)[i] = input_getc()) == '\0') {
        ret = i;
        break;
      }
    }
  }

  lock_release(&filesys_lock);
  return ret;
}

int write (int fd, const void* buffer, unsigned size) 
{
  check_address(buffer, 1);
  lock_acquire(&filesys_lock);
  int ret = -1;
  if(fd != 1) {
    if(fd < 0 || fd > 127) {
      lock_release(&filesys_lock);
      exit(-1);
    }
    struct thread* t = thread_current();
    if(t->fd[fd] == NULL) {
      lock_release(&filesys_lock);
      exit(-1);
    }
    ret = file_write(t->fd[fd], buffer, size);
  }
  else {
    putbuf(buffer, size);
    ret = size;
  }
  lock_release(&filesys_lock);
  return ret;
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


bool create (const char *file, unsigned initial_size) {
  lock_acquire(&filesys_lock);
  if(file == NULL) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  lock_release(&filesys_lock);
  return filesys_create(file, initial_size);
}

bool remove (const char *file) {
  lock_acquire(&filesys_lock);
  if(file == NULL) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  lock_release(&filesys_lock);
  return filesys_remove(file);
}

int open (const char *file) {
  //open 실패 : -1
  int ret = -1;
  check_address(file, 1);
  lock_acquire(&filesys_lock);
  struct file* fp = filesys_open(file);
  if(fp == NULL) ret = -1;
  else {
    if(strcmp(file, thread_current()->name) == 0) file_deny_write(fp);
    struct thread* t = thread_current();
    for(int f = 3; f < 128; f++) if(t->fd[f] == NULL) {
      t->fd[f] = fp;
      ret = f;
      break;
    }
  }
  lock_release(&filesys_lock);
  return ret;
}

int filesize (int fd) {
  lock_acquire(&filesys_lock);
  struct thread* t = thread_current();
  if(t->fd[fd] == NULL) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  lock_release(&filesys_lock);
  return file_length(t->fd[fd]);
}

void seek (int fd, unsigned position) {
  lock_acquire(&filesys_lock);
  struct thread* t = thread_current();
  if(t->fd[fd] == NULL) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  lock_release(&filesys_lock);
  return file_seek(t->fd[fd], position);
}

unsigned tell (int fd) {
  lock_acquire(&filesys_lock);
  struct thread* t = thread_current();
  if(t->fd[fd] == NULL) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  lock_release(&filesys_lock);
  return file_tell(t->fd[fd]);
}

void close (int fd) {
  lock_acquire(&filesys_lock);
  struct thread* t = thread_current();
  if(t->fd[fd] == NULL) {
    lock_release(&filesys_lock);
    exit(-1);
  }  
  struct file* fp = t->fd[fd]; //close twice.. 복사해두고 
  t->fd[fd] = NULL; //close twice.. NULL로 변경
  lock_release(&filesys_lock);
  return file_close(fp);
}
