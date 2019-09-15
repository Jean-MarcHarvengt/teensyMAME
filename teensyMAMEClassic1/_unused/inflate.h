#ifndef __INFLATE_H
#define	__INFLATE_H

/***************************************************************************
 * Support for inflate data
 ***************************************************************************/

/* Inflate a file
   in:
	in_file stream to inflate
	in_size size of the compressed data to read
	out_size size of decompressed data
   out:
	out_data buffer for decompressed data
   return:
	==0 ok
*/
int inflate_file(FILE* in_file, unsigned in_size, unsigned char* out_data, unsigned out_size);

/* Inflate a memory region
   in:
	in_data data to inflate
	in_size size of the compressed data to read
	out_size size of decompressed data
   out:
	out_data buffer for decompressed data
   return:
	==0 ok
*/
int inflate_memory(const unsigned char* in_data, unsigned in_size, unsigned char* out_data, unsigned out_size);

#endif
