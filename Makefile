# NAME: Christopher Ngai
# EMAIL: cngai1223@gmail.com
# ID: 404795904

CC = gcc
CFLAGS = -Wall -Wextra

default:
	$(CC) $(CFLAGS) -lmraa -lm -o lab4b lab4b.c

check: default
	@echo "Starting Smoke Test\n"
	@printf "SCALE=F\nOFF\n" | ./lab4b --period=1 --scale=C --log="log1.txt"; \
	if [ $$? -eq 0 ]; then \
		echo "SCALE=F Test: Passed"; \
	else \
		echo "SCALE=F Test: Failed"; \
	fi
	@printf "SCALE=C\nOFF\n" | ./lab4b --period=1 --scale=F --log="log2.txt"; \
	if [ $$? -eq 0 ]; then \
		echo "SCALE=C Test: Passed"; \
	else \
		echo "SCALE=C Test: Failed"; \
	fi
	@printf "PERIOD=3\nOFF\n" | ./lab4b --period=1 --scale=C --log="log3.txt"; \
	if [ $$? -eq 0 ]; then \
		echo "PERIOD Test: Passed"; \
	else \
		echo "PERIOD Test: Failed"; \
	fi
	@printf "STOP\nOFF\n" | ./lab4b --period=1 --scale=C --log="log4.txt"; \
	if [ $$? -eq 0 ]; then \
		echo "STOP Test: Passed"; \
	else \
		echo "STOP Test: Failed"; \
	fi
	@printf "START\nOFF\n" | ./lab4b --period=1 --scale=C --log="log5.txt"; \
	if [ $$? -eq 0 ]; then \
		echo "START Test: Passed"; \
	else \
		echo "START Test: Failed"; \
	fi
	@printf "LOG test\nOFF\n" | ./lab4b --period=1 --scale=C --log="log6.txt"; \
	if [ $$? -eq 0 ]; then \
		echo "LOG Test: Passed"; \
	else \
		echo "LOG Test: Failed"; \
	fi
	@printf "OFF\n" | ./lab4b --period=1 --scale=C --log="log7.txt"; \
	if [ $$? -eq 0 ]; then \
		echo "OFF Test: Passed"; \
	else \
		echo "OFF Test: Failed"; \
	fi
	@printf "INVALID COMMAND\nOFF\n" | ./lab4b --period=1 --scale=C --log="log8.txt"; \
	if [ $$? -eq 0 ]; then \
		echo "Invalid Command Test: Passed"; \
	else \
		echo "Invalid Command Test: Failed"; \
	fi
	@rm -f log1.txt log2.txt log3.txt log4.txt log5.txt log6.txt log7.txt log8.txt
	@echo "\nSmoke Test Complete"

clean:
	rm -rf lab4b lab4b-404795904.tar.gz

dist:
	tar -czf lab4b-404795904.tar.gz lab4b.c README Makefile
