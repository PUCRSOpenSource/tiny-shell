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
	unsigned char data[CLUSTER_SIZE];
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

void load(void)
{
	FILE* ptr_file;
	int i;
	ptr_file = fopen(fat_name, "rb");
	fseek(ptr_file, sizeof(boot_block), SEEK_SET);
	fread(fat, sizeof(fat), 1, ptr_file);
	fclose(ptr_file);
}

void save_fat(void)
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

void wipe_cluster(int block)
{
	data_cluster* cluster;
	cluster = calloc(1, sizeof(data_cluster));
	write_cluster(block, cluster);
}
data_cluster* find_parent(data_cluster* current_cluster, char* path, int* addr)
{
	char path_aux[strlen(path)];
	strcpy(path_aux, path);
	char* dir_name = strtok(path_aux, "/");
	char* rest     = strtok(NULL, "\0");

	dir_entry_t* current_dir = current_cluster->dir;

	int i=0;
	while (i < 32) {
		dir_entry_t child = current_dir[i];
		if (strcmp(child.filename, dir_name) == 0 && rest){
			data_cluster* cluster = load_cluster(child.first_block);
			*addr = child.first_block;
			return find_parent(cluster, rest, addr);
		}
		else if (strcmp(child.filename, dir_name) == 0 && !rest){
			*addr = child.first_block;
			return NULL;
		}
		i++;
	}

	if (!rest)
		return current_cluster;

	return NULL;
}

data_cluster* find(data_cluster* current_cluster, char* path, int* addr)
{
	if (!path || strcmp(path, "/") == 0)
		return current_cluster;

	char path_aux[strlen(path)];
	strcpy(path_aux, path);
	char* dir_name = strtok(path_aux, "/");
	char* rest     = strtok(NULL, "\0");

	dir_entry_t* current_dir = current_cluster->dir;

	int i=0;
	while (i < 32) {
		dir_entry_t child = current_dir[i];
		if (strcmp(child.filename, dir_name) == 0){
			data_cluster* cluster = load_cluster(child.first_block);
			*addr = child.first_block;
			return find(cluster, rest, addr);
		}
		i++;
	}
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

void copy_str(char* target, char* src)
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
			copy_str(cluster_parent->dir[free_position].filename, dir_name);
			cluster_parent->dir[free_position].attributes = 1;
			cluster_parent->dir[free_position].first_block = fat_block;
			write_cluster(root_addr, cluster_parent);
		}
	}
	else
		printf("PATH NOT FOUND\n");
}

void ls(char* path)
{
	int root_addr = 9;
	data_cluster* root_cluster = load_cluster(9);
	data_cluster* cluster = find(root_cluster, path, &root_addr);
	int i;
	if (cluster){
		printf("\n");
		for (i = 0; i < 32; ++i){
			if (cluster->dir[i].attributes == 1 || cluster->dir[i].attributes == 2)
				printf("%s\n", cluster->dir[i].filename);
		}
		printf("\n");

	}
	else
		printf("PATH NOT FOUND\n");
}

void create(char* path)
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
			copy_str(cluster_parent->dir[free_position].filename, dir_name);
			cluster_parent->dir[free_position].attributes = 2;
			cluster_parent->dir[free_position].first_block = fat_block;
			write_cluster(root_addr, cluster_parent);
		}
	}
	else
		printf("PATH NOT FOUND\n");
}

void write(char* path, char* content)
{
	int root_addr = 9;
	data_cluster* root_cluster = load_cluster(9);
	data_cluster* cluster = find(root_cluster, path, &root_addr);
	if (cluster){
		copy_str(cluster->data, content);
		write_cluster(root_addr, cluster);
	}
	else
		printf("FILE NOT FOUND\n");

}
int empty(int block)
{
	data_cluster* cluster = load_cluster(block);
	int i;
	for (i = 0; i < 32; ++i)
		if(cluster->dir[i].attributes != 0)
			return 0;

	return 1;
}

