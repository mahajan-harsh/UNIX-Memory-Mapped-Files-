# UNIX-Memory-Mapped-Files-
A C program that manages a set of shared resources using UNIX memory mapped files. The resources are in res.txt
The res.txt is the form where the first column is the type of resource and second is the number of units of that resource.
alloc.c implements a resource allocator program. It opens the res.txt and maps it to a memor region using mmap(). It then deducts resources from respective resource type (if available) and then invokes system call, msync() to synchronise the content of mapped file with the physical file.
prov-rep.c creates two concurrent processes - the parent process adds resources to the respective resource type and teh child process reports the state of the resources every 10 seconds. 
