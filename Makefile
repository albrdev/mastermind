CC			:= pio

DIR_TEST	:= test

.PHONY: all
all:
	$(CC) run -t upload

.PHONY: dry
dry:
	$(CC) run

.PHONY: test
test:
	$(MAKE) -C $(DIR_TEST)

.PHONY: clean
clean: clean
	$(CC) run -t clean
