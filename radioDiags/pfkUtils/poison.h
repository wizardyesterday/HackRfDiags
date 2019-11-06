
#ifndef __pfk_poison_h__
#define __pfk_poison_h__

#pragma GCC poison alloca
#pragma GCC poison asctime
#pragma GCC poison gets
#pragma GCC poison ctime
#pragma GCC poison getdate
#pragma GCC poison gethostbyname
#pragma GCC poison gethostbyname2 
#pragma GCC poison getnetent
#pragma GCC poison getnetbyname
#pragma GCC poison getservent
#pragma GCC poison getservbyname
#pragma GCC poison getservbyport
#pragma GCC poison getpwent
#pragma GCC poison getpwuid
#pragma GCC poison getpwnam
#pragma GCC poison gmtime
#pragma GCC poison localtime
#pragma GCC poison readdir
#pragma GCC poison readdir64
#pragma GCC poison sprintf
#pragma GCC poison strcpy
#pragma GCC poison strerror
#pragma GCC poison strcat
#pragma GCC poison strtok
#pragma GCC poison vsprintf

// memcpy (use memmove)
// or make a memcpy that prints a warning if args overlap

#endif /* __pfk_poison_h__ */
