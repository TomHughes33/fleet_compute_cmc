CREATE TABLE users (id SERIAL PRIMARY KEY,
                    username TEXT UNIQUE NOT NULL,password TEXT NOT NULL);
INSERT INTO users (username, password) VALUES ('admin', 'Sha256:10000:Hq+L9F8x1zQ3gP8FZL/cug==:ra4Ho6znzsUJnGVIuezMCA==');
CREATE TABLE servers_compute ( id SERIAL PRIMARY KEY,
                               name varchar(20) ,
                               hostname varchar(20) ,
                               login varchar(20), 
                               password varchar(14),type integer NOT NULL,
			       isonline integer NOT NULL default 0,
			       customer_number varchar(40),
			       guid_number varchar(40),
		               hardware_serial_number varchar(50),
		               management_ip_address varchar(20),
		               corporate_ip_address varchar(20),
		               is_active bool,
		               management_vlan_id varchar(40),
		               corporate_vlan_id varchar(40),
		               last_identity_update timestamp,
		               registration_time timestamp);
INSERT INTO servers_compute (id,name,hostname,type) VALUES (1,'vessel1','169.254.144.101',1);
CREATE TABLE create_flavor ( id SERIAL PRIMARY KEY, 
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

