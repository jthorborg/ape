#ifndef RUNTIME_C

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif
typedef void(*PFV)();

EXTERN void runtime_init();
EXTERN void runtime_exit();

#endif