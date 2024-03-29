#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "map.h"
#include "memory.h"
#include "vm.h"

void vm_error(VM *vm, char *errmsg);
void vm_errorf(VM *vm, char *format, ...);

uint8_t fetch_code(VM *vm);
Value fetch_constant(VM *vm);
int fetch_int16(VM *vm);
void run_instruction(VM *vm, uint8_t i);

void op_constant(VM *vm);
void op_negative(VM *vm);
void op_not(VM *vm);
void op_binary(VM *vm, uint8_t op);
void op_global(VM *vm);
void op_set_global(VM *vm);
void op_get_global(VM *vm);
void op_set_local(VM *vm);
void op_get_local(VM *vm);
void op_set_upvalue(VM *vm);
void op_get_upvalue(VM *vm);
void op_jmp(VM *vm);
void op_jmp_back(VM *vm);
void op_jmp_on_false(VM *vm);
void op_call(VM *vm);
void op_closure(VM *vm);
void op_class(VM *vm);
void op_get_filed(VM *vm);
void op_set_filed(VM *vm);
void op_method(VM *vm);
void op_invoke(VM *vm);
void op_derive(VM *vm);
void op_get_super(VM *vm);
void op_return(VM *vm);
void op_print(VM *vm);

static Map *globals(VM *vm);

static void vm_gc(VM *vm);
static void vm_debug(VM *vm);

static void define_native(VM *vm, char *name, int arity, native_fn method)
{
  Value native = value_make_native(arity, method);
  map_put(&vm->globals, value_make_string(name, strlen(name)), native);
}

void vm_init(VM *vm)
{
  vm->sp = vm->stack - 1;
  vm->error = 0;
  vm->vmain = value_make_fun(0, as_string(value_make_string("script", 6)));

  value_array_init(&vm->constants);
  value_array_write(&vm->constants, value_make_object(string_copy("init", 4)));

  map_init(&vm->globals);

  vm->gc_threshold = 1024 * 1024;

  define_native(vm, "clock", 0, native_clock);
}

void vm_push(VM *vm, Value v)
{
  if (vm->sp == vm->stack + STACK_MAX - 1) {
    // TODO: Add error handling
    return;
  }
  vm->sp++;
  *vm->sp = v;
}

Value vm_pop(VM *vm)
{
  if (vm->sp < vm->stack) {
    // TODO: Add error handling
    assert(0);
  }
  Value v = *vm->sp;
  vm->sp--;
  return v;
}

Value vm_top(VM *vm)
{
  if (vm->sp < vm->stack) {
    // TODO: Add error handling
    assert(0);
  }
  return *vm->sp;
}

Value vm_topn(VM *vm, int n)
{
  Value *p = vm->sp - n;
  assert(p >= vm->stack);
  return *p;
}

static ObjectUpValue *open_upvalue(VM *vm, Value *location)
{
  ObjectUpValue **nextp = &vm->open_upvalues;
  while (*nextp && (*nextp)->location > location) {
    nextp = &((*nextp)->next);
  }
  if (*nextp && (*nextp)->location == location) {
    return *nextp;
  }
  ObjectUpValue *new = upvalue_new(location);
  new->next = *nextp;
  *nextp = new;
  return new;
}

static void close_upvalue(VM *vm, Value *location)
{
  ObjectUpValue **head = &vm->open_upvalues;
  while (*head && (*head)->location >= location) {
    upvalue_close(*head);
    *head = (*head)->next;
  }
}

static inline CallFrame *cur_frame(VM *vm)
{
  return &vm->frames[vm->cur_frame];
}

static inline Chunk *cur_chunk(VM *vm)
{
  return &cur_frame(vm)->closure->proto->chunk;
}

static inline void frame_push(VM *vm, ObjectClosure *closure)
{
  vm->cur_frame++;
  vm->frames[vm->cur_frame].pc = 0;
  vm->frames[vm->cur_frame].closure = closure;
  // the first slot of this frame is the fun itself
  vm->frames[vm->cur_frame].bp = vm->sp - closure->proto->arity;
}

static inline void frame_pop(VM *vm)
{
  vm->sp = cur_frame(vm)->bp;
  close_upvalue(vm, vm->sp);
  vm_pop(vm);
  vm->cur_frame--;
}

