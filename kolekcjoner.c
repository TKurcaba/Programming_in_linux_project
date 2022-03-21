#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#define FAILURE_EXIT_CODE 11
#define KI_FACTOR 1024
// 1024^2
#define MI_FACTOR 1048576
// 2^16
#define MAX_SHORT 65536
#define BUFFER_SIZE MAX_SHORT // when volume will be too much, we will use buffer
#define SLEEP_TIME 480000000L

struct __attribute__ ((__packed__)) record {
    unsigned short value;
    pid_t pid;
};

struct arguments{
    char* source;
    long long volume;
    char* volume_unit;
    char* block;
    char* successess;
    char* raports;
    int workers;
    int counts[6];
};

struct files{
    int fd_source;
    int fd_successsess;
    int fd_raports;
} Files_default = {-1,-1,-1};

// [0] -- read, [1] -- write
struct pipes{
    int fd_in[2];
    int fd_out[2];
};

typedef struct arguments arguments;
typedef struct files files;
typedef struct pipes pipes;

int unique_numbers = 0;

long long min(long long a, long long b) {
    if(a < b) {
        return a;
    }
    return b;
}

void end_program(int exit_code, files files) {
    if (files.fd_source != -1) {
        close(files.fd_source );
    }
    if (files.fd_successsess != -1) {
        close(files.fd_successsess);
    }
    if (files.fd_raports != -1) {
        close(files.fd_raports );
    }
    exit(exit_code);
}

long long get_number(char* arg, char* unit) {
    if(unit==NULL)
        unit = (char*)malloc(sizeof(char));
    long long result = strtol(arg, &unit, 10);

    // if result is less than 1
    if(result <= 0) {
        return -1;
    }
    if(*unit != '\0'){
        if(*(unit+1) == 'i' && *(unit+2) == '\0'){
            switch(unit[0]){
                case 'K':
                    // if Ki
                    return result * KI_FACTOR;
                case 'M':
                    return result * MI_FACTOR;
                default:
                    return -1;
            }
        }
    }

    return result;
}

// returns argument if all arguments are passed properly
// else return  null
arguments* parse_arguments(int argc, char *argv[]) {
    int opt;
    arguments*  arg = (arguments*) malloc(sizeof(arguments));
    while((opt = getopt(argc, argv, ":d:s:w:f:l:p:")) != -1)
    {
        switch(opt)
        {
            case 'd':
                arg->source = optarg;
                arg->counts[0]++;
                break;
            case 's':
                if((arg->volume = get_number(optarg, arg->volume_unit))==-1){
                    printf("Bad volume argument\n");
                    return NULL;
                }
                arg->counts[1]++;
                break;
            case 'w':
                if(get_number(optarg, NULL)==-1){
                    printf("Bad block argument\n");
                    return NULL;
                }
                arg->block = optarg;
                arg->counts[2]++;
                break;
            case 'f':
                arg->successess = optarg;
                arg->counts[3]++;
                break;
            case 'l':
                arg->raports = optarg;
                arg->counts[4]++;
                break;
            case 'p':
                arg->workers = strtol(optarg, NULL, 10);
                if(arg->workers <1){
                    printf("Bad prac argument\n");
                    return NULL;
                }
                arg->counts[5]++;
                break;
            case ':':
                printf("Missing arg for %c\n", optopt);
                break;
            default:
                break;
        }
    }

    if(arg->counts[0] != 1 || arg->counts[1] != 1  || arg->counts[2] != 1  || arg->counts[3] != 1  || arg->counts[4] != 1  || arg->counts[5] != 1 ) {
        printf( "Program requires exactly 6 arguments: -d -s -w -f -l -p\n");
        return NULL;
    }

    arg->volume *= 2;
    return arg;
}


void create_pipes(pipes* pipes) {
    pipe(pipes->fd_in);
    pipe(pipes->fd_out);
    // setting pipes to non-blocking
    int flags = fcntl(pipes->fd_in[0], F_GETFL, 0);
    fcntl(pipes->fd_in[0], F_SETFL, flags | O_NONBLOCK);
}

int log_creation(int fd, pid_t process) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    char buffer[1024];
    sprintf(buffer, "%ld:%ld creation of %d\n", t.tv_sec, t.tv_nsec, process);
    if((write(fd, buffer, strlen(buffer)))==-1){
        printf("log_creation error\n");
        return -1;
    }
    return 1;
}

int log_termination(int fd, pid_t process, int exit_status) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    char buffer[1024];
    sprintf(buffer, "%ld:%ld termination of %d with exit code %d\n", t.tv_sec, t.tv_nsec, process, exit_status);
    if((write(fd, buffer, strlen(buffer)))==-1){
        printf("log_creation error\n");
        return -1;
    }
    return 1;
}

int create_worker(int fd, char* block, pipes pipes) {
    pid_t chld_pid;
    chld_pid = fork();

    if (!chld_pid) {
        close(pipes.fd_in[0]);
        dup2(pipes.fd_in[1], STDOUT_FILENO);

        close(pipes.fd_out[1]);
        dup2(pipes.fd_out[0], STDIN_FILENO);

        char* args[3];
        args[0] = "./poszukiwacz.out";
        args[1] = block;
        args[2] = 0;
        execv("./poszukiwacz.out", args);
    }
    return log_creation(fd,chld_pid);
}

