#ifdef UNICODE
#undef UNICODE
#endif
#include <windows.h>
#include <sql.h>
#include <sqlext.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_QUERY_LEN 500
#define MAX_VARCHAR_LEN 30
#define BOOK_NOT_BORROWED 1
#define BOOK_NOT_FOUND 2
#define ID_NOT_FOUND 3
#define BOOK_BORROWED 4
#define STOCK_EMPTY 5

#define LINKSTART(DSN,UID,PASSWD) \
	HENV env;\
	HDBC conn;\
	SQLAllocEnv(&env);\
	assert(env);\
	SQLAllocConnect(env, &conn);\
	assert(conn);\
	SQLConnect(conn, (SQLCHAR*) DSN, SQL_NTS, (SQLCHAR*) UID, SQL_NTS,\
		(SQLCHAR*) PASSWD, SQL_NTS);\
	\
	HSTMT stmt;\
	SQLAllocStmt(conn, &stmt);\
	assert(stmt);


#define LINKEND \
	SQLFreeStmt(stmt, SQL_DROP);\
	SQLDisconnect(conn);\
	SQLFreeConnect(conn);\
	SQLFreeEnv(env);\

#define BindStringCol(i,attr) SQLBindCol(stmt, i,SQL_C_CHAR,attr,MAX_VARCHAR_LEN,(SQLLEN *)NULL)
#define BindIntCol(i,attr) SQLBindCol(stmt, i,SQL_C_LONG,&attr,0,(SQLLEN *)0)
#define BindFloatCol(i,attr) SQLBindCol(stmt, i,SQL_C_FLOAT,&attr,0,(SQLLEN *)0)

void string_process(char* s,const char* NTS);
SQLRETURN search_books(HSTMT stmt);
void get_date(char* date);
SQLRETURN FindBorrowedBooks(int id,HSTMT stmt);
SQLRETURN BorrowBooks(int id,char* bno,char* LastIn,SQLHDBC conn,SQLHENV env,HSTMT stmt);
SQLRETURN ReturnBooks(int id,char* bno,SQLHDBC conn,SQLHENV env,HSTMT stmt);
SQLRETURN AddLicence(int id,char*name,char* workplace,char*type,SQLHDBC conn,SQLHENV env,HSTMT stmt);
void PrintBookInfo(HSTMT stmt);
SQLRETURN add_book_benches(const char* filename,HSTMT stmt);
SQLRETURN Add_Book(HSTMT stmt);
int empty_str(char*s);
void time_add(char* date,int d);
SQLRETURN ShowAllBooks(HSTMT stmt);
SQLRETURN test(HSTMT stmt,HDBC conn,HENV env);
SQLRETURN DropLicence(int id,SQLHDBC conn,SQLHENV env,HSTMT stmt);
SQLRETURN Interface_addlicence(HSTMT stmt,HDBC conn,HENV env);
SQLRETURN Interface_borrowbooks(HSTMT stmt,HDBC conn,HENV env);
SQLRETURN Interface_returnbooks(HSTMT stmt,HDBC conn,HENV env);
SQLRETURN Interface_updatelicence(HSTMT stmt,HDBC conn,HENV env);

int book_exist(char* bno,HSTMT stmt);
int ID_exist(int id,HSTMT stmt);
void Diag(HSTMT stmt,int i);
char* next_arg(char* buf);
char* strip(char*p);

char trash[MAX_QUERY_LEN];

int main(){
	LINKSTART("LIBRARY","root","Zlz193507"){
		RETCODE error=test(stmt,conn,env);
	}
	LINKEND
	return 0;
}

void prompt(){
	puts("");
	puts("0.help");
	puts("1.create a licence");
	puts("2.borrow a book");
	puts("3.delete a licence");
	puts("4.show all books");
	puts("5.find borrowed book");
	puts("6.return a book");
	puts("7.add a new book");
	puts("8.add book benches");
	puts("9.search book");
	puts("10.update a licence");
	puts("and enter any other words to quit");
}

