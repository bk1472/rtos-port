#ifndef __UTIL_H__
#define __UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern	void	hexdump		(const char *name, void *vcp, int size);
extern	void	err_print	(char *file, char *string);
extern	void	errcase		(void);
extern	int		exitcode	(void);

#ifdef __cplusplus
}
#endif

#endif/*__UTIL_H__*/
