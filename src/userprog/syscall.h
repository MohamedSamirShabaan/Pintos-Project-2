#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "threads/thread.h"
#include <list.h>
#include "threads/synch.h"


struct lock file_lock;       /*lock an unlock access file with multi thread*/

struct fd_element
{
    int fd;                        /*file descriptors ID*/
    struct file *myfile;           /* the real file*/
    struct list_elem element;      /*list elem to add fd_element in fd_list*/
};


void syscall_init (void);
void halt (void);
void exit (int status);
tid_t exec (const char *cmd_line);
int wait (tid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

void close_all(struct list * fd_list);
struct child_element* get_child(tid_t tid,struct list *mylist);


#endif /* userprog/syscall.h */
