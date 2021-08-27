#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include "main.h"

char dirpath[PATHLEN];//.../20172626까지의 경로
char checkdirpath[PATHLEN];//check디렉토리까지의 경로를 저장해놓는 문자열 배열
char trashdirpath[PATHLEN]; //trash디렉토리까지 경로를 저장해놓은 문자열배열
char filesdirpath[PATHLEN]; //trash/files디렉토리까지 경로를 저장해놓은 문자열 배열
char infodirpath[PATHLEN]; //trash/info디렉토리까지 경로를 저장해놓은 문자열 배열

char delfilepath[PATHLEN];//삭제할 파일의 경로 문자열을 가리키는 문자열 포인터 함수
char deletefilename[FILENAMELEN];

int check=0; //tree를 만들때 check디렉토리 구조를 출력해야함으로 맨처음에 check를 써줘야하는지 확인해주는 변수
int depth=1; //디렉토리 구조를 트리로 출력할 때 트리 깊이를 계산해주는 함수
int dcheck = 0;
int icheck = 0; //check 디렉토리 안에 파일이 있는지 확인해주는 check 변수
int checkdepth = 0;//check디렉토리의 depth를 계산해주는 변수

int main(void) 
{
	pid_t pid;
	char command[COMMANDLEN];
	memset(command, '\0', COMMANDLEN);
	
	//자식 프로세스 생성
	if((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);

	}
	
	//디몬프로세스를 생성하는 새로운 프로세스를 수행
	if(pid==0){

		//execl(const char *pathname, const char *arg0, ...);
		execl("./ssu_daemon", "ssu_daemon", NULL);

	}
	
	while(1){
		//fflush(stdin);
		getcwd(dirpath, PATHLEN);//현재 작업디렉토리를 #include <dirent.h> 가져옴
		
		//dirpath에 home/.../check 경로 저장
		sprintf(checkdirpath, "%s/%s", dirpath, "check");//checkdirpath에 check 디렉토리까지의 경로 저장
		
		//check 디렉토리의 depth를 계산해주는 함수
		check_dir_depth(dirpath, "check");
		
		memset(command, '\0', COMMANDLEN);                           
		////////////////////////////////////////////////////////////////////////////////////////////////////
		
		printf("20172626>");
		//명령어를 입력받음
		fflush(stdin);
		scanf("%[^\n]s", command);
		getchar();
	
		
		if(command[0] == '\0')
		{//엔터만 입력받을 시 프롬프트 재출력
			continue;
	
		}

		else if(strncmp(command, "delete", 6)==0)
		{
			deleteFile(command);
			icheck=0;
		}

		else if(strncmp(command, "size", 4)==0){
			//printf("%s\n", command);
			printSize(command);
		}

		else if(strncmp(command, "recover", 7)==0){
			recoverFile(command);	
		}
		else if(strncmp(command, "tree", 4)==0){
			makeTree(dirpath,"check");
			//depth=1;
			printf("\n");
			check=0;
			fflush(stdout);
			continue;
		}

		else if(strncmp(command, "exit", 4)==0){
			
			exit(0); //프로그램 종료

		}

		else if(strncmp(command, "help", 4)==0){
			print_usage();
			continue;		
		}
		else{        
			printf("Unknown command %s\n", command);
			continue;
		}
		
		
	}

}

