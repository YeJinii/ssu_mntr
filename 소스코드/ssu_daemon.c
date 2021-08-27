#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include "ssu_daemon.h"

//check디렉토리 안의 파일 변경사항을 작성해놓을 텍스트 파일명
char *fname = "log.txt"; 
char *dname = "check"; //서브 디렉토리명
struct dirent **namelist; //check디렉토리 안의 파일명을 저장하는 배열
struct dirent **tmpnamelist;//namelist배열과 비교할 배열
char pathname[DPATH_MAX]; // home/.../ssu_mntr
char checkdirpath[DPATH_MAX]; //home/.../ssu_mntr/check
struct stat statbuf;
struct tm t;

int count; //check 디렉토리 안에 존재하는 파일 및 디렉토리 개수
int check = 0;

FILE *lfp;//log.txt 파일 포인터	

typedef struct list {
	time_t mt;//mtime
	char name[NAMELEN];// file/directory name
} List;
List l[FILENUM];//파일 이름과 mtime을 저장하는 구조체 배열

int main(void)
{
	pid_t pid;
	pid = getpid();
	//daemon process 생성
	if(ssu_daemon_init() < 0) {
		fprintf(stderr, "ssu_daemon_init failed\n");
		exit(1);
	}
	exit(0);
}

//백그라운드에서 실행될 daemon process를 생성하는 함수
int ssu_daemon_init(void) {

	int fd, maxfd;
	pid_t pid;
	char pathlog[DPATH_MAX];

	getcwd(pathname, DPATH_MAX); //현재 작업 디렉토리의 경로를 저장해놓음
	sprintf(pathlog, "%s/%s", pathname, "log.txt");

	
	//자식 프로세스 생성 
	if((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	//백그라운드 수행(부모 수행 종료)
	else if (pid != 0)
		exit(0);

	pid = getpid();
		
	setsid();

	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();

	for(fd = 0; fd < maxfd; fd++)
		close(fd);

	umask(0);
	
	chdir("/");
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);

	lfp= fopen(pathlog ,"a+"); //log.txt파일 생성
	setbuf(lfp, NULL);
	
	sprintf(checkdirpath, "%s/%s", pathname, dname);
	//작업 디렉토리 변경 (/ssu_mntr)
	chdir(checkdirpath);
	//맨 처음에 check 디렉토리 안의 파일명을 배열에 저장해주는 함수		
	check_dir_file();
	
	while(1){
		//exit(0);	
		record_file_change();

		sleep(1);
	}

	return 0;
	//현재 탐색하려는 디렉토리까
}



//맨 처음에 check 디렉토리 안의 파일을 확인하는 함수 
void check_dir_file(){
	
	int idx;
	
	//check 디렉토리까지의 경로를 설정
	
	
	//check 디렉토리 안의 파일과 디렉토리를 scan하고 오름차순으로 목록을 namelist 배열에 저장한다.	
	count  = scandir(checkdirpath, &namelist, *filter, alphasort);
	
	//구조체 배열에 파일||디렉토리 이름과 mtime 저장
	for(idx=0; idx<count; idx++){
		strcpy(l[idx].name, namelist[idx]->d_name);
		stat(namelist[idx]->d_name, &statbuf);
		l[idx].mt=statbuf.st_mtime;
		
	}

	return ;

}

//".", ".."은 빼고 나머지 파일명을 배열에 저장하는 필터 함수
static int filter(const struct dirent *dirent){
	if(!(strcmp(dirent->d_name, ".")) || !(strcmp(dirent->d_name, ".."))) return 0;
	else if(dirent->d_name[0]=='.'){return 0;}
	else return 1;
}

//지정한 디렉토리 안의 파일의 변경사항을 log.txt에 기록하는 함수
void record_file_change(void){
	
	int ccount;//current count
	
	//check 디렉토리 안의 파일과 디렉토리 명을 다시 불러옴
	ccount  = scandir(checkdirpath, &tmpnamelist, *filter, alphasort);
	
	if(ccount >= count){ccheck(ccount);}
	else{
		dcheck(ccount);	
	}
}

//파일 생성(creat)을 check 하는 함수
void ccheck(int ccount){

	
	//struct stat statbuf2;
	for(int i=0; i<count; ){
		for(int j=0; j<ccount; ){
			if(strcmp(l[i].name, tmpnamelist[j]->d_name)!=0){ //파일명이 동일하지 않을 때
				stat(tmpnamelist[j]->d_name, &statbuf); //새로운 파일이 있는 것이므로 시간을 측정하고
				localtime_r(&statbuf.st_mtime, &t); 
				//log.txt파일에 작성
				fprintf(lfp, "[%d-%d-%d %02d:%02d:%02d][create_%s]\n",
					t.tm_year + 1900, t.tm_mon+1 , t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tmpnamelist[j]->d_name);
				fflush(lfp);
				for(int k=count; k>j; k--){
					memset(l[k].name, '\0', strlen(l[k].name));
					memcpy(l[k].name,l[k-1].name, strlen(l[k-1].name));
					l[k].mt=l[k-1].mt;
				}
				//새로운 파일이 생겼으므로 구조체 배열에 추가
				memset(l[i].name, '\0', strlen(l[i].name));
				memcpy(l[i].name, tmpnamelist[j]->d_name, strlen(tmpnamelist[j]->d_name));
				l[i].mt = statbuf.st_mtime;
				count = ccount;
				return ;
			}
			else{//파일명이 동일할 때
				stat(tmpnamelist[j]->d_name, &statbuf);
				if(l[i].mt!=statbuf.st_mtime){
					localtime_r(&statbuf.st_mtime, &t);
					fprintf(lfp, "[%d-%d-%d %02d:%02d:%02d][modify_%s]\n",
					t.tm_year + 1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tmpnamelist[j]->d_name);
					fflush(lfp);
					l[i].mt = statbuf.st_mtime; //최종 수정시간 변경
					return;
				}
				else {
					i++; 
					j++;
				}
			}		
		}
	}
	
	return ;

}


//파일 삭제를 check 하는 함수
void dcheck(int ccount){

	time_t dtime;
	for(int i=0; i<count; ){
		for(int j=0; j<ccount; ){
			if(strcmp(l[i].name, tmpnamelist[j]->d_name)!=0){ //파일명이 동일하지 않을 때
				dtime = time(NULL);
				//log.txt파일에 작성
				localtime_r(&dtime, &t);
				fprintf(lfp, "[%d-%d-%d %02d:%02d:%02d][delete_%s]\n",
					t.tm_year + 1900, t.tm_mon+1 , t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, l[i].name);
				fflush(lfp);
				for(int k=i; k<ccount; k++){
					memset(l[k].name, '\0', strlen(l[k].name));
					memcpy(l[k].name,l[k+1].name, strlen(l[k+1].name));
					l[k].mt=l[k+1].mt;
				}
				//파일이 삭제되었음으로 구조체 배열의 맨 마지막칸 초기화
				memset(l[ccount].name, '\0', strlen(l[i].name));
				
				l[ccount].mt = 0;
				count = ccount;
				return ;
			}
			else {
				i++;
				j++;
			}
					
		}
	}
	return ;
}