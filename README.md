# clox

CFG for program: 

```
program        → declaration* EOF ;

declaration → varDecl
            → funDecl
            | statement ;

varDecl     → VAR IDENTIFIER "=" expression ;

funDecl     → "fun" function ;
function    → IDENTIFIER "(" parameters? ")" blockStmt ;
parameters  → IDENTIFIER ("," IDENTIFIER)* ;

statement   → exprStmt
            | printStmt
            | blockStmt
            | ifStmt
            | whileStmt
            | forStmt
            | returnStmt
            | classStmt;

exprStmt    → expression ";" ;

printStmt   → "print" expression ";" ;

blockStmt   → "{" declaration* "}" ;

ifStmt      → "if" "(" expression ")" statement ("else" statement)? ;

whileStmt   → "while" "(" expression ")" statement;

forStmt     → "for" "(" (varDecl | exprStmt | ";") expression? ";" expression? ")" statement ;

returnStmt  → "return" expression? ";" ;

classStmt   → "class" IDENTIFIER "{" function* "}" ;
```

CFG for expression:

```
expression     → assignment ;

assignment     → ( call "." )? IDENTIFIER "=" assignment ;
               | logic_or ;

logic_or       → logic_and ("or" logic_and)* ;

logic_and      → equality ("and" equality)* ;

equality       → comparison ( ( "!=" | "==" ) comparison )* ;

comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;

term           → factor ( ( "-" | "+" ) factor )* ;

factor         → unary ( ( "/" | "*" ) unary )* ;

unary          → ( "!" | "-" ) unary
               | call ;

call           → primary ( "(" arguments? ")" | "." IDENTIFIER )* ;

primary        → NUMBER | STRING | "true" | "false" | "nil"
               | "(" expression ")"
               | IDENTIFIER ;

arguments      → expression ("," expression)*
```
