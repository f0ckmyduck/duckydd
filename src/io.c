#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "io.h"
#include "mbuffer.h"
#include "safe_lib.h"

#define PREFIX long_int
#define T long int
#include "mbuffertemplate.h"

#define PREFIX size_t
#define T size_t
#include "mbuffertemplate.h"

// config interpretation functions
static void cleaninput(char* input, const size_t size)
{
    size_t i;

    for (i = 0; i < size; i++) {
        char curr;

        curr = input[i];
        if ((curr <= '9' && curr >= '0') || (curr <= 'Z' && curr >= 'A') || (curr <= 'z' && curr >= 'a') || curr == '/') {
            input[i] = curr;

        } else if (curr == '\0') {
            break;

        } else {
            input[i] = ' ';
        }
    }
    input[i] = '\0';
}

static long int parse_longlong(const char input[], char** end)
{
    long int num;
    errno = 0;

    num = strtol(input, end, 0);
    if (end != NULL) {
        if (*end == input) {
            return -2;
        }
    }

    if ((num == LLONG_MAX || num == LLONG_MIN) && errno == ERANGE) {
        return -3;
    }

    return num;
}

static size_t readfile(int fd, char** output)
{
    char buffer[10];
    size_t readsize = -1;
    size_t size = 0;

    while (readsize != 0) { // read 10 bytes, reallocate the array and then copy them into the array
        char* temp;
        readsize = read(fd, buffer, sizeof(buffer));
        size += readsize;

        temp = realloc(*output, sizeof(char) * size);
        if (!temp) {
            free(*output);
            *output = NULL;
            return 0;
        }
        *output = temp;

        memcpy_s(&(*output)[size - readsize], readsize, buffer, readsize);
    }
    return size;
}

int readconfig(const char path[], struct configInfo* config)
{
    int err = 0;
    size_t size;
    size_t usedsize;
    size_t i;

    char* buffer = NULL;
    char* current = NULL;

    config->maxtime.tv_sec = 0;
    config->maxtime.tv_nsec = 0;
    config->maxcount = -1;

    config->logpath[0] = '\0';

    m_init(&config->blacklist, sizeof(long int));
    config->logkeys = false;
    config->xkeymaps = false;

    if (config->configfd == -1) {
        config->configfd = open(path, O_RDWR); // open the config
        if (config->configfd < 0) {
            ERR("open");
            err = -1;
            goto error_exit;
        }

        {
            struct flock lock;
            lock.l_type = F_WRLCK;
            lock.l_whence = SEEK_SET;

            lock.l_start = 0;
            lock.l_len = 0;

            if (fcntl(config->configfd, F_SETLK, &lock)) { // try to lock the file
                if (errno == EACCES || errno == EAGAIN) {
                    LOG(-1, "Another instance is probably running!\n");
                }
                ERR("fcntl");
                err = -2;
                goto error_exit;
            }
        }
    } else {
        m_free(&config->blacklist);
        lseek(config->configfd, 0, SEEK_SET);
    }

    size = readfile(config->configfd, &buffer);
    if (buffer == NULL) {
        LOG(-1, "readfile failed\n");
        err = -3;
        goto error_exit;
    }

    for (i = 0; i < size; i++) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
        }
    }

    current = buffer;
    usedsize = 0;
    while (size > usedsize) {
        size_t next;
        for (next = 0; next < (size - usedsize); next++) {
            if (current[next] == '\0') {
                usedsize += next + 1;
                break;
            }
        }
        cleaninput(current, next);

        if (!strncmp_ss(current, "maxtime", 7) && config->maxtime.tv_sec == 0 && config->maxtime.tv_nsec == 0) {
            char* end = NULL;

            config->maxtime.tv_sec = parse_longlong(&current[8], &end);
            config->maxtime.tv_nsec = parse_longlong(&end[1], &end);

            if (config->maxtime.tv_sec < 0 || config->maxtime.tv_nsec < 0) {
                LOG(-1, "The option of maxtime is malformed or out of range!\n");
                err = -4;
                goto error_exit;
            }

            LOG(1, "Maxtime set to %lds %ldns\n", config->maxtime.tv_sec,
                config->maxtime.tv_nsec);

        } else if (!strncmp_ss(current, "maxscore", 8) && config->maxcount == -1) {
            config->maxcount = parse_longlong(&current[9], NULL);

            if (config->maxcount < 0) {
                LOG(-1, "The option of maxscore is malformed or out of range!\n");
                err = -5;
                goto error_exit;
            }

            LOG(1, "Maxscore set to %ld\n", config->maxcount);

        } else if (!strncmp_ss(current, "blacklist", 8)) {
            char* end = &current[9];

            long int number;

            number = parse_longlong(&end[1], &end);
            if (number <= KEY_MAX) {
                if (m_append_member_long_int(&config->blacklist, number)) {
                    err = -6;
                    goto error_exit;
                }
                LOG(1, "Added %d to the blacklist\n", number);

            } else {
                LOG(1, "%d is not a valid keyboard scancode!\n");
            }

        } else if (!strncmp_ss(current, "logpath", 7)) {
            strcpy_s(config->logpath, MAX_SIZE_PATH, &current[8]);

            struct stat st;
            if (stat(config->logpath, &st) < 0) {
                if (ENOENT == errno) {
                    if (mkdir(config->logpath, 731)) {
                        ERR("mkdir");
                        err = -7;
                        goto error_exit;
                    }
                    LOG(0, "Created logging directory!\n");
                } else {
                    ERR("stat");
                    err = -8;
                    goto error_exit;
                }

            } else {
                if (S_ISDIR(st.st_mode)) {
                    LOG(1, "Set %s as the path for logging!\n", config->logpath);
                } else {
                    LOG(0, "Logpath does not point to a directory!\n");
                    err = -9;
                    goto error_exit;
                }
            }

        } else if (!strncmp_ss(current, "keylogging", 10)) {
            if (parse_longlong(&current[11], NULL) == 1) {
                config->logkeys = true;
                LOG(1, "Logging all potential attacks!\n");
            }
        } else if (!strncmp_ss(current, "usexkeymaps", 11)) {
            if (parse_longlong(&current[12], NULL) == 1) {
                config->xkeymaps = true;
                LOG(1, "Using x server keymaps!\n");
            }
        }

        current = &buffer[usedsize];
    }

    free(buffer);
    return err;

