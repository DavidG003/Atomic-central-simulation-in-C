
COMP_FLAGS = -Wvla -Wextra -Werror

all: master alimentazione attivatore atomo inibitore

clean:
	rm -f *.o master alimentazione attivatore atomo inibitore *~

master: master.c common_file.h Makefile
	gcc $(COMP_FLAGS) master.c -o master
	
alimentazione: alimentazione.c common_file.h Makefile
	gcc $(COMP_FLAGS) alimentazione.c -o alimentazione
	
attivatore: attivatore.c common_file.h Makefile
	gcc $(COMP_FLAGS) attivatore.c -o attivatore
	
atomo: atomo.c common_file.h Makefile
	gcc $(COMP_FLAGS) atomo.c -o atomo
	
inibitore: inibitore.c common_file.h Makefile
	gcc $(COMP_FLAGS) inibitore.c -o inibitore -lm

run:  all
	./master
