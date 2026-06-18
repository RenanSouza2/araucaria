ROOT = $(shell git rev-parse --show-toplevel)
include $(ROOT)/makefiles/flags.mk
include $(ROOT)/makefiles/vars.mk



build b: main.out
dbg d: debug.out



main.out: main.o $(LIB_FILE)
	echo " linking $(PRJ_NAME) exec $(DIR) main"
	gcc -o $@ $^ $(FLAGS) $(FLAGS_PRD) $(FLAGS_EXE)
	echo "Finished $(PRJ_NAME) exec $(DIR) debug"

debug.out: debug.o $(DBG_FULL_FILE)
	echo " linking $(PRJ_NAME) debug $(DIR)"
	gcc -o $@ $^ $(FLAGS) $(FLAGS_DBG) $(FLAGS_EXE)



main.o: main.c
	echo "building $(PRJ_NAME) object src main"
	gcc -c -o $@ $< $(FLAGS) $(FLAGS_PRD) $(FLAGS_CMP) $(FLAGS_EXTRA)

debug.o: main.c
	echo "building $(PRJ_NAME) object src debug"
	gcc -c -o $@ $< $(FLAGS) $(FLAGS_DBG) $(FLAGS_CMP) $(FLAGS_EXTRA)



$(LIB_FILE): FORCE
	$(MAKE) --directory=$(dir $@)

$(DBG_FULL_FILE): FORCE
	$(MAKE) dbg --directory=$(dir $@)



FORCE:



clean:
	echo "cleaning $(PRJ_NAME) $(DIR)"
	rm -rf *.out *.o
	$(MAKE) clean --directory=$(LIB_DIR)