SQLRETURN test(HSTMT stmt,HDBC conn,HENV env){
	int cmd;
	int id;
	char buf[MAX_QUERY_LEN];
	RETCODE error=SQL_SUCCESS;

	puts("Welcome to mini library system!");
	puts("Author: Zhu Lizhen");
	puts("-------------------------------");
	puts("Number of Options:");
	prompt();
	

	while(1){
		puts("\nenter your option:");
		if(!scanf("%d",&cmd))cmd=-1;
		switch(cmd){
			case 0:
				prompt();continue;
			case 1:
				error=Interface_addlicence(stmt,conn,env);
				break;
			case 2:
				error=Interface_borrowbooks(stmt,conn,env);
				break;
			case 3:
				puts("your id:");
				if(!scanf("%d",&id)){
					fgets(trash,MAX_QUERY_LEN,stdin);
					puts("invalid id!");
					error=SQL_ERROR;
				}
				else if(!ID_exist(id,stmt)){
					puts("id not exist!");
					error=SQL_ERROR;
				}
				else error=DropLicence(id,conn,env,stmt);
				break;				
			case 4:
				error=ShowAllBooks(stmt);
				SQLFreeStmt(stmt,SQL_UNBIND);
				break;
			case 5:
				puts("your id:");
				if(!scanf("%d",&id)){
					fgets(trash,MAX_QUERY_LEN,stdin);
					puts("invalid id!");
					error=SQL_ERROR;
				}
				else
				if(!ID_exist(id,stmt)){
					puts("id not exist!");
					error=SQL_ERROR;
				}
				else error=FindBorrowedBooks(id,stmt);
				SQLFreeStmt(stmt,SQL_UNBIND);
				break;
			case 6:
				error=Interface_returnbooks(stmt,conn,env);
				break;
			case 7:
				getchar();
				puts("\nbefore insert");
				ShowAllBooks(stmt);
				SQLFreeStmt(stmt,SQL_UNBIND);

				error=Add_Book(stmt);

				puts("\nafter insert");
				ShowAllBooks(stmt);
				SQLFreeStmt(stmt,SQL_UNBIND);

				break;
			case 8:
				getchar();
				puts("\nbefore insert");
				ShowAllBooks(stmt);
				SQLFreeStmt(stmt,SQL_UNBIND);

				puts("\nenter the filename");
				gets(buf);
				error=add_book_benches(buf,stmt);

				puts("\nafter insert");
				ShowAllBooks(stmt);
				SQLFreeStmt(stmt,SQL_UNBIND);
				break;
			case 9:
				getchar();
				error=search_books(stmt);
				if(error==SQL_ERROR) puts("invalid input!");
				break;
			case 10:
				getchar();
				error=Interface_updatelicence(stmt,conn,env);
				break;
			default: puts("\nbye!");return error;
		}
		if(error==SQL_SUCCESS) puts("\nsuccess!");
	}
}

SQLRETURN Interface_borrowbooks(HSTMT stmt,HDBC conn,HENV env){
	char buf[MAX_VARCHAR_LEN];
	char bno[MAX_VARCHAR_LEN];
	int id;
	RETCODE error;

	puts("your id:");
	if(!scanf("%d",&id)){
		puts("invalid input!");
		fgets(trash,MAX_QUERY_LEN,stdin);
		return SQL_ERROR;
	};
	getchar();
	puts("book id:");
	gets(bno);

	error=BorrowBooks(id,bno,buf,conn,env,stmt);
	switch(error){
		case BOOK_NOT_FOUND: puts("book not found!");break;
		case ID_NOT_FOUND: puts("id not found"); break;
		case BOOK_BORROWED: puts("your have borrowed this book!"); break;
		case STOCK_EMPTY:
			puts("This book has been borrowed out.");
			time_add(buf,2);
			printf("You can borrow the book at %s at the soonest\n",buf);
		default: break;
	}
	return error;
}

SQLRETURN Interface_returnbooks(HSTMT stmt,HDBC conn,HENV env){
	char bno[MAX_VARCHAR_LEN];
	int id;
	RETCODE error;

	puts("your id:");
	if(scanf("%d",&id)==0){
		puts("invalid input!");
		fgets(trash,MAX_QUERY_LEN,stdin);
		return SQL_ERROR;
	}
	getchar();
	puts("book id:");
	gets(bno);

	error=ReturnBooks(id,bno,conn,env,stmt);
	switch(error){
		case BOOK_NOT_FOUND: puts("book not found!");break;
		case ID_NOT_FOUND: puts("id not found!"); break;
		case BOOK_NOT_BORROWED: puts("your haven't borrowed this book yet!"); break;
		default: break;
	}
	return error;
}

