#include "dx8asm_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
int asm_parse(const char *src, asm_program *prog,char **err){ (void)err; prog->code=NULL; prog->count=prog->capacity=0; const char *cur=src; while(*cur){ while(*cur=='\n'||*cur=='\r') ++cur; const char *ls=cur; while(*cur&&*cur!='\n') ++cur; size_t len=cur-ls; if(*cur=='\n') ++cur; if(len==0) continue; char *buf=strndup(ls,len); char *sc=strchr(buf,';'); if(sc) *sc='\0'; asm_instr inst={0}; if(sscanf(buf,"%7s %31[^,], %31[^,], %31s", inst.opcode,inst.dst,inst.src0,inst.src1)>=1){ sb_push(prog->code,inst);} free(buf);} prog->count=sb_count(prog->code); prog->capacity=sb_capacity(prog->code); return 0; }
void asm_program_free(asm_program *p){ sb_free(p->code); p->code=NULL; p->count=p->capacity=0; }
