/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#include <stdio.h>
#endif

#include "aesd-circular-buffer.h"

#define DO_DBG 0

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
#if !defined(__KERNEL__) && DO_DBG
        printf("aesd_circular_buffer_find_entry_offset_for_fpos()\n");
        printf("char_offset = %lu\n", char_offset);
        printf("buffer->in_offs = %d\n", buffer->in_offs);
        printf("buffer->out_offs = %d\n", buffer->out_offs);
        printf("buffer->entry[%d].size  = %lu\n", buffer->out_offs, buffer->entry[buffer->out_offs].size);
        for (int z=0; z<buffer->entry[buffer->out_offs].size; z++) {
            printf("\%c", buffer->entry[buffer->out_offs].buffptr[z]);
        }
        printf("buffer->full = %d\n", buffer->full);
        printf("\n");
#endif
    /* Condition for buffer empty. */
    if (!buffer->full && buffer->in_offs == buffer->out_offs) {
#if !defined(__KERNEL__) && DO_DBG
        printf("The buffer is empty. Cannot read.\n");
#endif
        return NULL;
    }

    /* How many elements are not read, yet? */
    unsigned int num_avail_elem = buffer->full ? \
        AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED : \
      ((AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED + buffer->in_offs - buffer->out_offs)% \
        AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED);
    unsigned int dbg_avail_elem = num_avail_elem;

    /* Check in all the available elements the char_offset to read from. */
    size_t total_entry_size = 0;
    size_t curr_entry_size = 0;
    /* Save the start read pntr local. If char_offset is valid,
    * the buffer->out_offs will be updated back with this value. */
    unsigned int entry_idx = buffer->out_offs;

    while(num_avail_elem) {
        curr_entry_size = buffer->entry[entry_idx].size;
        total_entry_size += curr_entry_size;

#if !defined(__KERNEL__) && DO_DBG
        printf("num_avail_elem = %d\n", num_avail_elem);
        printf("char_offset = %lu\n", char_offset);
        printf("curr_entry_size = %lu\n", curr_entry_size);
        printf("total_entry_size = %lu\n", total_entry_size);
        printf("\n");
#endif

        if (char_offset >= total_entry_size) {
            /* go to the next element */
            entry_idx = (entry_idx + 1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
#if 0
            /* Since elements were read already, the buffer cannot be full anymore.
            * However, the tests from assignment 7 fail when clearing it.
            */
           buffer->full = false;
#endif
            --num_avail_elem;
            continue;
        }
        break;
    };

    if (!num_avail_elem) {
        /* All elements were checked. But the 'char_offset' goes
        * beyond the size of the written entries in the buffer. */
        return NULL;
    } else {
#if !defined(__KERNEL__) && DO_DBG
        printf("The char_offset = %lu consumed %d elements \n", char_offset, dbg_avail_elem - num_avail_elem + 1);
#endif
#if 0
        /* Set the read ptr to the found entry.*/
        /* Normally, after returning an element, the read ptr should increment, but this is not
         *  how the test was designed. Commeting it out for now.
        */
        buffer->out_offs = entry_idx; //(entry_idx + 1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;;
#endif
        /* Calculate relative index in the current frame. This is where the searched
        * character is located.*/
        unsigned int rel_idx = char_offset - (total_entry_size - curr_entry_size);
        *entry_offset_byte_rtn = (size_t)rel_idx;

#if !defined(__KERNEL__) && DO_DBG
        printf("new position: buffer->out_offs = %d\n", buffer->out_offs);
        printf("buffer->full = %d\n", buffer->full);
        printf("Returning &buffer->entry[%d]\n", entry_idx);
        printf("rel_idx = %d\n", rel_idx);
        printf("\n");
#endif

        /* Return the address of the entry.*/
        return &(buffer->entry[entry_idx]);
    }
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    if (buffer && add_entry) {
        if (buffer->full) {
            buffer->out_offs =
                (buffer->out_offs + 1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        }
        // copy the whole struct to the buffer.
        // the important data is inside the buffer.
        uint8_t inp = buffer->in_offs;
        memcpy(
            (&buffer->entry[inp]),
            add_entry, sizeof(struct aesd_buffer_entry)
        );

        buffer->in_offs = (buffer->in_offs + 1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

        if (buffer->in_offs == buffer->out_offs) {
            buffer->full = true;
        }
    }
    else {
#if !defined(__KERNEL__) && DO_DBG
        if (!buffer) {
            printf("buffer is NULL.\n");
        }
        if (!add_entry) {
            printf("add_entry is NULL.\n");
        }
#endif
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