SQLRETURN Interface_addlicence(HSTMT stmt,HDBC conn,HENV env){
	char name[MAX_VARCHAR_LEN]="no name";
	char type[MAX_VARCHAR_LEN];
	char workplace[MAX_VARCHAR_LEN]="no place";
	int id;
	int typeid;

	RETCODE error=SQL_SUCCESS;

	puts("new id:");
	if(scanf("%d",&id)==0){
		puts("invalid id!");
		fgets(trash,MAX_QUERY_LEN,stdin);
		return SQL_ERROR;
	};
	if(ID_exist(id,stmt)){
		puts("id exist already!");
		return SQL_ERROR;
	}
	getchar();
	puts("put your name:");
	gets(name);
	if(empty_str(name)) {puts("invalid input!");return SQL_ERROR;}
	puts("your work place:");
	gets(workplace);
	if(empty_str(name)) {puts("invalid input!");return SQL_ERROR;}
	puts("licence type:(default student)");
	puts("1.student");
	puts("2.teacher");
	puts("3.administrator");
	if(!scanf("%d",&typeid)){puts("invalid input!");fgets(trash,MAX_VARCHAR_LEN,stdin);return SQL_ERROR;}
	switch(typeid){
		case 1:strcpy(type,"student"); break;
		case 2:strcpy(type,"teacher"); break;
		case 3:strcpy(type,"administrator"); break;
		default: strcpy(type,"student"); break;
	}
	return AddLicence(id,name,workplace,type,conn,env,stmt);
}

void string_process(char* s,const char* NTS){
	char* p=s;
	int i=0;
	while(*p){
		for(i=0;NTS[i];i++){
			if(*p==NTS[i]) break;
		}
		if(NTS[i]) *p=' ';
		p++;
	}
}

int empty_str(char*s){return !s||!*s;}

SQLRETURN search_books(HSTMT stmt){
	char buf[MAX_QUERY_LEN];
	char sqlquery[MAX_QUERY_LEN]="select * from books where";

	char prompt[][MAX_VARCHAR_LEN]={"bno","category","title","press","year","author","price","total","stock"};
	int param_type[]={SQL_C_CHAR,SQL_C_CHAR,SQL_C_CHAR,SQL_C_CHAR,SQL_C_LONG,SQL_C_CHAR,SQL_C_FLOAT,SQL_C_LONG,SQL_C_LONG};

	int i;
	int valid=FALSE;
	int first_param=TRUE;
	int i_from,i_to;
	float f_from,f_to;

	char* p;

	puts("please fill the query conditions.\n");
	puts("you can press enter to pass some arguments.\n");
	for(i=1;i<7;i++){
		printf("%s?\n",prompt[i]);
		gets(buf);
		if(!buf[0]) continue;
		valid=TRUE;
		if(first_param) first_param=FALSE;
		else sprintf(sqlquery,"%s and",sqlquery);
		switch(param_type[i]){
			case SQL_C_CHAR:
				sprintf(sqlquery,"%s %s=\"%s\"",sqlquery,prompt[i],buf);
				break;
			case SQL_C_LONG:
				if(p=strstr(buf,",")){
					*p=' ';
					if(sscanf(buf,"%d%d",&i_from,&i_to)<2){
						return SQL_ERROR;
					}
					sprintf(sqlquery,"%s %s between %d and %d",sqlquery,prompt[i],i_from,i_to);
				}
				else sprintf(sqlquery,"%s %s=%s",sqlquery,prompt[i],buf);
				break;
			case SQL_C_FLOAT:
				if(p=strstr(buf,",")){
					*p=' ';
					if(sscanf(buf,"%f%f",&f_from,&f_to)<2){
						return SQL_ERROR;
					}
					sprintf(sqlquery,"%s %s between %f and %f",sqlquery,prompt[i],f_from,f_to);
				}
				else sprintf(sqlquery,"%s %s=%s",sqlquery,prompt[i],buf);
				break;
			default: valid=FALSE; break;
		}
	}
	if(!valid){
		p=strstr(sqlquery,"where");
		if(p)*p='\0';
	}

	// puts(sqlquery);

	if(SQLExecDirect(stmt,(SQLCHAR*)sqlquery,SQL_NTS)==SQL_SUCCESS)
		PrintBookInfo(stmt);
	else return SQL_ERROR;
	return SQL_SUCCESS;
}

void get_date(char* date){
    time_t t;
    struct tm* T;
    time(&t);
    T=localtime(&t);
    sprintf(date,"%d-%d-%d %d:%d:%d",T->tm_year+1900,T->tm_mon+1,T->tm_mday,T->tm_hour,T->tm_min,T->tm_sec);
}