//파일을 삭제하는 함수
void deleteFile(char * command){
	
	int t=0;
	int signo=0;
	time_t rawtime;
	long size;
	char temppath[PATHLEN];
	char token[MAXARGC][MAXLEN];

	struct stat statbuf;
	struct tm timeinfo;
	
	memset(deletefilename, '\0', FILENAMELEN);
	memset(delfilepath, '\0', PATHLEN);
	memset(trashdirpath, '\0', PATHLEN);
	memset(filesdirpath, '\0', PATHLEN);
	memset(infodirpath, '\0', PATHLEN);

	char *ptr ;
	ptr = strtok(command, " ");

	//인자로 전달받은 문자열을 토큰별로 나눠서 token 배열에 저장
	//token[0] : delete
	//token[1] : filename
	//token[2] : delete date
	//token[3] : delete time
	//token[4] : option (r : 지정한 시간에 삭제시 삭제 여부 확인)
	//                    (i : 파일정보를 이동시키지 않고 바로 삭제)
	
	while(ptr != NULL){
		strcpy(token[t], ptr);
		ptr = strtok(NULL, " ");
		t++; 
	}

	if(t < 2){//인자 부족시 에러 처리 (endtime은 안써도 되므로)
		fprintf(stderr, "usage : delete [FILENAME] [END_TIME] [OPTION]\n");
		return ;
	}

	fileExist(dirpath, "check", token[1]);

	sprintf(deletefilename, "%s", token[1]);

	//file이 존재하지 않는 경우, 에러처리후 프롬프트 출력
	if(icheck==0){
		printf("파일이 존재하지 않음\n");
		return ;
	}

	//trash 디렉토리 없을 경우 생성
	sprintf(trashdirpath, "%s/%s", dirpath, "trash");
	
	if(access(trashdirpath,F_OK)<0){
		mkdir(trashdirpath, 0777);
	}

	//trash 디렉토리 하위에 files 디렉토리가 없을 경우 생성
	sprintf(filesdirpath, "%s/%s", trashdirpath, "files");

	if(access(filesdirpath,F_OK)<0){
		mkdir(filesdirpath, 0777);
	}

	//trash 디렉토리 하위에 info 디렉토리가 없을 경우 생성
	sprintf(infodirpath, "%s/%s", trashdirpath, "info");

	if(access(infodirpath,F_OK)<0){
		mkdir(infodirpath, 0777);
	}

	get_info_size();
	
	//info디렉토리의 크기가 정해진 크기(2KB)를 초과할 경우
	
	//시간정보를 주지 않았을 때
	if(t==2){
		delete_right_now();
	}
	//시간정보를 주었을때 지정한 시간에 파일을 삭제
	else if(t==4){
		int d=0; //date
		char date[3][6];
		
		char *ptr2;
		ptr2 = strtok(token[2], "-");

		while(ptr2 != NULL){
			strcpy(date[d], ptr2);
			ptr2 = strtok(NULL, "-");
			d++;
		}//ex)2020-05-05를 data[0] : 2020, data[1] : 05, data[2] :05로 저장

		int c=0; //clock
		char clock[3][3];

		char *ptr3;
		ptr3 = strtok(token[3], ":");

		while(ptr3 != NULL){
			strcpy(clock[c], ptr3);
			ptr3=strtok(NULL, ":");
			c++;
		}//ex)10:00를 clock[0] : "10", clock[1] : "00" 저장

		//문자열 시간을 tidelete_right_nowme_t 으로 변경
		localtime_r(&rawtime, &timeinfo);
		timeinfo.tm_year = atoi(date[0])-1900;//년도
		timeinfo.tm_mon = atoi(date[1])-1;//월
		timeinfo.tm_mday = atoi(date[2]);//일
		timeinfo.tm_hour = atoi(clock[0]);//시
		timeinfo.tm_min = atoi(clock[1]);//분
		timeinfo.tm_sec = atoi(clock[2]);//초

		rawtime=mktime(&timeinfo);

		time_t ntime = time(NULL);
		//삭제할 시각을 과거 시각으로 입력시
		if(ntime>rawtime){
			fprintf(stderr, "지난 시간 입력");
			return;
		}
		else{
			signo = (int)(rawtime-ntime);
			//printf("%d\n", signo);
		}
		signal(SIGALRM, delete_signal_handler);
		//delete_signal_handler 호출
		alarm(signo);
		return;
	}
	
}

//지정된 시간에 파일을 삭제하는 함수
void delete_signal_handler(int signo){
	
	FILE * ifp;
	char oldpath[PATHLEN]; //원래 경로
	char filesnewpath[PATHLEN]; // files디렉토리까지의 경로
	char infonewpath[PATHLEN];
	time_t delete_time = time(NULL);
	time_t modify_time;
	struct tm dt;//delete time
	struct tm mt;//modify time
	struct stat statbuf;

	//fflush(ifp);

	lstat(delfilepath,&statbuf);
	modify_time = statbuf.st_mtime;

	sprintf(oldpath, "%s", delfilepath);
	sprintf(filesnewpath, "%s/%s", filesdirpath, deletefilename);
	rename(oldpath, filesnewpath); //원래 파일경로에서 trash/info디렉토리로 옮기는 코드

	sprintf(infonewpath, "%s/%s", infodirpath, deletefilename);
	ifp = fopen(infonewpath, "w+");
	
	localtime_r(&delete_time, &dt);
	localtime_r(&modify_time ,&mt);
	fprintf(ifp, "[Trash info]\n%s\nD : %04d-%02d-%02d %02d:%02d:%02d\nM : %04d-%02d-%02d %02d:%02d:%02d", delfilepath, 
		dt.tm_year + 1900, dt.tm_mon+1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec, 
		mt.tm_year + 1900, mt.tm_mon+1, mt.tm_mday, mt.tm_hour, mt.tm_min, mt.tm_sec);
	
	fclose(ifp);

}

