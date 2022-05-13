// kw.go
//
// A small program that converts keyword def file to corresponding finite state machine
// in C code for validating keyword. Only used in my clox implementation.
//
// Example file format:
// {
// token list
// }
//
// Keyword1 token1
// Keyword2 token2
// ...
//

package main

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"os"
	"strconv"
	"strings"
)

const cout = "keyword.c" // output c file name
const hout = "keyword.h" // output header file name

var file *os.File
var scanner *bufio.Scanner
var source io.Writer
var header io.Writer

var tokens map[string]struct{} = make(map[string]struct{})

type node struct {
	isKey bool
	next  map[rune]*node
	val   string
}

func newNode() *node {
	return &node{
		isKey: false,
		next:  make(map[rune]*node),
	}
}

func newValueNode(val string) *node {
	return &node{
		isKey: true,
		next:  make(map[rune]*node),
		val:   val,
	}
}

// addNext adds a new rune to the next map if not exists,
// return the next node.
func (n *node) addNode(r rune) *node {
	if nn, exist := n.next[r]; exist {
		return nn
	}
	nn := newNode()
	n.next[r] = nn
	return nn
}

func (n *node) addValueNode(r rune, val string) *node {
	if nn, exist := n.next[r]; exist {
		nn.isKey = true
		nn.val = val
		return nn
	}
	nn := newValueNode(val)
	n.next[r] = nn
	return nn
}

// Add a keyword recursively.
func (n *node) Add(kw []rune, tk string) {
	if len(kw) == 0 {
		panic("len(kw) == 0")
	}
	if len(kw) == 1 {
		n.addValueNode(kw[0], tk)
		return
	}
	nn := n.addNode(kw[0])
	nn.Add(kw[1:], tk)
}

func (n *node) Val() string {
	return n.val
}

func (n *node) Iter(prefix string, f func(*node, string, bool)) {
	f(n, prefix, n.isKey)
	for r, nn := range n.next {
		nn.Iter(prefix+string(r), f)
	}
}

func (n *node) ChildLen() int {
	return len(n.next)
}

// global head
var head *node = newNode()

func addToken(tk string) {
	tokens[tk] = struct{}{}
}

func addKeyword(s string, tk string) {
	head.Add([]rune(s), tk)
}

func emitPadding(size int) {
	for i := 0; i < size; i++ {
		fmt.Fprintf(source, " ")
	}
}

func emitLine(s string, a ...interface{}) {
	fmt.Fprintf(source, s+"\n", a...)
}

func emitHeaderLine(s string, a ...interface{}) {
	fmt.Fprintf(header, s+"\n", a...)
}

func emitEmptyLine() {
	emitLine("")
}

func emitPaddingLine(padding int, s string, a ...interface{}) {
	emitPadding(padding)
	emitLine(s, a...)
}

func emitCaseStmt(n *node, prefix string) {
	for r := range n.next {
		emitPaddingLine(4, "case %v:", strconv.QuoteRune(r))
		emitPaddingLine(8, "return __step_%s(++s, --len);", prefix+string(r))
	}
}

func emitStepFn(n *node, prefix string, isKey bool) {
	stepperName := fmt.Sprintf("__step_%s", prefix)
	recordHeader("int %s(char* s, int len);", stepperName)

	emitEmptyLine()
	emitLine("int %s(char* s, int len) {", stepperName)

	if isKey {
		emitPaddingLine(4, "// KEYWORD -- %s", prefix)
		emitPaddingLine(4, "if (len == 0) return %s;", n.Val())
	} else {
		emitPaddingLine(4, "if (len == 0) return -1;")
	}

	if n.ChildLen() > 0 {
		emitPaddingLine(4, "switch (*s) {")
		emitCaseStmt(n, prefix)
		emitPaddingLine(4, "}")
	}

	emitPaddingLine(4, "return -1;")

	emitLine("}")
}

func emitSteppers() {
	head.Iter("", emitStepFn)
}

func beginHeader() {
	emitHeaderLine("#ifndef clox_keyword_h")
	emitHeaderLine("#define clox_keyword_h")
	emitHeaderLine("")

	base := 1024
	for token := range tokens {
		emitHeaderLine("#define %s %d", token, base)
		base++
	}
	emitHeaderLine("")
	emitHeaderLine("int CloxKeyword(char* s, int len);")
}

func recordHeader(s string, a ...interface{}) {
	emitHeaderLine(s, a...)
}

func finishHeader() {
	emitHeaderLine("")
	emitHeaderLine("#endif")
}

func beginSource() {
	emitLine("#include \"keyword.h\"")
	emitEmptyLine()
	emitLine("int CloxKeyword(char* s, int len) {")
	emitPaddingLine(4, "return __step_(s, len);")
	emitLine("}")
	emitEmptyLine()
}

func genCode() {
	beginHeader()
	beginSource()
	emitSteppers()
	finishHeader()
}

func setup() {
	var err error

	if len(os.Args) != 2 {
		fmt.Println("usage: kw [file]")
		os.Exit(1)
	}

	file, err = os.Open(os.Args[1])
	if err != nil {
		log.Fatal(err)
	}

	scanner = bufio.NewScanner(file)

	source, err = os.OpenFile(cout, os.O_RDWR|os.O_CREATE, 0644)
	if err != nil {
		log.Fatal(err)
	}

	header, err = os.OpenFile(hout, os.O_RDWR|os.O_CREATE, 0644)
	if err != nil {
		log.Fatal(err)
	}
}

func readTokens() {
	if !scanner.Scan() || scanner.Text() != "{" {
		log.Fatal("expect {")
	}
	for scanner.Scan() {
		if scanner.Text() == "}" {
			return
		}
		addToken(scanner.Text())
	}
	if err := scanner.Err(); err != nil {
		log.Fatal(err)
	}
	log.Fatal("expect }")
}

func readKeywords() {
	for scanner.Scan() {
		words := strings.Fields(scanner.Text())
		if len(words) == 0 {
			continue
		}
		if len(words) != 2 {
			log.Fatalf("expect key - value pair for each line, but get: %s", scanner.Text())
		}
		addKeyword(words[0], words[1])
	}
	if err := scanner.Err(); err != nil {
		log.Fatal(err)
	}
}

func main() {
	setup()
	readTokens()
	readKeywords()
	genCode()
}