SQLRETURN FindBorrowedBooks(int id,HSTMT stmt){
	if(!ID_exist(id,stmt))
		return ID_NOT_FOUND;
	SQLPrepare(stmt,(SQLCHAR*)"\
		select * from books\
		where bno in(\
			select bno\
			from borrow_record\
			where id= ? and indate is null\
		);",SQL_NTS);
	SQLBindParameter(stmt,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&id,0,(SQLLEN *)0);
	if(SQLExecute(stmt)==SQL_SUCCESS) PrintBookInfo(stmt);
	else return SQL_ERROR;
	return SQL_SUCCESS;
}

int ID_exist(int id,HSTMT stmt){
	char buf[MAX_QUERY_LEN];
	RETCODE error;
	sprintf(buf,"select id from licence where id = %d;",id);
	SQLExecDirect(stmt,buf,SQL_NTS);
	error=SQLFetch(stmt);
	SQLCloseCursor(stmt);
	return error==SQL_SUCCESS;
}

int book_exist(char* bno,HSTMT stmt){
	char buf[MAX_QUERY_LEN];
	RETCODE error;
	sprintf(buf,"select* from books where bno = '%s';",bno);
	SQLExecDirect(stmt,buf,SQL_NTS);
	error=SQLFetch(stmt);
	SQLCloseCursor(stmt);
	return error==SQL_SUCCESS;
}

int book_is_borrowed_by(int id,char* bno,HSTMT stmt){
	char buf[MAX_QUERY_LEN];
	RETCODE error;
	sprintf(buf,"select* from borrow_record where id = %d and bno='%s' and indate is null;",id,bno);
	SQLExecDirect(stmt,buf,SQL_NTS);
	error=SQLFetch(stmt);
	SQLCloseCursor(stmt);
	return error==SQL_SUCCESS;	
}

void time_add(char* date,int d){
	char buf[MAX_VARCHAR_LEN];
	char*p=strstr(date,"-");
	int m;
	p++;
	char*q=strstr(p,"-");
	strcpy(buf,q);
	*q='\0';
	sscanf(p,"%d",&m);
	m+=d;
	sprintf(p,"%d",m);
	strcat(p,buf);
}

SQLRETURN BorrowBooks(int id,char* bno,char* LastIn,SQLHDBC conn,SQLHENV env,HSTMT stmt){
	int stock;
	char buf[MAX_QUERY_LEN];
	char date[MAX_VARCHAR_LEN];
	long long len=SQL_NTS;
	short len1=MAX_QUERY_LEN;
	RETCODE error;

	if(!ID_exist(id,stmt)) return ID_NOT_FOUND;

	SQLPrepare(stmt,(SQLCHAR*)"\
	select stock from books\
	where bno = ? ;",SQL_NTS);

	SQLBindParameter(stmt,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,MAX_VARCHAR_LEN,0,bno,MAX_VARCHAR_LEN,(SQLLEN *)&len);
	if(SQLExecute(stmt)==SQL_SUCCESS){
		BindIntCol(1,stock);
		error=SQLFetch(stmt);
		if(error!=SQL_SUCCESS){
			printf("bno:%s\n",bno);
			return BOOK_NOT_FOUND;
		}
		SQLCloseCursor(stmt);
	}
	else return SQL_ERROR;

	SQLFreeStmt(stmt,SQL_UNBIND);

	if(book_is_borrowed_by(id,bno,stmt)) return BOOK_BORROWED;

	if(stock>0){
		get_date(date);

		sprintf(buf,"\
			update books\
			set stock=stock-1\
			where bno= \"%s\";",
		bno);

		SQLSetConnectOption(conn,SQL_AUTOCOMMIT,0);
		if(SQLExecDirect(stmt,buf,SQL_NTS)!=SQL_SUCCESS){
			Diag(stmt,1);
			SQLSetConnectOption(conn,SQL_AUTOCOMMIT,1);
			return SQL_ERROR;
		}

		sprintf(buf,
		"insert borrow_record\
		values(\'%s\',%d,\'%s\',null);",bno,id,date);

		if(SQLExecDirect(stmt,buf,SQL_NTS)!=SQL_SUCCESS){
			Diag(stmt,1);
			error=SQL_ERROR;
			SQLTransact(env,conn,SQL_ROLLBACK);
		}
		else SQLTransact(env,conn,SQL_COMMIT);

		SQLSetConnectOption(conn,SQL_AUTOCOMMIT,1);
		*LastIn='\0';
	}
	else{
		sprintf(buf,
			"select min(outdate)\
			from borrow_record\
			where bno='%s' and indate is null;"
		,bno);
		if(SQLExecDirect(stmt,buf,SQL_NTS)!=SQL_SUCCESS){
			Diag(stmt,1);
			return SQL_ERROR;
		}
		BindStringCol(1,LastIn);
		SQLFetch(stmt);
		SQLCloseCursor(stmt);
		error=STOCK_EMPTY;
	}
	return error;
}

