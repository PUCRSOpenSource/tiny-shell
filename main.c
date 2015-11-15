#include <stdio.h>
#include <stdlib.h>

#define SECTOR_SIZE	512
#define CLUSTER_SIZE	2*SECTOR_SIZE
#define ENTRY_BY_CLUSTER CLUSTER_SIZE /sizeof(dir_entry_t)
#define NUM_CLUSTER	4096

unsigned short fat[NUM_CLUSTER];
unsigned char boot_block[CLUSTER_SIZE];
unsigned char root_dir[CLUSTER_SIZE];
unsigned char root_dir[CLUSTER_SIZE];
struct _dir_entry_t
{
	unsigned char filenam[18];
	unsigned char attributes;
	unsigned char reserved[7];
	unsigned short first_block;
	unsigned int size;
};
typedef struct _dir_entry_t  dir_entry_t;

union _data_cluster
{
	dir_entry_t dir[CLUSTER_SIZE / sizeof(dir_entry_t)];
};

typedef union _data_cluster data_cluster;


int main(int argc, char *argv[])
{
	FILE* ptr_file;
	ptr_file = fopen("fat.part","w");
	int i;
	for (i = 0; i < CLUSTER_SIZE; ++i) 
		boot_block[i] = 0xbb;
	
	fwrite(&boot_block, sizeof(boot_block), 1,ptr_file);

	fat[0] = 0xfffd;
	for (i = 1; i < 9; ++i)
		fat[i] = 0xfffe;
	
	fat[9] = 0xffff;
	for (i = 10; i < NUM_CLUSTER; ++i)
		fat[i] = 0x0000;
	
	fwrite(&fat, sizeof(fat), 1, ptr_file);

	fwrite(&root_dir, sizeof(root_dir), 1,ptr_file);



	fclose(ptr_file);
	return 0;
}
