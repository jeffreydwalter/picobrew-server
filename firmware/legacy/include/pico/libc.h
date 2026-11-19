#ifndef PICO_LIBC_H
#define PICO_LIBC_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

double libc_atof(char *text);
int libc_atoi(char *text);
void *libc_malloc(uint32_t count);
void libc_free(void *allocation);
int libc_memcmp(void *left, void *right, uint32_t count);
void *libc_memcpy(void *destination, void *source, uint32_t count);
void *libc_memmove(void *destination, void *source, uint32_t count);
void *libc_memset(void *destination_2, int value, uint32_t count);
void *libc_realloc(void *allocation, uint32_t count);
int libc_snprintf(char *destination, uint32_t capacity, char *format, ...);
char *libc_strcat(char *destination, char *source);
int libc_strcmp(char *left, char *right);
char *libc_strcpy(char *destination, char *source);
uint32_t libc_strlen(char *text);
int libc_strncmp(char *left, char *right, uint32_t count);
char *libc_strncpy(char *destination, char *source, uint32_t count);
uint32_t libc_strtoul(char *text, char **endPointer, int base);
void *libc_memchr(void *buffer, int value, uint32_t count);

#endif /* PICO_LIBC_H */
