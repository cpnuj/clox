# clox

```
CFG for program: 

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