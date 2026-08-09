include $(shell git rev-parse --show-toplevel)/makefiles/bundle.mk


dbg d: $(DBG_FULL_FILE)



$(DBG_FULL_FILE): debug.o $(CLU_FILE)
	echo " linking $(PRJ_NAME) full debug $(DIR)"
	gcc -o $@ $^ $(FLAGS) $(FLAGS_DBG) $(FLAGS_LNK) $(FLAGS_EXTRA)



# FORCE comes from bundle.mk; see the note there on why not .PHONY.
$(CLU_FILE): FORCE
	$(MAKE) --directory=$(dir $@)



_clean::
	$(MAKE) clean --directory=$(dir $(CLU_FILE))