SQLRETURN ReturnBooks(int id,char* bno,SQLHDBC conn,SQLHENV env,HSTMT stmt){
	int stock;
	char buf[MAX_QUERY_LEN];
	char date[MAX_VARCHAR_LEN];

	if(!ID_exist(id,stmt)) return ID_NOT_FOUND;
	if(!book_exist(bno,stmt)) return BOOK_NOT_FOUND;
	sprintf(buf,"\
		select*\
		from borrow_record\
		where bno='%s' and id=%d and indate is null;",bno,id);

	if(SQLExecDirect(stmt,buf,SQL_NTS)!=SQL_SUCCESS){
		Diag(stmt,1);
		return SQL_ERROR;
	}
	if(SQLFetch(stmt)!=SQL_SUCCESS){
		// Diag(stmt,1);
		return BOOK_NOT_BORROWED;
	}
	SQLCloseCursor(stmt);

	get_date(date);

	sprintf(buf,"\
		update books\
		set stock=stock+1\
		where bno=\'%s\';",
	bno);

	SQLSetConnectOption(conn,SQL_AUTOCOMMIT,0);
	if(SQLExecDirect(stmt,buf,SQL_NTS)!=SQL_SUCCESS){
		Diag(stmt,1);
		return SQL_ERROR;
	}

	sprintf(buf,"\
	update borrow_record\
	set indate=\'%s\'\
	where id=%d and bno='%s';",date,id,bno);

	if(SQLExecDirect(stmt,buf,SQL_NTS)!=SQL_SUCCESS){
		Diag(stmt,1);
		return SQL_ERROR;
	}

	SQLTransact(env,conn,SQL_COMMIT);
	SQLSetConnectOption(conn,SQL_AUTOCOMMIT,1);

	return SQL_SUCCESS;
}

SQLRETURN DropLicence(int id,SQLHDBC conn,SQLHENV env,HSTMT stmt){
	char buf[MAX_QUERY_LEN];
	RETCODE error;

	SQLPrepare(stmt,(SQLCHAR*)"\
		select * from books\
		where bno in(\
			select bno\
			from borrow_record\
			where id= ? and indate is null\
		);",SQL_NTS);
	SQLBindParameter(stmt,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&id,0,(SQLLEN *)0);
	if(SQLExecute(stmt)==SQL_SUCCESS){
		if(SQLFetch(stmt)==SQL_SUCCESS){
			puts("Some books you borrowed haven't returned yet! Can't drop the licence.");
			error=SQL_ERROR;
		}
	}else{Diag(stmt,1);}

	SQLCloseCursor(stmt);
	SQLFreeStmt(stmt,SQL_UNBIND);
	if(error!=SQL_SUCCESS) return error;

	sprintf(buf,"\
		delete from borrow_record\
		where id=%d;",id);

	SQLSetConnectOption(conn,SQL_AUTOCOMMIT,0);
	if(SQLExecDirect(stmt,buf,SQL_NTS)!=SQL_SUCCESS){
		Diag(stmt,1);
		return SQL_ERROR;
	}

	sprintf(buf,"\
		delete from licence\
		where id=%d;",id);

	if(SQLExecDirect(stmt,buf,SQL_NTS)!=SQL_SUCCESS){
		Diag(stmt,1);
		return SQL_ERROR;
	}

	SQLTransact(env,conn,SQL_COMMIT);
	SQLSetConnectOption(conn,SQL_AUTOCOMMIT,1);
	return SQL_SUCCESS;
}

SQLRETURN AddLicence(int id,char*name,char* workplace,char*type,SQLHDBC conn,SQLHENV env,HSTMT stmt){
	char buf[MAX_QUERY_LEN];
	sprintf(buf,"\
		insert into licence\
		values\
		(%d,\"%s\",\"%s\",\"%s\");",id,name,workplace,type);
	if(SQLExecDirect(stmt,buf,SQL_NTS)!=SQL_SUCCESS)return SQL_ERROR;
	return SQL_SUCCESS;
}

