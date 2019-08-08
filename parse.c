#include "ycc.h"

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
    LVar *next; // 次の変数かNULL
    char *name; // 変数の名前
    int len;    // 名前の長さ
    int offset; // RBPからのオフセット
};

// ローカル変数
static LVar *locals;

static LVar *find_lvar(Token *tok);

static bool isAlpha(char *p);

/**
 * local変数をlinked list形状で保存しているlocalsから
 * @param tok
 * @return
 */
LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next)
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
            return var;
    return NULL;
}


Node *code[100];

Node *assign() {
    Node *node = equality();
    if (consume("="))
        node = new_node(ND_ASSIGN, node, assign());
    return node;
}

Node *expr() {
    return assign();
}

Node *stmt() {
    Node *node = expr();
    expect(";");
    return node;

}

void program() {
    int i = 0;
    while (!at_eof())
        code[i++] = stmt();
    code[i] = NULL;
}

Node *unary() {
    if (consume("+"))
        return term();
    if (consume("-"))
        return new_node(ND_SUB, new_node_num(0), term());
    return term();
}

Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume("==")) {
            node = new_node(ND_EQU, node, relational());
        } else if (consume("!=")) {
            node = new_node(ND_NEQ, node, relational());
        } else
            return node;
    }
}

Node *relational() {
    Node *node = add();
    for (;;) {
        if (consume("<")) {
            node = new_node(ND_LSS, node, add());
        } else if (consume("<=")) {
            node = new_node(ND_LEQ, node, add());
        } else if (consume(">")) {
            node = new_node(ND_GTR, node, add());
        } else if (consume(">=")) {
            node = new_node(ND_GEQ, node, add());
        }
        return node;
    }
}

Node *add() {
    Node *node = mul();

    for (;;) {
        if (consume("+"))
            node = new_node(ND_ADD, node, mul());
        else if (consume("-"))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

//積商の項をつくる
Node *mul() {
    Node *node = unary();

    for (;;) {
        if (consume("*"))
            node = new_node(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

/**
 * アルファベットか判定する
 * @param p 判定したい文字を参照するポインタ
 * @return true;アルファベット(a-z)の場合 false:それ以外
 */
bool isAlpha(char *p) {
    if ('a' <= *p && *p <= 'z') {
        return true;
    }
    return false;
}

//終端記号(terminal symbol)をつくる
Node *term() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;

        LVar *lvar = find_lvar(tok);
        if (lvar) {
            node->offset = lvar->offset;
        } else {
            lvar = calloc(1, sizeof(LVar));
            lvar->next = locals;
            lvar->name = tok->str;
            lvar->len = tok->len;
            lvar->offset = locals->offset + 8;
            node->offset = lvar->offset;
            locals = lvar;
        }
        //node->offset = (tok->str[0] - 'a' + 1) * 8;
        return node;
    }


    // そうでなければ数値のはず
    return new_node_num(expect_number());
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

// トークンが変数として有効な記号のときには、トークンを一つ読み進めて
//　それ以外の場合にはNULLを返す
Token *consume_ident() {
    if (token->kind == TK_IDENT) {
        Token *rtn = token;
        token = token->next;
        return rtn;
    }
    return NULL;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        error("'%c'ではありません", op);
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "数ではありません");
    int val = token->val;
    token = token->next;
    return val;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = 1;
    cur->next = tok;
    return tok;
}

//長さ付きトークンの作成メソッド
//TODO:need to refactor
Token *new_token_with_len(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = new_token(kind, cur, str);
    tok->len = len;
    return tok;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    locals = calloc(1, sizeof(LVar));
    locals->offset = 0;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }


        if (isAlpha(p)) {
            char *tmp = p;
            p++;
            int len = 1;
            while (isAlpha(p)) {
                len++;
                p++;
            }
            cur = new_token(TK_IDENT, cur, tmp);
            cur->len = len;
            continue;
        }

        if (strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0 ||
            strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0
                ) {
            cur = new_token_with_len(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if (*p == '>' || *p == '<' || *p == '+' || *p == '-' || *p == '*' || *p == '/' ||
            *p == '(' || *p == ')' || *p == ';' || *p == '=') {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}
