i-banco: commandlinereader.o contas.o i-banco.o
	gcc -pthread -o i-banco commandlinereader.o contas.o i-banco.o

commandlinereader.o: commandlinereader.c commandlinereader.h
	gcc -g -Wall -pedantic -c commandlinereader.c

contas.o: contas.c contas.h
	gcc -g -Wall -pedantic -c contas.c

i-banco.o: i-banco.c commandlinereader.h contas.h
	gcc -g -Wall -pedantic -c i-banco.c

clean:
	rm -f *.o i-banco