error_exit:
    if (close(config->configfd)) {
        ERR("close");
    }
    m_free(&config->blacklist);
    free(buffer);
    return err;
}

int handleargs(int argc, char* argv[], struct argInfo* data)
{
    size_t i;
    bool unrecognized = false;
    bool help = false;
    data->configpath[0] = '\0';

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'c':
                if (i + 1 < argc) {
                    strcpy_s(data->configpath, MAX_SIZE_PATH, argv[i + 1]); // config path
                }
                break;

            case 'd':
                daemonize = true;
                break;

            case 'v':
                if (loglvl < 2) {
                    loglvl += 1;
                } else {
                    LOG(0, "Can't increment loglevel any more!\n");
                }
                break;

            case 'h':
                printf("duckydd v%s\n"
                       "Usage: duckydd [Options]\n"
                       "\t\t-c [file]\tSpecify a config file location\n"
                       "\t\t-d\t\tDaemonize the process\n"
                       "\t\t-v\t\tIncrease loglevel\n"
                       "\t\t-h\t\tShow this help section\n\n"
                       "For config options please look at the README.md\n\n",
                    VER);
                help = true;
                break;

            default:
                LOG(0, "%s is not a recognized option. \n", argv[i]);
                break;
            }
        }
    }

    if (unrecognized) {
        LOG(0, "Try -h for a list of supported options\n");
    }

    if (help) {
        return -1;
    } else if (data->configpath[0] == '\0') {
        LOG(0, "Please provide a config location!\n");
        return -1;
    }
    return 0;
}

char* binexpand(uint8_t bin, size_t size)
{
    size_t k;
    char* out;

    out = malloc((size + 1) * sizeof(char));
    if (out == NULL) {
        ERR("malloc");
        return NULL;
    }

    for (k = 0; k < size; k++) {
        if (bin & (0x1 << (size - (k - 1)))) {
            out[k] = '1';
        } else {
            out[k] = '0';
        }
    }
    out[size] = '\0';
    return out;
}

void _logger(short loglevel, const char func[], const char format[], ...)
{
    if (loglevel <= loglvl) {
        va_list args;
        va_start(args, format);

        char appended[MAX_SIZE_FORMAT_STRING + MAX_SIZE_FUNCTION_NAME];
        char prefix;
        FILE* fd;

        switch (loglevel) {
        case -1:
            prefix = '!';
            fd = stderr;
            break;

        default:
            prefix = '*';
            fd = stdout;
            break;
        }

        if (func != NULL) {
            sprintf(appended, "[%c][%s] %s", prefix, func, format);
        } else {
            sprintf(appended, "[%c] %s", prefix, format);
        }
        vfprintf(fd, appended, args);

        va_end(args);
    }
}

// memsave strcmp functions
int strcmp_ss(const char str1[], const char str2[])
{
    size_t i = 0;

    while (str1[i] == str2[i]) {
        if (str1[i] == '\0' || str2[i] == '\0') {
            break;
        }
        i++;
    }

    return str1[i] - str2[i];
}

int strncmp_ss(const char str1[], const char str2[], size_t length)
{
    size_t i = 0;

    while (str1[i] == str2[i] && i >= length) {
        if (str1[i] == '\0' || str2[i] == '\0') {
            break;
        }
        i++;
    }

    return str1[i] - str2[i];
}

const char* find_file(const char* input)
{
    size_t i;

    for (i = strnlen_s(input, MAX_SIZE_PATH); i > 0; i--) { // returns the filename
        if (input[i] == '/') {
            return &input[i + 1];
        }
    }
    return NULL;
}