void vm_run(VM *vm)
{
  vm->done = 0;
  vm->cur_frame = -1;
  vm->main_closure = as_closure(value_make_closure(as_function(vm->vmain)));
  vm_push(vm, vm->vmain);
  frame_push(vm, vm->main_closure);
  while (1) {

#define GC_HEAP_GROW_FACTOR 2

    if (mem_alloc() >= vm->gc_threshold) {
      vm_gc(vm);
      vm->gc_threshold = mem_alloc() * GC_HEAP_GROW_FACTOR;
    }

#ifdef DEBUG_RUNTIME
    vm_debug(vm);
#endif

#ifdef DEBUG_GC
    vm_gc(vm);
#endif

    if (cur_frame(vm)->pc >= cur_chunk(vm)->len) {
      vm_error(vm, "VM error: pc out of bound");
    }
    if (vm->error) {
      while (fetch_code(vm) != OP_RETURN)
        ;
      return;
    }
    uint8_t instruction = fetch_code(vm);
    run_instruction(vm, instruction);
    if (vm->done) {
      return;
    }
  }
}

void vm_error(VM *vm, char *errmsg)
{
  vm->error = 1;
  sprintf(vm->errmsg, "%s", errmsg);
}

static void trace_stack(VM *vm)
{
  int i;
  for (i = vm->cur_frame; i >= 0; i--) {
    CallFrame *frame = &vm->frames[i];
    fprintf(stderr, "[line %d] in %s",
            frame->closure->proto->chunk.lines[frame->pc - 1],
            frame->closure->proto->name->str);
    if (i != 0) {
      fprintf(stderr, "()");
    }
    fprintf(stderr, "\n");
  }
}

void vm_errorf(VM *vm, char *format, ...)
{
  vm->error = 1;
  va_list ap;
  va_start(ap, format);
  int printed = vsprintf(vm->errmsg, format, ap);
  fprintf(stderr, "%s\n", vm->errmsg);
  trace_stack(vm);
}

// fetch_code fetch and return the next code from vm chunk.
uint8_t fetch_code(VM *vm)
{
  if (cur_frame(vm)->pc >= cur_chunk(vm)->len) {
    panic("VM error: fetch_code overflow");
  }
  uint8_t code = cur_chunk(vm)->code[cur_frame(vm)->pc];
  cur_frame(vm)->pc++;
  return code;
}

Value fetch_constant(VM *vm)
{
  uint8_t off = fetch_code(vm);
  return vm->constants.value[off];
}

int fetch_int16(VM *vm)
{
  int h8 = fetch_code(vm); // high 8 bit
  int l8 = fetch_code(vm); // low  8 bit
  return (h8 << 8) | l8;
}

void run_instruction(VM *vm, uint8_t i)
{
  switch (i) {
  case OP_CONSTANT:
    return op_constant(vm);
  case OP_NEGATIVE:
    return op_negative(vm);
  case OP_NOT:
    return op_not(vm);

  case OP_ADD:
  case OP_MINUS:
  case OP_MUL:
  case OP_DIV:
  case OP_BANG:
  case OP_BANG_EQUAL:
  case OP_EQUAL:
  case OP_EQUAL_EQUAL:
  case OP_GREATER:
  case OP_GREATER_EQUAL:
  case OP_LESS:
  case OP_LESS_EQUAL:
    return op_binary(vm, i);

  case OP_POP:
    vm_pop(vm);
    return;
  case OP_CLOSE:
    close_upvalue(vm, vm->sp);
    vm_pop(vm);
    return;

  case OP_GLOBAL:
    return op_global(vm);

  case OP_SET_GLOBAL:
    return op_set_global(vm);
  case OP_GET_GLOBAL:
    return op_get_global(vm);

  case OP_SET_LOCAL:
    return op_set_local(vm);
  case OP_GET_LOCAL:
    return op_get_local(vm);

  case OP_SET_UPVALUE:
    return op_set_upvalue(vm);
  case OP_GET_UPVALUE:
    return op_get_upvalue(vm);

  case OP_JMP:
    return op_jmp(vm);
  case OP_JMP_BACK:
    return op_jmp_back(vm);
  case OP_JMP_ON_FALSE:
    return op_jmp_on_false(vm);

  case OP_CALL:
    return op_call(vm);
  case OP_CLOSURE:
    return op_closure(vm);
  case OP_CLASS:
    return op_class(vm);
  case OP_GET_FIELD:
    return op_get_filed(vm);
  case OP_SET_FIELD:
    return op_set_filed(vm);
  case OP_METHOD:
    return op_method(vm);
  case OP_INVOKE:
    return op_invoke(vm);
  case OP_DERIVE:
    return op_derive(vm);
  case OP_GET_SUPER:
    return op_get_super(vm);

  case OP_RETURN:
    return op_return(vm);

  case OP_PRINT:
    return op_print(vm);
  }
  // TODO: panic on unknown code
  vm_error(vm, "unknown code");
}

