#ifndef __MMAP_STRING_H__
#define __MMAP_STRING_H__

#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
#define TMPDIR "/tmp"
*/

#define MMAP_UNAVAILABLE	1

	typedef struct _MMAPString MMAPString;

	struct _MMAPString
	{
		char *str;
		size_t len;
		size_t allocated_len;
		int fd;
		size_t mmapped_size;
		/*
		   char * old_non_mmapped_str;
		 */
	};

/* configure location of mmaped files */

	void mmap_string_set_tmpdir (char *directory);

/* Strings
 */

	MMAPString *mmap_string_new (const char *init);

	MMAPString *mmap_string_new_len (const char *init, size_t len);

	MMAPString *mmap_string_sized_new (size_t dfl_size);

	void mmap_string_free (MMAPString * string);

	MMAPString *mmap_string_assign (MMAPString * string,
					const char *rval);

	MMAPString *mmap_string_truncate (MMAPString * string, size_t len);

	MMAPString *mmap_string_set_size (MMAPString * string, size_t len);

	MMAPString *mmap_string_insert_len (MMAPString * string,
					    size_t pos,
					    const char *val, size_t len);

	MMAPString *mmap_string_append (MMAPString * string, const char *val);

	MMAPString *mmap_string_append_len (MMAPString * string,
					    const char *val, size_t len);

	MMAPString *mmap_string_append_c (MMAPString * string, char c);

	MMAPString *mmap_string_prepend (MMAPString * string,
					 const char *val);

	MMAPString *mmap_string_prepend_c (MMAPString * string, char c);

	MMAPString *mmap_string_prepend_len (MMAPString * string,
					     const char *val, size_t len);

	MMAPString *mmap_string_insert (MMAPString * string,
					size_t pos, const char *val);

	MMAPString *mmap_string_insert_c (MMAPString * string,
					  size_t pos, char c);

	MMAPString *mmap_string_erase (MMAPString * string,
				       size_t pos, size_t len);

	void mmap_string_set_ceil (size_t ceil);

	int mmap_string_ref (MMAPString * string);
	int mmap_string_unref (char *str);

#ifdef __cplusplus
}
#endif


#endif				/* __MMAP_STRING_H__ */
