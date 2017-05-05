#include "filesystem.h"

using namespace std;

inode_type getRootInode(){
	inode_d_type dinode;
	readDisk(&dinode, sizeof(sb_type), sizeof(inode_d_type));
	inode_type inode;
	inode.i_mode = dinode.d_mode;
	inode.i_size = dinode.d_size;
	memcpy(inode.i_addr, dinode.d_addr, sizeof(inode.i_addr));
	inode.i_number = 0;
	inode.i_count = 1;
	return inode;
}

inode_type getInode(int ino){
	inode_d_type dinode;
	readDisk(&dinode, sizeof(sb_type) + (ino * sizeof(inode_d_type)), sizeof(inode_d_type));
	inode_type inode;
	inode.i_mode = dinode.d_mode;
	inode.i_size = dinode.d_size;
	memcpy(inode.i_addr, dinode.d_addr, sizeof(inode.i_addr));
	inode.i_number = ino;
	inode.i_count = 1;
	return inode;
}

void setInode(inode_type inode){
	inode_d_type dinode;
	dinode.d_mode = inode.i_mode;
	dinode.d_size = inode.i_size;
	memcpy(dinode.d_addr, inode.i_addr, sizeof(inode.i_addr));
	writeDisk(&dinode, sizeof(sb_type) + (inode.i_number * sizeof(inode_d_type)), sizeof(inode_d_type));
}

buf_type *getBuffer(int blkno, int mode){
	buf_type *p = kernel.freeBufHead;
	for (int i = 0; i < kernel.freeBufCount; ++i){
		if (p->b_blkno == blkno)
			return p;
		p = p->b_forw;
	}
	if (kernel.freeBufCount == BUFCOUNT){ //����Ҳ���
	    p = kernel.freeBufHead;//��Ҫ��ɵ�һ��
		kernel.freeBufHead = kernel.freeBufHead->b_forw;
		//if (p->b_flags & B_ASYNC)
		IOSend(p->data, p->b_blkno * BLOCKSIZE, BLOCKSIZE);
	}else {//�ҿ��õ�
		++kernel.freeBufCount;
		for (int i = 0; i < BUFCOUNT; ++i)
			if (!(kernel.buffers[i].b_flags & B_BUSY)){
				p = &kernel.buffers[i];
				break;
			}
	}
    if (kernel.freeBufCount > 1){//����
		kernel.freeBufTail->b_forw = p;
		p->b_back = kernel.freeBufTail;
		kernel.freeBufTail = p;
	}else{
		kernel.freeBufHead = p;
		kernel.freeBufTail = p;
	}
	//������
	p->b_flags = mode;
	p->b_flags |= B_BUSY;
	p->b_blkno = blkno;
	IORecv(p->data, blkno * BLOCKSIZE, BLOCKSIZE);
	return p;
}

void releaseBuffers(){//�ͷ����л���
	buf_type *p = kernel.freeBufHead;
	for (int i = 0; i < kernel.freeBufCount; ++i){
		IOSend(p->data, p->b_blkno * BLOCKSIZE, BLOCKSIZE);
		p = p->b_forw;
	}
}

int readBlock(void *p, int blkno, int offset, int size){//���������
	buf_type *buf = getBuffer(blkno, B_READ);
	if (offset + size > BLOCKSIZE)
		size = BLOCKSIZE - offset;
	memcpy(p, buf->data + offset, size);
	return size;
}

int writeBlock(void *p, int blkno, int offset, int size){//��д����
	buf_type *buf = getBuffer(blkno, B_READ);
	if (offset + size > BLOCKSIZE)
		size = BLOCKSIZE - offset;
    //cout << "wb "  << blkno <<  " " << offset << ' ' << size <<endl;
	memcpy(buf->data + offset, p, size);
	return size;
}

void readDisk(void *p, int offset, int size){
	int rsize;
	while (size > 0){
		rsize = readBlock(p, offset / BLOCKSIZE, offset % BLOCKSIZE, size);
		size -= rsize;
		offset += rsize;
		p = (char *)p + rsize;
	}
}