void op_constant(VM *vm) { vm_push(vm, fetch_constant(vm)); }

void op_negative(VM *vm)
{
  Value value = vm_pop(vm);
  if (value.type != VT_NUM) {
    vm_errorf(vm, "Operand must be a number.");
    return;
  }
  double number = as_number(value);
  vm_push(vm, value_make_number(-number));
}

void op_not(VM *vm)
{
  Value value = vm_pop(vm);
  bool res = !value_truable(value);
  vm_push(vm, value_make_bool(res));
}

Value concatenate(Value v1, Value v2)
{
  ObjectString *s1, *s2;
  s1 = as_string(v1);
  s2 = as_string(v2);
  return value_make_object(string_concat(s1, s2));
}

void op_binary(VM *vm, uint8_t op)
{
  Value v2 = vm_pop(vm);
  Value v1 = vm_pop(vm);

  Value v;
  int error = 0;

#define op_calculation(op)                                                     \
  if (is_number(v1) && is_number(v2))                                          \
    v = value_make_number(as_number(v1) op as_number(v2));                     \
  else                                                                         \
    vm_errorf(vm, "Operands must be numbers.")

#define op_comparison(op)                                                      \
  if (is_number(v1) && is_number(v2))                                          \
    v = value_make_bool(as_number(v1) op as_number(v2));                       \
  else                                                                         \
    vm_errorf(vm, "Operands must be numbers.")

  switch (op) {
  // calculation
  case OP_ADD:
    if (is_number(v1) && is_number(v2))
      v = value_make_number(as_number(v1) + as_number(v2));
    else if (is_string(v1) && is_string(v2))
      v = concatenate(v1, v2);
    else
      vm_errorf(vm, "Operands must be two numbers or two strings.");
    break;

  case OP_MINUS:
    op_calculation(-);
    break;

  case OP_MUL:
    op_calculation(*);
    break;

  case OP_DIV:
    op_calculation(/);
    break;

  // comparision '!=' '==' work on all value
  case OP_BANG_EQUAL:
    v = value_make_bool(!value_equal(v1, v2));
    break;

  case OP_EQUAL_EQUAL:
    v = value_make_bool(value_equal(v1, v2));
    break;

  // comparision on numbers
  case OP_GREATER:
    op_comparison(>);
    break;

  case OP_GREATER_EQUAL:
    op_comparison(>=);
    break;

  case OP_LESS:
    op_comparison(<);
    break;

  case OP_LESS_EQUAL:
    op_comparison(<=);
    break;
  }

  if (error)
    // TODO: make error msg clear
    vm_error(vm, "type error");
  else
    vm_push(vm, v);

#undef BINARY_OP_CAL
#undef BINARY_OP_COMP
}

static Map *globals(VM *vm) { return &vm->globals; }

void op_global(VM *vm)
{
  Value name = fetch_constant(vm);
  Value value = vm_pop(vm);
  if (!is_string(name)) {
    panic("programming error: OP_GLOBAL operates on a non-ident name");
  }
  map_put(globals(vm), name, value);
}

void op_set_global(VM *vm)
{
  Value name = fetch_constant(vm);
  Value value = vm_pop(vm);

  if (!map_get(globals(vm), name, NULL)) {
    vm_errorf(vm, "Undefined variable '%s'.", as_string(name)->str);
    return;
  }

  map_put(globals(vm), name, value);
  vm_push(vm, value);
}

