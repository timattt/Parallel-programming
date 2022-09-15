EXEC_NAME_INT = run
TRASH = *.o *.gcda *.gcno *.info *.txt
REPORT_NAME = report
FLAGS = -D_GNU_SOURCE
LIBS = -lm -pthread

integrator: Starter.o Integrator.o
	gcc $(FLAGS) $^ -o $(EXEC_NAME_INT) $(LIBS)
	rm -f $(TRASH)
%.o: %.c
	gcc $(FLAGS) -c -o $@ $^ $(LIBS)

cleanup:
	rm -f $(TRASH)
