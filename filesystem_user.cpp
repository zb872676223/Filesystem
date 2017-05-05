#include "filesystem.h"

using namespace std;

const int MAXBUF = 2048 * 1000;
kernel_type kernel;
char extrabuffer[MAXBUF];

void testWrite(char *name, char *content){
	int fd = fopen(name, IWRITE);
	//cout << "got fd" << " " << fd <<endl;
	//fseek(fd,sizeof(de_type));
	if (fd >= 0) {
        fwrite(fd, content, strlen(content));
        fclose(fd);
        cout << "�ɹ�д�� " << strlen(content) << "�ֽ�" << endl;
	}else{
        cout << "ERROR�����ʷǷ���" << endl;
	}
}

void testRead(char *name){
	char content[2048];
	memset(content, 0, sizeof(content));
	int fd = fopen(name, IREAD);
	if (fd >= 0) {
        fread(fd, content, 2048);
        fclose(fd);
        cout << content << endl;
	}else{
        cout << "ERROR�����ʷǷ���" << endl;
	}
}

void init(){//��ʼ������
	kernel.openFileCount = 0;
	kernel.freeBufCount = 0;
	for (int i = 0; i < BUFCOUNT; ++i)
		memset(&kernel.buffers[i].b_flags, 0, sizeof(unsigned int));
	for (int i = 0; i < FILECOUNT; ++i)
		memset(&kernel.openFiles[i].i_flag, 0, sizeof(unsigned int));
	loadSuperBlock();
}

void autotest(){//�Զ�����api
    char arg[2048];
	cout << endl;
	cout << "=======================�Զ����Կ�ʼ=======================" << endl;
	cout << "���� ls /" << endl;
	strcpy(arg,"/");
	ls(arg);
	cout << "---------------------------" <<endl;
	cout << "���� mkdir /mkdirtest" << endl;
    strcpy(arg,"/mkdirtest");
    fcreat(arg, IFDIR);
    cout << "/ Ŀ¼��ϢΪ:\n";
    strcpy(arg,"/");
	ls(arg);
	cout << "---------------------------" <<endl;
	cout << "���� mk /mkdirtest/mktest" << endl;
	strcpy(arg,"/mkdirtest/mktest");
	fcreat(arg, 0);
	cout << "/mkdirtest Ŀ¼��ϢΪ:\n";
	strcpy(arg,"/mkdirtest");
	ls(arg);
	cout << "���� mkdir /mkdirtest/mktest2" << endl;
	strcpy(arg,"/mkdirtest/mktest2");
	fcreat(arg, IFDIR);
	cout << "/mkdirtest Ŀ¼��ϢΪ:\n";
	strcpy(arg,"/mkdirtest");
	ls(arg);
	cout << "---------------------------" <<endl;
	cout << "���� write /mkdirtest/mktest Hello World!" << endl;
	strcpy(arg,"/mkdirtest/mktest");
	testWrite(arg, "Hello World!");
	cout << "---------------------------" <<endl;
	cout << "���� read /mkdirtest/mktest" << endl;
	strcpy(arg,"/mkdirtest/mktest");
	testRead(arg);
	cout << "---------------------------" <<endl;
	cout << "���� rm /mkdirtest/mktest"<<endl;
	strcpy(arg,"/mkdirtest/mktest");
	fdelete(arg);
	cout << "/mkdirtest ��Ŀ¼��ϢΪ:\n";
	strcpy(arg,"/mkdirtest");
	ls(arg);
	cout << "---------------------------" <<endl;
	cout << "=======================�Զ����Խ���=======================" << endl;
}

