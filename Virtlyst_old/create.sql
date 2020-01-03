PRAGMA journal_mode = WAL;
CREATE TABLE users (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
                    username TEXT UNIQUE NOT NULL,password TEXT NOT NULL);
INSERT INTO users (username, password) VALUES ('admin', 'Sha256:10000:Hq+L9F8x1zQ3gP8FZL/cug==:ra4Ho6znzsUJnGVIuezMCA==');
CREATE TABLE servers_compute ( id integer NOT NULL PRIMARY KEY,
                               name varchar(20) NOT NULL UNIQUE,
                               hostname varchar(20) NOT NULL UNIQUE,
                               login varchar(20), 
                               password varchar(14),type integer NOT NULL,
			       isonline integer NOT NULL default 0);
INSERT INTO servers_compute (id,name,hostname,type) VALUES (1,'vessel1','169.254.144.101',1);
INSERT INTO servers_compute (id,name,hostname,type) VALUES (2,'vessel2','169.254.144.102',1);
INSERT INTO servers_compute (id,name,hostname,type) VALUES (3,'vessel3','169.254.144.103',1);
INSERT INTO servers_compute (id,name,hostname,type) VALUES (4,'vessel4','169.254.144.104',1);
INSERT INTO servers_compute (id,name,hostname,type) VALUES (5,'vessel5','169.254.144.105',1);
INSERT INTO servers_compute (id,name,hostname,type) VALUES (6,'vessel6','169.254.144.106',1);
INSERT INTO servers_compute (id,name,hostname,type) VALUES (7,'vessel7','169.254.144.107',1);
INSERT INTO servers_compute (id,name,hostname,type) VALUES (8,'vessel8','169.254.144.108',1);
INSERT INTO servers_compute (id,name,hostname,type) VALUES (9,'vessel9','169.254.144.109',1);
INSERT INTO servers_compute (id,name,hostname,type) VALUES (10,'vessel10','169.254.144.110',1);
CREATE TABLE create_flavor ( id integer NOT NULL PRIMARY KEY, 
                             label varchar(12) NOT NULL, 
                             memory integer NOT NULL, 
                             vcpu integer NOT NULL, 
                             disk integer NOT NULL);
INSERT INTO create_flavor (label, memory, vcpu, disk) values ('micro', 512, 1, 20);
INSERT INTO create_flavor (label, memory, vcpu, disk) values ('mini', 1024, 2, 30);
INSERT INTO create_flavor (label, memory, vcpu, disk) values ('small', 2048, 2, 40);
INSERT INTO create_flavor (label, memory, vcpu, disk) values ('medium', 4096, 2, 60);
INSERT INTO create_flavor (label, memory, vcpu, disk) values ('large', 8192, 4, 80);
INSERT INTO create_flavor (label, memory, vcpu, disk) values ('xlarge', 16348, 8, 160);

