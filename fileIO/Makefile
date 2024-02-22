all: main reset_inst

main:	all_upper.c
	gcc -o all_upper all_upper.c -Wall

reset_inst:	reset_inst.c
	gcc -o reset_inst reset_inst.c -Wall

.PHONY: clean
clean:
	rm -f all_upper reset_inst