SQLRETURN Interface_updatelicence(HSTMT stmt,HDBC conn,HENV env){
	char type[MAX_VARCHAR_LEN];
	int id;
	int typeid;

	RETCODE error=SQL_SUCCESS;

	char buf[MAX_QUERY_LEN];
	char sqlquery[MAX_QUERY_LEN]="update licence set ";

	char prompt[][MAX_VARCHAR_LEN]={"name","workplace","type"};
	int param_type[]={SQL_C_CHAR,SQL_C_CHAR,SQL_C_LONG};

	int i;
	int valid=FALSE;
	int first_param=TRUE;

	char* p;

	puts("new id:");
	if(scanf("%d",&id)==0){
		puts("invalid id!");
		fgets(trash,MAX_QUERY_LEN,stdin);
		return SQL_ERROR;
	};
	if(!ID_exist(id,stmt)){
		puts("id not exist!");
		return SQL_ERROR;
	}
	getchar();
	puts("please fill the update infomations.\n");
	puts("you can press enter to pass some arguments.\n");
	for(i=0;i<3;i++){
		if(i<2) printf("\n%s:\n",prompt[i]);
		else{
			puts("\nlicence type:(default student)");
			puts("1.student");
			puts("2.teacher");
			puts("3.administrator");
		}

		gets(buf);
		if(!buf[0]) continue;
		valid=TRUE;
		if(first_param) first_param=FALSE;
		else sprintf(sqlquery,"%s,",sqlquery);
		switch(param_type[i]){
			case SQL_C_CHAR:
				sprintf(sqlquery,"%s %s=\"%s\"",sqlquery,prompt[i],buf);
				break;
			case SQL_C_LONG:
				if(!sscanf(buf,"%d",&typeid)){puts("invalid input!");fgets(trash,MAX_VARCHAR_LEN,stdin);return SQL_ERROR;}
				switch(typeid){
					case 1:strcpy(type,"student"); break;
					case 2:strcpy(type,"teacher"); break;
					case 3:strcpy(type,"administrator"); break;
					default: strcpy(type,"student"); break;
				}
				sprintf(sqlquery,"%s %s=\"%s\"",sqlquery,prompt[i],type);
				break;
			default: valid=FALSE; break;
		}
	}
	if(!valid){
		p=strstr(sqlquery,"set");
		if(p)*p='\0';
	}
	else sprintf(sqlquery,"%s where id=%d;",sqlquery,id);

	// puts(sqlquery);

	if(SQLExecDirect(stmt,(SQLCHAR*)sqlquery,SQL_NTS)!=SQL_SUCCESS){
		puts("invalid input!");
		return SQL_ERROR;
	}
	return SQL_SUCCESS;
}

SQLRETURN ShowAllBooks(HSTMT stmt){
	char sqlquery[MAX_QUERY_LEN]="select * from books";
	if(SQLExecDirect(stmt,(SQLCHAR*)sqlquery,SQL_NTS)==SQL_SUCCESS)
		PrintBookInfo(stmt);
	else return SQL_ERROR;
	return SQL_SUCCESS;
}

void PrintBookInfo(HSTMT stmt){
	int year,total,stock;
	int empty=1;
	float price;
	char bno[MAX_VARCHAR_LEN];
	char category[MAX_VARCHAR_LEN];
	char title[MAX_VARCHAR_LEN];
	char press[MAX_VARCHAR_LEN];
	char author[MAX_VARCHAR_LEN];

	BindStringCol(1,bno);
	BindStringCol(2,category);
	BindStringCol(3,title);
	BindStringCol(4,press);
	BindIntCol(5,year);
	BindStringCol(6,author);
	BindFloatCol(7,price);
	BindIntCol(8,total);
	BindIntCol(9,stock);

	while(SQLFetch(stmt)==SQL_SUCCESS){
		empty=0;
		printf("(%s,%s,%s,%s,%d,%s,%f,%d,%d)\n",bno,category,title,press,year,author,price,total,stock);
	}
	if(empty) puts("no data");
}