int  fill_successess_with_zeros(int fd) {
    pid_t zeros_buffer[MAX_SHORT] = {0};
    if((write(fd, zeros_buffer, sizeof(pid_t)*MAX_SHORT))<0){
        printf("write() file success error");
        return -1;
    }else
    return 0;
}

// returns 0 if all files were opened successfully
// else return -1
int open_files(arguments arg, files* files) {
    files->fd_source = open(arg.source, O_RDONLY);
    if(files->fd_source  < 0) {
        return -1;
    }

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    files->fd_successsess= open(arg.successess, O_RDWR | O_TRUNC | O_CREAT, mode);
    if(files->fd_successsess < 0) {
        return -1;
    }

    files->fd_raports = open(arg.raports, O_WRONLY | O_TRUNC | O_CREAT, mode);
    if( files->fd_raports  < 0) {
        return -1;
    }

    return fill_successess_with_zeros(files->fd_successsess);
}

// returns number of read bytes if successfully
// else return -1
int read_source(int fd, char buffer[], long long volume) {
    int r = read(fd, buffer, min( BUFFER_SIZE, volume));
    return r;
}

int write_to_workers(char buffer[], int bytes_to_write, int fd_out[]) {
    int w = write(fd_out[1], buffer, bytes_to_write);
    if(w < 0 && errno == EAGAIN) {
        return 0;
    }
    return w;
}

int save_result(int fd, unsigned short number, pid_t pid) {
    if((  lseek(fd, sizeof(pid_t) * number, SEEK_SET))==-1) {
        return -1;
    }
    pid_t saved;
   if(( read(fd, &saved, sizeof(pid_t)))==-1) {
        return -1;
    }

    if(saved == 0) {
        if((lseek(fd, sizeof(pid_t) * number, SEEK_SET))==-1) {
            return -1;
        }
        if((write(fd, &pid, sizeof(pid_t)))==-1) {
            return -1;
        }
        unique_numbers ++;
    }
    return 0;
}

// returns 0 if read something
// else returns 1
// else return -2 for errors
int read_results(int fd, int fd_in[]) {
    struct record record;
    int r;
    int result = 1;
    while(1) {
        r = read(fd_in[0], &record, sizeof(record));
        if (r < 0)
            break;
        unsigned short number = record.value;
        pid_t pid = record.pid;
        if(save_result(fd, number, pid)){
            printf("Problem in save_result() function.\n");
            return -2;
        }
        result= 0;
    }
    return result;
}

// returns 0 if gathered someone
// else returns 1
int gather_terminated(int fd, char* block, pipes pipes) {
    int status;
    pid_t e_pid;
    int gathered = 1;
    while(1) {
        e_pid = waitpid(-1, &status, WNOHANG);
        if(e_pid == 0) {
            break;
        }
        if(e_pid == -1)
            return -1;
        if(log_termination(fd,e_pid, WEXITSTATUS(status)) ==-1)
            return -1;

        if(WEXITSTATUS(status) <= 10 && (MAX_SHORT * 3) / 4 > unique_numbers) {
            create_worker(fd,block,pipes);
        }
        gathered = 0;
    }
    return gathered;
}

int main(int argc, char *argv[]) {
    arguments* arg = parse_arguments(argc, argv);
    files files = Files_default;
    pipes pipes;

    // parsing the arguments
    if(arg==NULL) {
        end_program(FAILURE_EXIT_CODE, files);
    }

    // creating the pipes
    create_pipes(&pipes);

    // opening necessary files
    if(open_files(*arg, &files) ) {
        end_program(FAILURE_EXIT_CODE,files);
    }

    // creating the workers
    for(int i = 0; i < arg->workers; i++) {
        if((create_worker(files.fd_raports,arg->block,pipes) )==-1)
            end_program(FAILURE_EXIT_CODE, files);
    }

    char buffer[BUFFER_SIZE];
    int r = 0, w,wa=0, gt,pipe=1,bytesAvailable;;
    struct timespec sleep_timespec;
    long long vb = arg->volume;

    while(1) {
        // pipes fill
        ioctl(pipes.fd_out[1], FIONREAD, &bytesAvailable);

        // if buffer is empty
        // reading from file
        if(!bytesAvailable){
            if (r == 0 && arg->volume > 0) {
                r = read_source(files.fd_source, buffer, arg->volume );
                if (r <= 0) {
                    printf("Program cant read data from source file\n");
                    end_program(FAILURE_EXIT_CODE, files);
                }
                wa += r;
                arg->volume -= r;
            }
        }

        // writing to pipe
        if(pipe == 1 && r != 0)
            w = write_to_workers(buffer, r, pipes.fd_out);
        else
            w=0;
        if (w < 0 && (wa - vb) > 0) {
            printf("Program cant send data to workers\n");
            end_program(FAILURE_EXIT_CODE, files);
        }

        // close pipe, to terminate child waiting for data, wa is read data from source
        if ((wa - vb) == 0) {
            pipe=0;
            close(pipes.fd_out[1]);
        }
        r -= w;

        int rr = read_results(files.fd_successsess,pipes.fd_in);

        if(rr == -2){
            end_program(FAILURE_EXIT_CODE, files);
        }
        gt = gather_terminated(files.fd_raports, arg->block,pipes);
        if(gt ==-1){
            end_program(0,files);
        }

        if((rr && gt) == 1) {
            sleep_timespec.tv_sec = 0;
            sleep_timespec.tv_nsec = SLEEP_TIME;
            clock_nanosleep(CLOCK_REALTIME, 0, &sleep_timespec, NULL);
        }
    }
}