void writeDisk(void *p, int offset, int size){
	int wsize;
	while (size > 0){
		wsize = writeBlock(p, offset / BLOCKSIZE, offset % BLOCKSIZE, size);
		size -= wsize;
		offset += wsize;
		p = (char *)p + wsize;
	}
}

int openFile(int ino, int mode){
	if (kernel.openFileCount == FILECOUNT)
		cout << "ERROR:�ļ��������࣡" << endl;
	int fd = 0;
	for (fd = 0; fd < FILECOUNT; ++fd)
		if (!(kernel.openFiles[fd].i_flag & IALLOC))
			break;
	kernel.openFiles[fd] = getInode(ino);
	kernel.openFiles[fd].i_flag |= IALLOC;
	kernel.openFiles[fd].i_offset = 0;
	return fd;
}

int fseek(int fd, int offset){
	kernel.openFiles[fd].i_offset = offset;
	return offset;
}

int getBlockNum(inode_type* node){//�������
	const static int DBS = BLOCKSIZE / sizeof(int);
	int abs = node->i_offset / BLOCKSIZE;
	if (abs < 6){//1��
		return node->i_addr[abs];
	}
	else if (abs < 6 + 2 * DBS){//2��
		int ab = 6 + (abs - 6) / DBS;
		int bb = (abs - 6) % DBS;
		int blkno;
		readBlock(&blkno, node->i_addr[ab], bb * sizeof(int), sizeof(int));
		return blkno;
	}else{//3��
		int t = (abs - 6 - 2 * DBS) / DBS;
		int ab = t / DBS + 8;
		int bb = t % DBS;
		int cb = (abs - 6 - 2 * DBS) % DBS;
		int blkno_, blkno;
		readBlock(&blkno_, node->i_addr[ab], bb * sizeof(int), sizeof(int));
		readBlock(&blkno, blkno_, cb * sizeof(int), sizeof(int));
		return blkno;
	}
	cout << "ERROR : getBlockNum() RE!" << endl;
}

int fread(int fd, void *buffer, int len){
	inode_type *node = &kernel.openFiles[fd];
	if (node->i_size - node->i_offset < len)
		len = node->i_size - node->i_offset;
    //cout << "fread fd = " << fd << endl;
    //cout << "fread len = " << len << endl;
	while (len > 0){
		int blkno = getBlockNum(node);
		int rsize = readBlock(buffer, blkno, node->i_offset % BLOCKSIZE, len);
		buffer = (char *)buffer + rsize;
		node->i_offset += rsize;
		len -= rsize;
	}
	return len;
}