SQLRETURN add_book_benches(const char* filename,HSTMT stmt){
	FILE* fp=fopen(filename,"r");
	if(!fp){
		puts("file not found");
		return SQL_ERROR;
	};

	RETCODE error=SQL_SUCCESS;
	int year,total,stock;
	char bno[MAX_VARCHAR_LEN];
	char category[MAX_VARCHAR_LEN];
	char title[MAX_VARCHAR_LEN];
	char press[MAX_VARCHAR_LEN];
	char author[MAX_VARCHAR_LEN];
	char buf[MAX_QUERY_LEN];
	char* p,*next;
	float price;

	SQLPrepare(stmt,(SQLCHAR*)"insert into books\
		values (?,?,?,?,?,?,?,?,?);",SQL_NTS);
	SQLBindParameter(stmt,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,MAX_VARCHAR_LEN,0,bno,MAX_VARCHAR_LEN,(SQLLEN *)NULL);
	SQLBindParameter(stmt,2,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,MAX_VARCHAR_LEN,0,category,MAX_VARCHAR_LEN,(SQLLEN *)NULL);
	SQLBindParameter(stmt,3,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,MAX_VARCHAR_LEN,0,title,MAX_VARCHAR_LEN,(SQLLEN *)NULL);
	SQLBindParameter(stmt,4,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,MAX_VARCHAR_LEN,0,press,MAX_VARCHAR_LEN,(SQLLEN *)NULL);
	SQLBindParameter(stmt,5,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&year,0,(SQLLEN *)0);
	SQLBindParameter(stmt,6,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,MAX_VARCHAR_LEN,0,author,MAX_VARCHAR_LEN,(SQLLEN *)NULL);
	SQLBindParameter(stmt,7,SQL_PARAM_INPUT,SQL_C_FLOAT,SQL_VARCHAR,0,0,&price,0,(SQLLEN *)0);
	SQLBindParameter(stmt,8,SQL_PARAM_INPUT,SQL_C_LONG,SQL_VARCHAR,0,0,&total,0,(SQLLEN *)0);
	SQLBindParameter(stmt,9,SQL_PARAM_INPUT,SQL_C_LONG,SQL_VARCHAR,0,0,&stock,0,(SQLLEN *)0);

	while(fgets(buf,MAX_QUERY_LEN,fp)){
		string_process(buf,"()");
		p=buf;
		if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
		p=strip(p);
		strcpy(bno,p);
		p=next;
		if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
		p=strip(p);
		strcpy(category,p);
		p=next;
		if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
		p=strip(p);
		strcpy(title,p);
		p=next;
		if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
		p=strip(p);
		strcpy(press,p);
		p=next;
		if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
		year=atoi(p);
		p=next;
		if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
		p=strip(p);
		strcpy(author,p);
		p=next;
		if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
		price=atof(p);
		p=next;
		total=atoi(p);

		stock=total;
		if(SQLExecute(stmt)!=SQL_SUCCESS){
			error=SQL_ERROR;
			break;
		}
	}
	SQLFreeStmt(stmt,SQL_UNBIND);

	fclose(fp);
	return error;
}

