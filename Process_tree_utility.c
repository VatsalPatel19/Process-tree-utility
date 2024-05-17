#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#define MAX_PATH_LEN 256
#define MAX_CHILDREN 100

// *****Here I have written the Function to check if a process belongs to the process tree rooted at the root_process*****
int isaDescendant(pid_t process_id, pid_t root_process) {
    char path[64];
    FILE *fp;
    pid_t parent_pid;
    // To Open the status file for the given procesID
    snprintf(path, sizeof(path), "/proc/%d/status", process_id);
    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("There is a error in opening the file");
        return 0;
    }
    // Read the parent processid from the status file
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "PPid:", 5) == 0) {
            sscanf(line + 6, "%d", &parent_pid);
            break;
        }
    }
    fclose(fp);
    // Traverse the process tree until reaching the root process
    while (parent_pid != root_process && parent_pid != 1) {
        if (parent_pid == process_id) {
            return 1;
        }
        snprintf(path, sizeof(path), "/proc/%d/status", parent_pid);
        fp = fopen(path, "r");
        if (fp == NULL) {
            perror("There is a error in opening the file");
            return 0;
        }
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "PPid:", 5) == 0) {
                sscanf(line + 6, "%d", &parent_pid);
                break;
            }
        }
        fclose(fp);
    }
    return (parent_pid == root_process);
}

// *****Here I have written the Function to kill a process if it belongs to the process tree rooted at root_process*****
void killIfItIsaDescendant(pid_t process_id, pid_t root_process) {
    if (isaDescendant(process_id, root_process)) {
        if (kill(process_id, SIGKILL) == 0) {
            printf("Process %d killed\n", process_id);
        } else {
            perror("There is a error in killing the process");
        }
    }
}

// *****Here I have written the Function to print the PID and PPID of the process_id*****
void printTheProcessInfo(pid_t process_id) {
    char path[64];
    FILE *fp;
    pid_t parent_pid;
    snprintf(path, sizeof(path), "/proc/%d/status", process_id);
    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("There is a error in opening the file");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "PPid:", 5) == 0) {
            sscanf(line + 6, "%d", &parent_pid);
            break;
        }
    }
    fclose(fp);
    printf("%d %d\n", process_id, parent_pid);
}

// *****Here I have written the Function to check if a process is defunct (zombie)******
bool isaDefunctprocess(pid_t process_id) {
    char path[MAX_PATH_LEN];
    FILE *status_file;
    char line[256];
    snprintf(path, sizeof(path), "/proc/%d/status", process_id);
    status_file = fopen(path, "r");
    if (status_file == NULL) {
        perror("There is a error in opening the status file");
        return false; // Error occurred
    }
    while (fgets(line, sizeof(line), status_file)) {
        if (strncmp(line, "State:", 6) == 0) {
            char state;
            sscanf(line + 7, " %c", &state);
            fclose(status_file);
            printf("State of the process %d: %c\n", process_id, state);
            return (state == 'Z');
        }
    }
    fclose(status_file);
    return false;
}

// *****Here I have written the Function to print the status of a process (Defunct/Not Defunct)*****
void printTheProcessStatus(pid_t process_id) {
    int defunct = isaDefunctprocess(process_id);
    if (defunct == 1) {
        printf("Process %d is Defunct/Zombie\n", process_id);
    } else if (defunct == 0) {
        printf("Process %d is Not Defunct/Not Zombie\n", process_id);
    } else {
        printf("There is a error in determining the process status\n");
    }
}

