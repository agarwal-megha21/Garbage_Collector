#include <stdio.h>
#include <stdlib.h>

#define STACK_MAX 256

/*
 * We need to get some "preliminaries" out of the way first. We need stuff to
 * actually garbage-collect! Normally, this would be done through super-fancy
 * macro voodoo magic, or by implementing an entire language, but for
 * simplicity's sake we'll just implement two different types of objects - an
 * int and a pair.
*/

// define objecttype either a integer value or a pair

typedef enum {
	OBJ_INT,
	OBJ_PAIR,
} ObjectType;

// define the object
// Here I have used union because we don't want an object to have both a int value and a pair value 

typedef struct sObject {
	// type int or pair
	ObjectType type;
	// mark the object
	unsigned char marked;
	// node references
	sObject* next;
	// union to hold the data for the int or pair
	union {
		int value;

		struct {
			sObject* head;
			sObject* tail;
		};
	};
} Object;

// Define a virtual machine
typedef struct {
	// stack
	Object* stack[STACK_MAX];
	// num of object
	int numObjects;
	// max object
	int maxObjects;
	// current size of stack
	int stackSize;
	// head
	Object* firstObject;
} VM;

//constructor
VM* newVM() {
	VM* vm = (VM*)malloc(sizeof(VM));
	vm->stackSize = 0;
	return vm;
}

//marking the object still in reference
void mark(Object* object) {
	if(object->marked)
		return ;
	object->marked = 1;
	if(object->type == OBJ_PAIR)
	{
		mark(object->head);
		mark(object->tail);
	}
}

// mark fucntion
void markAll(VM* vm) {
	printf("Marking objects");
	for(int i=0;i<vm->stackSize;i++)
		mark(vm->stack[i]);
}

// sweeping 
void sweep(VM* vm) {
	Object** object = &vm->firstObject;
	int swept = 0;
	int freed = 0;
	while(*object) {
		if(!(*object)->marked) {
			Object* unreached = *object;
			*object = unreached->next;
			free(unreached);
			freed++;
			vm->numObjects--;
		}
		else {
			(*object)->marked = 0;
			object = &(*object)->next;
		}
		swept++;
	}
}

// calling gc
void gc(VM* vm) {
	printf("Entering gc");
	int numObjects = vm->numObjects;
	markAll(vm);
	sweep(vm);
	vm->maxObjects = 2*vm->numObjects;
}

// push in vm
void push(VM* vm, Object* value) {
	if(vm->stackSize == STACK_MAX)
	{
		printf("STACK OVERFLOW");
		return ;
	}
	vm->stack[vm->stackSize++] = value;
}

// pop in vm
Object* pop(VM* vm) {
	if(vm->stackSize==0)
	{
		printf("STACK UNDERFLOW");
		return NULL;
	}
	return vm->stack[--vm->stackSize];
}

// new object
Object* newObject(VM* vm, ObjectType type) {
	if(vm->numObjects ==  vm->maxObjects) {
		printf("GC needed");
		gc(vm);
	}

	Object* object = (Object*)malloc(sizeof(Object));
	object->type = type;
	object->marked = 0;
	object->next = vm->firstObject;
	vm->firstObject = object;
	vm->numObjects++;
	return object;
}

// push pair
Object* pushPair(VM* vm) {
	Object* object = newObject(vm,OBJ_PAIR);
	object->tail = pop(vm);
	object->head = pop(vm);
	push(vm,object);
	return object;
}

void pushINT(VM* vm, int val) {
	Object* object = newObject(vm,OBJ_INT);
	object->value = val;
	push(vm,object);
}

void freeVM(VM* vm) {
	vm->stackSize = 0;
	gc(vm);
	free(vm);
}

// Print the contents of an Cbject to stdout.
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
	pushINT(vm, 1);
	pushINT(vm, 2);

	gc(vm);
	freeVM(vm);
}

void test2() {
	printf("Test 2: Unreached objects are collected.\n");
	VM* vm = newVM();
	pushINT(vm, 1);
	pushINT(vm, 2);
	pop(vm);
	pop(vm);

	gc(vm);
	freeVM(vm);
}

void test3() {
	printf("Test 3: Reach nested objects.\n");
	VM* vm = newVM();

	pushINT(vm, 1);
	pushINT(vm, 2);
	pushPair(vm);

	pushINT(vm, 3);
	pushINT(vm, 4);
	pushPair(vm);

	pushPair(vm);

	gc(vm);
	freeVM(vm);
}

void test4() {
	printf("Test 4: Handle cycles.\n");
	VM* vm = newVM();
	
	pushINT(vm, 1);
	pushINT(vm, 2);
	Object* a = pushPair(vm);
	printf("\tPushed a: "); objectPrint(a); printf("\n");

	pushINT(vm, 3);
	pushINT(vm, 4);
	Object* b = pushPair(vm);
	printf("\tPushed b: "); objectPrint(b); printf("\n");

	// Set up a cycle, also making OBJ_INT(2) and OBJ_INT(4) unreachable and
	// collectible.
	printf("\tSetting up cyclical references between a's and b's tails.\n");
	a->tail = b;
	b->tail = a;

	gc(vm);
	freeVM(vm);
}

void perfTest() {
	printf("Starting performance test.\n");
	VM* vm = newVM();

	for (int i = 0; i < 10000; i++) {
		for (int j = 0; j < 20; j++) {
			pushINT(vm, i);
		}

		for (int k = 0; k < 20; k++) {
			pop(vm);
		}
	}
	
	freeVM(vm);
}

int main(int argc, const char* argv[]) 
{
	test1();
	test2();
	test3();
	test4();
	perfTest();
}
