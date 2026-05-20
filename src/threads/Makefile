include ../Makefile.kernel

TEST ?= alarm-multiple
# flags de comandos para ele funcionar com o mlfqs
FLAGS = -q 
TIMEOUT = 60
PINTOS_RELATIVE_PATH= ../utils/pintos
BUILD_PATH = build/tests/threads
ifeq ($(findstring mlfqs, $(TEST)), mlfqs)
# se for passado um teste de mlfqs ele deve adicionar uma flag e mudar o tempo de timeout
    FLAGS += -mlfqs
	TIMEOUT = 480
endif

test:
# primeiro da clean e build e depois ele roda o teste em si
	$(MAKE) clean; 
	$(MAKE) all; 

# ../utils/pintos -v -k -T 60 $(SIMULATOR) -- run (TEST)
	$(PINTOS_RELATIVE_PATH) -v -k -T $(TIMEOUT) $(SIMULATOR) -- $(FLAGS) run $(TEST) < /dev/null 2> $(BUILD_PATH)/$(TEST).errors > $(BUILD_PATH)/$(TEST).output
	perl -I../ ../tests/threads/$(TEST).ck $(BUILD_PATH)/$(TEST) $(BUILD_PATH)/
