#include<ctype.h>
#include<stdarg.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

//抽象公分木ノードの種類
typedef enum{
    ND_ADD,//加算
    ND_SUB,//減算
    ND_MUL,//乗算
    ND_DIV,//除算
    ND_NUM,//整数
}NodeKind;

//抽象構文木ノード型
typedef struct Node Node;
struct Node{
    NodeKind kind;//ノード型の種類
    Node *lhs;//左辺
    Node *rhs;//右辺
    int val;//ND_NUMの時用の数値
};


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

char *user_input;

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
void error_at(char *loc, char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");//pos個の空白出力
    fprintf(stderr, "^ ");
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
        error_at(token->str, "[%c]ではありません", op);
    }
    token = token->next;
}


/*
    次のトークンが数値の時は1つ読み進めその数値を返す
    それ以外の場合、エラーを報告する
*/
int expect_number(){
    if(token->kind != TK_NUM){
        error_at(token->str, "数ではありません:%d", token->kind);
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

Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val){
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node *expr();
Node *primary();
Node *mul();
Node *unary();

//単項演算子(+, -)処理
Node *unary(){
    if(consume('+')){ return unary(); }//+はそのまま返す
    if (consume('-')){
        //exp:-1の場合内部的に0-1に置き換える
        return new_binary(ND_SUB, new_node_num(0), unary());
    }
    return primary();
}

//構文木解析の括弧処理
Node *primary(){
    if(consume('(')){//括弧"("なら"(" expr ")"のはず
        Node *node = expr();
        expect(')');
        return node;
    }

    //上記でないなら数値
    return new_node_num(expect_number());
}


//構文木解析の乗除算処理
// mul = unary ("*" unary | "/" unary)*
Node *mul(){
    Node *node = unary();

    for (;;){
        if(consume('*')){
            node = new_binary(ND_MUL, node, unary());
        }
        else if(consume('/')){
            node = new_binary(ND_DIV, node, unary());
        }
        else{
            return node;
        }
    }
}


//構文木解析の加減算処理
Node *expr(){
    Node *node = mul();

    for (;;){
        if(consume('+')){
            node = new_binary(ND_ADD, node, mul());
        }
        else if(consume('-')){
            node = new_binary(ND_SUB, node, mul());
        }
        else{
            return node;
        }
    }
}


void gen(Node *node){
    if(node->kind == ND_NUM){
        printf("    push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind){
    case ND_ADD:
        printf("    add rax, rdi\n");
        break;
    case ND_SUB:
        printf("    sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("    imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("    cqo\n");
        printf("    idiv rdi\n");
        break;
    }

    printf("    push rax\n");
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

        //文字が既知のトークンだったらそのトークンを追加
        if(strchr("+-*/()", *p)){
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if(isdigit(*p)){
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error_at(p, "既知でないトークン検出");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}


int main(int argc, char **argv){
    if(argc != 2){
        error("%s: 引数の数が変", argv[0]);
    }

    //トークナイズ+パース
    user_input = argv[1];
    token = tokenize(user_input);
    Node *node = expr();

    //天ぷら
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    //抽象構文木を下りながらコード生成
    gen(node);
    
    //スタックのトップに式全体の値があるはず
    //それをRAXにロードして返り値とする
    printf("    pop rax\n");
    printf("    ret\n");
    return 0;
}
