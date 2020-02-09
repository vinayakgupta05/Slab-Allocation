#define STRLEN   16

struct A {
	char string_a[STRLEN]; // Any string
	char string_b[STRLEN]; // Must have vowel or add to end
	struct B *ptr_c; // 
	struct C *ptr_d; // 
	int (*op0)(struct A *objA);
	unsigned char *(*op1)(struct A *objA);
};
struct B {
	int num_a; // Any integer
	int num_b; // >0 or set to 0
	int num_c; // Any integer
	int num_d; // Any integer
	char string_e[STRLEN]; // Capitalize Strings
};
struct C {
	int num_a; // Any integer
	int num_b; // Any integer
	int num_c; // <0 or set to 0
};
