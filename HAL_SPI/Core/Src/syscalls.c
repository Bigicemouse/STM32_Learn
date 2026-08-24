/*
 * Minimal newlib syscall layer for the bare-metal GNU build.
 * The current firmware has no console device, so read/write are placeholders.
 */
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

void _exit(int status)
{
    (void)status;
    while (1)
    {
    }
}

int _close(int file)
{
    (void)file;
    errno = ENOSYS;
    return -1;
}

int _fstat(int file, struct stat* st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _getpid(void)
{
    return 1;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _kill(int pid, int signal)
{
    (void)pid;
    (void)signal;
    errno = EINVAL;
    return -1;
}

int _lseek(int file, int pointer, int direction)
{
    (void)file;
    (void)pointer;
    (void)direction;
    return 0;
}

int _read(int file, char* buffer, int length)
{
    (void)file;
    (void)buffer;
    (void)length;
    return 0;
}

int _write(int file, char* buffer, int length)
{
    (void)file;
    (void)buffer;
    return length;
}
