assembler: main.o pre_assembler.o first_pass.o second_pass.o utils.o
	gcc -g -Wall -ansi -pedantic main.o pre_assembler.o first_pass.o second_pass.o utils.o -o assembler

main.o: main.c assembler.h
	gcc -c -g -Wall -ansi -pedantic main.c -o main.o

pre_assembler.o: pre_assembler.c assembler.h
	gcc -c -g -Wall -ansi -pedantic pre_assembler.c -o pre_assembler.o

first_pass.o: first_pass.c assembler.h
	gcc -c -g -Wall -ansi -pedantic first_pass.c -o first_pass.o

second_pass.o: second_pass.c assembler.h
	gcc -c -g -Wall -ansi -pedantic second_pass.c -o second_pass.o

utils.o: utils.c assembler.h
	gcc -c -g -Wall -ansi -pedantic utils.c -o utils.o

clean:
	rm -f *.o assembler

