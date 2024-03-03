#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <sys//stat.h>
#include <unistd.h>
#include <errno.h>

#ifdef PATH_MAX
static long pathmax = PATH_MAX;
#else
static long pathmax = 0;
#endif

static long posix_version = 0;
static long xsi_version = 0;

#define PATH_MAX_GUESS 1024 //In case PATH_MAX is indeterminate (no guarantee that this is adequate)

void err_quit(char *str) {
    fprintf(stderr, "%s\n", str);
    exit(EXIT_FAILURE);
}

typedef int Func(char *, const struct stat *, int); // For convenience, buddy!

static Func myfunc;
static int myftw(char *, Func *);
static int dopath(Func *);
static char *path_alloc(size_t *);
static long nreg, ndir, nblk, nchr, nfifo, nslink, nsock, ntot;

int main(int argc, char *argv[]) {
    int ret;

    if (argc != 2) {
        err_quit("usage of program: ftw <starting-pathname>");
    }
}

enum {FTW_F, FTW_D, FTW_DNR, FTW_NS};
static char *fullpath;
static size_t pathlen;

static int ftw(char *pathname, Func *func) {
    fullpath = path_alloc(&pathlen); // Beginning at root
    
    if (pathlen <= strlen(pathname)) {
        pathlen = strlen(pathname) * 2;
        if ((fullpath = realloc(fullpath, pathlen)) == NULL) {
            err_quit("Couldn't allocate more memory for fullpath");
        }
    }

    strcpy(fullpath, pathname);
    return dopath(func);
}

// Allocates a path placeholder, taking care of the actual POSIX and XSI versions.
// Checking these version numbers helps decide between allocation methods (including or not null byte).
// Without this, malloc-ing a string is sufficient.
static char *path_alloc(size_t *s) {
    char *ptr;
    size_t size;
    if (posix_version == 0) {
        posix_version = sysconf(_SC_VERSION);
    }

    if (xsi_version == 0) {
        xsi_version = sysconf(_SC_XOPEN_VERSION);
    }

    if (pathmax == 0) { // First time through
        errno = 0; // Differtiate error and undeterminate
        if ((pathmax = pathconf("/"/*from root*/, _PC_PATH_MAX)) < 0) {
            if (errno == 0) { // Indeterminate
                pathmax = PATH_MAX_GUESS; 
            } else { // Error
                err_quit("pathconf error while getting info for root");
            }
        }
        else {
            pathmax++; // add one since it is relative to root
        }
    }

    if ((posix_version < 200112L) && (xsi_version < 4)) {
        size = pathmax + 1; // null byte
    } else {
        size = pathmax;
    }

    if ((ptr = malloc(size)) == NULL) {
        err_quit("path allocation error");
    }

    if (s != NULL) { // Assign the length of allocated path if the caller wills for it.
        *s = size;
    }

    return ptr;
}

