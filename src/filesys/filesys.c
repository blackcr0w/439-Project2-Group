#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "userprog/syscall.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
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
bool
filesys_create (const char *name, off_t initial_size) 
{
  // added this below segment to get the directory to add to and
  // get the actual name of the directory (not the whole path)
  // help, is this the correct thing to do (also the correct place to do it)
  char * last;
  char * dir_check = dir_path;
  char *dir_cpy = name;   // copying path

  int size_expected = strlen(dir_check) + strlen(dir_cpy);

  if(name[0] == '/') // absolute
    dir_check = name;
  else // relative
  {
    int size_cat = strlcat (dir_check, dir_cpy, size_expected);
    if(size_cat != size_expected)
      return false;
  }

  // check if path is valid
  struct inode *cur_inode = calloc (1, sizeof (struct inode)); // allocate memory
  struct dir *directory = dir_open_root ();

  char s[strlen(dir_check)];
  strlcpy(s, dir_check, strlen (dir_check)+1);  //moves path copy into s, add 1 for null
  char * token, save_ptr;

  // go into each directory checking for validity
  for (token = strtok_r (s, "/", &save_ptr); token != NULL;
        token = strtok_r (NULL, "/", &save_ptr))
  {    
    // help dir_lookup causes a panic. Why? Why. Why!
    /*if(!dir_lookup (directory, token, cur_inode)) // make sure directory exists
    {  
      return false;
    }*/

    directory = dir_open (cur_inode);
  }

  get_last(name, last);

  block_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root ();
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);
 //printf("got to filesys_create: %s\n\n\n", name);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if (dir != NULL)
  {
    dir_lookup (dir, name, &inode);  //single pointer?
  }
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
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
