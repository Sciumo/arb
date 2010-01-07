/*
 * ARB kernel interface.
 *
 * This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 *
 */

#ifndef AD_K_PROT_H
#define AD_K_PROT_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* adstring.cxx */
void GBK_dump_backtrace(FILE *out, GB_ERROR error);
void GBK_install_SIGSEGV_handler(bool install);
void GBK_terminate(const char *error);
void GBK_terminatef(const char *templat, ...) __ATTR__FORMAT(1);
GB_ERROR GBK_assert_msg(const char *assertion, const char *file, int linenr);

#else
#error ad_k_prot.h included twice
#endif /* AD_K_PROT_H */
