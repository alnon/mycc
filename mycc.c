#include<ctype.h>
#include<stdarg.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef enum{
    TK_RESERVED,    //記号トークン
    TK_NUM,         //整数トークン
    TK_EOF,         //入力終了判定
} TokenKind;

typedef struct Token Token;
struct Token{
    TokenKind kind; //トークンの型
    Token *next;    //次の入力トークン
    int val;        //kindがnumの時の数値
    char *str;      //トークン文字列
};

Token *token;//現在のトークン

/*
    エラー報告関数
    printfと同じ引数をとる
*/
void error(char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}


/*
    次のトークンが期待している記号の時は1つ読み進めTureを返す
    それ以外の場合、Falseを返す
*/
bool consume(char op){
    if(token->kind != TK_RESERVED || token->str[0] != op){
        return false;
    }
    token = token->next;
    return true;
}

/*
    次のトークンが期待している記号の時は1つ読み進めTureを返す
    それ以外の場合、エラーを報告する
*/
void expect(char op){
    if(token->kind != TK_RESERVED || token->str[0] != op){
        error("[%c]ではありません", op);
    }
    token = token->next;
}


/*
    次のトークンが数値の時は1つ読み進めその数値を返す
    それ以外の場合、エラーを報告する
*/
int expect_number(){
    if(token->kind != TK_NUM){
        error("数ではありません:%d", token->kind);
    }
    int val = token->val;
    token = token->next;
    return val;
}

/*
    EOF判定
*/
bool at_eof(){
    return token->kind == TK_EOF;
}

//新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str){
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

//入力文字列pをトークナイズして返す
Token *tokenize(char *p){
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p)
    {
        //空白文字をスキップ
        if(isspace(*p)){ p++; continue; }

        //文字が[+]か[-]だったらそのトークンを追加
        if(*p == '+' || *p == '-'){
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if(isdigit(*p)){
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error("トークナイズ失敗");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}


int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "引数の個数が変\n");
        return 1;
    }

    //トークナイズする
    token = tokenize(argv[1]);

    //printf("%d, %x, %s\n", token->kind, token->next, token->str);

    //天ぷら
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    //最初は数であるハズなので確認してmov命令出力
    printf("    mov rax, %d\n", expect_number());

    while (!at_eof()){
        if(consume('+')){
            printf("    add rax, %d\n", expect_number());
            continue;
        }
        expect('-');
        printf("    sub rax, %d\n", expect_number());
    }
    
    printf("    ret\n");
    return 0;
}
