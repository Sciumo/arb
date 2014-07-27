// ============================================================= //
//                                                               //
//   File      : arb_cs.cxx                                      //
//   Purpose   : Basics for client/server communication          //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "arb_cs.h"
#include "arb_msg.h"
#include "arb_pattern.h"
#include <smartptr.h>

#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/tcp.h>


void arb_gethostbyname(const char *name, struct hostent *& he, GB_ERROR& err) {
    he = gethostbyname(name);
    // Note: gethostbyname is marked obsolete.
    // replacement getnameinfo seems less portable atm.

    if (he) {
        err = NULL;
    }
    else {
        err = GBS_global_string("Cannot resolve hostname: '%s' (h_errno=%i='%s')",
                                name, h_errno, hstrerror(h_errno));
    }
}

const char *arb_gethostname() {
    static SmartCharPtr hostname;
    if (hostname.isNull()) {
        char buffer[4096];
        gethostname(buffer, 4095);
        hostname = strdup(buffer);
    }
    return &*hostname;
}

size_t arb_socket_read(int socket, char* ptr, size_t size) {
    size_t to_read = size;
    while(to_read) {
        ssize_t read_len = read(socket, ptr, to_read);
        if (read_len <= 0) { // read failed
            // FIXME: GB_export_error?
            return 0;
        }
        ptr += read_len;
        to_read -= read_len;
    }
    return size;
}

ssize_t arb_socket_write(int socket, const char* ptr, size_t size) {
    size_t to_write = size;

    while (to_write) {
#ifdef LINUX
        ssize_t write_len = send(socket, ptr, to_write, MSG_NOSIGNAL);
#else
        ssize_t write_len = write(socket, ptr, to_write);
#endif
        if (write_len <= 0) { // write failed 
            if (write_len == EPIPE) {
                fputs("pipe broken\n", stderr);
            }
            // FIXME: GB_export_error?
            return -1;
        }
        ptr += write_len;
        to_write -= write_len;
    }
    return size;
}



static GB_ERROR arb_open_unix_socket(char* name, bool do_connect, int *fd);
static GB_ERROR arb_open_tcp_socket(char* name, bool do_connect, int *fd);

/**
 * Opens and prepares a socket
 *
 * If @param name begins with ":", the remainder is shell expanded and 
 * a unix socket is created. If @param contains no ":" it must be numeric,
 * giving the TCPport number to open. If @param contains a ":" in the middle,
 * the first part is considered the hostname and the latter part the port.
 *
 * @param  name          name of port   {[<host>:]<port>|:<filename>} 
 * @param  do_connect    connect if true (client), otherwise bind (server)
 * @param  fd            file descriptor of opened socket (out)
 * @param  filename_out  filename of unix socket (out)
 *
 * @result NULL if all went fine, otherwise error message
 */
GB_ERROR arb_open_socket(const char* name, bool do_connect, int *fd, char** filename_out) {
    if (!name || strlen(name) == 0) {
        return "Error opening socket: empty name";
    }

    GB_ERROR error;
    if (name[0] == ':') {
        // expand variables in path
        char *filename = arb_shell_expand(name+1);
        if (GB_have_error()) {
            return GB_await_error();
        } 

        error = arb_open_unix_socket(filename, do_connect, fd);
        if (error) {
            free(filename);
        } else {
            reassign(*filename_out, filename);
        }
    } 
    else {
        char *socket_name = strdup(name);
        error = arb_open_tcp_socket(socket_name, do_connect, fd);
        free(socket_name);
        freenull(*filename_out);
    }
            
    return error;
}

static GB_ERROR arb_open_unix_socket(char* filename, bool do_connect, int *fd) {   
    // create structure for connect/bind
    sockaddr_un unix_socket;
    unix_socket.sun_family = AF_UNIX;
    if (strlen(filename)+1 > sizeof(unix_socket.sun_path)) {
        return GBS_global_string("Failed to create unix socket: "
                                 "\"%s\" is longer than the allowed %li characters",
                                 filename, sizeof(unix_socket.sun_path));
    }
    strncpy(unix_socket.sun_path, filename, sizeof(unix_socket.sun_path));
    
    // create socket
    *fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (*fd < 0) {
        return GBS_global_string("Failed to create unix socket: %s", strerror(errno));
    }
    
    // connect or bind socket
    if (do_connect) {
        if (connect(*fd, (sockaddr*)&unix_socket, sizeof(sockaddr_un))) {
            return GBS_global_string("Failed to connect unix socket \"%s\": %s",
                                     filename, strerror(errno));
        }
    }
    else {
        // re-use existing file
        int one = 1;
        if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one))){
            fprintf(stderr, "Warning: setsockopt(...REUSEADDR...) failed: %s", strerror(errno));
        }
        if (bind(*fd, (sockaddr*)&unix_socket, sizeof(sockaddr_un))) {
            return  GBS_global_string("Failed to bind unix socket \"%s\": %s",
                                      filename, strerror(errno));
        }
    }

    return NULL;
}

