/* vi:ts=8
   DWARF 2/3/4 support.
   Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
   2004, 2005 Free Software Foundation, Inc.

   Adapted from gdb/dwarf2read.c by Gavin Koch of Cygnus Solutions
   (gavin@cygnus.com).

   From the dwarf2read.c header:
   Adapted by Gary Funck (gary@intrepid.com), Intrepid Technology,
   Inc.  with support from Florida State University (under contract
   with the Ada Joint Program Office), and Silicon Graphics, Inc.
   Initial contribution by Brent Benson, Harris Computer Systems, Inc.,
   based on Fred Fish's (Cygnus Support) implementation of DWARF 1
   support in dwarfread.c

   This file is part of BFD.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include "def.h"
#include "dwarf.h"

extern int	debug_lvl_pc;

unsigned char	*pDebugStr = NULL, *pDebugInfo = NULL, *pDebugAbbr = NULL;
size_t		debugStrSize, debugInfoSize, debugAbbrSize;

/* Attributes have a name and a value.  */

struct attribute
{
  enum dwarf_attribute name;
  enum dwarf_form form;
  union
  {
    char *str;
    struct dwarf_block *blk;
    uint64_t val;
    int64_t sval;
  }
  u;
};

/* Blocks are a bunch of untyped bytes.  */
struct dwarf_block
{
  unsigned int size;
  unsigned char *data;
};

/* A minimal decoding of DWARF2 compilation units.  We only decode
   what's needed to get to the line number information.  */

struct comp_unit
{
  /* Chain the previously read compilation units.  */
  struct comp_unit *next_unit;
  struct comp_unit *prev_unit;


  /* The DW_AT_name attribute (for error messages).  */
  char *name;

  /* The abbrev hash table.  */
  struct abbrev_info **abbrevs;

  /* Note that an error was found by comp_unit_find_nearest_line.  */
  int error;

  /* The DW_AT_comp_dir attribute.  */
  char *comp_dir;

  /* TRUE if there is a line number table associated with this comp. unit.  */
  int stmtlist;

  /* The offset into .debug_line of the line number table.  */
  unsigned long line_offset;

  /* Address size for this unit - from unit header.  */
  unsigned char addr_size;

  /* Offset size for this unit - from unit header.  */
  unsigned char offset_size;

  /* Base address for this unit - from DW_AT_low_pc attribute of
     DW_TAG_compile_unit DIE */
  uint64_t base_address;

  /* End address for this unit */
  uint64_t end_address;
};

/* This data structure holds the information of an abbrev.  */
struct abbrev_info
{
  unsigned int number;		/* Number identifying abbrev.  */
  enum dwarf_tag tag;		/* DWARF tag.  */
  int has_children;		/* Boolean.  */
  unsigned int num_attrs;	/* Number of attributes.  */
  struct attr_abbrev *attrs;	/* An array of attribute descriptions.  */
  struct abbrev_info *next;	/* Next in chain.  */
};

struct attr_abbrev
{
  enum dwarf_attribute name;
  enum dwarf_form form;
};

#ifndef ABBREV_HASH_SIZE
#define ABBREV_HASH_SIZE 121
#endif
#ifndef ATTR_ALLOC_CHUNK
#define ATTR_ALLOC_CHUNK 4
#endif

/* VERBATIM
   The following function up to the END VERBATIM mark are
   copied directly from dwarf2read.c.  */

/* Read dwarf information from a buffer.  */

static unsigned int
read_1_byte (unsigned char *buf)
{
  return (unsigned int)*buf;
}

static int
read_1_signed_byte (unsigned char *buf)
{
  return (signed int)*buf;
}

static unsigned int
read_2_bytes (unsigned char *buf)
{
  extern short getShort(char *pSrc);
  return (getShort(buf));
}

static unsigned int
read_4_bytes (unsigned char *buf)
{
  extern int getLong(char *pSrc);
  return (getLong(buf));
}

static uint64_t
read_8_bytes (unsigned char *buf)
{
  extern int64_t getLLong(char *pSrc);
  return (getLLong(buf));
}

#if 0
static unsigned char *
read_n_bytes (unsigned char *buf, unsigned int size)
{
  /* If the size of a host char is 8 bits, we can return a pointer
     to the buffer, otherwise we have to copy the data to a buffer
     allocated on the temporary obstack.  */
  return buf;
}
#endif

