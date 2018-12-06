#sources
C_SRCS = \
./DFS1/dfs1.c \
./DFS2/dfs2.c \
./DFS3/dfs3.c \
./DFS4/dfs4.c \
./DFC/dfc.c 


###########################################################################
# Define LINUX compiler flags
CFLAGS=  \
-Wall \
-Werror \
-g
##########################################################################

CC=gcc

SOURCES=$(C_SRCS)

COMPILE: dfs1 dfs2 dfs3 dfs4 dfc

dfs1: DFS1/dfs1.c 
	gcc -Wall DFS1/dfs1.c -o DFS1/dfs1

dfs2: DFS2/dfs2.c 
	gcc -Wall DFS2/dfs2.c -o DFS2/dfs2

dfs3: DFS3/dfs3.c 
	gcc -Wall DFS3/dfs3.c -o DFS3/dfs3

dfs4: DFS4/dfs4.c 
	gcc -Wall DFS4/dfs4.c -o DFS4/dfs4

dfc: DFC/dfc.c 
	gcc -Wall DFC/dfc.c -o dfc

clean: 
	rm -f *.o dfc DFS1/dfs1 DFS2/dfs2 DFS3/dfs3 DFS4/dfs4



