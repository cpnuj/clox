
int __step_(char* s, int len) {
    if (len == 0) return -1;
    switch (*s) {
    case 'h':
        return __step_h(++s, --len);
    }
    return -1;
}


int __step_h(char* s, int len) {
    if (len == 0) return -1;
    switch (*s) {
    case 'e':
        return __step_he(++s, --len);
    }
    return -1;
}


int __step_he(char* s, int len) {
    if (len == 0) return -1;
    switch (*s) {
    case 'l':
        return __step_hel(++s, --len);
    case 'h':
        return __step_heh(++s, --len);
    }
    return -1;
}


int __step_hel(char* s, int len) {
    if (len == 0) return -1;
    switch (*s) {
    case 'l':
        return __step_hell(++s, --len);
    }
    return -1;
}


int __step_hell(char* s, int len) {
    if (len == 0) return -1;
    switch (*s) {
    case 'o':
        return __step_hello(++s, --len);
    }
    return -1;
}


int __step_hello(char* s, int len) {
    // KEYWORD -- hello
    if (len == 0) return TK_HELLO;
    return -1;
}


int __step_heh(char* s, int len) {
    if (len == 0) return -1;
    switch (*s) {
    case 'e':
        return __step_hehe(++s, --len);
    }
    return -1;
}


int __step_hehe(char* s, int len) {
    // KEYWORD -- hehe
    if (len == 0) return TK_HEHE;
    return -1;
}

