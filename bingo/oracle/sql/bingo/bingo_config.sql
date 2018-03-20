-- Copyright (C) 2009-2015 EPAM Systems
-- 
-- This file is part of Indigo toolkit.
-- 
-- This file may be distributed and/or modified under the terms of the
-- GNU General Public License version 3 as published by the Free Software
-- Foundation and appearing in the file LICENSE.GPL included in the
-- packaging of this file.
-- 
-- This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
-- WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

Set Verify Off;
spool bingo_config;

create table CONFIG_STR (n int, name varchar2(100), value varchar2(4000));
create index CONFIG_STR_N on CONFIG_STR(n);
create table CONFIG_INT (n int, name varchar2(100), value int);
create index CONFIG_INT_N on CONFIG_INT(n);
create table CONFIG_FLOAT (n int, name varchar2(100), value float);
create index CONFIG_FLOAT_N on CONFIG_FLOAT(n);
create table CONFIG_CLOB (n int, name varchar2(100), value CLOB);
create index CONFIG_CLOB_N on CONFIG_CLOB(n);
create table CONFIG_BLOB (n int, name varchar2(100), value BLOB) NOLOGGING LOB(value) STORE AS (NOCACHE NOLOGGING);
create index CONFIG_BLOB_N on CONFIG_BLOB(n);

create table TAUTOMER_RULES (id INT PRIMARY KEY, beg VARCHAR2(100), end VARCHAR2(100));

insert into TAUTOMER_RULES values (1, 'N,O,P,S,As,Se,Sb,Te', 'N,O,P,S,As,Se,Sb,Te');
insert into TAUTOMER_RULES values (2, '0C', 'N,O,P,S');
insert into TAUTOMER_RULES values (3, '1C', 'N,O');

grant insert, select, delete on TAUTOMER_RULES to public;

insert into CONFIG_INT values(0, 'FP_ORD_SIZE', 25);
insert into CONFIG_INT values(0, 'FP_ANY_SIZE', 15);
insert into CONFIG_INT values(0, 'FP_TAU_SIZE', 10);
insert into CONFIG_INT values(0, 'FP_SIM_SIZE', 8);
insert into CONFIG_INT values(0, 'FP_STORAGE_CHUNK', 1024);
insert into CONFIG_INT values(0, 'SUB_SCREENING_MAX_BITS', 8);
insert into CONFIG_INT values(0, 'SUB_SCREENING_PASS_MARK', 128);
insert into CONFIG_INT values(0, 'SIM_SCREENING_PASS_MARK', 128);

insert into CONFIG_INT values(0, 'TREAT_X_AS_PSEUDOATOM', 0);
insert into CONFIG_INT values(0, 'IGNORE_CLOSING_BOND_DIRECTION_MISMATCH', 0);
insert into CONFIG_INT values(0, 'IGNORE_STEREOCENTER_ERRORS', 0);
insert into CONFIG_INT values(0, 'IGNORE_CISTRANS_ERRORS', 0);
insert into CONFIG_INT values(0, 'ALLOW_NON_UNIQUE_DEAROMATIZATION', 0);
insert into CONFIG_INT values(0, 'ZERO_UNKNOWN_AROMATIC_HYDROGENS', 0);
insert into CONFIG_INT values(0, 'STEREOCHEMISTRY_BIDIRECTIONAL_MODE', 0);
insert into CONFIG_INT values(0, 'STEREOCHEMISTRY_DETECT_HAWORTH_PROJECTION', 0);	
insert into CONFIG_INT values(0, 'REJECT_INVALID_STRUCTURES', 0);	
insert into CONFIG_INT values(0, 'IGNORE_BAD_VALENCE', 0);

create or replace procedure SetRelativeAtomicMass (list in VARCHAR2) is
begin
  ConfigSetString(0, 'RELATIVE_ATOMIC_MASS', list);
end SetRelativeAtomicMass;
/ 
grant execute on SetRelativeAtomicMass to public;

create or replace procedure TreatXAsPseudoatom (k in number) is
begin
  ConfigSetInt(0, 'TREAT_X_AS_PSEUDOATOM', k);
end TreatXAsPseudoatom;
/ 
grant execute on TreatXAsPseudoatom to public;

create or replace procedure IgnoreCBDirectionMismatch (k in number) is
begin
  ConfigSetInt(0, 'IGNORE_CLOSING_BOND_DIRECTION_MISMATCH', k);
end IgnoreCBDirectionMismatch;
/ 
grant execute on IgnoreCBDirectionMismatch to public;

spool off;

exit;
