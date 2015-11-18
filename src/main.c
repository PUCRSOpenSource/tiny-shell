
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
	unsigned char filename[18];
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

void
init(void)
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

void
load()
{
	FILE* ptr_file;
	int i;
	ptr_file = fopen(fat_name, "rb");
	fseek(ptr_file, sizeof(boot_block), SEEK_SET);
	fread(fat, sizeof(fat), 1, ptr_file);
	fread(root_dir, sizeof(root_dir), 1, ptr_file);
	fclose(ptr_file);
}
void
ls()
{
}

void
mkdir(char path[])
{
	dir_entry_t* dir_entry = calloc(1, sizeof(dir_entry_t));
}

dir_entry_t*
find(dir_entry_t* current_dir, char* path)
{
	char path_aux[strlen(path)];
	strcpy(path_aux, path);
	char* dir_name = strtok(path_aux, "/");
	char* rest     = strtok(NULL, "\0");
	dir_entry_t* child = &current_dir[0];

	int len = sizeof(current_dir)/sizeof(dir_entry_t);

	int i;
	for (i = 0; i < len; ++i) {
		child = &current_dir[i];
		if (child->filename == dir_name && rest)
			return find(child, strtok(NULL, "\0"));
		else if (child->filename == dir_name && !rest)
			return NULL;

	}
	if (!rest)
		return current_dir;
		
	return NULL;
	/*dir = strtok(NULL, "\0");*/
}

void
create()
{
}

void
unlink()
{
}

void
write()
{
}

void
append()
{
}

void
read()
{
}

int
main(void)
{
	init();
	/*load();*/
	/*char path[] = "/usr";*/
	/*char* dir = strtok(path, "/");*/
	/*printf("%s\n", dir);*/
	/*dir = strtok(NULL, "\0");*/
	/*printf("%d\n", NULL == dir);*/
	/*printf("%s\n", dir);*/
	/*dir = strtok(dir, "/");*/
	/*printf("%s\n", dir);*/
	find(root_dir, "/usr");
	find(root_dir, "/usr/home");
	return 0;
}