static GB_ERROR arb_open_tcp_socket(char* name, bool do_connect, int *fd) {
    GB_ERROR error = NULL;

    // create socket
    *fd = socket(PF_INET, SOCK_STREAM, 0);
    if (*fd < 0) {
        return GBS_global_string("Failed to create tcp socket: %s", strerror(errno));
    }

    // create sockaddr struct
    sockaddr_in tcp_socket;
    tcp_socket.sin_family = AF_INET;

    struct hostent *he;    
    // get port and host
    char *p = strchr(name, ':');
    if (!p) { // <port>
        tcp_socket.sin_port = htons(atoi(name));
        arb_gethostbyname(arb_gethostname(), he, error);
    }
    else { // <host>:<port>
        tcp_socket.sin_port = htons(atoi(p+1));
        p[0]='\0';
        arb_gethostbyname(name, he, error);
        p[0]=':';
    }
    if (tcp_socket.sin_port == 0) {
        return "Cannot open tcp socket on port 0. Is the port name malformed?";
    }
    if (error) {
        return error;
    }
    memcpy(&tcp_socket.sin_addr, he->h_addr_list[0], he->h_length);

    int one = 1;
    if (do_connect) {
        if (connect(*fd, (sockaddr*)&tcp_socket, sizeof(tcp_socket))) {
            return GBS_global_string("Failed to connect TCP socket \"%s\": %s",
                                     name, strerror(errno));
        }
    }
    else { // no connect (bind)
        if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) {
            fprintf(stderr, "Warning: setsockopt(...REUSEADDR...) failed: %s", strerror(errno));
        }
        if (bind(*fd, (sockaddr*)&tcp_socket, sizeof(tcp_socket))) {
            return GBS_global_string("Failed to bind TCP socket \"%s\": %s",
                                     name, strerror(errno));
        }
    }
    
    if (setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one))) {
        fprintf(stderr, "Warning: setsockopt(...TCP_NODELAY...) failed: %s", strerror(errno));
    }

    return NULL;
}

////////// UNIT TESTS ///////////

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_open_socket() {
    int fd;
    char *filename = NULL;
    
    TEST_EXPECT_NULL(arb_open_socket("32039", false, &fd, &filename));
    TEST_EXPECT(fd>0);
    TEST_EXPECT_NULL(filename);
    TEST_EXPECT_EQUAL(close(fd), 0);

    TEST_EXPECT_NULL(arb_open_socket("localhost:32039", false, &fd, &filename));
    TEST_EXPECT(fd>0);
    TEST_EXPECT_NULL(filename);
    TEST_EXPECT_EQUAL(close(fd), 0);

    TEST_EXPECT_NULL(arb_open_socket(":/tmp/$USER-$$", false, &fd, &filename));
    TEST_EXPECT(fd>0);
    TEST_REJECT_NULL(filename);
    TEST_EXPECT_EQUAL(close(fd), 0);
    TEST_EXPECT_EQUAL(unlink(filename), 0);
    freenull(filename);

    TEST_EXPECT_NULL(system("nc -l 32039&"));
    usleep(10000);
    TEST_EXPECT_NULL(arb_open_socket("32039", true, &fd, &filename));
    TEST_EXPECT(fd>0);
    TEST_EXPECT_NULL(filename);
    TEST_EXPECT_EQUAL(close(fd), 0);

    TEST_EXPECT_NULL(system("nc -l 32039&"));
    usleep(10000);
    TEST_EXPECT_NULL(arb_open_socket("localhost:32039", true, &fd, &filename));
    TEST_EXPECT(fd>0);
    TEST_EXPECT_NULL(filename);
    TEST_EXPECT_EQUAL(close(fd), 0);

    char fname[500], buf[500];
    snprintf(fname, sizeof(fname), "/tmp/test-socket-%hi", getpid());
    snprintf(buf, sizeof(buf), "nc -l -U %s&", fname);
    TEST_EXPECT_NULL(system(buf));
    snprintf(buf, sizeof(buf), ":%s", fname);
    usleep(10000);
    TEST_EXPECT_NULL(arb_open_socket(buf, true, &fd, &filename));
    TEST_EXPECT(fd>0);
    TEST_EXPECT_EQUAL(filename, fname);
    TEST_EXPECT_EQUAL(close(fd), 0);
    TEST_EXPECT_EQUAL(unlink(filename), 0);
    freenull(filename);

    // cannot test do_connect=true w/o server to connect to
    // (entire process is blocking)
}

#endif // UNIT_TESTS