int main(){
	init();
	string cmd = "help", arg1, arg2, now = "/";
	string nnext;
	char src[2048], arg1c[2048], arg2c[2048];
	char ckt[2048];
    printf("=====================================�����ļ�ϵͳ=====================================\n");
    printf("                                                                 ����:������(1352652)\n");
	while (cmd != "exit"){
		if (cmd == "help"){
			printf("ָ�������ע�⣬һ��Ҫʹ��exit�˳�����ʹ�þ���·��������'/'��ͷ\n");
			printf("help                               |ָ����\n");
			printf("autotest                           |�Զ�����\n");
			printf("write     [�ļ���] [�����ַ���]    |���ļ�д����  |ʾ��: write /A/B Hello World\n");
			printf("                                                  |ʾ��: write B Hello World\n");
            printf("read      [�ļ���]                 |���ļ�������  |ʾ����read /A/B\n");
            printf("                                                  |ʾ��: read B\n");
            printf("ls        [Ŀ¼��]                 |��ʾĿ¼������|ʾ����ls /A\n");
            printf("                                                  |ʾ��: ls\n");
            printf("mk        [�ļ���]                 |�����ļ�      |ʾ����mk /A/B\n");
            printf("                                                  |ʾ��: mk B\n");
            printf("mkdir     [Ŀ¼��]                 |����Ŀ¼      |ʾ����mkdir /A\n");
            printf("                                                  |ʾ��: mkdir A\n");
            printf("rm        [�ļ���Ŀ¼��]           |ɾ���ļ���Ŀ¼|ʾ����rm /A/B\n");
            printf("                                                  |ʾ��: rm B\n");
            printf("fin       [�ⲿ�ļ���] [�ڲ��ļ���]|���ⲿϵͳ����|ʾ����fin input.txt /A/B\n");
            printf("                                                  |ʾ��: fin input.txt B\n");
            printf("fout      [�ڲ��ļ���] [�ⲿ�ļ���]|������ⲿϵͳ|ʾ����fout /A/B output.txt\n");
            printf("                                                  |ʾ��: fout B output.txt\n");
            printf("cd        [Ŀ¼�������·����'..'] |���Ĺ���·��  |ʾ����cd /A\n");
            printf("                                                  |ʾ��: cd A\n");
            printf("                                                  |ʾ��: cd ..\n");
		}
		else if (cmd == "autotest"){
			autotest();
		}
		else if (cmd == "write"){
            if (arg1c[0] == '/')
                testWrite(arg1c, arg2c);
            else{
                nnext = now + arg1c;
                strcpy(ckt,nnext.c_str());
                testWrite(ckt, arg2c);
            }
		}
		else if (cmd == "read"){
            if (arg1c[0] == '/')
                testRead(arg1c);
            else{
                nnext = now + arg1c;
                strcpy(ckt,nnext.c_str());
                testRead(ckt);
            }
		}
		else if (cmd == "ls"){
            if (arg1c[0] == '/')
                ls(arg1c);
            else{
                if(now != "/" )
                    nnext = now.substr(0,now.length() - 1);
                else
                    nnext = now;
                strcpy(ckt,nnext.c_str());
                ls(ckt);
            }
		}
		else if (cmd == "mk"){
            if (arg1c[0] == '/')
                fcreat(arg1c, 0);
            else{
                nnext = now + arg1c;
                strcpy(ckt,nnext.c_str());
                fcreat(ckt, 0);
            }
		}
		else if (cmd == "mkdir"){
			if (arg1c[0] == '/')
                fcreat(arg1c, IFDIR);
            else{
                nnext = now + arg1c;
                strcpy(ckt,nnext.c_str());
                fcreat(ckt, IFDIR);
            }
		}
		else if (cmd == "rm"){
            if (arg1c[0] == '/')
                fdelete(arg1c);
            else{
                nnext = now + arg1c;
                strcpy(ckt,nnext.c_str());
                fdelete(ckt);
            }
		}
		else if (cmd == "fin"){
            if (arg2c[0] == '/') {
                FILE *externalFile = fopen(arg1c, "rb");
                fseek(externalFile, 0, SEEK_END);
                int size = ftell(externalFile);
                fseek(externalFile, 0, 0);
                fread(extrabuffer, size, 1, externalFile);
                int wd = fopen(arg2c, IWRITE);
                if (wd != -1) {
                    fwrite(wd, extrabuffer, size);
                    fclose(wd);
                }
                fclose(externalFile);
            }else{
                nnext = now + arg2c;
                strcpy(ckt,nnext.c_str());
                FILE *externalFile = fopen(arg1c, "rb");
                fseek(externalFile, 0, SEEK_END);
                int size = ftell(externalFile);
                fseek(externalFile, 0, 0);
                fread(extrabuffer, size, 1, externalFile);
                int wd = fopen(ckt, IWRITE);
                if (wd != -1) {
                    fwrite(wd, extrabuffer, size);
                    fclose(wd);
                }
                fclose(externalFile);
            }
		}
		else if (cmd == "fout"){
            if (arg1c[0] == '/') {
                int rd = fopen(arg1c, IREAD);
                if (rd != -1) {
                    int flen = kernel.openFiles[rd].i_size;
                    fread(rd, extrabuffer, flen);
                    FILE *extrafile = fopen(arg2c, "wb");
                    fwrite(extrabuffer, flen, 1, extrafile);
                    fclose(extrafile);
                }
                fclose(rd);
            }else{
                nnext = now + arg1c;
                strcpy(ckt,nnext.c_str());
                int rd = fopen(ckt, IREAD);
                if (rd != -1) {
                    int flen = kernel.openFiles[rd].i_size;
                    fread(rd, extrabuffer, flen);
                    FILE *extrafile = fopen(arg2c, "wb");
                    fwrite(extrabuffer, flen, 1, extrafile);
                    fclose(extrafile);
                }
                fclose(rd);
            }
		}
		else if (cmd == "cd") {
            if (arg1c[0] == '/') {
                if (strlen(arg1c) > 1) {
                    if (ck(arg1c))
                        now = string(arg1c) + "/";
                }
                else
                    now = "/";
            }else if(arg1c[0] == '.' && arg1c[1] == '.') {
                if (now != "/") {
                    int k;
                    for (k = now.length() - 2;k > 0;k--) {
                        if(now[k] == '/')
                            break;
                    }
                    now = now.substr(0,k + 1);
                }else
                    cout << "�Ѿ��Ǹ�Ŀ¼�ˣ�" << endl;
            }else{
                nnext = now + arg1c;
                strcpy(ckt,nnext.c_str());
                if (ck(ckt))
                    now = nnext + "/";
            }
		}
		else{
            cout << "ERROR��ָ����ڣ�����help�鿴���" << endl;
		}
		cout << "zhouyuxing(1352652)-FileSystem "<< now << ">";
		cin.getline(src, 2048);
		char *p = src;
		cmd = "";
		arg1 = "";
		arg2 = "";
        while ((*p != ' ') && (*p != 0)){
			cmd += *p;
			++p;
		}
		if (*p!= 0) ++p;
		while ((*p != ' ') && (*p != 0)){
			arg1 += *p;
			++p;
		}
		if (*p!= 0) ++p;
		while ((*p != ' ') && (*p != 0)){
			arg2 += *p;
			++p;
		}
		strcpy(arg1c, arg1.c_str());
		strcpy(arg2c, arg2.c_str());
	}
	releaseBuffers();//�ͷ�ȫ�����棬д�����
}
