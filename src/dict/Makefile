INC=../inc
OBJ=../obj

INCLUDE_PATHS=-I $(INC)

objs=$(OBJ)/dictWord.o $(OBJ)/dictObject.o $(OBJ)/dictStack.o \
	 $(OBJ)/dictControl.o $(OBJ)/dictIO.o $(OBJ)/dictMath.o \
	 $(OBJ)/dictString.o $(OBJ)/dictArray.o $(OBJ)/dictList.o \
	 $(OBJ)/dictParallel.o  $(OBJ)/dictOptimize.o

.PHONY: all ext clean

all: $(objs) ext

DICT_DEPS=$(INC)/externals.h \
		  $(INC)/typedValue.h $(INC)/paraphrase.h \
		  $(INC)/stack.h \
		  $(INC)/word.h $(INC)/array.h \
		  $(INC)/context.h $(INC)/outer.h \
		  $(INC)/chan.h \
		  $(INC)/mathMacro.h

$(OBJ)/%.o: %.cpp $(DICT_DEPS)
	@if [ ! -d $(OBJ) ]; then \
		echo "mkdir -p $(OBJ)"; mkdir -p $(OBJ); \
	fi
	$(CC) $(CPPFLAGS) $(INCLUDE_PATHS) -o $@ -c $*.cpp

ext:
ifeq ($(LIB_NAME),libPP.so)
	make -C ext
endif

clean:
	rm -f $(objs)
	make -C ext clean