void op_get_global(VM *vm)
{
  Value name = fetch_constant(vm);
  Value value;

  if (!map_get(globals(vm), name, &value)) {
    vm_errorf(vm, "Undefined variable '%s'.", as_string(name)->str);
    return;
  }
  vm_push(vm, value);
}

void op_set_local(VM *vm)
{
  uint8_t off = fetch_code(vm);
  Value *plocal = cur_frame(vm)->bp + off;
  *plocal = vm_top(vm);
}

void op_get_local(VM *vm)
{
  uint8_t off = fetch_code(vm);
  Value *plocal = &cur_frame(vm)->bp[off];
  vm_push(vm, *plocal);
}

void op_set_upvalue(VM *vm)
{
  uint8_t idx = fetch_code(vm);
  Value *location = cur_frame(vm)->closure->upvalues[idx]->location;
  *location = vm_top(vm);
}

void op_get_upvalue(VM *vm)
{
  uint8_t idx = fetch_code(vm);
  Value *location = cur_frame(vm)->closure->upvalues[idx]->location;
  vm_push(vm, *location);
}

void op_jmp(VM *vm) { cur_frame(vm)->pc += fetch_int16(vm); }

void op_jmp_back(VM *vm) { cur_frame(vm)->pc -= fetch_int16(vm); }

// op_jmp_on_false does not pop the value
void op_jmp_on_false(VM *vm)
{
  int offset = fetch_int16(vm);
  if (!value_truable(vm_top(vm))) {
    cur_frame(vm)->pc += offset;
  }
}

static void call_fun(VM *vm, int arity, ObjectClosure *callee)
{
  if (arity != callee->proto->arity) {
    vm_errorf(vm, "Expected %d arguments but got %d.", callee->proto->arity,
              arity);
    return;
  }
  frame_push(vm, callee);
}

static void call_native(VM *vm, int arity, ObjectNative *native)
{
  if (arity != native->arity) {
    vm_errorf(vm, "Expected %d arguments but got %d.", native->arity, arity);
    return;
  }
  Value value = native->method(arity, vm->sp - arity + 1);
  vm->sp -= arity + 1;
  vm_push(vm, value);
}

static inline Value get_init_const(VM *vm)
{
  return value_array_get(&vm->constants, 0);
}

static void call_bound_method(VM *vm, int arity, ObjectBoundMethod *bm)
{
  call_fun(vm, arity, bm->method);
  *cur_frame(vm)->bp = value_make_object(bm->receiver);
}

static void call_initializer(VM *vm, int arity, ObjectClass *klass)
{
  ObjectInstance *ins = instance_new(klass);

  Value initializer_value;
  if (map_get(&klass->methods, get_init_const(vm), &initializer_value)) {
    ObjectClosure *initializer = (ObjectClosure *)as_object(initializer_value);
    ObjectBoundMethod *bm = bound_method_new(initializer, ins);
    call_bound_method(vm, arity, bm);
  } else {
    if (arity > 0) {
      vm_errorf(vm, "Expected 0 arguments but got %d.", arity);
      return;
    }
    vm_push(vm, value_make_object(ins));
  }
}

static void call_value(VM *vm, int arity, Value value)
{
  if (is_closure(value)) {
    call_fun(vm, arity, as_closure(value));
  } else if (is_native(value)) {
    call_native(vm, arity, as_native(value));
  } else if (is_class(value)) {
    call_initializer(vm, arity, as_class(value));
  } else if (is_bound_method(value)) {
    call_bound_method(vm, arity, as_bound_method(value));
  } else {
    vm_errorf(vm, "Can only call functions and classes.");
  }
}

void op_call(VM *vm)
{
  uint8_t arity = fetch_code(vm);
  Value value = *(vm->sp - arity);
  call_value(vm, arity, value);
}

void op_closure(VM *vm)
{
  Value proto = fetch_constant(vm);
  ObjectClosure *closure = closure_new(as_function(proto));
  for (int i = 0; i < closure->upvalue_size; i++) {
    uint8_t idx = fetch_code(vm);
    uint8_t from_local = fetch_code(vm);
    Value *location = from_local
                          ? cur_frame(vm)->bp + idx
                          : cur_frame(vm)->closure->upvalues[idx]->location;
    closure->upvalues[i] = open_upvalue(vm, location);
  }
  vm_push(vm, value_make_object((Object *)closure));
}

