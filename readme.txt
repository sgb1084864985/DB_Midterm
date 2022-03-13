源代码为src.c

为方便调试，该源代码的数据库的三个参数是固定的，如要本地化，
请在编译前将源码75行的“LINKSTART("LIBRARY","root","Zlz193507"){”
中的3个参数换成自己的DSN，UID和PASSWD.

另外，在该DSN所指定的database中，
需先执行如下的数据表定义语句：
		create table borrow_record
		(
			bno varchar(30),
			id int,
			Outdate datetime,
			Indate datetime,
			Primary Key(bno,id,Outdate),
			foreign key(bno) references books(bno),
			foreign key(id) references licence(id)
		);

        		create table licence(
			 id int,
			 name varchar(30),
			 workplace varchar(30),
			 type varchar(30),
			 Primary Key(id),
			 check (type in ('teacher','student','administrator'))
		);

		create table books(
			 bno varchar(30),
			 Category varchar(30),
			 Title varchar(30),
			 Press varchar(30),
			 Year  int,
			 Author varchar(30),
			 Price float(2),
			 Total int,
		     	 Stock int,
			 Primary Key(bno),
			 check (Stock>=0),
			 check (Total>=Stock),
			 check (Price>0)
		);

--------------------------------------------------------------------------
编译：

在Windows10 32位以上环境下，使用gcc 8.2.0或其他编译器编译，gcc的编译指令示例如下：

>gcc src.c -lodbc32 -o mylib

其中,-lodbc32 选项是必要的，否则无法通过编译。