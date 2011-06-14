drop table Person;
drop table Transfers;

create table Person (
       Username  varchar(16),
       Password  varchar(32),
       Salt      varchar(16),
       Token     varchar(32),
       Zoobars   integer default 10,
       Profile   varchar(1024)
);

create table Transfers (
       TransferID  serial,
       Sender      varchar(16),
       Recipient   varchar(16),
       Amount      int,
       Time        abstime
);

alter table Person add column start_ts timestamp with time zone;
alter table Person add column end_ts timestamp with time zone;
alter table Person add constraint unique_user unique (Username, end_ts);
alter table Person add column start_pid int;
alter table Person add column end_pid int;
create trigger timetravel 
	before insert or delete or update on Person
	for each row 
	execute procedure 
	timetravel (start_ts, end_ts, start_pid, end_pid);

alter table Transfers add column start_ts timestamp with time zone;
alter table Transfers add column end_ts timestamp with time zone;
alter table Transfers add constraint unique_tid unique (TransferID, end_ts);
alter table Transfers add column start_pid int;
alter table Transfers add column end_pid int;
create trigger timetravel 
	before insert or delete or update on Transfers
	for each row 
	execute procedure 
	timetravel (start_ts, end_ts, start_pid, end_pid);
