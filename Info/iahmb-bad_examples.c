#include <stdio.h>
#include <string.h>

// Very small to illustrate a point
#define MAX_NAME_LEN 10
#define MAX_BUF_SIZE 16

struct student {
	char name[MAX_NAME_LEN];
	int id;
	float gpa;
};

void print_student(const struct student *s);
void print_array(const char *array);

int main() {

	char *long_str = "A \"const\" string array that is really, really long and is going to cause problems with how long it is to prove a point";

	// Put some junk variables as padding around buf.
	char padding_prior[MAX_BUF_SIZE];

	// This is a general array that you can read and write.
	char buf[MAX_BUF_SIZE]; // A char array (NOT a string yet)

	// Put some junk variables as padding around buf.
	char padding_after[MAX_BUF_SIZE];

	// Let's start with some common pointer/array/string errors.
	char *c_ptr;
	printf("---------------------------------\n");
	printf("Some pointer/array examples\n");
	printf("---------------------------------\n");
	printf("Source:\n");
	printf("\tbuf[MAX_BUF_SIZE]; // MAX_BUF_SIZE = %d\n", MAX_BUF_SIZE);
	printf("\tchar *c_ptr;\n");
	printf("\t// Both uninitialized\n");
	printf("sizeof(c_ptr) = %d <- Size of pointer, not target\n", sizeof(c_ptr));
	printf("sizeof(buf) = %d\n", sizeof(buf));
	printf("After:\n\tc_ptr = buf;\n");
	printf("sizeof(c_ptr) = %d <-- Still size of pointer, not array\n", sizeof(c_ptr));
	printf("sizeof(buf) = %d\n", sizeof(buf));
	
	printf("\nstrlen(buf) = %d <-- This can vary across runs and OSes!\n", strlen(buf));

	// Initialize padding to known values
	memset(padding_prior, 0, MAX_BUF_SIZE);
	memset(padding_after, 0, MAX_BUF_SIZE);

	printf("---------------------------------\n");
	printf("Some buffer overflow examples\n");
	printf("---------------------------------\n");
	printf("Buffer output format:\n  HEX(%%02x) -- CHAR(%%c) -- STRING(%%s)\n");
	// Initial contents
	printf("\nUninitialized contents of buf\n");
	printf("Padding Prior:");
	print_array(padding_prior);
	printf("buf          :");
	print_array(buf);
	printf("Padding After:");
	print_array(padding_after);

	// Put a string in buf
	strcpy(buf,"Go Networks!"); // Now buf contains a string
	printf("\nAfter strcpy(buf,\"Go Networks!\")\n");
	printf("Padding Prior:");
	print_array(padding_prior);
	printf("buf          :");
	print_array(buf);
	printf("Padding After:");
	print_array(padding_after);

	// Let's break things
	strcpy(buf, long_str); // Buffer overflow here
	// This could equivalently occur in any function that stores data
	// into an array, such as read and getline, where you provide the
	// incorrect maximum size.

	printf("\nAfter strcpy(buf,long_str), which causes a buffer overflow\n");
	printf("Padding Prior:");
	print_array(padding_prior);
	printf("buf          :");
	print_array(buf);
	printf("Padding After:");
	print_array(padding_after);

	// Create some students
	struct student jim, pam;
	struct student *s_ptr;

	strcpy(jim.name, "Jim"); // Okay since a constant string contains a NULL.
	jim.id = 1234;
	jim.gpa = 1.3; // Jim, see me in office hours.

	s_ptr = &pam;
	strcpy(s_ptr->name, "Pam");
	s_ptr->id = 2345;
	s_ptr->gpa = 3.8; // Pam, can you tutor Jim?

	printf("\nJim:\n");
	print_student(&jim);
	printf("\nPam:\n");
	print_student(&pam);
	printf("\nPam via pointer:\n");
	print_student(s_ptr); // Print Pam again

	// Let's break things, redux
	strcpy(jim.name, buf); // String in buf larger than name array
	printf("\nJim after buffer overflow copy:\n");
	print_student(&jim);

	printf("\nPam after buffer overflow copy:\n");
	print_student(&pam);

	printf("\nbuf and padding after student buffer overflow copy:\n");
	printf("Padding Prior:");
	print_array(padding_prior);
	printf("buf          :");
	print_array(buf);
	printf("Padding After:");
	print_array(padding_after);

	// Some casting scenarios
	int num, *num_p;
	num = 5;
	num_p = &num;

	printf("\nInitial value of num via pointer: %d\n", *num_p);
	num_p = (int*)s_ptr; // Don't do this
	printf("\nInterpreting pam as an int: %d (0x%08x)", *num_p, *num_p);
	printf(" \"");
	unsigned char *num_c_p = (unsigned char*)num_p;
	for( int i = 0; i < sizeof(int); ++i, ++num_c_p ){
		printf("%c",*num_c_p);
	}
	printf("\"\n");

	s_ptr = (struct student*)&num; // Don't do this
	printf("\nInterpreting num as a student:\n");
	print_student(s_ptr);

	printf("\n\n\nHere comes the segfault.....\n\n\n");

	// These are not allowed and yield syntax errors
	// num = (int)jim;
	// jim = (struct student)num;

	return 0;
}

// Just print the student's info
void print_student(const struct student *s) {
	printf("Name: %s\n", s->name);
	printf("ID  : %d\n", s->id);
	printf("GPA : %g\n", s->gpa);
}

// Print the array up to MAX_BUF_SIZE bytes
void print_array(const char *array) {
	const char *p = array;
	// Print as HEX
	printf("%02x", (unsigned char)*p); // Make unsigned to print cleanly
	++p;
	for( int i = 1; i < MAX_BUF_SIZE; ++i, ++p ){
		printf(" %02x", (unsigned char)*p);
	}
	// Print as characters
	p = array;
	printf(" -- %c", *p);
	++p;
	for( int i = 1; i < MAX_BUF_SIZE; ++i, ++p ){
		printf(" %c", *p);
	}
	// Print as string
	printf(" -- %s\n", array);
}