//시간 차없이 바로 파일을 삭제해주는 함수
void delete_right_now(){
	
	FILE * ifp;
	char oldpath[PATHLEN]; //원래 경로
	char filesnewpath[PATHLEN]; // files디렉토리까지의 경로
	char infonewpath[PATHLEN];
	time_t delete_time = time(NULL);
	time_t modify_time;
	struct tm dt;//delete time
	struct tm mt;//modify time
	struct stat statbuf;

	//fflush(ifp);

	lstat(delfilepath,&statbuf);
	modify_time = statbuf.st_mtime;

	sprintf(oldpath, "%s", delfilepath);
	sprintf(filesnewpath, "%s/%s", filesdirpath, deletefilename);
	rename(oldpath, filesnewpath); //원래 파일경로에서 trash/info디렉토리로 옮기는 코드

	sprintf(infonewpath, "%s/%s", infodirpath, deletefilename);
	ifp = fopen(infonewpath, "w+");
	
	localtime_r(&delete_time, &dt);
	localtime_r(&modify_time ,&mt);
	fprintf(ifp, "[Trash info]\n%s\nD : %04d-%02d-%02d %02d:%02d:%02d\nM : %04d-%02d-%02d %02d:%02d:%02d", delfilepath, 
		dt.tm_year + 1900, dt.tm_mon+1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec, 
		mt.tm_year + 1900, mt.tm_mon+1, mt.tm_mday, mt.tm_hour, mt.tm_min, mt.tm_sec);
	
	fclose(ifp);

}

//info 디렉토리의 사이즈를 구하는 함수
void get_info_size(void){
	
	off_t size=0;
	struct stat statbuf;
	struct dirent **namelist;
	char temppath[PATHLEN];
	while(1){
		int count = scandir(infodirpath, &namelist, *filter, alphasort);
				
		for(int i=0; i<count; i++){
			memset(temppath, '\0', PATHLEN);
			sprintf(temppath, "%s/%s", infodirpath, namelist[i]->d_name);
			stat(temppath, &statbuf);
			size = size + statbuf.st_size;
		}
		
		//info디렉토리 사이즈가 2040(2KB) 가 넘을때
		if(size > 2048){
			first_made_file_delete(count);
			size=0;
			continue;
		}
		else{
			break;
		}
	}
	return;
}