void op_class(VM *vm)
{
  Value cname = fetch_constant(vm);
  ObjectClass *klass = class_new(as_string(cname));
  vm_push(vm, value_make_object(klass));
}

void op_get_filed(VM *vm)
{
  Value field = fetch_constant(vm);
  Value vins = vm_pop(vm);
  if (!is_instance(vins)) {
    vm_errorf(vm, "Only instances have properties.");
    return;
  }

  ObjectInstance *ins = as_instance(vins);
  Value value;
  if (map_get(&ins->fields, field, &value)) {
    vm_push(vm, value);
  } else if (map_get(&ins->klass->methods, field, &value)) {
    ObjectClosure *method = (ObjectClosure *)as_object(value);
    vm_push(vm, value_make_object(bound_method_new(method, ins)));
  } else {
    ObjectString *field_name = as_string(field);
    vm_errorf(vm, "Undefined property '%s'.", field_name->str);
  }
}

void op_set_filed(VM *vm)
{
  Value field = fetch_constant(vm);
  Value value = vm_pop(vm);
  Value vins = vm_pop(vm);
  if (!is_instance(vins)) {
    vm_errorf(vm, "Only instances have fields.");
    return;
  }
  ObjectInstance *ins = as_instance(vins);
  map_put(&ins->fields, field, value);
  vm_push(vm, value);
}

void op_method(VM *vm)
{
  Value name = fetch_constant(vm);
  Value method = vm_pop(vm);
  ObjectClass *klass = (ObjectClass *)as_object(vm_top(vm));
  map_put(&klass->methods, name, method);
}

void op_invoke(VM *vm)
{
  uint8_t arity = fetch_code(vm);
  Value field = fetch_constant(vm);

  ObjectInstance *ins = (ObjectInstance *)as_object(vm_topn(vm, arity));
  ObjectClass *klass = ins->klass;

  Value method;

  if (map_get(&ins->fields, field, &method)) {
    *(vm->sp - arity) = method;
    call_value(vm, arity, method);
    return;
  }

  if (!map_get(&klass->methods, field, &method)) {
    vm_errorf(vm, "Undefined property '%s'.", as_string(field)->str);
    return;
  }
  call_fun(vm, arity, as_closure(method));
}

void op_derive(VM *vm)
{
  if (!is_class(vm_topn(vm, 1))) {
    vm_errorf(vm, "Superclass must be a class.");
    return;
  }

  ObjectClass *klass = as_class(vm_top(vm));
  ObjectClass *super = as_class(vm_topn(vm, 1));

  MapIter *iter = map_iter_new(&super->methods);
  while (map_iter_next(iter)) {
    map_put(&klass->methods, iter->key, iter->val);
  }
  map_iter_close(iter);
}

void op_get_super(VM *vm)
{
  Value name = fetch_constant(vm);
  ObjectClass *_super = as_class(vm_pop(vm));
  ObjectInstance *ins = as_instance(vm_pop(vm));

  Value value;
  if (!map_get(&_super->methods, name, &value)) {
    vm_errorf(vm, "Undefined property '%s'.", as_string(name)->str);
    return;
  }

  ObjectBoundMethod *bm = bound_method_new(as_closure(value), ins);
  vm_push(vm, value_make_object(bm));
}

void op_return(VM *vm)
{
  Value retval = vm_pop(vm);
  frame_pop(vm);
  if (vm->cur_frame < 0) {
    vm->done = 1;
    return;
  }
  vm_push(vm, retval);
}

void op_print(VM *vm)
{
  Value value = vm_pop(vm);
  if (vm->error) {
    return;
  }
  value_print(value);
  printf("\n");
}

static void mark_map(Map *map, ValueArray *wset)
{
  MapIter *iter = map_iter_new(map);
  while (map_iter_next(iter)) {
    value_array_write(wset, iter->key);
    value_array_write(wset, iter->val);
  }
  map_iter_close(iter);
}