static char *
read_string (unsigned char *buf, unsigned int *bytes_read_ptr)
{
  /* Return a pointer to the embedded string.  */
  char *str = (char *) buf;
  if (*str == '\0')
    {
      *bytes_read_ptr = 1;
      return NULL;
    }

  *bytes_read_ptr = strlen (str) + 1;
  return str;
}

static unsigned int read_unsigned_leb128(unsigned char *cp, unsigned int *pBytesRead)
{
  unsigned int  result;
  int           shift;
  unsigned char data;

  result   = 0;
  shift    = 0;
  *pBytesRead = 0;

  do
  {
    data = *cp++; (*pBytesRead)++;
    result |= ((data & 0x7f) << shift);
    shift += 7;
  }
  while (data & 0x80);

  return result;
}

static signed int read_signed_leb128(unsigned char *cp, unsigned int *pBytesRead)
{
  int           result;
  int           shift;
  unsigned char data;

  result = 0;
  shift = 0;

  do
  {
    data = *cp++; (*pBytesRead)++;
    result |= ((data & 0x7f) << shift);
    shift += 7;
  }
  while (data & 0x80);

  if ((shift < 32) && (data & 0x40))
    result |= -(1 << shift);

  return result;
}

static char *
read_indirect_string (struct comp_unit* unit, unsigned char *buf, unsigned int *bytes_read_ptr)
{
  unsigned int offset;
  char *str;

  offset = read_4_bytes (buf);
  *bytes_read_ptr = unit->offset_size;

  if (pDebugStr)
    str = (char *) pDebugStr + offset;
  else
    str = (char *)".";
  if (*str == '\0')
    return NULL;
  return str;
}

/* END VERBATIM */

static unsigned long long
read_address (struct comp_unit *unit, unsigned char *buf)
{
  switch (unit->addr_size)
    {
    case 8:
      return read_8_bytes (buf);
    case 4:
      return read_4_bytes (buf);
    case 2:
      return read_2_bytes (buf);
    default:
      fprintf(stdout, "error in address size\n");
    }
}

/* Lookup an abbrev_info structure in the abbrev hash table.  */

static struct abbrev_info *
lookup_abbrev (unsigned int number, struct abbrev_info **abbrevs)
{
  unsigned int hash_number;
  struct abbrev_info *abbrev;

  hash_number = number % ABBREV_HASH_SIZE;
  abbrev = abbrevs[hash_number];

  while (abbrev)
    {
      if (abbrev->number == number)
	return abbrev;
      else
	abbrev = abbrev->next;
    }

  return NULL;
}

/* In DWARF version 2, the description of the debugging information is
   stored in a separate .debug_abbrev section.  Before we read any
   dies from a section we read in all abbreviations and install them
   in a hash table.  */