//info디렉토리의 크기가 2KB 초과 시 맨처음 지워진 파일과 파일 정보를 삭제해주는 함수
void first_made_file_delete(int count){
	
	typedef struct fileinfo {
		time_t dt; //deletetime
		char filename[FILENAMELEN];//파일 이름을 저장하는 배열
	} fileinfo;
	fileinfo info[count];
	fileinfo temp;
	struct stat statbuf;
	struct dirent **namelist;
	char filepath[PATHLEN];
	char filespath[PATHLEN];
	char infopath[PATHLEN];

	int ccount = scandir(infodirpath, &namelist, *filter, alphasort);
	for(int i=0; i<ccount; i++){
		
		memset(filepath, '\0', PATHLEN);
		//info 하위의 파일까지의 정보를 저장
		sprintf(filepath, "%s/%s", infodirpath, namelist[i]->d_name);
		lstat(filepath, &statbuf);

		info[i].dt=statbuf.st_mtime;
		strcpy(info[i].filename, namelist[i]->d_name);

	}
	
	for(int i=0; i<count-1; i++){
		for(int j=1; j<count-i; j++){
			if(info[j].dt>info[j+1].dt){
				temp = info[j];
				info[j]=info[j+1];
				info[j+1]=temp;
			}
		}
	}

	/*for(int i=0; i<count; i++){
		printf("\n%d: %ld %s", i, info[i].dt, info[i].filename);
	}*/
	printf("delete : %s\n", info[0].filename);
	sprintf(filespath, "%s/%s", filesdirpath, info[0].filename);
	sprintf(infopath, "%s/%s", infodirpath, info[0].filename);
	unlink(filespath);
	unlink(infopath);

}
void recoverFile(char * command){
	
	memset(trashdirpath, '\0', PATHLEN);
	memset(filesdirpath, '\0', PATHLEN);
	memset(infodirpath, '\0', PATHLEN);

	FILE * rfp; //읽어올 파일에 대한 파일 포인터

	struct dirent **namelist;
	char token[MAXARGC][MAXLEN];
	char infofilepath[PATHLEN];
	char filesfilepath[PATHLEN];
	char saveinfotext[3][50];
	char trash[50];
	char buf[LINELEN];
	char **samefile;
	//saveinfotext[0] : 경로
	//saveinfotext[1] : 삭제시간
	//saveinfotext[2] : 최종 수정시간
	int filecount=0;
	int t=0;
	int num=0;

	sprintf(trashdirpath, "%s/%s", dirpath, "trash");
	sprintf(filesdirpath, "%s/%s", trashdirpath, "files");
	sprintf(infodirpath, "%s/%s", trashdirpath, "info");

	char *ptr ;
	ptr = strtok(command, " ");

	//인자로 전달받은 문자열을 토큰별로 나눠서 token 배열에 저장
	//token[0] : recover
	//token[1] : filename
	//token[2] : option
	
	while(ptr != NULL){
		strcpy(token[t], ptr);
		ptr = strtok(NULL, " ");
		t++; 
	}

	int count =scandir(filesdirpath, &namelist, *filter, alphasort);

	for(int i=0; i<count; i++){
		if(strcmp(namelist[i]->d_name, token[1])==0){
			filecount++; // 파일이 존재하는지 확인 count = 1:존재 count = 0:미존재 count > 1: 중복 존재
		}
	}

	if(filecount==0){
		fprintf(stderr, "There is no '%s' in the 'trash' directory\n", token[1]);
		return ;
	}
	else if(filecount == 1){

		sprintf(infofilepath, "%s/%s", infodirpath, token[1]);
		//printf("%s\n",infofilepath);
		rfp = fopen(infofilepath,"r");
		fseek(rfp, (off_t)0, SEEK_SET);

		fgets(trash, LINELEN, rfp);//첫줄은 무시
		for(int i=0; i<3; i++){
			fgets(saveinfotext[i], LINELEN, rfp);
		}
		saveinfotext[0][strlen(saveinfotext[0])-1]='\0';
		//printf("%s\n", saveinfotext[0]);
		sprintf(filesfilepath, "%s/%s", filesdirpath, token[1]);		
		
		unlink(infofilepath);
		rename(filesfilepath, saveinfotext[0]);
		
	}
	else{//중복으로 파일이 존재할때(동일이름)
			
			samefile = (char **) malloc (sizeof(char*)*filecount);
			
			for(int i=0; i<filecount; i++){
				samefile[i] = (char*)malloc (sizeof(char)* SLINELEN);
			} //출력할 내용을 저장할 버퍼 동적 생성

			for(int i=0; i<count; i++){
				if(strcmp(namelist[i]->d_name, token[1])==0){
					num++;
					sprintf(infofilepath, "%s/%s", infodirpath, token[1]);
					rfp = fopen(infofilepath, "r");
					fseek(rfp, (off_t)0, SEEK_SET);

					fgets(trash, LINELEN, rfp);
					for(int i=0; i<3; i++){
						fgets(saveinfotext[i], LINELEN, rfp);
						saveinfotext[i][strlen(saveinfotext[i])-1]='\0';
					}
					sprintf(samefile[num], "%d. %s %s %s\n", num, token[1], saveinfotext[1], saveinfotext[2]);
				}
			}
			for(int i=0; i<filecount; i++){
				printf("%s\n", samefile[i]);
			}
		}
}

//check디렉토리 구조를 Tree 형태로 출력하는 함수
void makeTree(char * curdirpath, char *dname){
	
	struct stat statbuf;
	struct dirent **namelist;
	char indent[] = {"               "};
	char checkpath[PATHLEN];
	
	//현재 탐색하려는 디렉토리까지의 경로 저장
	//printf("%s %s", curdirpath, dname);
	sprintf(curdirpath, "%s/%s", curdirpath, dname);

	int count  = scandir(curdirpath, &namelist, *filter, alphasort);
	//printf("count :%d", count);


	if(check==0){
		printf("%s",dname);
		check=1;
	}
	
	for(int k=0; k<strlen(indent)-strlen(dname); k++) printf("%c", ' ');
	
	for(int i=0; i<count; i++){
		
		if(i!=0){
			for(int j=0; j<depth; j++){
				printf("%s", indent);
			}
		}
		printf("%s", namelist[i]->d_name);
		sprintf(checkpath, "%s/%s", curdirpath, namelist[i]->d_name);
		lstat(checkpath, &statbuf);
		if(S_ISDIR(statbuf.st_mode)){
			depth++;		
			makeTree(curdirpath, namelist[i]->d_name);
			depth--;
		}
		if(i!=count-1){
			printf("\n\n\n");
		}
	}
	
}

