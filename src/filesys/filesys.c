#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "userprog/syscall.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;
int i;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
// Dakota driving
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();

  i = 0;
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
// Dakota driving
bool
filesys_create (const char *name, off_t initial_size) 
{

  bool success = true;

  if (i <= 1) // Help: plz
  {
    block_sector_t inode_sector = 0;
    struct dir *dir = dir_open_root ();  // changed to current directory from root
    success = (dir != NULL
                    && free_map_allocate (1, &inode_sector)
                    && inode_create (inode_sector, initial_size)
                    && dir_add (dir, name, inode_sector));
    i++;
  }
  else
  {
    char *token, *save_ptr;       // for spliter
    char s[strlen(name)];
    char *save_tok;

    strlcpy (s, name, strlen (name)+1);  //moves path copy into s, add 1 for null

    for (token = strtok_r (s, "/", &save_ptr); token != NULL;
          token = strtok_r (NULL, "/", &save_ptr))
      if(token != NULL)  
        save_tok = token;

    char save[strlen(save_tok)];
    strlcpy (save, save_tok, strlen (save_tok)+1); 

 

    block_sector_t inode_sector = 0;
    struct dir *dir = thread_current()->current_dir;  // Help: changed to current directory from root
    success = (dir != NULL
                    && free_map_allocate (1, &inode_sector)
                    && inode_create (inode_sector, initial_size)
                    && dir_add (dir, save, inode_sector));
    }
 
  return success;
}
/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
// Jefferson driving
struct file *
filesys_open (const char *name)
{
  bool success = true;
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if(strcmp(name, "/") == 0)
  {
    return file_open(dir->inode);
  }

  if (dir != NULL && name[0] == '/')
  {
    success = dir_lookup (dir, name+1, &inode);
  } 

  else
  {
    success = dir_lookup (dir, name, &inode); // incomplete Help:
  }
  
  dir_close (dir);
  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
// Spencer driving
bool
filesys_remove (const char *path) 
{
  
  if(strcmp(path, "/")==0)
    return false;

  struct dir *dir = get_dir (path);

  if(!dir->empty)
  {
    return false;
  }

  // getting the file to be removed
  char *token, *save_ptr;       // for spliter
  char s[strlen(path)];
  char *save_tok;

  strlcpy(s, path, strlen (path)+1);  //moves path copy into s, add 1 for null

  for (token = strtok_r (s, "/", &save_ptr); token != NULL;
        token = strtok_r (NULL, "/", &save_ptr))
  {
    if(token != NULL)
    {    
      save_tok = token;
    }
  }
  char delete[strlen(save_tok)];
  strlcpy (delete, save_tok, strlen (save_tok)+1); 


  // if(dir->inode == dir_open_root ()->inode) // help: not equal to the root, should be able to just use dir
    // printf("\n\ndir is the root\n\n\n\n");

  bool success = dir != NULL && dir_remove (dir_open_root (), delete);
  dir_close (dir); 

  return success;
} 
 
/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
