--user root

drop database if exists test;
create database test;
show databases;
use test;
create table Player_List(
	name varchar(20) not null, 
	id varchar(20) not null, 
	password varchar(20) not null,

	primary key (id)
);

show tables;
describe Player_List;

insert into Player_List(name, id, password) values
		('jinho', 'jinho0810', 'jang1234'),
		('keonhwa', 'keonhwa0810', 'jung1234'),
		('Seungmo', 'Seungmo0810', 'gang1234');

select * from Player_List;
