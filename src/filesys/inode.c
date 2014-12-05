#include "filesys/inode.h"
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

enum sector_loc location;

struct indirect
{
  block_sector_t sec[128];
};


/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}


/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) // math goes here for figuring out offset
{
  ASSERT (inode != NULL);

  int spot = pos/BLOCK_SECTOR_SIZE;

  if (0 <= spot && spot <= 122)  // if the sector index is in the directly linked sections
  {
    location = DIRECT;  // update for use in write
    return inode -> data.sectors[spot];  // get the sector of the disk at that index
  }

  if (122 < spot && spot <= 250)  // If the sector index is somewhere in the 1st indirect block
  {
    int indir = spot - 122;

    struct indirect temp_indir;
    block_read (fs_device, inode -> data.sectors[123], &temp_indir);  // read the block into struct indirect

    location = INDIRECT;  // update for use in write
    return temp_indir.sec[indir];  // return index in that indirect sector
  }

  if (250 < spot)  // If sector index in doubly indirect
  {
    int first_spot = ((spot - (122 + 128)) / 128);
    int second_spot = (spot - (122 + 128)) - (128 * first_spot);
    
    struct indirect temp_indir1;
    block_read (fs_device, inode -> data.sectors[124], &temp_indir1);  // read first indirect into indirect struct temp_indir1

    struct indirect temp_indir2;
    block_read (fs_device, inode -> data.sectors[first_spot], &temp_indir2);  // read index into second indirect into indirect struct temp_indir2

    location = DOUBLY_INDIRECT;  // update for use in write
    return temp_indir2.sec[second_spot];  // return sector index in the second indirect
  }
  return -1;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
  struct inode_disk *disk_inode = NULL;
  struct indirect *indirect = NULL;
  struct indirect *indirect2 = NULL;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);
  ASSERT (sizeof *indirect == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);

  if (disk_inode != NULL)
    {
      int i;
      int j;
      int k = 0;
      size_t sectors = bytes_to_sectors (length); // make this non consecutive
      int sec_save = sectors;
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;

      block_write (fs_device, sector, disk_inode);
      static char zeros[BLOCK_SECTOR_SIZE];

      for (i = 0;sec_save != 0; sec_save--, i++)  // for every sector needed
      {
        block_sector_t * sec_pointer = &(disk_inode -> sectors[i]);

        if (free_map_allocate (1, sec_pointer) && i < 123)
        {
           block_write (fs_device, disk_inode->sectors[i], zeros); // zeroing out?
        }
        else if ((free_map_allocate (1, sec_pointer) && i == 123))  // make this a indirect sector
        {

          indirect = disk_inode -> sectors[123];
          for(j = 0; j < 128; j++)
          {
            if(free_map_allocate (1, &(indirect->sec[j])))
            {
              block_write (fs_device, indirect -> sec[j], zeros); // zeroing out?
            }
          }           
        }
        else if (free_map_allocate (1, sec_pointer) && i == 124) // make this a double indirect
        {
          indirect = disk_inode -> sectors[124];
          for(j = 0; j < 128; j++)
          {
            indirect2 = indirect -> sec[j];

            for(k = 0; k < 128; k++)
            {
              if(free_map_allocate (1, &(indirect2 -> sec[k])))
              {
                block_write (fs_device, indirect2 -> sec[k], zeros); // zeroing out?
              }
            }            
          }
        }
        else
          return false;

         block_write (fs_device, sector, disk_inode);
      }
    }
  return true;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk. (Does it?  Check code.)
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);
          free_map_release (inode->data.start,
                            bytes_to_sectors (inode->data.length)); 
        }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. Resets length of inode to 0*/
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
  inode->data.length = 0;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) //not finding name in the dir
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);

      // if the data starts beyond it
      if(inode->data.length <= offset)
        return 0;

      uint32_t endpoint_of_file = offset + inode->data.start;
      uint32_t endpoint_of_read = inode->data.start + size;

      // if trying to read beyond the file (partial read)
      if(endpoint_of_read > endpoint_of_file)
      {
        // help
        // do we need to do anything here or is this 
        // already handled below?
      }
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)  
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;
  struct inode_disk *disk_inode = NULL;
  disk_inode = calloc (1, sizeof *disk_inode);

  disk_inode->length = size + offset; // adds size plus offset as write can overwright data in inode

  disk_inode->magic = INODE_MAGIC;

  if (inode->deny_write_cnt)
    return 0;

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
      {
        // figure out if part of direct, indirect or doubly indirect.


        off_t remaining_size = size - bytes_written;
        int sectors_to_alloc = divide_up (remaining_size);

        // maybe put our code in here
        free_map_allocate (sectors_to_alloc, disk_inode -> sectors[sector_idx]);
        //break;
      }

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

int divide_up (int size)
{
  return (size + (BLOCK_SECTOR_SIZE-1)) / BLOCK_SECTOR_SIZE;
}
