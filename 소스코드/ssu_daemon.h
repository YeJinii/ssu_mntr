#define DIRECTORY_SIZE MAXNAMLEN
#define DPATH_MAX 1024
#define NAMELEN 20
#define FILENUM 1024

int ssu_daemon_init(void);
void check_dir_file();
void record_file_change();
void ccheck(int ccount);
void mcheck(char * name);
void dcheck(int ccount);
static int filter(const struct dirent *dirent);