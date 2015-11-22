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
	//init();
	
	char name[4096];
	fgets(name,4096,stdin);


	//printf("%s", name);		
	if(name[0] == 'a')
	{	
		//printf("%s", name);	
		char append[7];
		char append2[4000];
		int i;
		//printf("%s", name);
		for(i=0; i < 7; ++i)
		{
			//printf("%s", name);	
			append[i] = name[i];
			//printf("Append++\n");					
		}
		//printf("%s", append);
		//printf("%s", name);
		if ( strcmp(append, "append ") == 0 && name[7] == '/')
		{
			//printf("%s", append);
			for(i = 7; i < strlen(name)-1; ++i)
			{
				append2[i-7] = name[i];
			}
			//append(append2);
			//printf("%s", name);
			printf("ChamaAppend\n");			
			printf("%s", append2);
			
        	}

	}
	else if(name[0] == 'c')
	{	
		char create[7];
		char create2[4000];
		int i;	
		for(i=0; i < 7; ++i)
		{
			create[i] = name[i];					
		}		
		if ( strcmp(create, "create ") == 0 && name[7] == '/')
		{		
			for(i = 7; i < strlen(name) - 1; ++i)
			{
				create2[i-7] = name[i];
			}
			//create(create2);
			printf("ChamaCreate");			
			printf("%s", create2);
        	}

	}
	else if(name[0] == 'i')
	{	
		char init[4];	
		int i;	
		for(i=0; i < 4; ++i)
		{
			init[i] = name[i];					
		}		
		if ( strcmp(init, "init") == 0)
		{
			//init(init);
			printf("ChamaInit\n");			
			printf("%s", init);			
        	}

	}
	else if(name[0] == 'l' && name[1] == 'o')
	{	
		char load[4];	
		int i;	
		for(i=0; i < 4; ++i)
		{
			load[i] = name[i];					
		}		
		if ( strcmp(load, "load") == 0)
		{
			//load(load);
			printf("ChamaLoad\n");			
			printf("%s", load);			
        	}

	}
	else if(name[0] == 'l')
	{	
		char ls[3];
		char ls2[4000];
		int i;	
		for(i=0; i < 3; ++i)
		{
			ls[i] = name[i];					
		}		
		if ( strcmp(ls, "ls ") == 0 && name[3] == '/')
		{		
			for(i = 3; i < strlen(name)-1; ++i)
			{
				ls2[i-3] = name[i];
			}
			//ls(ls2);
			printf("ChamaLs\n");			
			printf("%s", ls2);			
        	}

	}
	else if(name[0] == 'm')
	{	
		char mkdir[6];
		char mkdir2[4000];
		int i;	
		for(i=0; i < 6; ++i)
		{
			mkdir[i] = name[i];					
		}		
		if ( strcmp(mkdir, "mkdir ") == 0 && name[6] == '/')
		{		
			for(i = 6; i < strlen(name)-1; ++i)
			{
				mkdir2[i-6] = name[i];
			}
			//mkdir(mkdir2);
			printf("ChamaMkdir\n");			
			printf("%s", mkdir2);			
        	}

	}
	else if(name[0] == 'r')
	{	
		char read[5];
		char read2[4000];
		int i;	
		for(i=0; i < 5; ++i)
		{
			read[i] = name[i];					
		}		
		if ( strcmp(read, "read ") == 0 && name[5] == '/')
		{		
			for(i = 5; i < strlen(name)-1; ++i)
			{
				read2[i-5] = name[i];
			}
			//read(read2);
			printf("ChamaRead\n");			
			printf("%s", read2);			
        	}

	}
	else if(name[0] == 'u')
	{	
		char unlink[7];
		char unlink2[4000];
		int i;	
		for(i=0; i < 7; ++i)
		{
			unlink[i] = name[i];					
		}		
		if ( strcmp(unlink, "unlink ") == 0 && name[7] == '/')
		{		
			for(i = 7; i < strlen(name)-1; ++i)
			{
				unlink2[i-7] = name[i];
			}
			//unlink(unlink2);	
			printf("ChamaUnlink\n");			
			printf("%s", unlink2);		
        	}

	}
	else if(name[0] == 'w')
	{	
		char write[6];
		char write2[4000];
		int i;	
		for(i=0; i < 6; ++i)
		{
			write[i] = name[i];					
		}		
		if ( strcmp(write, "write ") == 0 && name[6] == '/')
		{		
			for(i = 6; i < strlen(name)-1; ++i)
			{
				write2[i-6] = name[i];
			}
			//write(write2);	
			printf("ChamaWrite\n");			
			printf("%s", write2);		
        	}

	}
	
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

	//char* path  = "/usr";
	//mkdir(path);


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