int fwrite(int fd, void *buffer, int len){
	const static int DBS = BLOCKSIZE / sizeof(int);
	//cout << "i_size = " << kernel.openFiles[fd].i_size << endl;
	//cout << "i_offset = " << kernel.openFiles[fd].i_offset << endl;
    //cout << len << endl;
	if (kernel.openFiles[fd].i_size < (kernel.openFiles[fd].i_offset + len)){//���������
	    //cout << "bugoule " << endl;
		int finalsize = kernel.openFiles[fd].i_offset + len;
		int finalabsblkno = (finalsize - 1) / BLOCKSIZE;
		int currentabsblkno = (kernel.openFiles[fd].i_size - 1) / BLOCKSIZE;
		if (kernel.openFiles[fd].i_size == 0)
			currentabsblkno = -1;
		kernel.openFiles[fd].i_size = finalsize;
		while (currentabsblkno < finalabsblkno){
			if (currentabsblkno < 5){//1���͹�
				++currentabsblkno;
				//cout << fd<<  ' ' <<currentabsblkno << "  1111111111111" <<endl;
				kernel.openFiles[fd].i_addr[currentabsblkno] = alloc();
			}
			else if (currentabsblkno < 5 + DBS * 2){ //2��
				int cab = ++currentabsblkno;
				if (cab == 7)
					kernel.openFiles[fd].i_addr[6] = alloc();
				if (cab == 7 + DBS)
					kernel.openFiles[fd].i_addr[7] = alloc();
				int ab = (cab - 6) / DBS;
				int bb = (cab - 6) % DBS;
				int newb = alloc();
				//cout << currentabsblkno << "  22222222222222" <<endl;
				writeBlock(&newb, kernel.openFiles[fd].i_addr[ab], bb * sizeof(int), sizeof(int));
			}
			else{//3��
				int cab = ++currentabsblkno;
				if (cab == 6 + DBS * 2)
					kernel.openFiles[fd].i_addr[8] = alloc();
				if (cab == 6 + DBS * 2 + DBS * DBS)
					kernel.openFiles[fd].i_addr[9] = alloc();
				int ab = (cab - 6 - 2 * DBS) / DBS / DBS + 8;
				int bb = (cab - 6 - 2 * DBS) / DBS % DBS;
				int cb = (cab - 6 - 2 * DBS) % DBS;
				int newb;
				if (cb == 0){
					newb = alloc();
					writeBlock(&newb, kernel.openFiles[fd].i_addr[ab], bb * sizeof(int), sizeof(int));
				}
				else
					readBlock(&newb, kernel.openFiles[fd].i_addr[ab], bb * sizeof(int), sizeof(int));
				int newc = alloc();
				//cout << currentabsblkno << "  33333333333333" <<endl;
				writeBlock(&newc, newb, cb * sizeof(int), sizeof(int));
			}
		}
	}
	inode_type *node = &kernel.openFiles[fd];
	while (len > 0){ //д����
		int blkno = getBlockNum(node);
		int wsize = writeBlock(buffer, blkno, node->i_offset % BLOCKSIZE, len);
		buffer = (char *)buffer + wsize;
		node->i_offset += wsize;
		len -= wsize;
	}
	return len;
}

void fclose(int fd){//�رղ�����inode
	kernel.openFiles[fd].i_flag &= !IALLOC;
	inode_type *node = &kernel.openFiles[fd];
	int blkno = getBlockNum(node);
	setInode(kernel.openFiles[fd]);
}

void readFile(int ino, void *p, int offset, int size){//���ļ�
	int fd = openFile(ino, IREAD);
	//cout << "rdf-fd = " << fd <<endl;
	inode_type *node = &kernel.openFiles[fd];
	//cout << "rdf-ino = " << node->i_number << endl;
	int blkno = getBlockNum(node);
	//cout << "rdf-blkno = " << blkno <<endl;
	fseek(fd, offset);
	fread(fd, p, size);
	fclose(fd);
}

void writeFile(inode_type &is, void *p, int offset, int size){//д�ļ�
	int fd = openFile(is.i_number, IWRITE);
	fseek(fd, offset);
	fwrite(fd, p, size);
    memcpy(&is.i_addr,&kernel.openFiles[0].i_addr,sizeof(int) * 10);
	fclose(fd);
}

inode_type getDirInode(char *name, char *end){//����Ŀ¼��inode
    //cout << "---------------" <<name << endl;
    //cout << "--------------=" << end-name << endl;
	if (*name != '/'){//��ʽ���
		cout <<"ERROR: ��ʽ�Ƿ���" << endl;
		return inode_type();
	}
	++name;
	char *tail = name;
	inode_type inode = getRootInode();
	while ((*tail != '/') && (*tail != 0 ) && (tail != end))//�����βλ��
		++tail;
	char tname[32];
	memset(tname, 0, sizeof(tname));
	de_type direntry;
	direntry.m_ino = inode.i_number;
	while (tail != end){
        //cout << name << ' ' << tail - name << endl;
		strncpy(tname, name, tail - name);//��ȡ����
        tname[tail - name] = '\0';
		int entryCnt = inode.i_size / sizeof(de_type);
		//cout << inode.i_number << endl;
		bool found = false;
		for (int i = 0; i < entryCnt; ++i){//����
			readFile(inode.i_number, &direntry, i * sizeof(de_type), sizeof(de_type));
			//cout << direntry.m_name << ' ' << tname << ' ' << direntry.m_ino << endl;
			if ((strcmp(direntry.m_name, tname) == 0) && (direntry.m_ino != -1)){
				found = true;
				break;
			}
		}
		if ((!found) || (direntry.m_ino == -1)){//�ж�·���Ƿ����
		    inode_type err;
		    err.i_number = -1;
			cout << "ERROR: ·�������ڣ�" << endl;
			return err;
		}
		inode = getInode(direntry.m_ino);
		name = ++tail;
		while ((*tail != '/') && (tail != end))
			++tail;
	}
	return getInode(direntry.m_ino);
}

