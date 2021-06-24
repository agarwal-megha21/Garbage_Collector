#include "./main.h"
#include <stdlib.h>
#include <stdio.h>

void assert(int condition, const char* message) 
{
	if (!condition) {
		printf("%s\n", message);
		exit(1);
	}
}

/**
 * We need to get some "preliminaries" out of the way first. We need stuff to
 * actually garbage-collect! Normally, this would be done through super-fancy
 * macro voodoo magic, or by implementing an entire language, but for
 * simplicity's sake we'll just implement two different types of objects - an
 * int and a pair.
 */

/**
 * Define what an object is; in this case as a tagged union.
 * (either an OBJ_INT or an OBJ_PAIR)
 */

/**
 * Now, we implement a "virtual machine" which has a stack to store all the variables.
 */

/** A simple virtual machine for managing out-of-scope variables. */

/**
 * Creates and initializes a new VM.
 * @return A pointer to the new VM
 */
 
VM* newVM() {
	VM* vm = malloc(sizeof(VM));
	vm->stackSize = 0;
	vm->firstObject = NULL;
	vm->numObjects = 0;
	vm->maxObjects = INITIAL_GC_THRESHOLD;
	return vm;
}

/**
 * Push an Object to a VM's stack.
 */

void push(VM* vm, Object* obj) {
	assert(vm->stackSize < STACK_MAX, "Stack overflow!");
	vm->stack[vm->stackSize++] = obj;
}

/**
 * Pop an Object from the VM's stack.
 */

Object* pop(VM* vm) {
	assert(vm->stackSize > 0, "Stack underflow!");
	return vm->stack[--vm->stackSize];
}

/**
 * Instantiate a new Object.
 */

Object* newObject(VM* vm, ObjectType type) {
	// Run GC if we're reaching max number of Objects.
	if (vm->numObjects == vm->maxObjects) gc(vm);

	Object* object = malloc(sizeof(Object));
	object->type = type;
	object->marked = 0;

	// Insert into the list of allocated Objects.
	object->next = vm->firstObject;
	vm->firstObject = object;
	
	vm->numObjects++;
	return object;
}

/**
 * Push a new int Object to the VM's stack.
 */

void pushInt(VM* vm, int val) {
	Object* obj = newObject(vm, OBJ_INT);
	obj->value = val;
	push(vm, obj);
}

/**
 * Instantiate and push a new pair Object to the VM's stack.
 * The pair's tail will be the top object on the stack, and it's head will be
 * the object on the stack just underneath that.
 */

Object* pushPair(VM* vm) {
	Object* obj = newObject(vm, OBJ_PAIR);
	obj->tail = pop(vm);
	obj->head = pop(vm);

	push(vm, obj);
	return obj;
}

/**
 * Mark an Object as reachable.
 */

void mark(Object* object) {
	// Check to see if the object is already marked. This allows us to avoid
	// infinite recursion loops.
	if (object->marked) return;

	object->marked = 1;

	if (object->type == OBJ_PAIR) {
		mark(object->head);
		mark(object->tail);
	}
}

/**
 * Mark all reachable Object's.
 */

void markAll(VM* vm) {
	for (int i = 0; i < vm->stackSize; i++) {
		mark(vm->stack[i]);
	}
}

/**
 * Sweep through the VM's stack and delete unmarked Objects.
 */

void sweep(VM* vm) {
	Object** object = &vm->firstObject;

	while (*object) {
		if (!(*object)->marked) {
			// This object wasn't reached, so remove it from the
			// list and free it.
			Object* unreached = *object;

			*object = unreached->next;
			free(unreached);

			vm->numObjects--;
		}
		else {
			// This Object was reached, so unmark it (for the next GC) and move
			// on to the next.
			(*object)->marked = 0;
			object = &(*object)->next;
		}
	}
}

/**
 * Run the garbage collector.
 */

void gc(VM* vm) {
	int numObjects = vm->numObjects;

	markAll(vm);
	sweep(vm);

	vm->maxObjects = vm->numObjects * 2;
}

/**
 * Free the VM from memory.
 */

void freeVM(VM* vm) {
	vm->stackSize = 0;
	gc(vm);
	free(vm);
}

/**
 * Print the contents of an Cbject to stdout.
 */

void objectPrint(Object* obj) {
	switch (obj->type) {
		case OBJ_INT:
			printf("%d", obj->value);
			break;

		case OBJ_PAIR:
			printf("(");
			objectPrint(obj->head);
			printf(", ");
			objectPrint(obj->tail);
			printf(")");
			break;
	}
}

/*******|
| TESTS |
|*******/

void test1() {
	printf("Test 1: Objects on stack are preserved.\n");
	VM* vm = newVM();
	pushInt(vm, 1);
	pushInt(vm, 2);

	gc(vm);
	assert(vm->numObjects == 2, "Should have preserved objects.");
	freeVM(vm);
}

void test2() {
	printf("Test 2: Unreached objects are collected.\n");
	VM* vm = newVM();
	pushInt(vm, 1);
	pushInt(vm, 2);
	pop(vm);
	pop(vm);

	gc(vm);
	assert(vm->numObjects == 0, "Should have collected objects.");
	freeVM(vm);
}

void test3() {
	printf("Test 3: Reach nested objects.\n");
	VM* vm = newVM();

	pushInt(vm, 1);
	pushInt(vm, 2);
	pushPair(vm);

	pushInt(vm, 3);
	pushInt(vm, 4);
	pushPair(vm);

	pushPair(vm);

	gc(vm);
	assert(vm->numObjects == 7, "Should have reached objects.");
	freeVM(vm);
}

void test4() {
	printf("Test 4: Handle cycles.\n");
	VM* vm = newVM();
	
	pushInt(vm, 1);
	pushInt(vm, 2);
	Object* a = pushPair(vm);
	printf("\tPushed a: "); objectPrint(a); printf("\n");

	pushInt(vm, 3);
	pushInt(vm, 4);
	Object* b = pushPair(vm);
	printf("\tPushed b: "); objectPrint(b); printf("\n");

	// Set up a cycle, also making OBJ_INT(2) and OBJ_INT(4) unreachable and
	// collectible.
	printf("\tSetting up cyclical references between a's and b's tails.\n");
	a->tail = b;
	b->tail = a;

	gc(vm);
	assert(vm->numObjects == 4, "Should have collected objects.");
	freeVM(vm);
}

void perfTest() {
	printf("Starting performance test.\n");
	VM* vm = newVM();

	for (int i = 0; i < 10000; i++) {
		for (int j = 0; j < 20; j++) {
			pushInt(vm, i);
		}

		for (int k = 0; k < 20; k++) {
			pop(vm);
		}
	}
	
	freeVM(vm);
}

int main(int argc, const char* argv[]) {
	test1();
	test2();
	test3();
	test4();
	perfTest();

