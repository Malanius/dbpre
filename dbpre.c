# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>

# ifndef TRUE
   # define TRUE (0==0)
   # define FALSE (0!=0)
# endif

# define DBPREVERSION "V 0.2"
# define DBPREVERSIONLONG "V 0.2 2012-07-24"

char infilename[256];
int tabstop=3;
int freestyle=FALSE;
char conf_copybookpath[256]="";

char replace_old[80]="";
char replace_new[80]="";

int execsqlmode=FALSE;
int execsqlcount=0;
int proceduremode=FALSE;
int sequencecnt=1;

int exec_sql_error=FALSE;

int collect_cursor_statement=FALSE;
int collect_select_statement=FALSE;
int collect_fetch_statement=FALSE;
int collect_delete_statement=FALSE;
int collect_update_statement=FALSE;
int collect_insert_statement=FALSE;
unsigned char collect_container[32768]="";

# define MAX_CURSORS 64
char collect_cursor_name[32]="";
char fetch_cursor_name[32]="";
// MAX_CURSORS+1 is used for SELECT statements
char *cursor_container_tab[MAX_CURSORS+1];
char cursor_name_tab[MAX_CURSORS+1][32];
char open_cursor_line[128]="";
int cursorcnt=0;

FILE *fpin[32], *fpout, *fplst;
int fpincnt=0;

int errorcnt=0;

/******************************************************************************/
void trim(char *s)
{
		  int l;

		  l=strlen(s)-1;
		  for(;;){
					 if(l<1) break;
					 if(s[l]!=' ' && s[l]!='\t') break;
					 s[l]=0;
					 l--;
		  }
		  if(l==0) return;
		  for(;;){
					 if(s[0]==0) break;
					 if(s[0]!=' ' && s[0]!='\t') break;
					 memmove(s, s+1, strlen(s));
		  }
}
/******************************************************************************/
void rtrim(char *s)
{
		  int l;

		  l=strlen(s)-1;
		  for(;;){
					 if(l<1) break;
					 if(s[l]!=' ' && s[l]!='\t') break;
					 s[l]=0;
					 l--;
		  }
}
/******************************************************************************/
int replace(char *s, char *alt, char *neu)
{
		  int l, la, ln;
		  void *p;
		  long off;

		  l=strlen(s);
		  la=strlen(alt);
		  ln=strlen(neu);

		  if(strcmp(alt, neu)==0){
					 return(FALSE);
		  }

		  p=(void*)strstr(s, alt);
		  if(p==NULL){
					 return(FALSE);
		  }

		  off=(long)p-(long)s;

		  if(la==ln){
					 memmove(s+off, neu, ln);
					 return(TRUE);
		  }
		  if(la>ln){
					 memmove(s+off, neu, ln);
					 memmove(s+off+ln, s+off+la, l-la+1);
					 return(TRUE);
		  }
		  if(la<ln){
					 memmove(s+off+ln, s+off+la, l-la);
					 memmove(s+off, neu, ln);
					 return(TRUE);
		  }
		  return(FALSE);

}
/******************************************************************************/
/* Retrieves word No. n from string *s and stores it in *ziel.
 * n starts with 1, not with 0 !!!
 * Escape-Character is \, strings are delimited by ".
 */
char *__xpjg_wordptr=NULL;
int __xpjg_wordpos=0;
void word(char *s, char *ziel, int n)
{
		  int anz=0;
		  int t;
		  int f=0;
		  int z=0;
		  int start;
		  int m=1;
		  int esc=0;
		  int l;

		  __xpjg_wordptr=s;

		  l=(int)strlen((char*)s);
		  for(start=0; start<l; start++) if (s[start]>' ') break;

		  for(t=start;t<l; t++){
					 if(s[t]=='\\'){
								if(esc==0){
										  esc=1;
										  if(anz==n) ziel[z++]='\\';
								}
								else{
										  if(anz==n) ziel[z++]='\\';
										  esc=0;
								}
					 }
					 else{
								if(s[t]=='"' && esc==0)  m*=-1;
								if (s[t] == ' ' && m==1) f=0;
								else f++;
								if (f==1) anz++;
								if (anz == n && f > 0){
										  ziel[z++]=s[t];
								}
								if (anz > n && f > 0) break;
								esc=0;
					 }
		  } /* next t */
		  ziel[z]=0;
		  __xpjg_wordpos=t;
		  l=strlen((char*)ziel)-1;   /* Trim trailing spaces */
		  for(;;){
					 if(l<1) break;
					 if(ziel[l]!=' ') break;
					 ziel[l]=0;
					 l--;
		  }
}
/********************************************************************/
void next_word(unsigned char *ziel)
{
		  int anz=0;
		  int t;
		  int f=0;
		  int z=0;
		  int m=1;
		  int esc=0;
		  int l;
		  char *s;

		  s=__xpjg_wordptr;
		  if(s==NULL){
					 ziel[0]=0;
					 return;
		  }
		  l=strlen((char*)s);
		  for(t=__xpjg_wordpos;t<l; t++){
					 if(s[t]=='\\'){
								if(esc==0){
										  esc=1;
										  ziel[z++]='\\';
								}
								else{
										  ziel[z++]='\\';
										  esc=0;
								}
					 }
					 else{
								if(s[t]=='"' && esc==0)  m*=-1;
								if (s[t] == ' ' && m==1) f=0;
								else f++;
								if (f==1) anz++;
								if (anz > 1 && f > 0) break;
								if (f > 0){
										  ziel[z++]=s[t];
								}
								esc=0;
					 }
		  } /* next t */
		  ziel[z]=0;
		  if(z==0){
					 __xpjg_wordptr=NULL;
					 __xpjg_wordpos=0;
		  }
		  else{
					 __xpjg_wordpos=t;
		  }
		  l=strlen((char*)ziel)-1;   /* Trim trailing spaces */
		  for(;;){
					 if(l<1) break;
					 if(ziel[l]!=' ') break;
					 ziel[l]=0;
					 l--;
		  }
}
/********************************************************************/
/* Counts all words in *s and returns the count.
 */