SQLRETURN Add_Book(HSTMT stmt){

	RETCODE error=SQL_SUCCESS;
	int year,total,stock;
	char bno[MAX_VARCHAR_LEN];
	char category[MAX_VARCHAR_LEN];
	char title[MAX_VARCHAR_LEN];
	char press[MAX_VARCHAR_LEN];
	char author[MAX_VARCHAR_LEN];
	char buf[MAX_QUERY_LEN];
	char query[MAX_QUERY_LEN];
	char* next,*p;
	float price;

	puts("\nputs the new book information in the followed format:");
	puts("(book id,category,title,press,year,author,price,total)\n");
	fgets(buf,MAX_QUERY_LEN,stdin);
	// printf("the buf:%s",buf);
	string_process(buf,"()");
	p=buf;
	if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
	p=strip(p);
	strcpy(bno,p);
	p=next;
	if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
	p=strip(p);
	strcpy(category,p);
	p=next;
	if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
	p=strip(p);
	strcpy(title,p);
	p=next;
	if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
	p=strip(p);
	strcpy(press,p);
	p=next;
	if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
	year=atoi(p);
	p=next;
	if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
	p=strip(p);
	strcpy(author,p);
	p=next;
	if(!(next=next_arg(p))){puts("invalid input");return SQL_ERROR;}
	price=atof(p);
	p=next;
	total=atoi(p);

	stock=total;

	// printf("%s,%s,%s,%s,%d,%s,%lf,%d\n",bno,category,title,press,year,author,price,total);
	if(book_exist(bno,stmt)){
		puts("You are updating a book.And what's the new stock?");
		if(!scanf("%d",&stock)){puts("invalid input");return SQL_ERROR;}
		sprintf(query,"update books \
		set bno=?,category=?,title=?,press=?,year=?,author=?,price=?,total=?,stock=? where bno='%s';",bno);
		SQLPrepare(stmt,(SQLCHAR*)query,SQL_NTS);
	}
	else SQLPrepare(stmt,(SQLCHAR*)"insert into books \
		values (?,?,?,?,?,?,?,?,?);",SQL_NTS);
	SQLBindParameter(stmt,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,MAX_VARCHAR_LEN,0,bno,MAX_VARCHAR_LEN,(SQLLEN *)NULL);
	SQLBindParameter(stmt,2,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,MAX_VARCHAR_LEN,0,category,MAX_VARCHAR_LEN,(SQLLEN *)NULL);
	SQLBindParameter(stmt,3,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,MAX_VARCHAR_LEN,0,title,MAX_VARCHAR_LEN,(SQLLEN *)NULL);
	SQLBindParameter(stmt,4,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,MAX_VARCHAR_LEN,0,press,MAX_VARCHAR_LEN,(SQLLEN *)NULL);
	SQLBindParameter(stmt,5,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&year,0,(SQLLEN *)0);
	SQLBindParameter(stmt,6,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,MAX_VARCHAR_LEN,0,author,MAX_VARCHAR_LEN,(SQLLEN *)NULL);
	SQLBindParameter(stmt,7,SQL_PARAM_INPUT,SQL_C_FLOAT,SQL_VARCHAR,0,0,&price,0,(SQLLEN *)0);
	SQLBindParameter(stmt,8,SQL_PARAM_INPUT,SQL_C_LONG,SQL_VARCHAR,0,0,&total,0,(SQLLEN *)0);
	SQLBindParameter(stmt,9,SQL_PARAM_INPUT,SQL_C_LONG,SQL_VARCHAR,0,0,&stock,0,(SQLLEN *)0);

	if(SQLExecute(stmt)!=SQL_SUCCESS){
		Diag(stmt,1);
		error=SQL_ERROR;
	}
	SQLFreeStmt(stmt,SQL_UNBIND);
	return error;
}

char* next_arg(char* buf){
	char* pos=buf,*next_pos;
	next_pos=strstr(pos,",");
	if(next_pos) {
		*(next_pos++)='\0';
	}
	return next_pos;
}

char* strip(char*p){
	char* start,*end=NULL;
	while(*p==' '||*p=='\t'||*p=='\n') p++;
	start=p;
	while(*p){
		if(*p!=' '&&*p!='\t'&&*p!='\n') end=NULL;
		else if(!end) end=p;
		p++;
	}
	if(end)*end='\0';
	return start;
}

void Diag(HSTMT stmt,int i){
	char msg[MAX_QUERY_LEN];
	char state[MAX_VARCHAR_LEN];
	short len=MAX_QUERY_LEN;
	SQLINTEGER code;
	SQLGetDiagRec(SQL_HANDLE_STMT,stmt,i,state,&code,msg,MAX_QUERY_LEN,&len);
	printf("%s %ld %s\n",state,code,msg);
}

void ODBCexample()
{
	RETCODE error;
	HENV env; /* environment */
	HDBC conn; /* database connection */
	SQLAllocEnv(&env);
	SQLAllocConnect(env, &conn);
	SQLConnect(conn, (SQLCHAR*)"LIBRARY", SQL_NTS, (SQLCHAR*)"root", SQL_NTS,
		(SQLCHAR*)"Zlz193507", SQL_NTS);
	
	char deptname[80];
	float salary;
	int lenOut1, lenOut2;
	HSTMT stmt;
	SQLCHAR sqlquery[200] = "select dept name, sum (salary)"
		"from instructor"
		"group by dept name";
	
	SQLAllocStmt(conn, &stmt);
	error = SQLExecDirect(stmt, sqlquery, SQL_NTS);
	if (error == SQL_SUCCESS) {
		SQLBindCol(stmt, 1, SQL_C_CHAR, deptname, 80, (SQLINTEGER*)&lenOut1);
		SQLBindCol(stmt, 2, SQL_C_FLOAT, &salary, 0, (SQLINTEGER*)&lenOut2);
		while (SQLFetch(stmt) == SQL_SUCCESS) {
			printf(" %s %g\n", deptname, salary);
		}
	}
	SQLFreeStmt(stmt, SQL_DROP);
	
	SQLDisconnect(conn);
	SQLFreeConnect(conn);
	SQLFreeEnv(env);
}