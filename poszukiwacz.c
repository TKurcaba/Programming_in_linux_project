#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FAILURE_EXIT_CODE 11
#define KI_FACTOR 1024
// 1024^2
#define MI_FACTOR 1048576
// 2^16
#define MAX_SHORT 65536

char seen[MAX_SHORT];
int unique_numbers = 0;

struct __attribute__ ((__packed__)) record {
    unsigned short value;
    pid_t pid;
};

// returns 1 if standard input is a pipe
// else returns 0
int input_is_pipe() {
    struct stat buf;

    if(fstat(STDIN_FILENO, &buf)) {
        return 0;
    }

    if(!S_ISFIFO(buf.st_mode)) {
        return 0;
    }

    return 1;
}

// returns -1 if argument is incorrect
// else returns number with applied unit
long long get_number(char* arg) {
    char* unit;
    long long result = strtol(arg, &unit, 10);

    // if size is less than 0
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
// returns -1 if error occurred
// else returns 0
int first_occurance(unsigned short number) {
    unique_numbers ++;
    struct record rec;
    rec.value = number;
    rec.pid = getpid();
    if(write(STDOUT_FILENO, &rec, sizeof(rec)) < 0) {
        return -1;
    }
    return 0;
}

// returns -1 if error occurred
// else returns 0
int read_number() {
    unsigned short number;
    if(read(STDIN_FILENO, &number, 2) != 2) {
        return -1;
    }

    if(!seen[number]) {
        if(first_occurance(number)==-1) {
            return -1;
        }
    }
    seen[number] = 1;
    return 0;
}

int get_exit_code(long long input_size) {
    double factor = 1.;
    for(int i = 0; i < 10; i ++) {
        if(unique_numbers >= factor * input_size) {
            return i;
        }
        factor -= 0.1;
    }
    return 10;
}

int main(int argc, char *argv[]) {
    // Checking if input is pipe
    if(!input_is_pipe()) {
        exit(FAILURE_EXIT_CODE);
    }

    if(argc < 2) {
        exit(FAILURE_EXIT_CODE);
    }
    long long input_size = get_number(argv[1]);
    if(input_size < 0) {
        exit(FAILURE_EXIT_CODE);
    }

    for(long long i = 0; i < input_size; i ++) {
        if(read_number()==-1){
            exit(FAILURE_EXIT_CODE);
        }
    }

    exit(get_exit_code(input_size));
}