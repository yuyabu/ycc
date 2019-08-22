/**
 * @file ycc.h
 * @brief Cコンパイラ
 *
 * 以下の文法に対応
 *
 * program    = stmt*
 * stmt       = expr ";" | "return" expr ";"
 * expr       = assign
 * assign     = equality ("=" assign)?
 * equality   = relational ("==" relational | "!=" relational)*
 * relational = add ("<" add | "<=" add | ">" add | ">=" add)*
 * add        = mul ("+" mul | "-" mul)*
 * mul        = unary ("*" unary | "/" unary)*
 * unary      = ("+" | "-")? primary
 * primary    = num | ident | "(" expr ")"
 *
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// 抽象構文木のノードの種類
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // 整数
    ND_EQU, // ==
    ND_NEQ, // !=  (node not equal)
    ND_LSS, // <   (less than)
    ND_LEQ, // <=  (less equal)
    ND_GTR, // >   (greater than)
    ND_GEQ,  // >=  (greater equal)
    ND_ASSIGN, // =
    ND_LVAR,    // ローカル変数
    ND_RETURN
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの型
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kindがND_NUMの場合のみ使う
    int offset;    // kindがND_LVARの場合のみ使う
};

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数トークン
    TK_EOF      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    int len;
};

// 現在着目しているトークン
extern Token *token;

// 入力プログラム
extern char *user_input;

extern Node *code[100];

extern Node *assign();

extern Node *stmt();

void program();

extern Node *expr();

extern Node *equality();

extern Node *relational();

extern Node *add();

extern Node *mul();

extern Node *unary();

extern Node *primary();

extern Node *new_node(NodeKind kind, Node *lhs, Node *rhs);

extern Node *new_node_num(int val);

extern bool consume(char *op);

extern Token *consume_ident();

extern void expect(char *op);

extern int expect_number();

extern void gen(Node *node);

extern Token *new_token(TokenKind kind, Token *cur, char *str);

extern Token *new_token_with_len(TokenKind kind, Token *cur, char *str, int len);

extern Token *tokenize(char *p);

extern bool at_eof();

extern void error_at(char *loc, char *fmt, ...);

extern void error(char *fmt, ...);