int fopen(char *name, int mode){
	char *tail = name;
	while (*tail != '\0')
		++tail;
    if (strlen(name) > 1) {
        *tail = '/';
        tail++;
        *tail = '/0';
    }
	inode_type pinode = getDirInode(name, tail + 1);
	if (pinode.i_mode != 0 || pinode.i_number == -1)
        return -1;
	//cout << pinode.i_number << ' ' << ' ' << name << ' ' << tail + 1 << endl;
	return openFile(pinode.i_number, mode);
}

void fcreat(char *name, int mode){
	char *tail = name;
	while (*tail != '\0')
		++tail;
	while (*tail != '/')
		--tail;
	++tail;
	inode_type pinode = getDirInode(name, tail);//�Ҹ��׵�inode
	if (pinode.i_number == -1)
        return;
	de_type direntry;
	int entryCnt = pinode.i_size / sizeof(de_type);
	for (int i = 0; i < entryCnt; ++i){
		readFile(pinode.i_number, &direntry, i * sizeof(de_type), sizeof(de_type));
		if (direntry.m_ino != -1 && (strcmp(direntry.m_name,tail) == 0)){
		    cout << "ERROR�����ļ����Ѿ����ڣ�" <<endl;
		    return;
		}
	}
	direntry.m_ino = ialloc();
	inode_type inode = getInode(direntry.m_ino);
	inode.i_size = 0;
	inode.i_mode = mode;
	inode.i_number = direntry.m_ino;
	strcpy(direntry.m_name, tail);
	writeFile(pinode, &direntry, pinode.i_size, sizeof(de_type));
	pinode.i_size += sizeof(de_type);
	setInode(pinode);
	entryCnt = pinode.i_size / sizeof(de_type);
	readFile(pinode.i_number, &direntry, (entryCnt-1)*sizeof(de_type), sizeof(de_type));
	setInode(inode);
}

void fdelete(char *name){
	char *tail = name;
	while (*tail != '\0')
		++tail;
	while (*tail != '/')
		--tail;
	char tname[30];
	strcpy(tname, tail + 1);
	++tail;
	inode_type pinode = getDirInode(name, tail);//�Ҹ��׵�inode
	if (pinode.i_number == -1)
        return;
	de_type direntry;
	direntry.m_ino = pinode.i_number;
	int entryCnt = pinode.i_size / sizeof(de_type);
	bool found = false;
	int i;
	for (i = 0; i < entryCnt; ++i){//����entry
		readFile(pinode.i_number, &direntry, i * sizeof(de_type), sizeof(de_type));
		if ((strcmp(direntry.m_name, tname) == 0) && (direntry.m_ino != -1)) {
			found = true;
			break;
		}
	}
	if ((!found) || (direntry.m_ino == -1)){
		cout << "ERROR: �Ҳ���ָ��·����" << endl;
		return;
	}
	direntry.m_ino = -1;//DELETED = -1
	writeFile(pinode, &direntry, i * sizeof(de_type), sizeof(de_type));
	inode_type inode = getInode(direntry.m_ino);
	releaseDataBlocks(inode);
	setInode(pinode);
}

