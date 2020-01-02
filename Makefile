CC	:= pio

.PHONY: all
all:
	$(CC) run -t upload

.PHONY: dry
dry:
	$(CC) run

.PHONY: clean
clean: clean
	$(CC) run -t clean