// *****Here I have written the Function to list the non-direct descendants of a process*****
void listTheNonDirectDescendants(pid_t process_id) {
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("There is a error in opening the /proc directory");
        return;
    }
    struct dirent *entry;
    bool found = false; // Flag to track if any non-direct descendants are found
    while ((entry = readdir(proc_dir)) != NULL) {
        // Check if the entry is a directory and represents a process
        if (entry->d_type == DT_DIR) {
            pid_t pid = atoi(entry->d_name);
            if (pid > 0 && pid != process_id && isaDescendant(pid, process_id)) {
                // Check if the process is a non-direct descendant of the given process ID
                bool is_direct_descendant = false;
                char status_file_path[64];
                snprintf(status_file_path, sizeof(status_file_path), "/proc/%d/status", pid);
                if (access(status_file_path, F_OK) == -1) {
                    continue;
                }
                FILE *status_file = fopen(status_file_path, "r");
                if (status_file != NULL) {
                    char line[256];
                    while (fgets(line, sizeof(line), status_file)) {
                        if (strncmp(line, "PPid:", 5) == 0) {
                            pid_t parent_pid;
                            sscanf(line + 6, "%d", &parent_pid);
                            if (parent_pid == process_id) {
                                is_direct_descendant = true;
                                break;
                            }
                        }
                    }
                    fclose(status_file);
                }
                if (!is_direct_descendant) {
                    found = true; // Set the flag to true if at least one non-direct descendant is found
                    printf("%d\n", pid);
                }
            }
        }
    }
    closedir(proc_dir);
    if (!found) {
        printf("There were no non-direct descendants found.\n");
    }
}

// *****Here I have written the Function to list the immediate descendants of a process*****
void listTheImmediateDescendants(pid_t process_id) {
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("There is a error in opening the /proc directory");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            pid_t pid = atoi(entry->d_name);
            if (pid > 0) {
                char proc_path[256];
                snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);
                if (access(proc_path, F_OK) == -1) {
                    continue;
                }
                pid_t parent_pid;
                char status_file_path[256];
                FILE *status_file;
                snprintf(status_file_path, sizeof(status_file_path), "/proc/%d/status", pid);
                status_file = fopen(status_file_path, "r");
                if (status_file == NULL) {
                    continue;
                }
                char line[256];
                while (fgets(line, sizeof(line), status_file)) {
                    if (strncmp(line, "PPid:", 5) == 0) {
                        sscanf(line + 6, "%d", &parent_pid);
                        break;
                    }
                }
                fclose(status_file);
                // Print the PID if it's an immediate descendant of the given process ID
                if (parent_pid == process_id) {
                    printf("%d\n", pid);
                }
            }
        }
    }
    closedir(proc_dir);
}

//*****Here I have written the function to list the sibling process*****
void listTheSiblingProcesses(pid_t process_id) {
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("There is a error in opening the /proc directory");
        return;
    }
    pid_t parent_pid = -1;
    char status_path[MAX_PATH_LEN];
    snprintf(status_path, sizeof(status_path), "/proc/%d/status", process_id);
    FILE *status_file = fopen(status_path, "r");
    if (status_file == NULL) {
        perror("Error opening status file");
        closedir(proc_dir);
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), status_file)) {
        if (strncmp(line, "PPid:", 5) == 0) {
            sscanf(line + 6, "%d", &parent_pid);
            break;
        }
    }
    fclose(status_file);
    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            pid_t pid = atoi(entry->d_name);
            if (pid > 0 && pid != process_id) {
                // Check if the process has the same parent PID as the given process
                char process_status_path[MAX_PATH_LEN];
                snprintf(process_status_path, sizeof(process_status_path), "/proc/%d/status", pid);
                FILE *process_status_file = fopen(process_status_path, "r");
                if (process_status_file != NULL) {
                    pid_t process_parent_pid = -1;
                    while (fgets(line, sizeof(line), process_status_file)) {
                        if (strncmp(line, "PPid:", 5) == 0) {
                            sscanf(line + 6, "%d", &process_parent_pid);
                            break;
                        }
                    }
                    fclose(process_status_file);
                    if (process_parent_pid == parent_pid) {
                        printf("%d\n", pid);
                    }
                }
            }
        }
    }
    closedir(proc_dir);
}

// *****Here I have written the Function to pause a process*****
void pauseTheProcess(pid_t process_id) {
    // Pause the specified process with SIGSTOP
    if (kill(process_id, SIGSTOP) == 0) {
        printf("Process %d is paused\n", process_id);
    } else {
        perror("Error");
    }
}

// *****Here I have written the Function to send SIGCONT to paused processes*****
void sendSIGCONT() {
    // Iterate through each process to check if it is paused and send SIGCONT if it is
    for (pid_t pid = 1; pid < 32768; ++pid) { 
        if (kill(pid, 0) == 0 && kill(pid, SIGCONT) == -1 && errno == ESRCH) {
            if (kill(pid, SIGCONT) == 0) {
                printf("SIGCONT is sent to the process with PID %d\n", pid);
            } else {
                perror("There is a error in sending the SIGCONT");
            }
        }
    }
}

