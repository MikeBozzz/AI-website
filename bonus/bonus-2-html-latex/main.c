#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIZE 100000

// TOKENS
typedef struct {
    char* type;
    char* content;
} TOKEN;

// TOKEN LIST
typedef struct {
    TOKEN toks[SIZE];
} TOKEN_LIST;

void init_list(TOKEN_LIST* newlist) {
    for(int i=0; i < SIZE; i++) { 
        newlist->toks[i].type    = "";
        newlist->toks[i].content = "";
    }
}

// SIZE OF FILE
long sizefile(FILE* file) {
    fpos_t org;

    if(fgetpos(file, &org) != 0) 
        perror("FGETPOS FAILED");

    fseek(file, 0, SEEK_END);

    long size = ftell(file);

    if(fsetpos(file, &org) != 0)
        perror("FSETPOS FAILED");
    
    return size;
}

// READ FILE
char* readfile(char* path) {
    FILE *file = fopen(path, "r");
    if(!file) {
        free(file);
        perror("FOPEN FAILED");
    }

    long size = sizefile(file);
    char* buf = malloc(size + 1);
    fread(buf, 1, size, file);
    buf[size] = '\0';

    return buf;
}

// WRITE FILE
void writefile(char* path, char* buf) {
    FILE* file = fopen(path, "w");
    if(!file) {
        free(file);
        perror("FOPEN FAILED");
    }

    fprintf(file, "%s", buf);

    fclose(file);
}

int check_tag(char* buf, int ind) {
    if(buf[ind] == '<' && (buf[ind+1] == 'p' || (buf[ind+1] == 'h' && (buf[ind+2] == '2' || buf[ind+2] == '3')))) 
        return 1;
    
    if(buf[ind] == '<' && (buf[ind+1] == 'm' && buf[ind+2] == 'e'))
        return 2;

    else
        return 0;
}

// PARSE HTML INTO TOKEN LIST
void parse(TOKEN_LIST* tokens, char* src) {
    size_t ind=0;
    size_t k=0;
    int tok_count = 0;

    char tostr[2];
    tostr[1] = '\0';

    char* ftyp = (char*)malloc(50 * sizeof(char));
    char* type = (char*)malloc(10 * sizeof(char));
    char* content = (char*)malloc(1000 * sizeof(char));

    while(src[ind]) {

        if(src[ind] == '\n')
            ind++;

        if(check_tag(src, ind) == 1) {
            ind++;
            while(src[ind] != '>') {
                tostr[0] = src[ind];
                strcat(ftyp, tostr);
                ind++;
            }
            if(ftyp[k] == 'p') 
                type = "p";
            if(ftyp[k] == 'h' && ftyp[k+1] == '2')
                type = "h2";
            if(ftyp[k] == 'h' && ftyp[k+1] == '3')
                type = "h3";

            ind++;
            while(src[ind] != '<') {
                tostr[0] = src[ind];
                if(src[ind] != '#' || src[ind] != '\\')
                	strcat(content, tostr);
                ind++;
            }
            if(strcmp(type, "") != 0 && strcmp(content, "") != 0) {
                tokens->toks[tok_count].type = type;
				tokens->toks[tok_count].content = content;
				tok_count++;
			}
            ftyp = (char*)malloc(50 * sizeof(char));
            type = (char*)malloc(10 * sizeof(char));
            content = (char*)malloc(1000 * sizeof(char));
        }

        if(check_tag(src, ind) == 2) {
            ind++;
            while(src[ind] != '>') {
                tostr[0] = src[ind];
                strcat(ftyp, tostr);
                ind++;
            }
            // author check
            if(ftyp[k] == 'm' && ftyp[k+1] == 'e') {

                size_t i=0;
                char* attr = (char*)malloc(10 * sizeof(char));
                char* auth = (char*)malloc(30 * sizeof(char));

                while(ftyp[i] != '"') {
                    i++;
                    ind++;
                }

                i++;
                ind++;
                while(ftyp[i] != '"') {
                    tostr[0] = ftyp[i];
                    strcat(attr, tostr);
                    i++;
                    ind++;
                }

                if(strcmp(attr, "author") == 0) {
                    i++;
                    ind++;

                    while(ftyp[i] != '"') {
                        i++;
                        ind++;
                    }
                    i++;
                    ind++;

                    while(ftyp[i] != '"') {
                        tostr[0] = ftyp[i];
                        strcat(auth, tostr);
                        i++;
                        ind++;
                    }
                    i++;
                    ind++;
                }

                if(strcmp(attr, "") != 0 &&
                    strcmp(auth, "") != 0) {
                    tokens->toks[tok_count].type = attr;
                    tokens->toks[tok_count].content = auth;
                    tok_count++;
                }

                attr = (char*)malloc(10 * sizeof(char));
                auth = (char*)malloc(30 * sizeof(char));
                ftyp = (char*)malloc(50 * sizeof(char));
                type = (char*)malloc(10 * sizeof(char));
                content = (char*)malloc(1000 * sizeof(char));
            }

        }

        else
            ind++;

    }
}

char* compile(TOKEN_LIST* tokens) {
    char* latex = (char*)malloc(SIZE * sizeof(char));

    char* header = (char*)malloc(200 * sizeof(char));
    strcat(latex, "\\documentclass{article}\n\n\\author{");

    int k=0;

    while(1) {
        char* type = (char*)malloc(10 * sizeof(char));
        char* content = (char*)malloc(1000 * sizeof(char));

        type    = tokens->toks[k].type;
        content = tokens->toks[k].content;

        if(strcmp(type, "author") == 0) {
            strcat(header, content);
            strcat(header, "}\n\\title{");
        }

        if(strcmp(type, "h2") == 0) {
            strcat(header, content);
            strcat(header, "}\n\\date{}\n\n\\begin{document}\n\\maketitle\n");
            strcat(latex, header);
        }

        if(strcmp(type, "h3") == 0) {
            strcat(latex, "\n\\section*{");
            strcat(latex, content);
            strcat(latex, "}\n");
        }


        if(strcmp(type, "p") == 0) {
            strcat(latex, content);
            strcat(latex, "\n");
        }

        k++;

        if(k == SIZE) {
            strcat(latex, "\n\\end{document}\n");
            break;
        }
    }
    return latex;
}

int main(int argc, char** argv) {
    if(argc != 3)
        perror("ERROR: usage: ./h-l file.html file.tex");
    
    char* path_to_html = argv[1];
    char* path_to_latex = argv[2];

    char* html_raw = readfile(path_to_html);

    TOKEN_LIST* tokens = (TOKEN_LIST*)malloc(sizeof(TOKEN_LIST));
    init_list(tokens);
    parse(tokens, html_raw);

    char* latex_code = compile(tokens);

    size_t i = 0;
    while(latex_code[i]) {
        if(latex_code[i] == '#' || latex_code[i] == '&')
            latex_code[i] = ' ';
        i++;
    }

    writefile(path_to_latex, latex_code);
    return 0;
}