int words(unsigned char *s)
{
		  int anz=0;
		  int t;
		  int f=0;
		  int start;
		  int m=1;
		  int esc=0;
		  int l;

		  l=(int)strlen((char*)s);
		  for(start=0; start<l; start++) if (s[start]>' ') break;

		  for(t=start;t<l; t++){
					 if(s[t]=='\\'){
								if(esc==0){
										  esc=1;
								}
								else{
										  esc=0;
								}
					 }
					 else{
								if(s[t]=='"' && esc==0)  m*=-1;
								if (s[t] == ' ' && m==1) f=0;
								else f++;
								if (f==1) anz++;
								esc=0;
					 }
		  } /* next t */
		  return(anz);
}
/******************************************************************************/
void lcase(char *s)
{
		  for(;;){
					 if(*s==0) break;
					 *s=tolower(*s);
					 s++;
		  }
}
/******************************************************************************/
void ucase(char *s)
{
		  for(;;){
					 if(*s==0) break;
					 *s=toupper(*s);
					 s++;
		  }
}
/******************************************************************************/
int replace_text(char *s, char *alt, char *neu)
{
		  int l, la, ln;
		  void *p;
		  long off;

		  //printf("replace_text('%s', '%s', '%s')\n", s, alt, neu);
		  l=strlen(s);
		  la=strlen(alt);
		  ln=strlen(neu);

		  if(strcmp(alt, neu)==0){
					 return(FALSE);
		  }

		  p=(void*)strcasestr(s, alt); // strcasestr = not case sensitive!!!
		  if(p==NULL){
					 return(FALSE);
		  }

		  off=(long)p-(long)s;

		  if(la==ln){
					 memmove(s+off, neu, ln);
					 return(TRUE);
		  }
		  if(la>ln){
					 memmove(s+off, neu, ln);
					 memmove(s+off+ln, s+off+la, l-la+1);
					 return(TRUE);
		  }
		  if(la<ln){
					 memmove(s+off+ln, s+off+la, l-la+1);
					 memmove(s+off, neu, ln);
					 s[l-la+ln]=0;
					 return(TRUE);
		  }
		  return(FALSE);
}
/******************************************************************************/
int strUcmp(char *s1, char *s2)
{
		  char zwi1[32768];
		  char zwi2[32768];

		  strcpy(zwi1, s1);
		  strcpy(zwi2, s2);
		  ucase(zwi1);
		  ucase(zwi2);

		  return(strcmp(zwi1, zwi2));
}
/******************************************************************************/
int strUncmp(char *s1, char *s2, int len)
{
		  char zwi1[32768];
		  char zwi2[32768];

		  strcpy(zwi1, s1);
		  strcpy(zwi2, s2);
		  ucase(zwi1);
		  ucase(zwi2);
		  zwi1[len]=0;
		  zwi2[len]=0;

		  return(strcmp(zwi1, zwi2));
}
/******************************************************************************/
int read_line(FILE *fp, char *s)
{
	int z;
	int c;
	int t;

	z=0;
	for(;;){
		c=fgetc(fp);
		if(c==EOF || c=='\r' || c=='\n'){
			s[z]=0;
			break;
		}
		else{
			if(c=='\t'){
				for(t=0; t<tabstop; t++){
					s[z++]=' ';
				}
			}
			else{
				s[z++]=c;
			}
		}
	}
	return(c);
}
/******************************************************************************/
void handle_options(int argc, char **argv)
{
	int t;
	int f;
	char option[256];

	strcpy(infilename, "");
	
	for(t=1; t<argc; t++){
		f=0;
		strcpy(option, argv[t]);
		if(option[0]=='-'){
			if(f==0 && strncmp(option, "-ts=", 4)==0){
				tabstop=atoi(option+4);
				printf("setting option tabstop=%d\n", tabstop);
				f=1;
			}
			if(f==0 && strncmp(option, "-I", 2)==0){
				strcpy(conf_copybookpath, option+2);
				if(conf_copybookpath[strlen(conf_copybookpath)-1]=='/'){
				   conf_copybookpath[strlen(conf_copybookpath)-1]=0;
				}
				printf("copybookpath=>%s<\n", conf_copybookpath);
				f=1;
			}
			if(f==0){
				if(strcmp(option, "-v")==0 ||
				   strcmp(option, "--version")==0){
   				printf("dbpre %s\n", DBPREVERSIONLONG);
   				exit(0);
				}
			}
			if(f==0){
				if(strcmp(option, "-h")==0 ||
				   strcmp(option, "--help")==0){
					printf("dbpre %s\n", DBPREVERSIONLONG);
					puts("dbpre [options] progname");
					puts("");
					puts("Options");
					puts("=======");
					puts("-h, --help     - this message");
					puts("-v, --version  - print version of dbpre");
					puts("-I             - specify path for copybooks, e.g -I/tmp/copybooks/");
					puts("-ts            - tab stop, tabs will be expanded to nnn spaces, e.g. -ts=3");
					puts("");
					puts("Developed by the_piper@web.de");
					puts("For more infos see here: http://pipersopencoboltoolbox.blogspot.de/");
					exit(0);
				}
			}
			if(f==0 && strcmp(option, "-f")==0){
				freestyle=TRUE;
				printf("setting option freestyle=TRUE\n");
				f=1;
			}
		}
		else{
			if(strlen(infilename)>0){
				fprintf(stderr, "DBPRE: duplicate infile name >%s< specified, aborting...\n", option);
				exit(1);
			}
			else{
				strcpy(infilename, option);
				f=1;
			}
		}
		if(f==0){
			fprintf(stderr, "DBPRE: unknown option >%s<, aborting...\n", option);
			exit(5);
		}

	}

}
/******************************************************************************/
void parse_fetch_statement(FILE *fp, FILE *fplst)
{
	char statement[8192];
	char variables[8192];
	char zwi[8192];
	char zwi2[256];
	int t;
	int z;
	int f;
	int idx;
	int stage;

	f=0;
	for(t=0; t<cursorcnt; t++){
		//printf("compare >%s<->%s<\n", cursor_name_tab[t], fetch_cursor_name);
		if(strcmp(cursor_name_tab[t], fetch_cursor_name)==0){
			f=1;
			idx=t;
			break;
		}
	}
	if(f==0){
		fprintf(fplst, "SYNTAX ERROR: CURSOR >%s< is not DECLARE'd.\n", fetch_cursor_name);
		errorcnt++;
		return;
	}
	strcpy(statement, "");
	strcpy(variables, "");
	stage=0;
	//printf("parse_fetch_statement >%s<\n", collect_container);
	for(t=1;;t++){
		word(collect_container, zwi, t);
		//printf("parse.. zwi=>%s<\n", zwi);
		if(strlen(zwi)==0) break;
		if(stage==1){
			if(zwi[0]!=':'){
				fprintf(fplst, "SYNTAX ERROR: not a host variable >%s<\n", zwi);
				errorcnt++;
				return;
			}
			else{
				sprintf(zwi2, "%s", zwi+1);
				strcat(variables, zwi2);
				strcat(variables, " ");
			}
		}
		if(strUcmp(zwi, "INTO")==0){
			stage=1;
		}
		if(stage==0){
			strcat(statement, zwi);
			strcat(statement, " ");
		}
	}
	if(stage==0){
		fprintf(fplst, "SYNTAX ERROR: invalid FETCH statement\n");
		errorcnt++;
		return;
	}
	//printf("statement=>%s<\n", statement);

	fprintf(fp, "DBPRE       IF SQLCA-CURSOR-CTRL (%d) = 0\n", idx+1);
	fprintf(fplst, "DBPRE       IF SQLCA-CURSOR-CTRL (%d) = 0\n", idx+1);
	fprintf(fp, "DBPRE          SET DB-CURSOR-NOT-OPEN TO TRUE\n");
	fprintf(fplst, "DBPRE          SET DB-CURSOR-NOT-OPEN TO TRUE\n");
	fprintf(fp, "DBPRE          PERFORM DB-STATUS\n");
	fprintf(fplst, "DBPRE          PERFORM DB-STATUS\n");
	fprintf(fp, "DBPRE       END-IF\n");
	fprintf(fplst, "DBPRE       END-IF\n");
	fprintf(fp,"DBPRE          CALL 'MySQL_fetch_row' USING SQLCA-RESULT (%d)\n",
	        idx+1);
	for(t=1;;t++){
		word(variables, zwi, t);
		if(strlen(zwi)==0) break;
		fprintf(fp, "DBPRE                                          %s\n", zwi);
	}
	fprintf(fp, "DBPRE          END-CALL\n");
	fprintf(fp, "DBPRE          IF SQLCA-RESULT (%d) = NULL\n", idx+1);
	fprintf(fp, "DBPRE             MOVE 100 TO SQLCODE\n");
	fprintf(fp, "DBPRE          ELSE\n");
	fprintf(fp, "DBPRE             MOVE 0 TO SQLCODE\n");
	fprintf(fp, "DBPRE          END-IF\n");
	fprintf(fplst, "DBPRE          CALL 'MySQL_fetch_row' USING SQLCA-RESULT (%d)\n", idx+1);
	for(t=1;;t++){
		word(variables, zwi, t);
		if(strlen(zwi)==0) break;
		fprintf(fplst, "DBPRE                                          %s\n", zwi);
	}
	fprintf(fplst, "DBPRE          END-CALL\n");
	fprintf(fplst, "DBPRE          IF SQLCA-RESULT (%d) = NULL\n", idx+1);
	fprintf(fplst, "DBPRE             MOVE 100 TO SQLCODE\n");
	fprintf(fplst, "DBPRE          ELSE\n");
	fprintf(fplst, "DBPRE             MOVE 0 TO SQLCODE\n");
	fprintf(fplst, "DBPRE          END-IF\n");
	//printf("variables=>%s<\n", variables);
}
/******************************************************************************/
void parse_delete_statement(FILE *fp, FILE *fplst)
{
	char statement[8192];
	char variables[8192];
	char zwi[8192];
	char zwi2[256];
	int t;
	int z;
	int stage;

	strcpy(statement, "");
	strcpy(variables, "");
	stage=0;
	fprintf(fp, "DBPRE   MOVE LOW-VALUES TO SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE   MOVE LOW-VALUES TO SQLCA-STATEMENT\n");
	fprintf(fp, "DBPRE   STRING\n");
	fprintf(fplst, "DBPRE   STRING\n");
	for(t=1;;t++){
		word(collect_container, zwi, t);
		if(strlen(zwi)==0) break;
		if(zwi[0]==':'){
			fprintf(fp, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fp, "DBPRE   %s DELIMITED SIZE\n", zwi+1);
			fprintf(fp, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fplst, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fplst, "DBPRE   %s DELIMITED SIZE\n", zwi+1);
			fprintf(fplst, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fp, "DBPRE   ' ' DELIMITED SIZE\n");
			fprintf(fplst, "DBPRE   ' ' DELIMITED SIZE\n");
		}
		else{
			strcat(zwi, " ");
			fprintf(fp, "DBPRE   '%s' DELIMITED SIZE\n", zwi);
			fprintf(fplst, "DBPRE   '%s' DELIMITED SIZE\n", zwi);
		}
	}
	fprintf(fp, "DBPRE   INTO SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE   INTO SQLCA-STATEMENT\n");
	fprintf(fp, "DBPRE   END-STRING\n");
	fprintf(fplst, "DBPRE   END-STRING\n");

	fprintf(fp, "DBPRE          CALL 'MySQL_query' USING SQLCA-STATEMENT\n");
	fprintf(fp, "DBPRE          END-CALL\n");
	fprintf(fp, "DBPRE          MOVE RETURN-CODE TO SQLCODE\n");

	fprintf(fplst, "DBPRE          CALL 'MySQL_query' USING SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE          END-CALL\n");
	fprintf(fplst, "DBPRE          MOVE RETURN-CODE TO SQLCODE\n");
}
/******************************************************************************/
void parse_update_statement(FILE *fp, FILE *fplst)
{
	char statement[8192];
	char variables[8192];
	char zwi[8192];
	char zwi2[256];
	int t;
	int z;
	int stage;

	strcpy(statement, "");
	strcpy(variables, "");
	stage=0;
	fprintf(fp, "DBPRE   MOVE LOW-VALUES TO SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE   MOVE LOW-VALUES TO SQLCA-STATEMENT\n");
	fprintf(fp, "DBPRE   STRING\n");
	fprintf(fplst, "DBPRE   STRING\n");
	for(t=1;;t++){
		word(collect_container, zwi, t);
		if(strlen(zwi)==0) break;
		if(zwi[0]==':'){
			fprintf(fp, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fp, "DBPRE   %s DELIMITED SIZE\n", zwi+1);
			fprintf(fp, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fplst, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fplst, "DBPRE   %s DELIMITED SIZE\n", zwi+1);
			fprintf(fplst, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fp, "DBPRE   ' ' DELIMITED SIZE\n");
			fprintf(fplst, "DBPRE   ' ' DELIMITED SIZE\n");
		}
		else{
			if(zwi[0]=='\''){
				sprintf(zwi2, "'%s' ", zwi);
				fprintf(fp, "DBPRE   '%s' DELIMITED SIZE\n", zwi2);
				fprintf(fplst, "DBPRE   '%s' DELIMITED SIZE\n", zwi2);
			}
			else{
				strcat(zwi, " ");
				fprintf(fp, "DBPRE   '%s' DELIMITED SIZE\n", zwi);
				fprintf(fplst, "DBPRE   '%s' DELIMITED SIZE\n", zwi);
			}
		}
	}
	fprintf(fp, "DBPRE   INTO SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE   INTO SQLCA-STATEMENT\n");
	fprintf(fp, "DBPRE   END-STRING\n");
	fprintf(fplst, "DBPRE   END-STRING\n");

	fprintf(fp, "DBPRE          CALL 'MySQL_query' USING SQLCA-STATEMENT\n");
	fprintf(fp, "DBPRE          END-CALL\n");
	fprintf(fp, "DBPRE          MOVE RETURN-CODE        TO SQLCODE\n");

	fprintf(fplst, "DBPRE          CALL 'MySQL_query' USING SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE          END-CALL\n");
	fprintf(fplst, "DBPRE          MOVE RETURN-CODE        TO SQLCODE\n");
}
/******************************************************************************/
void parse_insert_statement(FILE *fp, FILE *fplst)
{
	char statement[8192];
	char variables[8192];
	char zwi[8192];
	char zwi2[256];
	int t;
	int z;
	int stage;

	strcpy(statement, "");
	strcpy(variables, "");
	stage=0;
	fprintf(fp, "DBPRE   MOVE LOW-VALUES TO SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE   MOVE LOW-VALUES TO SQLCA-STATEMENT\n");
	fprintf(fp, "DBPRE   STRING\n");
	fprintf(fplst, "DBPRE   STRING\n");
	for(t=1;;t++){
		word(collect_container, zwi, t);
		if(strlen(zwi)==0) break;
		if(zwi[0]==':'){
			fprintf(fp, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fp, "DBPRE   %s DELIMITED SIZE\n", zwi+1);
			fprintf(fp, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fplst, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fplst, "DBPRE   %s DELIMITED SIZE\n", zwi+1);
			fprintf(fplst, "DBPRE   QUOTE DELIMITED SIZE\n");
			fprintf(fp, "DBPRE   ' ' DELIMITED SIZE\n");
			fprintf(fplst, "DBPRE   ' ' DELIMITED SIZE\n");
		}
		else{
			if(zwi[0]=='\''){
				sprintf(zwi2, "'%s' ", zwi);
				fprintf(fp, "DBPRE   '%s' DELIMITED SIZE\n", zwi2);
				fprintf(fplst, "DBPRE   '%s' DELIMITED SIZE\n", zwi2);
			}
			else{
				strcat(zwi, " ");
				fprintf(fp, "DBPRE   '%s' DELIMITED SIZE\n", zwi);
				fprintf(fplst, "DBPRE   '%s' DELIMITED SIZE\n", zwi);
			}
		}
	}
	fprintf(fp, "DBPRE   INTO SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE   INTO SQLCA-STATEMENT\n");
	fprintf(fp, "DBPRE   END-STRING\n");
	fprintf(fplst, "DBPRE   END-STRING\n");

	fprintf(fp, "DBPRE          CALL 'MySQL_query' USING SQLCA-STATEMENT\n");
	fprintf(fp, "DBPRE          END-CALL\n");
	fprintf(fp, "DBPRE          MOVE RETURN-CODE        TO SQLCODE\n");

	fprintf(fplst, "DBPRE          CALL 'MySQL_query' USING SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE          END-CALL\n");
	fprintf(fplst, "DBPRE          MOVE RETURN-CODE        TO SQLCODE\n");
}
/******************************************************************************/
void parse_cursor_statement(FILE *fp, FILE *fplst, int idx)
{
	char statement[8192];
	char variables[8192];
	char zwi[8192];
	char zwi2[256];
	int t;
	int z;
	int f;
	int stage;

	//puts("================== IN PARSE_CURSOR_STATEMENT ====================");

	strcpy(statement, "");
	strcpy(variables, "");
	stage=0;
	printf("parse_cursor_statement >%s<\n", cursor_container_tab[idx]);
	for(t=1;;t++){
		word(cursor_container_tab[idx], zwi, t);
		//printf("parse.. zwi=>%s<\n", zwi);
		if(strlen(zwi)==0) break;
		if(stage==1){
			if(strUcmp(zwi, "FROM")==0){
				stage=0;
			}
			else{
				if(zwi[0]!=':'){
					fprintf(fplst, "SYNTAX ERROR: not a host variable >%s<\n", zwi);
					errorcnt++;
					return;
				}
				else{
					sprintf(zwi2, "%s", zwi+1);
					strcat(variables, zwi2);
					strcat(variables, " ");
				}
			}
		}
		if(strUcmp(zwi, "INTO")==0){
			stage=1;
		}
		if(stage==0){
			strcat(statement, zwi);
			strcat(statement, " ");
		}
	}
	if(stage==1){
		fprintf(fplst, "SYNTAX ERROR: invalid CURSOR/SELECT statement\n");
		errorcnt++;
		return;
	}
	printf("parse cursor: statement=>%s<\n", statement);
	printf("parse cursor: open_cursor_line=>%s<\n", open_cursor_line);
	z=1;
	fprintf(fp, "DBPRE *  %s\n", open_cursor_line);
	fprintf(fplst, "DBPRE *  %s\n", open_cursor_line);
	fprintf(fp, "DBPRE       IF SQLCA-CURSOR-CTRL (%d) = 1\n", idx+1);
	fprintf(fplst, "DBPRE       IF SQLCA-CURSOR-CTRL (%d) = 1\n", idx+1);
	fprintf(fp, "DBPRE          SET DB-CURSOR-ALREADY-OPEN TO TRUE\n");
	fprintf(fplst, "DBPRE          SET DB-CURSOR-ALREADY-OPEN TO TRUE\n");
	fprintf(fp, "DBPRE          PERFORM DB-STATUS\n");
	fprintf(fplst, "DBPRE          PERFORM DB-STATUS\n");
	fprintf(fp, "DBPRE       END-IF\n");
	fprintf(fplst, "DBPRE       END-IF\n");
	fprintf(fp, "DBPRE       MOVE 1 TO SQLCA-CURSOR-CTRL (%d)\n", idx+1);
	fprintf(fplst, "DBPRE       MOVE 1 TO SQLCA-CURSOR-CTRL (%d)\n", idx+1);
	fprintf(fp, "DBPRE       MOVE LOW-VALUES TO SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE       MOVE LOW-VALUES TO SQLCA-STATEMENT\n");
	for(t=0;;t+=25){
		if(t>strlen(statement)){
			break;
		}
		strcpy(zwi, statement+t);
		zwi[25]=0;
		fprintf(fp, "DBPRE       MOVE '%s' TO SQLCA-STAT-LINE (%d)\n", zwi, z);
		fprintf(fplst, "DBPRE       MOVE '%s' TO SQLCA-STAT-LINE (%d)\n", zwi, z);
		z++;
	}
	fprintf(fp, "DBPRE       CALL 'MySQL_query' USING SQLCA-STATEMENT\n");
	fprintf(fp, "DBPRE       END-CALL\n");
	fprintf(fp, "DBPRE       MOVE RETURN-CODE TO SQLCODE\n");
	fprintf(fplst, "DBPRE       CALL 'MySQL_query' USING SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE       END-CALL\n");
	fprintf(fplst, "DBPRE       MOVE RETURN-CODE TO SQLCODE\n");
	fprintf(fp, "DBPRE       IF DB-OK\n");
	fprintf(fp, "DBPRE          CALL 'MySQL_use_result' USING SQLCA-RESULT (%d)\n", idx+1);
	fprintf(fp, "DBPRE          END-CALL\n");
	fprintf(fp, "DBPRE          IF SQLCA-RESULT (%d) = NULL\n", idx+1);
	fprintf(fp, "DBPRE             MOVE 100 TO SQLCODE\n");
	fprintf(fp, "DBPRE          ELSE\n");
	fprintf(fp, "DBPRE             MOVE 0 TO SQLCODE\n");
	fprintf(fp, "DBPRE          END-IF\n");
	fprintf(fp, "DBPRE       END-IF\n");
	fprintf(fplst, "DBPRE       IF DB-OK\n");
	fprintf(fplst, "DBPRE          CALL 'MySQL_use_result' USING SQLCA-RESULT (%d)\n", idx+1);
	fprintf(fplst, "DBPRE          END-CALL\n");
	fprintf(fplst, "DBPRE          IF SQLCA-RESULT (%d) = NULL\n", idx+1);
	fprintf(fplst, "DBPRE             MOVE 100 TO SQLCODE\n");
	fprintf(fplst, "DBPRE          ELSE\n");
	fprintf(fplst, "DBPRE             MOVE 0 TO SQLCODE\n");
	fprintf(fplst, "DBPRE          END-IF\n");
	fprintf(fplst, "DBPRE       END-IF\n");
	fprintf(fp, "DBPRE       IF DB-OK\n");
	// fetch_fields fetches the field NAMES, not the content
	//fprintf(fp, "DBPRE          CALL 'MySQL_fetch_fields' USING SQLCA-RESULT (%d)\n", idx+1);
	fprintf(fp, "DBPRE          CALL 'MySQL_fetch_row' USING SQLCA-RESULT (%d)\n", idx+1);
	for(t=1;;t++){
		word(variables, zwi, t);
		if(strlen(zwi)==0) break;
		fprintf(fp, "DBPRE                                          %s\n", zwi);
	}
	fprintf(fp, "DBPRE          END-CALL\n");
	fprintf(fp, "DBPRE          IF SQLCA-RESULT (%d) = NULL\n", idx+1);
	fprintf(fp, "DBPRE             MOVE 100 TO SQLCODE\n");
	fprintf(fp, "DBPRE          ELSE\n");
	fprintf(fp, "DBPRE             MOVE 0 TO SQLCODE\n");
	fprintf(fp, "DBPRE          END-IF\n");
	fprintf(fp, "DBPRE       END-IF\n");
	fprintf(fplst, "DBPRE       IF DB-OK\n");
	//fprintf(fplst, "DBPRE          CALL 'MySQL_fetch_fields' USING SQLCA-RESULT (%d)\n", idx+1);
	fprintf(fplst, "DBPRE          CALL 'MySQL_fetch_row' USING SQLCA-RESULT (%d)\n", idx+1);
	for(t=1;;t++){
		word(variables, zwi, t);
		if(strlen(zwi)==0) break;
		fprintf(fplst, "DBPRE                                          %s\n", zwi);
	}
	fprintf(fplst, "DBPRE          END-CALL\n");
	fprintf(fplst, "DBPRE          IF SQLCA-RESULT (%d) = NULL\n", idx+1);
	fprintf(fplst, "DBPRE             MOVE 100 TO SQLCODE\n");
	fprintf(fplst, "DBPRE          ELSE\n");
	fprintf(fplst, "DBPRE             MOVE 0 TO SQLCODE\n");
	fprintf(fplst, "DBPRE          END-IF\n");
	fprintf(fplst, "DBPRE       END-IF\n");
	//printf("variables=>%s<\n", variables);
}
/******************************************************************************/
void parse_select_statement(FILE *fp, FILE *fplst)
{
	char statement[8192];
	char variables[8192];
	char zwi[8192];
	char zwi2[256];
	int t;
	int z;
	int stage;

	strcpy(statement, "");
	strcpy(variables, "");
	stage=0;
	//printf("parse_select_statement >%s<\n", collect_container);
	for(t=1;;t++){
		word(collect_container, zwi, t);
		//printf("parse.. zwi=>%s<\n", zwi);
		if(strlen(zwi)==0) break;
		if(stage==1){
			if(strUcmp(zwi, "FROM")==0){
				stage=0;
			}
			else{
				if(zwi[0]!=':'){
					fprintf(fplst, "SYNTAX ERROR: not a host variable >%s<\n", zwi);
					errorcnt++;
					return;
				}
				else{
					sprintf(zwi2, "%s", zwi+1);
					strcat(variables, zwi2);
					strcat(variables, " ");
				}
			}
		}
		if(strUcmp(zwi, "INTO")==0){
			stage=1;
		}
		if(stage==0){
			strcat(statement, zwi);
			strcat(statement, " ");
		}
	}
	if(stage==1){
		fprintf(fplst, "SYNTAX ERROR: invalid SELECT statement\n");
		errorcnt++;
		return;
	}
	//printf("statement=>%s<\n", statement);
	z=1;
	fprintf(fp, "DBPRE       MOVE LOW-VALUES TO SQLCA-STATEMENT\n");
	for(t=0;;t+=25){
		if(t>strlen(statement)){
			break;
		}
		strcpy(zwi, statement+t);
		zwi[25]=0;
		fprintf(fp, "DBPRE       MOVE '%s' TO SQLCA-STAT-LINE (%d)\n", zwi, z);
		fprintf(fplst, "DBPRE       MOVE '%s' TO SQLCA-STAT-LINE (%d)\n", zwi, z);
		z++;
	}
	fprintf(fp, "DBPRE       CALL 'MySQL_query' USING SQLCA-STATEMENT\n");
	fprintf(fp, "DBPRE       END-CALL\n");
	fprintf(fp, "DBPRE       MOVE RETURN-CODE TO SQLCODE\n");
	fprintf(fplst, "DBPRE       CALL 'MySQL_query' USING SQLCA-STATEMENT\n");
	fprintf(fplst, "DBPRE       END-CALL\n");
	fprintf(fplst, "DBPRE       MOVE RETURN-CODE TO SQLCODE\n");
	fprintf(fp, "DBPRE       IF DB-OK\n");
	fprintf(fp, "DBPRE          CALL 'MySQL_use_result' USING SQLCA-RESULT (%d)\n", MAX_CURSORS+1);
	fprintf(fp, "DBPRE          END-CALL\n");
	fprintf(fp, "DBPRE          IF SQLCA-RESULT (%d) = NULL\n", MAX_CURSORS+1);
	fprintf(fp, "DBPRE             MOVE 100 TO SQLCODE\n");
	fprintf(fp, "DBPRE          ELSE\n");
	fprintf(fp, "DBPRE             MOVE 0 TO SQLCODE\n");
	fprintf(fp, "DBPRE          END-IF\n");
	fprintf(fp, "DBPRE       END-IF\n");
	fprintf(fplst, "DBPRE       IF DB-OK\n");
	fprintf(fplst, "DBPRE          CALL 'MySQL_use_result' USING SQLCA-RESULT (%d)\n", MAX_CURSORS+1);
	fprintf(fplst, "DBPRE          END-CALL\n");
	fprintf(fplst, "DBPRE          IF SQLCA-RESULT (%d) = NULL\n", MAX_CURSORS+1);
	fprintf(fplst, "DBPRE             MOVE 100 TO SQLCODE\n");
	fprintf(fplst, "DBPRE          ELSE\n");
	fprintf(fplst, "DBPRE             MOVE 0 TO SQLCODE\n");
	fprintf(fplst, "DBPRE          END-IF\n");
	fprintf(fplst, "DBPRE       END-IF\n");
	fprintf(fp, "DBPRE       IF DB-OK\n");
	// fetch_fields fetches the field NAMES, not the content
	//fprintf(fp, "DBPRE          CALL 'MySQL_fetch_fields' USING SQLCA-RESULT (%d)\n", MAX_CURSORS+1);
	fprintf(fp, "DBPRE          CALL 'MySQL_fetch_row' USING SQLCA-RESULT (%d)\n", MAX_CURSORS+1);
	for(t=1;;t++){
		word(variables, zwi, t);
		if(strlen(zwi)==0) break;
		fprintf(fp, "DBPRE                                          %s\n", zwi);
	}
	fprintf(fp, "DBPRE          END-CALL\n");
	fprintf(fp, "DBPRE          IF SQLCA-RESULT (%d) = NULL\n", MAX_CURSORS+1);
	fprintf(fp, "DBPRE             MOVE 100 TO SQLCODE\n");
	fprintf(fp, "DBPRE          ELSE\n");
	fprintf(fp, "DBPRE             MOVE 0 TO SQLCODE\n");
	fprintf(fp, "DBPRE          END-IF\n");
	fprintf(fp, "DBPRE       END-IF\n");
	fprintf(fplst, "DBPRE       IF DB-OK\n");
	//fprintf(fplst, "DBPRE          CALL 'MySQL_fetch_fields' USING SQLCA-RESULT (%d)\n", MAX_CURSORS+1);
	fprintf(fplst, "DBPRE          CALL 'MySQL_fetch_row' USING SQLCA-RESULT (%d)\n", MAX_CURSORS+1);
	for(t=1;;t++){
		word(variables, zwi, t);
		if(strlen(zwi)==0) break;
		fprintf(fplst, "DBPRE                                          %s\n", zwi);
	}
	fprintf(fplst, "DBPRE          END-CALL\n");
	fprintf(fplst, "DBPRE          IF SQLCA-RESULT (%d)= NULL\n", MAX_CURSORS+1);
	fprintf(fplst, "DBPRE             MOVE 100 TO SQLCODE\n");
	fprintf(fplst, "DBPRE          ELSE\n");
	fprintf(fplst, "DBPRE             MOVE 0 TO SQLCODE\n");
	fprintf(fplst, "DBPRE          END-IF\n");
	fprintf(fplst, "DBPRE       END-IF\n");
	//printf("variables=>%s<\n", variables);
}
/******************************************************************************/
int do_handle_input_line(FILE *fp, FILE *fplst, unsigned char *line, 
                                                unsigned char *fullline)
{
	char zwi[256];
	char w1[256];
	char w2[256];
	char w3[256];
	char w4[256];
	char w5[256];
	char w6[256];
	int f;
	int t, i;
	int offset;
	char zwioffset[256];

	if(freestyle==FALSE){
		if(line[6]=='*'){
			return(FALSE);
		}
	}

	word(line, w1, 1);
	if(strlen(w1)==0){
		return(FALSE);
	}

	word(line, w2, 2);
	word(line, w3, 3);
	word(line, w4, 4);

	ucase(w1);
	ucase(w2);
	ucase(w3);
	ucase(w4);

	// remove AUTHOR. and DATE-WRITEN. OpenCOBOL cant handle that
	if(strcmp(w1, "AUTHOR.")==0){
		if(freestyle==TRUE){
			sprintf(zwi, "//%s", fullline);
		}
		else{
			sprintf(zwi, "DBPRE *%s", line);
		}
		fprintf(fp, "%s\n", zwi);
		fprintf(fplst, "%s\n", zwi);
		return(TRUE);
	}
	if(strcmp(w1, "DATE-WRITTEN.")==0){
		if(freestyle==TRUE){
			sprintf(zwi, "//%s", fullline);
		}
		else{
			sprintf(zwi, "DBPRE *%s", line);
		}
		fprintf(fp, "%s\n", zwi);
		fprintf(fplst, "%s\n", zwi);
		return(TRUE);
	}

	if(strcmp(w1, "PROCEDURE")==0 && 
	   ( strcmp(w2, "DIVISION")==0 || strcmp(w2, "DIVISION.")==0) ){
		proceduremode=TRUE;
	}

	if(atoi(w1)>0 && atoi(w1)<99){ // numeric? must be a field declaration..
		return(FALSE);
	}

	if(strcmp(w1, "EXEC")==0){
		// so we count too EXEC CICS and such, because both are
		// terminated with END-EXEC.
		execsqlcount++;
	}
	if(strcmp(w1, "EXEC")==0 &&
	   strcmp(w2, "SQL")==0){
		if(strlen(w3)>0){
			fprintf(fp, "%s\n", line);
			fprintf(fplst, "%s\n", line);
			fprintf(fplst, "SYNTAX ERROR: invalid parameter >%s<\n", w3);
			errorcnt++;
			return(TRUE);
		}
		//if(execsqlmode==TRUE){
		//	fprintf(fp, "%s\n", fullline);
		//	fprintf(fplst, "%s\n", fullline);
		//	fprintf(fplst, "SYNTAX ERROR: nested EXEC SQL\n");
		//	errorcnt++;
		//	return(TRUE);
		//}
		execsqlmode=TRUE;
		for(t=0; t<80; t++){
			if(line[t]!=' ') break;
		}
		if(t>8){
			t=8;
		}
		for(i=0; i<t; i++){
			zwioffset[i]=' ';
		}
		zwioffset[i]=0;
		if(proceduremode==TRUE){
			if(freestyle==TRUE){
				sprintf(zwi, "MOVE %d             TO SQLCA-SEQUENCE", sequencecnt);
			}
			else{
				sprintf(zwi, "DBPRE  %sMOVE %d             TO SQLCA-SEQUENCE", 
								  zwioffset, sequencecnt);
			}
			sequencecnt++;
			fprintf(fp, "%s\n", zwi);
			fprintf(fplst, "%s\n", zwi);
		}
		if(freestyle==TRUE){
			sprintf(zwi, "//%s", line);
		}
		else{
			strcpy(zwi, fullline);
			zwi[6]='*';
		}
		fprintf(fp, "%s\n", zwi);
		fprintf(fplst, "%s\n", zwi);
		return(TRUE);
	}

	if(strcmp(w1, "END-EXEC.")==0){
		if(strlen(w2)>0){
			fprintf(fp, "%s\n", line);
			fprintf(fplst, "%s\n", line);
			fprintf(fplst, "SYNTAX ERROR: invalid parameter >%s<.\n", w2);
			errorcnt++;
			return(TRUE);
		}
		fprintf(fp, "DBPRE *%s\n", line);
		fprintf(fplst, "DBPRE *%s\n", line);
		if(execsqlcount<1){
			fprintf(fp, "%s\n", fullline);
			fprintf(fplst, "%s\n", fullline);
			fprintf(fplst, "SYNTAX ERROR: END-EXEC. without EXEC SQL.\n");
			errorcnt++;
			return(TRUE);
		}
		execsqlmode=FALSE;
		execsqlcount--;
		if(collect_cursor_statement==TRUE){
			trim(collect_container);
			printf("cursor: collect_container=>%s<\n", collect_container);
			cursor_container_tab[cursorcnt]=malloc(strlen(collect_container)+1);
			strcpy(cursor_container_tab[cursorcnt], collect_container);
			strcpy(cursor_name_tab[cursorcnt], collect_cursor_name);
			cursorcnt++;
			strcpy(collect_container, "");
			strcpy(collect_cursor_name, "");
			collect_cursor_statement=FALSE;
			return(TRUE);
		}
		if(collect_select_statement==TRUE){
			parse_select_statement(fp, fplst);
			strcpy(collect_container, "");
			//printf("collect_container=>%s<\n", collect_container);
			collect_select_statement=FALSE;
		}
		if(collect_fetch_statement==TRUE){
			parse_fetch_statement(fp, fplst);
			strcpy(collect_container, "");
			//printf("collect_container=>%s<\n", collect_container);
			collect_fetch_statement=FALSE;
		}
		if(collect_delete_statement==TRUE){
			parse_delete_statement(fp, fplst);
			strcpy(collect_container, "");
			//printf("collect_container=>%s<\n", collect_container);
			collect_delete_statement=FALSE;
		}
		if(collect_update_statement==TRUE){
			parse_update_statement(fp, fplst);
			strcpy(collect_container, "");
			//printf("collect_container=>%s<\n", collect_container);
			collect_update_statement=FALSE;
		}
		if(collect_insert_statement==TRUE){
			parse_insert_statement(fp, fplst);
			strcpy(collect_container, "");
			//printf("collect_container=>%s<\n", collect_container);
			collect_insert_statement=FALSE;
		}
		exec_sql_error=FALSE;
		return(TRUE);
	}

	if(execsqlmode==TRUE){
		//printf("1:execsqlmode=TRUE: w1=>%s< w2=>%s<\n", w1, w2);
	   // we had already an error in this EXEC SQL statement?
		// don't parse the remaining coding of it!!
	   if(exec_sql_error==TRUE){
			return(FALSE);
		}
		if(collect_select_statement==TRUE ||
		   collect_delete_statement==TRUE ||
		   collect_update_statement==TRUE ||
		   collect_insert_statement==TRUE ||
		   collect_cursor_statement==TRUE ||
		   collect_fetch_statement==TRUE){
			strcat(collect_container, " ");
			strcat(collect_container, line);
			fprintf(fp, "DBPRE *%s\n", line);
			fprintf(fplst, "DBPRE *%s\n", line);
			return(TRUE);
		}
		//printf("2:execsqlmode=TRUE: w1=>%s< w2=>%s<\n", w1, w2);
		if(strcmp(w1, "INCLUDE")==0){
			strcpy(replace_old, "");
			strcpy(replace_new, "");
			word(line, w3, 3);
			ucase(w3);
			if(strcmp(w3, "REPLACING")==0){
				word(line, w4, 4);
				word(line, w5, 5);
				ucase(w5);
				word(line, w6, 6);
				if(w4[0]!='\''){
					fprintf(fp, "%s\n", line);
					fprintf(fplst, "%s\n", line);
					fprintf(fplst, "SYNTAX ERROR: invalid parameter >%s<.\n", w5);
					errorcnt++;
					return(TRUE);
				}
				if(strcmp(w5, "BY")!=0){
					fprintf(fp, "%s\n", line);
					fprintf(fplst, "%s\n", line);
					fprintf(fplst, "SYNTAX ERROR: invalid parameter >%s<.\n", w5);
					errorcnt++;
					return(TRUE);
				}
				if(w6[0]!='\''){
					fprintf(fp, "%s\n", line);
					fprintf(fplst, "%s\n", line);
					fprintf(fplst, "SYNTAX ERROR: invalid parameter >%s<.\n", w5);
					errorcnt++;
					return(TRUE);
				}
				w4[strlen(w4)-1]=0; // remove the trailing '
				w6[strlen(w6)-1]=0;
				if(w6[strlen(w6)-1]=='\''){  // there might also have been a .
					w6[strlen(w6)-1]=0;
				}
				strcpy(replace_old, w4+1);
				strcpy(replace_new, w6+1);
				//printf("replace_old=>%s< replace_new=>%s<\n",replace_old, replace_new);
			}
			fpincnt++;
			printf("1) fpincnt=%d\n", fpincnt);
			if(w2[strlen(w2)-1]=='.'){
				w2[strlen(w2)-1]=0;
			}
			sprintf(zwi, "%s/%s", conf_copybookpath, w2);
			fpin[fpincnt]=fopen(zwi, "rb");
         if(fpin[fpincnt]==NULL){
				fpincnt--;
				errorcnt++;
				fprintf(fp, "%s\n", fullline);
				fprintf(fplst, "%s\n", fullline);
				fprintf(fplst, "SYNTAX ERROR: could not include file >%s<.\n", w2);
				return(TRUE);
			}
			if(freestyle==TRUE){
				sprintf(zwi, "//%s", line);
			}
			else{
				strcpy(zwi, fullline);
				zwi[6]='*';
			}
			fprintf(fp, "%s\n", zwi);
			fprintf(fplst, "%s\n", zwi);
			execsqlmode=FALSE; // or error detection wont work!
			return(TRUE);
		} // INCLUDE...
		if(strcmp(w1, "INIT")==0 && strcmp(w2, "DB")==0){
			for(t=0; t<80; t++){
				if(line[t]!=' ') break;
			}
			if(t>8){
				t=8;
			}
			for(i=0; i<t; i++){
				zwioffset[i]=' ';
			}
			zwioffset[i]=0;
			if(strlen(w3)>0){
				fprintf(fp, "%s\n", line);
				fprintf(fplst, "%s\n", line);
				fprintf(fplst, "SYNTAX ERROR: invalid parameter >%s<.\n", w2);
				errorcnt++;
				return(TRUE);
			}
			if(freestyle==TRUE){
				sprintf(zwi, "//%s", line);
			}
			else{
				strcpy(zwi, fullline);
				zwi[6]='*';
			}
			fprintf(fp, "%s\n", zwi);
			fprintf(fplst, "%s\n", zwi);
		 	fprintf(fp,"DBPRE %s CALL \"MySQL_init\"  USING SQLCA-CID\n",zwioffset);
		 	fprintf(fp, "DBPRE %s END-CALL\n", zwioffset);
		 	fprintf(fp, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s CALL \"MySQL_init\"  USING SQLCA-CID\n",zwioffset);
		 	fprintf(fplst, "DBPRE %s END-CALL\n",zwioffset);
		 	fprintf(fplst, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n",zwioffset);
			return(TRUE);
		} // INIT DB...
		if(strcmp(w1, "CLOSE")==0 && strcmp(w2, "DB")==0){
			for(t=0; t<80; t++){
				if(line[t]!=' ') break;
			}
			if(t>8){
				t=8;
			}
			for(i=0; i<t; i++){
				zwioffset[i]=' ';
			}
			zwioffset[i]=0;
			if(strlen(w3)>0){
				fprintf(fp, "%s\n", line);
				fprintf(fplst, "%s\n", line);
				fprintf(fplst, "SYNTAX ERROR: invalid parameter >%s<.\n", w2);
				errorcnt++;
				return(TRUE);
			}
			if(freestyle==TRUE){
				sprintf(zwi, "//%s", line);
			}
			else{
				strcpy(zwi, fullline);
				zwi[6]='*';
			}
			fprintf(fp, "%s\n", zwi);
			fprintf(fplst, "%s\n", zwi);
		 	fprintf(fp,"DBPRE %s CALL \"MySQL_close\"\n",zwioffset);
		 	fprintf(fp, "DBPRE %s END-CALL\n", zwioffset);
		 	fprintf(fp, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s CALL \"MySQL_close\"\n",zwioffset);
		 	fprintf(fplst, "DBPRE %s END-CALL\n",zwioffset);
		 	fprintf(fplst, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n",zwioffset);
			return(TRUE);
		} // CLOSE DB...
		if(strcmp(w1, "CLOSE")==0 && strcmp(w2, "DB")!=0){
			f=0;
			for(t=0; t<cursorcnt; t++){
				//printf("compare >%s<->%s<\n", cursor_name_tab[t], w2);
				if(strcmp(cursor_name_tab[t], w2)==0){
					f=1;
					break;
				}
			}
			if(f==0){
				fprintf(fp, "%s\n", fullline);
				fprintf(fplst, "%s\n", fullline);
				fprintf(fplst, "SYNTAX ERROR: CURSOR >%s< is not DECLARE'd.\n", w2);
				errorcnt++;
				exec_sql_error=TRUE;
				return(TRUE);
			}
			if(freestyle==TRUE){
				sprintf(zwi, "//%s", line);
			}
			else{
				strcpy(zwi, fullline);
				zwi[6]='*';
			}
			fprintf(fp, "%s\n", zwi);
			fprintf(fplst, "%s\n", zwi);
			//fprintf(fp, "DBPRE *  %s\n", open_cursor_line);
			//fprintf(fplst, "DBPRE *  %s\n", open_cursor_line);
			fprintf(fp, "DBPRE       IF SQLCA-CURSOR-CTRL (%d) = 0\n", t+1);
			fprintf(fplst, "DBPRE       IF SQLCA-CURSOR-CTRL (%d) = 0\n", t+1);
			fprintf(fp, "DBPRE          SET DB-CURSOR-NOT-OPEN TO TRUE\n");
			fprintf(fplst, "DBPRE          SET DB-CURSOR-NOT-OPEN TO TRUE\n");
			fprintf(fp, "DBPRE          PERFORM DB-STATUS\n");
			fprintf(fplst, "DBPRE          PERFORM DB-STATUS\n");
			fprintf(fp, "DBPRE       END-IF\n");
			fprintf(fplst, "DBPRE       END-IF\n");
			fprintf(fp, "DBPRE       MOVE 0 TO SQLCA-CURSOR-CTRL (%d)\n", t+1);
			fprintf(fplst, "DBPRE       MOVE 0 TO SQLCA-CURSOR-CTRL (%d)\n", t+1);
			return(TRUE);
		} // CLOSE CURSOR...

		if(strcmp(w1, "CONNECT")==0 && strcmp(w2, "DB")==0){
			for(t=0; t<80; t++){
				if(line[t]!=' ') break;
			}
			if(t>8){
				t=8;
			}
			for(i=0; i<t; i++){
				zwioffset[i]=' ';
			}
			zwioffset[i]=0;
			if(strlen(w3)>0){
				fprintf(fp, "%s\n", line);
				fprintf(fplst, "%s\n", line);
				fprintf(fplst, "SYNTAX ERROR: invalid parameter >%s<.\n", w2);
				errorcnt++;
				return(TRUE);
			}
			if(freestyle==TRUE){
				sprintf(zwi, "//%s", line);
			}
			else{
				strcpy(zwi, fullline);
				zwi[6]='*';
			}
			fprintf(fp, "%s\n", zwi);
			fprintf(fplst, "%s\n", zwi);
		 	fprintf(fp, "DBPRE %s CALL \"MySQL_real_connect\" USING\n",zwioffset);
		 	fprintf(fp, "DBPRE %s                           SQLCA-HOST\n", zwioffset);
		 	fprintf(fp, "DBPRE %s                           SQLCA-USER\n", zwioffset);
		 	fprintf(fp, "DBPRE %s                           SQLCA-PASSWD\n", zwioffset);
		 	fprintf(fp, "DBPRE %s                           SQLCA-DBNAME\n", zwioffset);
		 	fprintf(fp, "DBPRE %s                           SQLCA-PORT\n", zwioffset);
		 	fprintf(fp, "DBPRE %s                           SQLCA-SOCKET\n", zwioffset);
		 	fprintf(fp, "DBPRE %s END-CALL\n", zwioffset);
		 	fprintf(fp, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s CALL \"MySQL_real_connect\" USING\n",zwioffset);
		 	fprintf(fplst, "DBPRE %s                           SQLCA-HOST\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s                           SQLCA-USER\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s                           SQLCA-PASSWD\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s                           SQLCA-DBNAME\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s                           SQLCA-PORT\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s                           SQLCA-SOCKET\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s END-CALL\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n", zwioffset);
			return(TRUE);
		} // CONNECT DB...

		if(strcmp(w1, "SELECT")==0 && strcmp(w2, "DB")==0){
			for(t=0; t<80; t++){
				if(line[t]!=' ') break;
			}
			if(t>8){
				t=8;
			}
			for(i=0; i<t; i++){
				zwioffset[i]=' ';
			}
			zwioffset[i]=0;
			if(strlen(w3)>0){
				fprintf(fp, "%s\n", line);
				fprintf(fplst, "%s\n", line);
				fprintf(fplst, "SYNTAX ERROR: invalid parameter >%s<.\n", w2);
				errorcnt++;
				return(TRUE);
			}
			if(freestyle==TRUE){
				sprintf(zwi, "//%s", line);
			}
			else{
				strcpy(zwi, fullline);
				zwi[6]='*';
			}
			fprintf(fp, "%s\n", zwi);
			fprintf(fplst, "%s\n", zwi);
		 	fprintf(fp,"DBPRE %s CALL \"MySQL_selectdb\" USING SQLCA-DBNAME\n",zwioffset);
		 	fprintf(fp, "DBPRE %s END-CALL\n", zwioffset);
		 	fprintf(fp, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n", zwioffset);
		 	fprintf(fp, "DBPRE %s IF RETURN-CODE NOT = 0 THEN\n", zwioffset);
		 	fprintf(fp, "DBPRE %s    PERFORM DB-STATUSR\n", zwioffset);
		 	fprintf(fp, "DBPRE %s END-IF\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s CALL \"MySQL_selectdb\" USING SQLCA-DBNAME\n",zwioffset);
		 	fprintf(fplst, "DBPRE %s END-CALL\n",zwioffset);
		 	fprintf(fplst, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n",zwioffset);
		 	fprintf(fplst, "DBPRE %s IF RETURN-CODE NOT = 0 THEN\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s    PERFORM DB-STATUS\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s END-IF\n", zwioffset);
			return(TRUE);
		} // SELECT DB...

		if(strcmp(w1, "COMMIT")==0){
			for(t=0; t<80; t++){
				if(line[t]!=' ') break;
			}
			if(t>8){
				t=8;
			}
			for(i=0; i<t; i++){
				zwioffset[i]=' ';
			}
			zwioffset[i]=0;
			if(strlen(w2)>0){
				fprintf(fp, "%s\n", line);
				fprintf(fplst, "%s\n", line);
				fprintf(fplst, "SYNTAX ERROR: invalid parameter >%s<.\n", w2);
				errorcnt++;
				return(TRUE);
			}
			if(freestyle==TRUE){
				sprintf(zwi, "//%s", line);
			}
			else{
				strcpy(zwi, fullline);
				zwi[6]='*';
			}
			fprintf(fp, "%s\n", zwi);
			fprintf(fplst, "%s\n", zwi);
		 	fprintf(fp,"DBPRE %s CALL \"MySQL_commit\"\n", zwioffset);
		 	fprintf(fp, "DBPRE %s END-CALL\n", zwioffset);
		 	fprintf(fp, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n", zwioffset);
		 	fprintf(fp, "DBPRE %s IF RETURN-CODE NOT = 0 THEN\n", zwioffset);
		 	fprintf(fp, "DBPRE %s    PERFORM DB-STATUS\n", zwioffset);
		 	fprintf(fp, "DBPRE %s END-IF\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s CALL \"MySQL_commit\"\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s END-CALL\n",zwioffset);
		 	fprintf(fplst, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n",zwioffset);
		 	fprintf(fplst, "DBPRE %s IF RETURN-CODE NOT = 0 THEN\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s    PERFORM DB-STATUS\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s END-IF\n", zwioffset);
			return(TRUE);
		} // COMMIT

		if(strcmp(w1, "ROLLBACK")==0){
			for(t=0; t<80; t++){
				if(line[t]!=' ') break;
			}
			if(t>8){
				t=8;
			}
			for(i=0; i<t; i++){
				zwioffset[i]=' ';
			}
			zwioffset[i]=0;
			if(strlen(w2)>0){
				fprintf(fp, "%s\n", line);
				fprintf(fplst, "%s\n", line);
				fprintf(fplst, "SYNTAX ERROR: invalid parameter >%s<.\n", w2);
				errorcnt++;
				return(TRUE);
			}
			if(freestyle==TRUE){
				sprintf(zwi, "//%s", line);
			}
			else{
				strcpy(zwi, fullline);
				zwi[6]='*';
			}
			fprintf(fp, "%s\n", zwi);
			fprintf(fplst, "%s\n", zwi);
		 	fprintf(fp,"DBPRE %s CALL \"MySQL_rollback\"\n", zwioffset);
		 	fprintf(fp, "DBPRE %s END-CALL\n", zwioffset);
		 	fprintf(fp, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n", zwioffset);
			// dont call DB-STATUS, this will cause a recursion!!
		 	//fprintf(fp, "DBPRE %s IF RETURN-CODE NOT = 0 THEN\n", zwioffset);
		 	//fprintf(fp, "DBPRE %s    PERFORM DB-STATUS\n", zwioffset);
		 	//fprintf(fp, "DBPRE %s END-IF\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s CALL \"MySQL_rollback\"\n", zwioffset);
		 	fprintf(fplst, "DBPRE %s END-CALL\n",zwioffset);
		 	fprintf(fplst, "DBPRE %s MOVE RETURN-CODE    TO SQLCODE\n",zwioffset);
		 	//fprintf(fplst, "DBPRE %s IF RETURN-CODE NOT = 0 THEN\n", zwioffset);
		 	//fprintf(fplst, "DBPRE %s    PERFORM DB-STATUS\n", zwioffset);
		 	//fprintf(fplst, "DBPRE %s END-IF\n", zwioffset);
			return(TRUE);
		} // ROLLBACK

		if(strcmp(w1, "DECLARE")==0 && strcmp(w3, "CURSOR")==0 &&
		   strcmp(w4, "FOR")==0){
			if(cursorcnt>=MAX_CURSORS){
				fprintf(fp, "%s\n", fullline);
				fprintf(fplst, "%s\n", fullline);
				fprintf(fplst, "SYNTAX ERROR: More than %d CURSORs!!!\n", MAX_CURSORS);
				errorcnt++;
				exec_sql_error=TRUE;
				return(TRUE);
			}
			for(t=0; t<cursorcnt; t++){
				//printf("compare >%s<->%s<\n", cursor_name_tab[t], w2);
				if(strcmp(cursor_name_tab[t], w2)==0){
					fprintf(fp, "%s\n", fullline);
					fprintf(fplst, "%s\n", fullline);
					fprintf(fplst, "SYNTAX ERROR: CURSOR >%s< is already DECLARE'd.\n", w2);
					errorcnt++;
					exec_sql_error=TRUE;
					return(TRUE);
				}
			}
			collect_cursor_statement=TRUE;
			// ignore the DECLARE .. CURSOR line!!!
			//strcpy(collect_container, line);
			strcpy(collect_container, "");
			fprintf(fp, "DBPRE *%s\n", line);
			fprintf(fplst, "DBPRE *%s\n", line);
			strcpy(collect_cursor_name, w2);
			return(TRUE);
		} // collect DECLARE ... CURSOR FOR statement..

		//printf("w1=>%s< w2=>%s<\n", w1, w2);
		if(strcmp(w1, "OPEN")==0 && strlen(w2)!=0){
			//puts("OPEN CURSOR found!!!!!");
			f=0;
			for(t=0; t<cursorcnt; t++){
				//printf("compare >%s<->%s<\n", cursor_name_tab[t], w2);
				if(strcmp(cursor_name_tab[t], w2)==0){
					f=1;
					break;
				}
			}
			if(f==0){
				fprintf(fp, "%s\n", fullline);
				fprintf(fplst, "%s\n", fullline);
				fprintf(fplst, "SYNTAX ERROR: CURSOR >%s< is not DECLARE'd.\n", w2);
				errorcnt++;
				exec_sql_error=TRUE;
				return(TRUE);
			}
			strcpy(open_cursor_line, line);
			puts("vor parse_cursor_statement.......");
			parse_cursor_statement(fp, fplst, t);
			return(TRUE);
		} // collect OPEN ... CURSOR statement..

		if(strcmp(w1, "SELECT")==0 && strcmp(w2, "DB")!=0){
			collect_select_statement=TRUE;
			strcpy(collect_container, line);
			fprintf(fp, "DBPRE *%s\n", line);
			fprintf(fplst, "DBPRE *%s\n", line);
			return(TRUE);
		} // collect SELECT statement..

		if(strcmp(w1, "DELETE")==0){
			collect_delete_statement=TRUE;
			strcpy(collect_container, line);
			fprintf(fp, "DBPRE *%s\n", line);
			fprintf(fplst, "DBPRE *%s\n", line);
			return(TRUE);
		} // collect DELETE statement..

		if(strcmp(w1, "UPDATE")==0){
			collect_update_statement=TRUE;
			strcpy(collect_container, line);
			fprintf(fp, "DBPRE *%s\n", line);
			fprintf(fplst, "DBPRE *%s\n", line);
			return(TRUE);
		} // collect UPDATE statement..
		if(strcmp(w1, "INSERT")==0){
			collect_insert_statement=TRUE;
			strcpy(collect_container, line);
			fprintf(fp, "DBPRE *%s\n", line);
			fprintf(fplst, "DBPRE *%s\n", line);
			return(TRUE);
		} // collect INSERT statement..

		//printf("w1=>%s< w2=>%s<\n", w1, w2);
		/******************************************************
		if(strcmp(w1, "FETCH")==0 && strcmp(w2, "RESULT")==0){
			//printf("fetch result<=================================\n");
			collect_fetch_statement=TRUE;
			strcpy(collect_container, line);
			fprintf(fp, "DBPRE *%s\n", line);
			fprintf(fplst, "DBPRE *%s\n", line);
			return(TRUE);
		} // collect FETCH statement..
		*******************************************************/
		if(strcmp(w1, "FETCH")==0 && strlen(w2)>0){
			//printf("fetch cursor<=================================\n");
			collect_fetch_statement=TRUE;
			strcpy(fetch_cursor_name, w2);
			strcpy(collect_container, line);
			fprintf(fp, "DBPRE *%s\n", line);
			fprintf(fplst, "DBPRE *%s\n", line);
			return(TRUE);
		} // collect FETCH statement..

		fprintf(fp, "%s\n", fullline);
		fprintf(fplst, "%s\n", fullline);
		fprintf(fplst, "SYNTAX ERROR: unknown command >%s<\n", w1);
		errorcnt++;
		return(TRUE);
	}

	//fprintf(stderr, "1: >%s<\n", fullline);
	return(FALSE);
}
/******************************************************************************/
void handle_input_line(FILE *fp, FILE *fplst, unsigned char *line)
{
	char zwi[256];
	int rc;

	if(freestyle==FALSE){
		if(strlen(line)<7){
			strcpy(line, "                                                 ");
		}
		strncpy(zwi, line+7, 80);
		zwi[65]=0;
		rc=do_handle_input_line(fp, fplst, zwi, line);
	}
	else{
		rc=do_handle_input_line(fp, fplst, line, line);
	}

	if(rc==FALSE){
		fprintf(fp, "%s\n", line);
		fprintf(fplst, "%s\n", line);
	}
}
/******************************************************************************/
int main(int argc, char **argv)
{
	char infile[256];
	char outfile[256];
	char lstfile[256];
	unsigned char line[4096];
   time_t tsec;
   struct tm *s_tm;

	tsec=time(NULL);
   s_tm=localtime(&tsec);
   s_tm->tm_year+=1900;
   s_tm->tm_mon++;

	if(argc<1){
		fprintf(stderr, "DBPRE: missing argument(s), aborting...\n");
		exit(1);
	}

	handle_options(argc, argv);

	puts("dbpre 1.0");
	puts("=========");
	sprintf(infile, "%s.scb", infilename);
	sprintf(outfile, "%s.cob", infilename);
	sprintf(lstfile, "%s.lst", infilename);
	//printf("praeprocessing >%s<->%s<...\n", infile, outfile);

	fpin[fpincnt]=fopen(infile, "rb");
	if(fpin[fpincnt]==NULL){
		fprintf(stderr, "dpprae: Can't open input file >%s<, aborting..\n", infile);
		exit(2);
	}

	fpout=fopen(outfile, "wb");
	if(fpout==NULL){
		fprintf(stderr, "dpprae: Can't open output file >%s<, aborting..\n", outfile);
		exit(3);
	}
	
	fplst=fopen(lstfile, "wb");
	if(fplst==NULL){
		fprintf(stderr, "dpprae: Can't open list file >%s<, aborting..\n", 
					lstfile);
		exit(2);
	}

	fprintf(fpout, "      * dbpre %s: % -43s %04d%02d%02d-%02d%02d%02d\n",
	                        DBPREVERSION,
			                  outfile, s_tm->tm_year, s_tm->tm_mon, s_tm->tm_mday,
							      s_tm->tm_hour, s_tm->tm_min, s_tm->tm_sec);
	fprintf(fpout, "------*-------------------------------------------------------------------------\n");
	fprintf(fplst, "      * dbpre %s: % -43s %04d%02d%02d-%02d%02d%02d\n",
	                        DBPREVERSION,
			                  lstfile, s_tm->tm_year, s_tm->tm_mon, s_tm->tm_mday,
							      s_tm->tm_hour, s_tm->tm_min, s_tm->tm_sec);
	fprintf(fplst, "------*-------------------------------------------------------------------------\n");
	for(;;){
		if(read_line(fpin[fpincnt], line)==EOF){
			printf("2) fpincnt=%d\n", fpincnt);
			if(fpincnt>1){
				fclose(fpin[fpincnt]);
			}
			fpincnt--;
			if(fpincnt<0) break;
		}
		else{
			if(replace_old[0]!=0){
				replace(line, replace_old, replace_new);
			}
			handle_input_line(fpout, fplst, line);
		}
	}

	printf("%d Errors.\n", errorcnt);
	fprintf(fplst, "%d errors.\n", errorcnt);
	fclose(fplst);
	fclose(fpout);
			printf("3) fpincnt=%d\n", fpincnt);
	if(fpincnt>0){
		fclose(fpin[fpincnt]);
	}

	return(0);
}
/******************************************************************************/
