/**
 * @file parse.c
 * @brief トークナイズと再帰下降構文解析を行うプログラム
 * @author
 *
 */

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

static int is_alnum(char c);

int is_alnum(char c) {
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}

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

/**
 * 代入ノードを作成する
 *
 * @return
 */
Node *assign() {
    Node *node = equality();
    if (consume("="))
        node = new_node(ND_ASSIGN, node, assign());
    return node;
}


/**
 * tokenからprogramを作る。
 *
 * program = stmt*
 *
 */
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

/**
 * 文のノードを作成する
 *
 * stmt    = expr ";"
 *      | "return" expr ";"
 * @return
 */
Node *stmt() {
    Node *node;

    //  if (consume(TK_RETURN)) { //2019/08/12時点本のまちがってるところ
    if (consume("return")) { //これが正解
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
    } else {
        node = expr();
    }
    expect(";");
    return node;

}

/**
 * 式をつくる
 *
 * EBNF
 * expr       = assign
 *
 * @return
 */
Node *expr() {
    return assign();
}


/**
 * 2項の等価比較のnodeをつくる
 * @return
 */
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

/**
 * 2項の比較述語のnodeをつくる
 * @return
 */
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

/**
 * 加減算の項をつくる
 * @return tokenから作成したnode
 */
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

/**
 * 積商の項をつくる
 * @return　tokenから作成したnode
 */
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

/**
 * 終端記号(terminal symbol)をつくる
 * @return
 */
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
        return node;
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}

/**
 * 次のトークンが期待している記号のときには、トークンを1つ読み進めて\
 * 真を返す。それ以外の場合には偽を返す。
 * @param op
 * @return
 */
bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

/**
 * トークンが変数として有効な記号のときには、トークンを一つ読み進める
 * それ以外の場合にはNULLを返す
 * @return
 */
Token *consume_ident() {
    if (token->kind == TK_IDENT) {
        Token *rtn = token;
        token = token->next;
        return rtn;
    }
    return NULL;
}

/**
 * 次のトークンが期待している記号のときには、トークンを1つ読み進める。
 * それ以外の場合にはエラーを報告する。
 * @param op 期待する記号
 */
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        error("'%c'ではありません", op);
    token = token->next;
}


/**
 * 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
 * それ以外の場合にはエラーを報告する。
 * @return tokenの数値
 */
int expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "数ではありません");
    int val = token->val;
    token = token->next;
    return val;
}

/**
 * 新しいトークンを作成してcurに繋げる
 * @param kind
 * @param cur
 * @param str
 * @return
 */
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

        //6文字読み進めてreturn + (空白)だった場合、return文としてtokenizeする
        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token_with_len(TK_RESERVED, cur, p, 6);
            p += 6;
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
            cur = new_token_with_len(TK_IDENT, cur, tmp, len);
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
            cur = new_token_with_len(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token_with_len(TK_NUM, cur, p, 1);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}
