#define STDIN 0
#define STDOUT 1
#define COMMANDLEN 20
#define BUFLEN 1024
#define PATHLEN 1024
#define MAXARGC 5
#define MAXLEN 40
#define FILENAMELEN 15
#define LINELEN 50
#define SLINELEN 100

void makeTree();
void delete_signal_handler(int signo);
void printSize(char * command);
void check_dir_depth(char * path, char * dname);
void deleteFile(char * command);
void fileExist(char * path, char *dname,char *fname);
void delete_right_now();
void first_made_file_delete(int count);
void recoverFile(char * command);
static int filter(const struct dirent *dirent);
void print_usage();
void get_info_size();
