#ifndef P_
# if defined(__STDC__) || defined(__cplusplus)
#  define P_(s) s
# else
#  define P_(s) ()
# endif
#else
# error P_ already defined elsewhere
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <aisc_func_types.h>

/* server.c */
char *test_address_valid P_((void *address, long key));
int test_address_valid_end P_((void));
void *my_sig_violation P_((int sig, int code, struct sigcontext *scp, char *addr));
void *aisc_server_sigpipe P_((void));
int aisc_s_read P_((int socket, char *ptr, int size));
int aisc_s_write P_((int socket, char *ptr, int size));
const char *aisc_get_object_names P_((long i));
const char *aisc_get_object_attribute P_((long i, long j));
char *aisc_get_hostname P_((void));
const char *aisc_get_m_id P_((char *path, char **m_name, int *id));
const char *aisc_open_socket P_((char *path, int delay, int do_connect, int *psocket, char **unix_name));
struct Hs_struct *open_aisc_server P_((char *path, int timeout, int fork));
void aisc_s_add_to_bytes_queue P_((char *data, int size));
int aisc_s_send_bytes_queue P_((int socket));
long aisc_talking_get P_((long *in_buf, int size, long *out_buf, int max_size));
void aisc_talking_set_index P_((int *obj, int i));
int aisc_talking_get_index P_((int u, int o));
long aisc_talking_sets P_((long *in_buf, int size, long *out_buf, long *object, int object_type));
long aisc_talking_set P_((long *in_buf, int size, long *out_buf, int max_size));
long aisc_talking_nset P_((long *in_buf, int size, long *out_buf, int max_size));
long aisc_make_sets P_((long *obj));
long aisc_talking_create P_((long *in_buf, int size, long *out_buf, int max_size));
long aisc_talking_copy P_((long *in_buf, int size, long *out_buf, int max_size));
long aisc_talking_find P_((long *in_buf, int size, long *out_buf, int max_size));
long aisc_talking_init P_((long *in_buf, int size, long *out_buf, int max_size));
long aisc_fork_server P_((long *in_buf, int size, long *out_buf, int max_size));
long aisc_talking_delete P_((long *in_buf, int size, long *out_buf, int max_size));
long aisc_talking_debug_info P_((long *in_buf, int size, long *out_buf, int max_size));
int aisc_broadcast P_((struct Hs_struct *hs, int message_type, const char *message));
int aisc_private_message P_((int socket, int message_type, char *message));
int aisc_talking P_((int con));
struct Hs_struct *aisc_accept_calls P_((struct Hs_struct *hs));
void aisc_server_shutdown P_((struct Hs_struct *hs));
int aisc_get_key P_((int *obj));
int aisc_add_destroy_callback P_((aisc_callback_func callback, long clientdata));
void aisc_remove_destroy_callback P_((void));

#ifdef __cplusplus
}
#endif

#undef P_