static struct abbrev_info**
read_abbrevs (unsigned int offset)
{
  struct abbrev_info **abbrevs;
  unsigned char *abbrev_ptr;
  struct abbrev_info *cur_abbrev;
  unsigned int abbrev_number, bytes_read, abbrev_name;
  unsigned int abbrev_form, hash_number;
  size_t amt;

  amt = sizeof (struct abbrev_info*) * ABBREV_HASH_SIZE;
  abbrevs = calloc(1, amt);

  abbrev_ptr = pDebugAbbr + offset;
  abbrev_number = read_unsigned_leb128 (abbrev_ptr, &bytes_read);
  abbrev_ptr += bytes_read;

  /* Loop until we reach an abbrev number of 0.  */
  while (abbrev_number)
    {
      amt = sizeof (struct abbrev_info);
      cur_abbrev = calloc (1, amt);

      /* Read in abbrev header.  */
      cur_abbrev->number = abbrev_number;
      cur_abbrev->tag = (enum dwarf_tag) read_unsigned_leb128 (abbrev_ptr, &bytes_read);
      abbrev_ptr += bytes_read;
      cur_abbrev->has_children = read_1_byte (abbrev_ptr);
      abbrev_ptr += 1;

      /* Now read in declarations.  */
      abbrev_name = read_unsigned_leb128 (abbrev_ptr, &bytes_read);
      abbrev_ptr += bytes_read;
      abbrev_form = read_unsigned_leb128 (abbrev_ptr, &bytes_read);
      abbrev_ptr += bytes_read;

      while (abbrev_name)
	{
	  if ((cur_abbrev->num_attrs % ATTR_ALLOC_CHUNK) == 0)
	    {
	      struct attr_abbrev *tmp;

	      amt = cur_abbrev->num_attrs + ATTR_ALLOC_CHUNK;
	      amt *= sizeof (struct attr_abbrev);
	      tmp = (struct attr_abbrev *)realloc (cur_abbrev->attrs, amt);
	      if (tmp == NULL)
	        {
	          size_t i;

	          for (i = 0; i < ABBREV_HASH_SIZE; i++)
	            {
	            struct abbrev_info *abbrev = abbrevs[i];

	            while (abbrev)
	              {
	                free (abbrev->attrs);
	                abbrev = abbrev->next;
	              }
	            }
	          return NULL;
	        }
	      cur_abbrev->attrs = tmp;
	    }

	  cur_abbrev->attrs[cur_abbrev->num_attrs].name = (enum dwarf_attribute) abbrev_name;
	  cur_abbrev->attrs[cur_abbrev->num_attrs++].form = (enum dwarf_form) abbrev_form;
	  abbrev_name = read_unsigned_leb128 (abbrev_ptr, &bytes_read);
	  abbrev_ptr += bytes_read;
	  abbrev_form = read_unsigned_leb128 (abbrev_ptr, &bytes_read);
	  abbrev_ptr += bytes_read;
	}

      hash_number = abbrev_number % ABBREV_HASH_SIZE;
      cur_abbrev->next = abbrevs[hash_number];
      abbrevs[hash_number] = cur_abbrev;

      /* Get next abbreviation.
	 Under Irix6 the abbreviations for a compilation unit are not
	 always properly terminated with an abbrev number of 0.
	 Exit loop if we encounter an abbreviation which we have
	 already read (which means we are about to read the abbreviations
	 for the next compile unit) or if the end of the abbreviation
	 table is reached.  */
      if ((unsigned int) (abbrev_ptr - pDebugAbbr) >= debugAbbrSize)
	break;
      abbrev_number = read_unsigned_leb128 (abbrev_ptr, &bytes_read);
      abbrev_ptr += bytes_read;
      if (lookup_abbrev (abbrev_number,abbrevs) != NULL)
	break;
    }

  return abbrevs;
}

/* Read an attribute value described by an attribute form.  */