//chech directory에 파일이 존재하는지 확인해주고 있으면 icheck를 1로 변경, 파일의 절대경로를 리턴해주는 함수
void fileExist(char * path, char * dname, char * fname){
	
	struct dirent **namelist;
	struct stat statbuf;
	char curpath[PATHLEN];
	char temppath[PATHLEN];
	char filepath[PATHLEN];
	char * f_ptr;  
	
	memset(filepath, '\0', PATHLEN);
	sprintf(curpath, "%s/%s", path, dname);
		
	int count = scandir(curpath, &namelist, *filter, alphasort);
	
	for(int j=0; j<count; j++){
		
		memset(temppath, '\0', PATHLEN);
		
		sprintf(temppath, "%s/%s", curpath, namelist[j]->d_name);
		
		lstat(temppath, &statbuf);

		if(strcmp(fname, namelist[j]->d_name)==0){
			
			icheck = 1; // 파일을 찾았을 때
			sprintf(delfilepath, "%s", temppath);
			return ;
		}

		if(S_ISDIR(statbuf.st_mode)){
			fileExist(curpath, namelist[j]->d_name, fname);
		}
	}
	return ;

}
//check directory의 depth를 계산해주는 함수
void check_dir_depth(char * path, char * dname){
	
	struct dirent **namelist;
	struct stat statbuf;
	char cdirpath[PATHLEN];
	char newpath[PATHLEN];

	memset(cdirpath, '\0', PATHLEN);
	//디렉토리까지의 경로를 path에 저장
	sprintf(cdirpath, "%s/%s", path, dname);
	
	int count = scandir(cdirpath, &namelist, *filter, alphasort);
	
	if(count==0){
		return ;
	}
	for(int i=0; i<count; i++){
		memset(newpath, '\0', PATHLEN);
		sprintf(newpath, "%s/%s", cdirpath, namelist[i]->d_name);
		lstat(newpath, &statbuf);
		if(S_ISDIR(statbuf.st_mode)){//파일이 디렉토리이면
			checkdepth++;
			check_dir_depth(newpath, namelist[i]->d_name);
		}
		else continue;
	}
	
}

//사이즈를 출력하는 함수
void printSize(char * command){
	
	int t=0, c=0;
	char token[MAXARGC][MAXLEN];
	char *ptr = strtok(command, " ");
	struct stat statbuf;
	struct dirent **namelist;

	while(ptr != NULL){
		strcpy(token[t], ptr);
		ptr = strtok(NULL, " ");
		t++;
	}

	int count = scandir(dirpath, &namelist, *filter, alphasort);

	for(int i=0; i<count ; i++){
		if(strcmp(namelist[i]->d_name, token[1])){
			continue;
		}
		else {
			c=1; break;
		}
	}

	//존재하지 않는 파일을 입력했을 때
	if(c==0){
		printf("존재하지 않는 파일명입니다.\n");
		return;
	}
	
	//받는 인자의 개수가 2개일때
	if(t==2){
		lstat(token[1], &statbuf);
		printf("%ld       ./%s\n", statbuf.st_size, token[1]);
	}

	if(t==4){
		//옵션의 경우
	}

}
	
//파일 디렉토리 명에서 필요 없는 파일을 걸러주는 함수
static int filter(const struct dirent *dirent){
	if(!(strcmp(dirent->d_name, ".")) || !(strcmp(dirent->d_name, ".."))) return 0;
	else if(dirent->d_name[0]=='.'){return 0;}
	else return 1;
}

//프로그램 사용법을 출력하는 명령어
void print_usage(){

	printf("Usage : 20172626> [COMMAND]\n");
	printf("Command : \n");
	printf("------\n");
	printf("DELETE [FILENAME] [END_TIME] [OPTION] : Delete files automatically at specified time\n");
	printf("	Option : \n");
	printf("		-i : Delete files WITHOUT moving deleted files and information to the 'trash' directory\n");
	printf("		-r : When deleting at the specified time, RECONFIRM whether or not to delete\n");
	printf("------\n");
	printf("SIZE [FILENAME] [OPTION] : Print file path and file size\n");
	printf("	Option : \n");
	printf("		-d [NUMBER] : Print up to NUMBER level subdirectories\n");
	printf("------\n");
	printf("RECOVER [FILENAME] [OPTION] : Restore files in 'trash' directory to original path\n");
	printf("	Option : \n");
	printf("		-l : Print files and deletion times under the 'trash' directory in the order of the oldest deletion time\n");
	printf("------\n");
	printf("TREE : Print the structure of the 'check' directory in tree form\n");
	printf("------\n");
	printf("EXIT : Exit program\n");
	printf("------\n");
	printf("HELP : Print usage\n");

}