bool ck(char *name) {
    char *tail = name;
	while (*tail != '\0')
		++tail;
	while (*tail != '/')
		--tail;
	char tname[30];
	strcpy(tname, tail + 1);
	++tail;
	inode_type pinode = getDirInode(name, tail);//�Ҹ��׵�inode
	if (pinode.i_number == -1)
        return 0;
	de_type direntry;
	direntry.m_ino = pinode.i_number;
	int entryCnt = pinode.i_size / sizeof(de_type);
	bool found = false;
	int i;
	for (i = 0; i < entryCnt; ++i){//����entry
		readFile(pinode.i_number, &direntry, i * sizeof(de_type), sizeof(de_type));
		inode_type ss= getInode(direntry.m_ino);
		if ((strcmp(direntry.m_name, tname) == 0) && (direntry.m_ino != -1) && (ss.i_mode == IFDIR)) {
			found = true;
			break;
		}
	}
	if ((!found) || (direntry.m_ino == -1)){
		cout << "ERROR: �Ҳ���ָ��·����" << endl;
		return 0;
	}
	return 1;
}

void ls(char *name){
	char *tail = name;
	while (*tail != '\0')
		tail++;
    if (strlen(name) > 1) {
        *tail = '/';
        tail++;
        *tail = '/0';
    }
	inode_type inode = getDirInode(name, tail);
	if (inode.i_number == -1)
        return;
	//cout << "fa_i = " << inode.i_number<<endl;
	de_type direntry;
	int entryCnt = inode.i_size / sizeof(de_type);
    //cout << "inode.i_size=" << ' ' << inode.i_size <<endl;
	inode_type t;
	for (int i = 0; i < entryCnt; ++i){
		readFile(inode.i_number, &direntry, i * sizeof(de_type), sizeof(de_type));
		//cout << "direntry.m_ino " << direntry.m_ino << endl;
		inode_type ss= getInode(direntry.m_ino);
		//cout << "INO-BLK" << get(&ss) << endl;
		if (direntry.m_ino != -1){// && strlen(direntry.m_name)> 0) {
		    t = getInode(direntry.m_ino);
            if (t.i_mode == IFDIR)
                cout << '/';
			cout << direntry.m_name << endl;
		}
	}
	int k = 0;
	return;
}

void pushFreeBlockList(int blkno){//����տ�����
	if (blkno == 0)
		return;
	kernel.superBlock.freebl.blkno[++kernel.superBlock.freebl.n] = blkno;
}

void releaseDataBlocks(inode_type inode){//�ͷ�inode�õ���ȫ��
	int p[BLOCKSIZE / sizeof(int)];
	for (int i = 0; i < 10; ++i)
		pushFreeBlockList(inode.i_addr[i]);
}

void loadSuperBlock(){
	readDisk(&kernel.superBlock, 0, sizeof(sb_type));
	int k = 0;
}

int alloc(){
	if (kernel.superBlock.freebl.n == 0){
		int blkno = kernel.superBlock.freebl.blkno[0];
		readBlock(&kernel.superBlock.freebl, blkno, 0, sizeof(fbl_type));
		kernel.superBlock.freebl.blkno[++kernel.superBlock.freebl.n] = blkno;
	}
	return kernel.superBlock.freebl.blkno[kernel.superBlock.freebl.n--];
}

int ialloc(){
	if (kernel.superBlock.freeil.n == 0){
		int blkno = kernel.superBlock.freeil.ino[0];
		readBlock(&kernel.superBlock.freeil, blkno, 0, sizeof(fil_type));
		kernel.superBlock.freebl.blkno[++kernel.superBlock.freebl.n] = blkno;
	}
	return kernel.superBlock.freeil.ino[kernel.superBlock.freeil.n--];
}

void IORecv(void *p, int offset, int size){
	kernel.diskfile = fopen(DISKFILENAME, "rb");
	fseek(kernel.diskfile, offset, 0);
	fread(p, size, 1, kernel.diskfile);
	fclose(kernel.diskfile);
}

void IOSend(void *p, int offset, int size){
	kernel.diskfile = fopen(DISKFILENAME, "rb+");
	fseek(kernel.diskfile, offset, 0);
	fwrite(p, size, 1, kernel.diskfile);
	fclose(kernel.diskfile);
}