static unsigned char *
read_attribute_value (struct attribute *attr,
		      unsigned form,
		      struct comp_unit *unit,
		      unsigned char *info_ptr)
{
  unsigned int bytes_read;
  struct dwarf_block *blk;
  size_t amt;

  attr->form = (enum dwarf_form) form;

  switch (form)
    {
    case DW_FORM_addr:
      /* FIXME: DWARF3 draft says DW_FORM_ref_addr is offset_size.  */
    case DW_FORM_ref_addr:
      attr->u.val = read_address (unit, info_ptr);
      info_ptr += unit->addr_size;
      break;
    case DW_FORM_block2:
      #if 0
      amt = sizeof (struct dwarf_block);
      blk = (void *)malloc (amt);
      blk->size = read_2_bytes (info_ptr);
      info_ptr += 2;
      blk->data = read_n_bytes (info_ptr, blk->size);
      info_ptr += blk->size;
      attr->u.blk = blk;
      #else
      amt = read_2_bytes(info_ptr);
      info_ptr += 2 + amt;
      #endif
      break;
    case DW_FORM_block4:
      #if 0
      amt = sizeof (struct dwarf_block);
      blk = (void *)malloc (amt);
      blk->size = read_4_bytes (info_ptr);
      info_ptr += 4;
      blk->data = read_n_bytes (info_ptr, blk->size);
      info_ptr += blk->size;
      attr->u.blk = blk;
      #else
      amt = read_4_bytes(info_ptr);
      info_ptr += 4 + amt;
      #endif
      break;
    case DW_FORM_data2:
      attr->u.val = read_2_bytes (info_ptr);
      info_ptr += 2;
      break;
    case DW_FORM_data4:
    case DW_FORM_sec_offset: /*DWARF4 new FORM direction : replace data4*/
      attr->u.val = read_4_bytes (info_ptr);
      info_ptr += 4;
      break;
      break;
    case DW_FORM_data8:
      attr->u.val = read_8_bytes (info_ptr);
      info_ptr += 8;
      break;
    case DW_FORM_string:
      attr->u.str = read_string (info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_strp:
      attr->u.str = read_indirect_string (unit, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_block:
      #if 0
      amt = sizeof (struct dwarf_block);
      blk = (void *) malloc(amt);
      blk->size = read_unsigned_leb128 (info_ptr, &bytes_read);
      info_ptr += bytes_read;
      blk->data = read_n_bytes (info_ptr, blk->size);
      info_ptr += blk->size;
      attr->u.blk = blk;
      #else
      amt = read_unsigned_leb128 (info_ptr, &bytes_read);
      info_ptr += bytes_read + amt;
      #endif
      break;
    case DW_FORM_block1:
      #if 0
      amt = sizeof (struct dwarf_block);
      blk = (void *) malloc(amt);
      blk->size = read_1_byte (info_ptr);
      info_ptr += 1;
      blk->data = read_n_bytes (info_ptr, blk->size);
      info_ptr += blk->size;
      attr->u.blk = blk;
      #else
      amt = read_1_byte(info_ptr);
      info_ptr += 1 + amt;
      #endif
      break;
    case DW_FORM_data1:
      attr->u.val = read_1_byte (info_ptr);
      info_ptr += 1;
      break;
    case DW_FORM_flag:
      attr->u.val = read_1_byte (info_ptr);
      info_ptr += 1;
      break;
    case DW_FORM_sdata:
      attr->u.sval = read_signed_leb128 (info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_udata:
      attr->u.val = read_unsigned_leb128 (info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_ref1:
      attr->u.val = read_1_byte (info_ptr);
      info_ptr += 1;
      break;
    case DW_FORM_ref2:
      attr->u.val = read_2_bytes (info_ptr);
      info_ptr += 2;
      break;
    case DW_FORM_ref4:
      attr->u.val = read_4_bytes (info_ptr);
      info_ptr += 4;
      break;
    case DW_FORM_ref_udata:
      attr->u.val = read_unsigned_leb128 (info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_indirect:
      form = read_unsigned_leb128 (info_ptr, &bytes_read);
      info_ptr += bytes_read;
      info_ptr = read_attribute_value (attr, form, unit, info_ptr);
      break;
    default:
      fprintf(stdout, "Dwarf Error: Invalid or unhandled FORM value: %u(0x%x)\n", form, form);
    }
  return info_ptr;
}

/* Read an attribute described by an abbreviated attribute.  */

static unsigned char *
read_attribute (struct attribute *attr,
		struct attr_abbrev *abbrev,
		struct comp_unit *unit,
		unsigned char *info_ptr)
{
  attr->name = abbrev->name;
  info_ptr = read_attribute_value (attr, abbrev->form, unit, info_ptr);
  return info_ptr;
}

/* Parse a DWARF2 compilation unit starting at INFO_PTR.  This
   includes the compilation unit header that proceeds the DIE's, but
   does not include the length field that precedes each compilation
   unit header.  END_PTR points one past the end of this comp unit.
   OFFSET_SIZE is the size of DWARF2 offsets (either 4 or 8 bytes).

   This routine does not read the whole compilation unit; only enough
   to get to the line number information for the compilation unit.  */

static struct comp_unit *
parse_comp_unit (unsigned char *info_ptr,
		 unsigned int unit_length,
		 unsigned int offset_size)
{
  struct comp_unit* unit;
  unsigned int version;
  unsigned int abbrev_offset = 0;
  unsigned int addr_size;
  struct abbrev_info** abbrevs;
  unsigned int abbrev_number, bytes_read, i;
  struct abbrev_info *abbrev;
  struct attribute attr;
  size_t amt;
  unsigned long low_pc = 0;
  unsigned long high_pc = 0;

  version = read_2_bytes (info_ptr);
  info_ptr += 2;
  abbrev_offset = read_4_bytes (info_ptr);
  info_ptr += offset_size;
  addr_size = read_1_byte (info_ptr);
  info_ptr += 1;

  if (version > 4)
    {
      fprintf(stdout, "Dwarf Error: found dwarf version '%u', support version under 4 only\n", version);
      exit(-1);
    }

  /* Read the abbrevs for this compilation unit into a table.  */
  abbrevs = read_abbrevs (abbrev_offset);
  if (! abbrevs)
      return 0;

  abbrev_number = read_unsigned_leb128 (info_ptr, &bytes_read);
  info_ptr += bytes_read;
  if (! abbrev_number)
    {
      fprintf(stdout, "Dwarf Error: Bad abbrev number: %u\n", abbrev_number);
      return 0;
    }

  abbrev = lookup_abbrev (abbrev_number, abbrevs);
  if (! abbrev)
    {
      fprintf(stdout, "Dwarf Error: Could not find abbrev number %u\n", abbrev_number);
      return 0;
    }

  amt = sizeof (struct comp_unit);
  unit = calloc (1, amt);
  unit->addr_size = addr_size;
  unit->offset_size = offset_size;
  unit->abbrevs = abbrevs;
  unit->next_unit = NULL;

  for (i = 0; i < abbrev->num_attrs; ++i)
    {
      info_ptr = read_attribute (&attr, &abbrev->attrs[i], unit, info_ptr);

      /* Store the data if it is of an attribute we want to keep in a
	 partial symbol table.  */
      switch (attr.name)
	{
	case DW_AT_stmt_list:
	  unit->stmtlist = 1;
	  unit->line_offset = attr.u.val;
	  break;

	case DW_AT_name:
	  unit->name = attr.u.str;
	  break;

	case DW_AT_low_pc:
	  low_pc = attr.u.val;
	  /* If the compilation unit DIE has a DW_AT_low_pc attribute,
	     this is the base address to use when reading location
	     lists or range lists. */
	  unit->base_address = low_pc;
	  break;

	case DW_AT_high_pc:
	  high_pc = attr.u.val;
	  unit->end_address = high_pc;
	  break;


	case DW_AT_ranges:
	  break;

	case DW_AT_comp_dir:
	  {
	    char *comp_dir = attr.u.str;
	    if (comp_dir)
	      {
		/* Irix 6.2 native cc prepends <machine>.: to the compilation
		   directory, get rid of it.  */
		char *cp = strchr (comp_dir, ':');

		if (cp && cp != comp_dir && cp[-1] == '.' && cp[1] == '/')
		  comp_dir = cp + 1;
	      }
	    unit->comp_dir = comp_dir;
	    break;
	  }

	default:
	  break;
	}
    }
  return unit;
}

struct comp_unit cu_head;

/* Parse all compile units */
void parse_all_comp_units(void)
{
  unsigned char*	info_ptr = pDebugInfo;
  uint64_t		unit_length;
  struct comp_unit	*curr, *unit;

  curr = &cu_head;
  curr->next_unit = NULL;

  while (info_ptr < (pDebugInfo + debugInfoSize))
  {
    unsigned int addr_size = 8;
    unsigned int offs_size = 8;

    unit_length = read_4_bytes (info_ptr);

    if (unit_length == 0xffffffff)
    {
	offs_size = 8;
	unit_length = read_8_bytes(info_ptr+4);
    	info_ptr += 12;
    }
    else if (unit_length == 0)
    {
	offs_size = 8;
	unit_length = read_4_bytes(info_ptr+4);
    	info_ptr += 8;
    }
    else if (addr_size == 8)
    {
	offs_size = 4;
    	info_ptr += 4;
    }
    else
    {
    	info_ptr += 4;
    }

    unit = parse_comp_unit (info_ptr, unit_length, offs_size);

    #if	(DEBUG > 2)
    if(debug_lvl_pc > 1)
    {
    	printf("[0x%016lx..0x%016lx] 0x%08x, %s,  %s\n", unit->base_address, unit->end_address,
						unit->line_offset, unit->comp_dir, unit->name);
    }
    #endif

    curr->next_unit = unit;
    curr            = unit;
    info_ptr += unit_length;
  }

  return;
}

/* Find compile director of compilation unit having given line offset */
char *
find_comp_dir(unsigned long line_offset)
{
  struct comp_unit *unit;
  unit = cu_head.next_unit;
  while (unit)
  {
    if (unit->line_offset == line_offset)
      return(unit->comp_dir);
    unit = unit->next_unit;
  }
  // fprintf(stdout, "Can't find compilation unit with line offset 0x%x\n", line_offset);
  return(NULL);
}

/* Free all compilation units */
void release_all_comp_units(void)
{
  struct comp_unit *unit;
  int i;
  unit = cu_head.next_unit;

  while (unit)
  {
    cu_head.next_unit = unit->next_unit;

    #if 0
    printf("[0x%08x..0x%08x] 0x%06x, %s, %s\n", unit->base_address, unit->end_address,
				unit->line_offset, unit->comp_dir, unit->name);
    #endif

    for (i = 0; i < ABBREV_HASH_SIZE; i++)
    {
      struct abbrev_info *abbrev, *tobefree;

      abbrev = unit->abbrevs[i];

      while (abbrev)
      {
	tobefree = abbrev;
        abbrev = abbrev->next;
        free(tobefree->attrs);
	free(tobefree);
      }
    }
    free(unit->abbrevs);
    free(unit);
    unit = cu_head.next_unit;
  }

  return;
}