// *****Here I have written the Function to list grandchildren of a process*****
void listTheGrandchildren(pid_t process_id) {
    DIR *dir;
    struct dirent *entry;
    dir = opendir("/proc");
    if (dir == NULL) {
        perror("There is a error in opening the /proc directory");
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        // Skip entries that are not directories or do not represent process IDs
        if (entry->d_type != DT_DIR || !isdigit(*entry->d_name))
            continue;
        // Convert the directory name (process ID) to an integer
        pid_t pid = atoi(entry->d_name);
        // Skip the current process and its immediate children
        if (pid == getpid() || getppid() == pid)
            continue;
        // Check if the process is a grandchild of the given process ID
        char status_path[MAX_PATH_LEN];
        snprintf(status_path, sizeof(status_path), "/proc/%s/status", entry->d_name);
        FILE *status_file = fopen(status_path, "r");
        if (status_file != NULL) {
            char line[256];
            while (fgets(line, sizeof(line), status_file)) {
                if (strncmp(line, "PPid:", 5) == 0) {
                    pid_t parent_pid;
                    sscanf(line + 6, "%d", &parent_pid);
                    if (parent_pid == process_id) {
                        printf("Grandchild PID: %d\n", pid);
                        break;
                    }
                }
            }
            fclose(status_file);
        }
    }
    closedir(dir);
}

//*****main*****
int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc < 3 ) {
        fprintf(stderr, "Usage: %s [process_id] [root_process] [OPTION]\n", argv[0]);
        return 1;
    }
    pid_t process_id = atoi(argv[1]); // Get the process ID from command line argument
    pid_t root_process = atoi(argv[2]);
    if (!isaDescendant(process_id, root_process)) {
        printf("Process %d does not belong to the process tree rooted at %d\n", process_id, root_process);
        return 0;
    }
    if (argc >= 4) {
        char *option = argv[3];
        if (strcmp(option, "-rp") == 0) {
            // Kill the process_id if it belongs to the process tree rooted at root_process
            if (kill(process_id, SIGKILL) == 0) {
                printf("Process %d is killed\n", process_id);
            } else {
                perror("Error");
                return 1;
            }
        } else if (strcmp(option, "-pr") == 0) {
            // Kill the root_process if it is valid
            if (isaDescendant(root_process, 1)) {
                if (kill(root_process, SIGKILL) == 0) {
                    printf("The root process %d is killed\n", root_process);
                } else {
                    perror("Error");
                    return 1;
                }
            } else {
                printf("It is an Invalid root process %d\n", root_process);
            }
        } else if (strcmp(option, "-xd") == 0) {
            // List immediate descendants of the process_id
            printf("The Immediate descendants of the process %d:\n", process_id);
            listTheImmediateDescendants(process_id);
        } else if (strcmp(option, "-xn") == 0) {
            // List non-direct descendants of the process_id
            printf("The Non-direct descendants of the process %d:\n", process_id);
            listTheNonDirectDescendants(process_id);
        } else if (strcmp(option, "-xs") == 0) {
            // List sibling processes of the process_id
            printf("The Sibling processes of the process %d:\n", process_id);
            listTheSiblingProcesses(process_id);
        } else if (strcmp(option, "-xt") == 0) {
            // Pause the process_id
            printf("Paused the process %d\n", process_id);
            pauseTheProcess(process_id);
        } else if (strcmp(option, "-xc") == 0) {
            // Send SIGCONT to paused processes
            printf("Sent SIGCONT to the paused processes\n");
            sendSIGCONT();
        } else if (strcmp(option, "-xg") == 0) {
            // List grandchildren of the process_id
            printf("The Grandchildren of the process %d:\n", process_id);
            listTheGrandchildren(process_id);
        } else if (strcmp(option, "-zs") == 0) {
            // Print the status of process_id (Defunct/ Not Defunct)
            if (isaDefunctprocess(process_id)) {
                printf("The Process %d is a Defunct\n", process_id);
            } else {
                printf("The Process %d is Not a Defunct\n", process_id);
            }
        } else {
            fprintf(stderr, "This is an Invalid option\n");
            return 1;
        }
    } else {
        // No option argument provided
        printTheProcessInfo(process_id);
    }
    return 0;
}
