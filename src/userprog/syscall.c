#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "process.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/synch.h"


struct child_element* get_child(tid_t tid,struct list *mylist);
void fd_init(struct fd_element *file_d, int fd_, struct file *myfile_);
static void syscall_handler (struct intr_frame *);
struct fd_element* get_fd(int fd);
int write (int fd, const void *buffer_, unsigned size);
int wait (tid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
tid_t exec (const char *cmdline);
void exit (int status);
void get_args_3(struct intr_frame *f, int choose, void *args);
void get_args_2(struct intr_frame *f, int choose, void *args);
void get_args_1(struct intr_frame *f, int choose, void *args);


void check_valid_ptr (const void *pointer)
{
    if (!is_user_vaddr(pointer))
    {
        exit(-1);
    }

    void *check = pagedir_get_page(thread_current()->pagedir, pointer);
    if (check == NULL)
    {
        exit(-1);
    }
}

void
syscall_init (void)
{
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
    lock_init(&file_lock);
}

void get_args_1(struct intr_frame *f, int choose, void *args)
{
    int argv = *((int*) args);
    args += 4;

    if (choose == SYS_EXIT)
    {
        exit(argv);
    }
    else if (choose == SYS_EXEC)
    {
        check_valid_ptr((const void*) argv);
        f -> eax = exec((const char *)argv);
    }
    else if (choose == SYS_WAIT)
    {
        f -> eax = wait(argv);
    }
    else if (choose == SYS_REMOVE)
    {
        check_valid_ptr((const void*) argv);
        f -> eax = remove((const char *) argv);
    }
    else if(choose == SYS_OPEN)
    {
        check_valid_ptr((const void*) argv);
        f -> eax = open((const char *) argv);
    }
    else if (choose == SYS_FILESIZE)
    {
        f -> eax = filesize(argv);
    }
    else if (choose == SYS_TELL)
    {
        f -> eax = tell(argv);
    }
    else if (choose == SYS_TELL)
    {
        close(argv);
    }
}

void get_args_2(struct intr_frame *f, int choose, void *args)
{
    int argv = *((int*) args);
    args += 4;
    int argv_1 = *((int*) args);
    args += 4;

    if (choose == SYS_CREATE)
    {
        check_valid_ptr((const void*) argv);
        f -> eax = create((const char *) argv, (unsigned) argv_1);
    }
    else if(choose == SYS_SEEK)
    {
        seek(argv, (unsigned)argv_1);
    }
}


void get_args_3 (struct intr_frame *f, int choose, void *args)
{
    int argv = *((int*) args);
    args += 4;
    int argv_1 = *((int*) args);
    args += 4;
    int argv_2 = *((int*) args);
    args += 4;

    check_valid_ptr((const void*) argv_1);
    void * temp = ((void*) argv_1)+ argv_2 ;
    check_valid_ptr((const void*) temp);
    if (choose == SYS_WRITE)
    {
        f->eax = write (argv,(void *) argv_1,(unsigned) argv_2);
    }
    else f->eax = read (argv,(void *) argv_1, (unsigned) argv_2);
}

static void
syscall_handler (struct intr_frame *f )
{
    int syscall_number = 0;
    check_valid_ptr((const void*) f -> esp);
    void *args = f -> esp;
    syscall_number = *( (int *) f -> esp);
    args+=4;
    check_valid_ptr((const void*) args);
    switch(syscall_number)
    {
    case SYS_HALT:                  	/* Halt the operating system. */
        halt();
        break;
    case SYS_EXIT:                   /* Terminate this process. */
        get_args_1(f, SYS_EXIT,args);
        break;
    case SYS_EXEC:                   /* Start another process. */
        get_args_1(f, SYS_EXEC,args);
        break;
    case SYS_WAIT:                   /* Wait for a child process to die. */
        get_args_1(f, SYS_WAIT,args);
        break;
    case SYS_CREATE:                 /* Create a file. */
        get_args_2(f, SYS_CREATE,args);
        break;
    case SYS_REMOVE:                 /* Delete a file. */
        get_args_1(f, SYS_REMOVE,args);
        break;
    case SYS_OPEN:                   /* Open a file. */
        get_args_1(f, SYS_OPEN,args);
        break;
    case SYS_FILESIZE:               /* Obtain a file's size. */
        get_args_1(f, SYS_FILESIZE,args);
        break;
    case SYS_READ:                   /* Read from a file. */
        get_args_3(f, SYS_READ,args);
        break;
    case SYS_WRITE:                  /* Write to a file. */
        get_args_3(f, SYS_WRITE,args);
        break;
    case SYS_SEEK:                   /* Change position in a file. */
        get_args_2(f, SYS_SEEK,args);
        break;
    case SYS_TELL:                   /* Report current position in a file. */
        get_args_1(f, SYS_TELL,args);
        break;
    case SYS_CLOSE:                  /* Close a file. */
        get_args_1(f, SYS_CLOSE,args);
        break;
    default:
        exit(-1);
        break;
    }
}

void halt (void)
{
    shutdown_power_off();
}


void exit (int status)
{
    struct thread *cur = thread_current();
    printf ("%s: exit(%d)\n", cur -> name, status);

    //get me as a child
    struct child_element *child = get_child(cur->tid, &cur -> parent -> child_list);
    //setting my exit status
    child -> exit_status = status;
    // mark my current status
    if (status == -1)
    {
        child -> cur_status = WAS_KILLED;
    }
    else
    {
        child -> cur_status = HAD_EXITED;
    }

    thread_exit();
}

tid_t
exec (const char *cmd_line)
{
    struct thread* parent = thread_current();
    tid_t pid = -1;
    // create child process to execute cmd
    pid = process_execute(cmd_line);

    // get the created child
    struct child_element *child = get_child(pid,&parent -> child_list);
    // wait this child until load
    sema_down(&child-> real_child -> sema_exec);
    // after wake up check if child load successfully
    if(!child -> loaded_success)
    {
        //failed to load
        return -1;
    }
    return pid;
}

int wait (tid_t pid)
{
    return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{
    lock_acquire(&file_lock);
    bool ret = filesys_create(file, initial_size);
    lock_release(&file_lock);
    return ret;
}

bool remove (const char *file)
{
    lock_acquire(&file_lock);
    bool ret = filesys_remove(file);
    lock_release(&file_lock);
    return ret;
}

int open (const char *file)
{
    int ret = -1;
    lock_acquire(&file_lock);
    struct thread *cur = thread_current ();
    struct file * opened_file = filesys_open(file);
    lock_release(&file_lock);
    if(opened_file != NULL)
    {
        cur->fd_size = cur->fd_size + 1;
        ret = cur->fd_size;
        /*create and init new fd_element*/
        struct fd_element *file_d = (struct fd_element*) malloc(sizeof(struct fd_element));
        file_d->fd = ret;
        file_d->myfile = opened_file;
        // add this fd_element to this thread fd_list
        list_push_back(&cur->fd_list, &file_d->element);
    }
    return ret;
}

int filesize (int fd)
{
    struct file *myfile = get_fd(fd)->myfile;
    lock_acquire(&file_lock);
    int ret = file_length(myfile);
    lock_release(&file_lock);
    return ret;
}

int read (int fd, void *buffer, unsigned size)
{
    int ret = -1;
    if(fd == 0)
    {
        // read from the keyboard
        ret = input_getc();
    }
    else if(fd > 0)
    {
        //read from file
        //get the fd_element
        struct fd_element *fd_elem = get_fd(fd);
        if(fd_elem == NULL || buffer == NULL)
        {
            return -1;
        }
        //get the file
        struct file *myfile = fd_elem->myfile;
        lock_acquire(&file_lock);
        ret = file_read(myfile, buffer, size);
        lock_release(&file_lock);
        if(ret < (int)size && ret != 0)
        {
            //some error happened
            ret = -1;
        }
    }
    return ret;
}

int write (int fd, const void *buffer_, unsigned size)
{
    uint8_t * buffer = (uint8_t *) buffer_;
    int ret = -1;
    if (fd == 1)
    {
        // write in the consol
        putbuf( (char *)buffer, size);
        return (int)size;
    }
    else
    {
        //write in file
        //get the fd_element
        struct fd_element *fd_elem = get_fd(fd);
        if(fd_elem == NULL || buffer_ == NULL )
        {
            return -1;
        }
        //get the file
        struct file *myfile = fd_elem->myfile;
        lock_acquire(&file_lock);
        ret = file_write(myfile, buffer_, size);
        lock_release(&file_lock);
    }
    return ret;
}


void seek (int fd, unsigned position)
{
    struct fd_element *fd_elem = get_fd(fd);
    if(fd_elem == NULL)
    {
        return;
    }
    struct file *myfile = fd_elem->myfile;
    lock_acquire(&file_lock);
    file_seek(myfile,position);
    lock_release(&file_lock);
}

unsigned tell (int fd)
{
    struct fd_element *fd_elem = get_fd(fd);
    if(fd_elem == NULL)
    {
        return -1;
    }
    struct file *myfile = fd_elem->myfile;
    lock_acquire(&file_lock);
    unsigned ret = file_tell(myfile);
    lock_release(&file_lock);
    return ret;
}

void close (int fd)
{
    struct fd_element *fd_elem = get_fd(fd);
    if(fd_elem == NULL)
    {
        return;
    }
    struct file *myfile = fd_elem->myfile;
    lock_acquire(&file_lock);
    file_close(myfile);
    lock_release(&file_lock);
}

/**
close and free all file the current thread have
*/
void close_all(struct list *fd_list)
{
    struct list_elem *e;
    while(!list_empty(fd_list))
    {
        e = list_pop_front(fd_list);
        struct fd_element *fd_elem = list_entry (e, struct fd_element, element);
        file_close(fd_elem->myfile);
        list_remove(e);
        free(fd_elem);
    }
}

/**
 * iterate on the fd_list of the cuttrnt thread and get the file which
 * have the same fd
 * if nou found retuen NULL
 * */
struct fd_element*
get_fd(int fd)
{
    struct list_elem *e;
    for (e = list_begin (&thread_current()->fd_list); e != list_end (&thread_current()->fd_list);
            e = list_next (e))
    {
        struct fd_element *fd_elem = list_entry (e, struct fd_element, element);
        if(fd_elem->fd == fd)
        {
            return fd_elem;
        }
    }
    return NULL;
}


/**
intrate on mylist and return the child which have the tid
*/
struct child_element*
get_child(tid_t tid, struct list *mylist)
{
    struct list_elem* e;
    for (e = list_begin (mylist); e != list_end (mylist); e = list_next (e))
    {
        struct child_element *child = list_entry (e, struct child_element, child_elem);
        if(child -> child_pid == tid)
        {
            return child;
        }
    }
}
