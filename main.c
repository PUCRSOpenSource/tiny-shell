#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SECTOR_SIZE	512
#define CLUSTER_SIZE	2*SECTOR_SIZE
#define ENTRY_BY_CLUSTER CLUSTER_SIZE /sizeof(dir_entry_t)
#define NUM_CLUSTER	4096
#define fat_name	"fat.part"
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

unsigned short fat[NUM_CLUSTER];
unsigned char boot_block[CLUSTER_SIZE];
dir_entry_t root_dir[32];
data_cluster clusters[4086];

void init(void)
{
	FILE* ptr_file;
	int i;
	ptr_file = fopen(fat_name,"wb");
	for (i = 0; i < 2; ++i) 
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

	for (i = 0; i < 4086; ++i)
		fwrite(&clusters, sizeof(data_cluster), 1, ptr_file);

	fclose(ptr_file);
}

void load()
{
	FILE* ptr_file;
	int i;
	ptr_file = fopen(fat_name, "rb");
	fseek(ptr_file, sizeof(boot_block), SEEK_SET);
	int bytes = fread(fat, sizeof(fat), 1, ptr_file);
	fclose(ptr_file);
	FILE* load_tst;
	load_tst = fopen("load.tst", "wb");
	fwrite(&fat, sizeof(fat), 1, load_tst);
	fclose(load_tst);
}
void ls()
{
}
void mkdir(char* path)
{
	dir_entry_t* dir_entry = calloc(1, sizeof(dir_entry_t));
}
void create()
{
}
void unlink()
{
}
void write()
{
}
void append()
{
}
void read()
{
}
int main(void)
{
	init();
	char path[] = "/home/djornada/Downloads/ED";
	char token[2] = "/";
	char* dir = strtok(path, token);
	while (dir != NULL) {
		printf("%s -> %s\n", path, dir);
		dir = strtok(NULL, token);
	}
	return 0;
}