void unlink(char* path)
{
	load();
	int root_addr = 9;
	data_cluster* root_cluster = load_cluster(9);
	data_cluster* cluster = find(root_cluster, path, &root_addr);
	if (cluster) {
		int parent_addr = 9;
		data_cluster* root_cluster = load_cluster(parent_addr);
		find_parent(root_cluster, path, &root_addr);
		char* name = get_name(path);
		data_cluster* parent_cluster = load_cluster(root_addr);
		int i;
		for (i = 0; i < 32; ++i) {
			if (strcmp(parent_cluster->dir[i].filename, name)==0) {
				printf("iguais\n");
				printf("%d\n",parent_cluster->dir[i].attributes);
				printf("%d\n", empty(parent_cluster->dir[i].first_block));
				if((parent_cluster->dir[i].attributes == 1 && empty(parent_cluster->dir[i].first_block)) ||
						parent_cluster->dir[i].attributes == 2){
					parent_cluster->dir[i].attributes  = 0;
					parent_cluster->dir[i].first_block = 0;
					wipe_cluster(root_addr);
					fat[root_addr] = 0x0000;
					save_fat();
					printf("FILE REMOVED\n");
				}
				break;
			}
		}
	}
	else
		printf("FILE NOT FOUND\n");

}

void command(void)
{
	char name[4096];
	char nameCopy[4096];
	const char aux[2] = "/";
	char aux2[4096];

	char *token;
	int i;

	fgets(name,4096,stdin);

	strcpy(nameCopy,name);

	token = strtok(name,aux);

	if ( strcmp(token, "append ") == 0 && nameCopy[7] == '/')
	{
		for(i = 7; i < strlen(nameCopy)-1; ++i)
		{
			aux2[i-7] = nameCopy[i];
		}
		append(aux2);
	}
	else if ( strcmp(token, "create ") == 0 && nameCopy[7] == '/')
	{
		for(i = 7; i < strlen(nameCopy)-1; ++i)
		{
			aux2[i-7] = nameCopy[i];
		}
		create(aux2);
	}
	else if ( strcmp(token, "init\n") == 0)
	{
		init();
	}
	else if ( strcmp(token, "load\n") == 0)
	{
		load();
	}
	else if ( strcmp(token, "ls ") == 0 && nameCopy[3] == '/')
	{
		for(i = 3; i < strlen(nameCopy)-1; ++i)
		{
			aux2[i-3] = nameCopy[i];
		}
		ls(aux2);
	}
	else if ( strcmp(token, "mkdir ") == 0 && nameCopy[6] == '/')
	{
		for(i = 6; i < strlen(nameCopy)-1; ++i)
		{
			aux2[i-6] = nameCopy[i];
		}
		mkdir(aux2);
	}
	else if ( strcmp(token, "read ") == 0 && nameCopy[5] == '/')
	{
		for(i = 5; i < strlen(nameCopy)-1; ++i)
		{
			aux2[i-5] = nameCopy[i];
		}
		read(aux2);
	}
	else if ( strcmp(token, "unlink ") == 0 && nameCopy[7] == '/')
	{
		for(i = 7; i < strlen(nameCopy)-1; ++i)
		{
			aux2[i-7] = nameCopy[i];
		}
		unlink(aux2);
	}
	else if ( strcmp(token, "write ") == 0 && nameCopy[6] == '/')
	{
		for(i = 6; i < strlen(nameCopy)-1; ++i)
		{
			aux2[i-6] = nameCopy[i];
		}
		write(aux2);
	}
	else printf("nao foi possivel encontrar o comando digitado");
}

int main(void)
{
	//command();
	init();


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

	path = "/home/djornada/Desktop";
	mkdir(path);

	path = "/home/djornada/Downloads";
	mkdir(path);

	path = "/home/djornada/.vimrc";
	create(path);
	write(path, "linha 1 do vimrc, ahjashdjasd, ashdajksdhakjdh jkahdkjahsjk ajksdh");
	unlink(path);


	/*ls("/");*/
	ls("/home/djornada");

	return 0;
}