static void mark_object(Object *obj, ValueArray *wset)
{
  if (obj->marked) {
    return;
  }
  obj->marked = true;

  switch (obj->type) {

  case OBJ_STRING:
    break;

  case OBJ_FUNCTION: {
    ObjectFunction *function = (ObjectFunction *)obj;
    value_array_write(wset, value_make_object(function->name));
  } break;

  case OBJ_UPVALUE: {
    ObjectUpValue *upvalue = (ObjectUpValue *)obj;
    value_array_write(wset, *upvalue->location);
  } break;

  case OBJ_CLOSURE: {
    ObjectClosure *closure = (ObjectClosure *)obj;
    value_array_write(wset, value_make_object(closure->proto));
    for (int i = 0; i < closure->upvalue_size; i++) {
      value_array_write(wset, value_make_object(closure->upvalues[i]));
    }
  } break;

  case OBJ_NATIVE:
    break;

  case OBJ_CLASS: {
    ObjectClass *klass = (ObjectClass *)obj;
    value_array_write(wset, value_make_object(klass->name));
    mark_map(&klass->methods, wset);
  } break;

  case OBJ_INSTANCE: {
    ObjectInstance *ins = (ObjectInstance *)obj;
    value_array_write(wset, value_make_object(ins->klass));
    mark_map(&ins->fields, wset);
  } break;

  case OBJ_BOUND_METHOD: {
    ObjectBoundMethod *bm = (ObjectBoundMethod *)obj;
    value_array_write(wset, value_make_object(bm->method));
    value_array_write(wset, value_make_object(bm->receiver));
  } break;

  default:
    panic("mark_value: unknown object type.");
  }
}

static void mark_value(Value value, ValueArray *wset)
{
  if (!is_object(value)) {
    return;
  }
  mark_object(as_object(value), wset);
}

static void mark_root(VM *vm, ValueArray *wset)
{
  for (int i = 0; i < vm->constants.len; i++) {
    value_array_write(wset, value_array_get(&vm->constants, i));
  }

  mark_map(&vm->globals, wset);

  for (Value *ss = vm->stack; ss <= vm->sp; ss++) {
    value_array_write(wset, *ss);
  }

  for (int i = vm->cur_frame; i >= 0; i--) {
    value_array_write(wset, value_make_object(vm->frames[i].closure));
  }

  ObjectUpValue *upvalue = vm->open_upvalues;
  while (upvalue) {
    value_array_write(wset, value_make_object(upvalue));
    upvalue = upvalue->next;
  }
}

static void vm_gc(VM *vm)
{
  ValueArray wset;
  value_array_init(&wset);

  mark_root(vm, &wset);

  for (int i = 0; i < wset.len; i++) {
    mark_value(value_array_get(&wset, i), &wset);
  }

#ifdef DEBUG_GC
  trace_heap();
#endif

  sweep_heap();

#ifdef DEBUG_GC
  trace_heap();
#endif

  value_array_free(&wset);
}

static void vm_debug(VM *vm)
{
  printf("======= DEBUG VM ======\n");
  printf("PC: %4d BP: %4ld NEXT OP: ", cur_frame(vm)->pc,
         (uint64_t)(cur_frame(vm)->bp - vm->stack));
  debug_instruction(cur_chunk(vm), &vm->constants, cur_frame(vm)->pc);

  printf("Call Frame\n");
  for (int i = vm->cur_frame; i >= 0; i--) {
    value_print(value_make_object(vm->frames[i].closure));
    printf("\n");
  }
  printf("\n");

  printf("STACK\n");
  for (Value *i = vm->stack; i <= vm->sp; i++) {
    printf("%03ld  ", (uint64_t)(i - vm->stack));
    value_print(*i);
    printf("\n");
  }
  printf("\n");

  printf("OPEN UPVALUES\n");
  ObjectUpValue *opened = vm->open_upvalues;
  while (opened) {
    printf("%03ld -> ", (uint64_t)(opened->location - vm->stack));
    opened = opened->next;
  }
  printf("NULL\n\n");

  printf("UpValue\n");
  for (int i = 0; i < cur_frame(vm)->closure->upvalue_size; i++) {
    ObjectUpValue *up = cur_frame(vm)->closure->upvalues[i];
    if (up->location == &up->closed) {
      printf("%08ld  ", (uint64_t)(up->location));
    } else {
      printf("%08ld  ", (uint64_t)(up->location - vm->stack));
    }
    value_print(*up->location);
    printf("\n");
  }

  printf("\n");
}
