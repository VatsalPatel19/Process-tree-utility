a2prc: A C Program for Process Tree Management
a2prc is a C program designed to search for a process within a specified process tree, rooted at a given process, and perform various operations based on user-provided options. This program ensures process management and control within the user's environment while preventing issues like fork bombs by adhering to best practices for process creation and termination.

Critical Points to Note
MOST IMPORTANT: While creating process trees using fork(), use reasonable sleep intervals (3-4 minutes) instead of infinite loops to keep processes alive for testing a2prc.
Avoid Fork Bombs: Infinite loops can lead to fork bombs, which are severe. Students using infinite loops will receive a zero grade.
System Administrator Alerts: The system administrator can track users causing fork bombs, leading to potential disciplinary actions.
Clean Up: Before logging out, run killall -u username to terminate any remaining processes.
Synopsis
Basic Usage

a2prc [process_id] [root_process]
Prints the PID and PPID of process_id if it belongs to the process tree rooted at root_process. Otherwise, prints nothing.

process_id: PID of the process to search for.
root_process: PID of the root process.
Options

a2prc [process_id] [root_process] [OPTION]
Performs the specified action on process_id if it belongs to the process tree rooted at root_process. Prints appropriate success or failure messages.

Options:
-rp: Kills process_id if it belongs to the process tree.
-pr: Kills the root_process if valid.
-xn: Lists PIDs of all non-direct descendants of process_id.
-xd: Lists PIDs of all immediate descendants of process_id.
-xs: Lists PIDs of all sibling processes of process_id.
-xt: Pauses process_id with SIGSTOP.
-xc: Sends SIGCONT to all previously paused processes.
-xz: Lists PIDs of all descendants of process_id that are defunct (zombies).
-xg: Lists PIDs of all grandchildren of process_id.
-zs: Prints the status of process_id (Defunct/Not Defunct).
Sample Runs
sh
Copy code
$ a2prc 1009 1004
1009 1005 

$ a2prc 1072 1004 
1072 1071 

$ a2prc 1005 1007 
Does not belong to the process tree 

$ a2prc 1005 1004 -xn 
1010 
1030 
1090 

$ a2prc 1005 1004 -xd 
1008 
1009 
1029 

$ a2prc 1030 1004 -xs 
1090 

$ a2prc 1009 1004 -zs 
Defunct 

$ a2prc 1008 1004 -xg 
1030 
1090 
Additional Requirements
Process Tree Traversal: Uses the nftw() function to recursively visit all files/directories and call user-defined functions.
