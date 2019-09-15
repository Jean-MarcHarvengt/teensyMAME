#ifndef __UNZIP_H
#define __UNZIP_H

#include "types.h"
#include <stdio.h>

/***************************************************************************
 * Support for retrieving files from zipfiles
 ***************************************************************************/

struct zipent {
	dword	cent_file_header_sig;
	byte	version_made_by;
	byte	host_os;
	byte	version_needed_to_extract;
	byte	os_needed_to_extract;
	word	general_purpose_bit_flag;
	word	compression_method;
	word	last_mod_file_time;
	word	last_mod_file_date;
	dword	crc32;
	dword	compressed_size;
	dword	uncompressed_size;
	word	filename_length;
	word	extra_field_length;
	word	file_comment_length;
	word	disk_number_start;
	word	internal_file_attrib;
	dword	external_file_attrib;
	dword	offset_lcl_hdr_frm_frst_disk;
	char*   name; /* 0 terminated */
};

typedef struct _ZIP {
	char* zip; /* zip name */
	FILE* fp; /* zip handler */
	long length; /* length of zip file */

	char* ecd; /* end_of_cent_dir data */
	unsigned ecd_length; /* end_of_cent_dir length */

	char* cd; /* cent_dir data */

	unsigned cd_pos; /* position in cent_dir */

	struct zipent ent; /* buffer for readzip */

	/* end_of_cent_dir */
	dword	end_of_cent_dir_sig;
	word	number_of_this_disk;
	word	number_of_disk_start_cent_dir;
	word	total_entries_cent_dir_this_disk;
	word	total_entries_cent_dir;
	dword	size_of_cent_dir;
	dword	offset_to_start_of_cent_dir;
	word	zipfile_comment_length;
	char*	zipfile_comment; /* pointer in ecd */
} ZIP;

/* Opens a zip stream for reading
   return:
     !=0 success, zip stream
     ==0 error
*/
ZIP* openzip(const char* path);

/* Closes a zip stream */
void closezip(ZIP* zip);

/* Reads the current entry from a zip stream
   in:
     zip opened zip
   return:
     !=0 success
     ==0 error
*/
struct zipent* readzip(ZIP* zip);

/* Suspend access to a zip file (release file handler)
   in:
      zip opened zip
   note:
     A suspended zip is automatically reopened at first call of
     readuncompressd() or readcompressed() functions
*/
void suspendzip(ZIP* zip);

/* Resets a zip stream to the first entry
   in:
     zip opened zip
   note:
     ZIP file must be opened and not suspended
*/
void rewindzip(ZIP* zip);

/* Read compressed data from a zip entry
   in:
     zip opened zip
     ent entry to read
   out:
     data buffer for data, ent.compressed_size bytes allocated by the caller
   return:
     ==0 success
     <0 error
*/
int readcompresszip(ZIP* zip, struct zipent* ent, char* data);

/* Read decompressed data from a zip entry
   in:
     zip zip stream open
     ent entry to read
   out:
     data buffer for data, ent.uncompressed_size bytes allocated by the caller
   return:
     ==0 success
     <0 error
*/
int readuncompresszip(ZIP* zip, struct zipent* ent, char* data);

/* public functions */
int /* error */ load_zipped_file (const char *zipfile, const char *filename,
	unsigned char **buf, unsigned int *length);
int /* error */ checksum_zipped_file (const char *zipfile, const char *filename, unsigned int *length, unsigned int *sum);

/* public globals */
extern int	gUnzipQuiet;	/* flag controls error messages */

#endif
