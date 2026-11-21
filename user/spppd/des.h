/*
 * These functions use the DES algorithm.
 */

/* Setup DES tables according KEY.  */
extern void setkey (const char *__key) __THROW __nonnull ((1));

/* Encrypt data in BLOCK in place if EDFLAG is zero; otherwise decrypt
 *    block in place.  */
extern void encrypt (char *__block, int __edflag) __THROW __nonnull ((1));


