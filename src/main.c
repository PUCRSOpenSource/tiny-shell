/*INCLUDE*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*DEFINE*/
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

/*DATA DECLARATION*/
unsigned short fat[NUM_CLUSTER];
unsigned char boot_block[CLUSTER_SIZE];
dir_entry_t root_dir[32];
data_cluster clusters[4086];

/*Function declaration*/
void append(char* path, char* content);
void create(char* path);
void mkdir (char* path);
void read  (char* path);
void unlink(char* path);
int  find_free_space(dir_entry_t* dir);

void init(void)
{
	FILE* ptr_file;
	int i;
	ptr_file = fopen(fat_name,"wb");
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
	fread(fat, sizeof(fat), 1, ptr_file);
	fclose(ptr_file);
}

void save_fat()
{
	FILE* ptr_file;
	int i;
	ptr_file = fopen(fat_name, "r+b");
	fseek(ptr_file, sizeof(boot_block), SEEK_SET);
	fwrite(fat, sizeof(fat), 1, ptr_file);
	fclose(ptr_file);
}

data_cluster* load_cluster(int block)
{
	data_cluster* cluster;
	cluster = calloc(1, sizeof(data_cluster));
	FILE* ptr_file;
	ptr_file = fopen(fat_name, "rb");
	printf("block*sizeof(data_cluster) -> %d\n", block*sizeof(data_cluster));
	fseek(ptr_file, block*sizeof(data_cluster), SEEK_SET);
	fread(cluster, sizeof(data_cluster), 1, ptr_file);
	fclose(ptr_file);
	return cluster;
}

data_cluster* write_cluster(int block, data_cluster* cluster)
{
	FILE* ptr_file;
	ptr_file = fopen(fat_name, "r+b");
	fseek(ptr_file, block*sizeof(data_cluster), SEEK_SET);
	fwrite(cluster, sizeof(data_cluster), 1, ptr_file);
	fclose(ptr_file);
}

data_cluster* find_parent(data_cluster* current_cluster, char* path, int* addr)
{
	char path_aux[strlen(path)];
	strcpy(path_aux, path);
	char* dir_name = strtok(path_aux, "/");
	char* rest     = strtok(NULL, "\0");
	dir_entry_t* current_dir = current_cluster->dir;
	addr = current_dir->first_block;

	int len = sizeof(dir_entry_t)/sizeof(current_dir);
	printf("sizeof(current_dir)->%d\n", sizeof(current_dir));
	printf("sizeof(dir_entry_t)->%d\n", sizeof(dir_entry_t));
	printf("len-> %d\n", len);

	int i;
	while (i < 32) {
		dir_entry_t child = current_dir[i];
		printf("child.filename->%s\n",child.filename);
		if (strcmp(child.filename, dir_name) && rest){
			data_cluster* cluster = load_cluster(child.first_block);
			return find_parent(cluster, strtok(NULL, "\0"), addr);
		}
		else if (strcmp(child.filename, dir_name) && !rest)
			return NULL;
		++i;
	}
	
	if (!rest)
		return current_cluster;

	return NULL;
}

char* get_name(char* path)
{

	char name_aux[strlen(path)];
	strcpy(name_aux, path);

	char* name = strtok(name_aux, "/");
	char* rest = strtok(NULL, "\0");
	if (rest != NULL)
		return get_name(rest);

	return (char*) name;
}

int find_free_space(dir_entry_t* dir)
{
	int i;
	for (i = 0; i < 32; ++i){
		if (!dir->attributes)
			return i;
		dir++;
	}
	return -1;
}

void copy_name(char* target, char* src)
{
	int len = strlen(src);
	int i;
	for (i = 0; i < len; ++i) {
		target[i] = src[i];
	}
}

int search_fat_free_block(void)
{
	load();
	int i;
	for (i = 10; i < 4096; ++i)
		if(!fat[i]){
			fat[i] = -1;
			save_fat();
			return i;
		}
	return 0;
}

void mkdir(char* path)
{
	if(path == "/")
		return;

	int root_addr = 9;
	data_cluster* root_cluster = load_cluster(9);
	data_cluster* cluster_parent = find_parent(root_cluster, path, &root_addr);

	if (cluster_parent){
		int free_position = find_free_space(cluster_parent->dir);
		int fat_block = search_fat_free_block();
		if (fat_block && free_position != -1) {
			char* dir_name = get_name(path);
			copy_name(cluster_parent->dir[free_position].filename, dir_name);
			cluster_parent->dir[free_position].attributes = 1;
			cluster_parent->dir[free_position].first_block = fat_block;
			write_cluster(root_addr, cluster_parent);
		}
	}
	else
		printf("PATH NOT FOUND\n");
}

int main(void)
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
	/*find(root_dir, "/usr");*/
	/*find(root_dir, "/usr/home");*/
	/*printf("get_name(\"%s\") -> %s\n", path ,(char*)get_name(path));*/
	/*printf("get_name(\"%s\") -> %s\n", path2 ,(char*)get_name(path2));*/

	char* path  = "/usr";
	mkdir(path);

	path = "/bin";
	mkdir(path);

	path = "/home";
	mkdir(path);

	path = "/barra";
	mkdir(path);

	path = "/home/djornada";
	mkdir(path);

	/*char* path2 = "/usr/bin/7z";*/
	/*mkdir(path2);*/
	/*mkdir("/");*/
	return 0;
}
