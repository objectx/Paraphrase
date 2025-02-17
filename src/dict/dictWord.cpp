#include "externals.h"
#include "typedValue.h"
#include "stack.h"
#include "word.h"
#include "context.h"

static bool colon(const char *inName,Context& inContext,bool inDefineShortend);
static bool compileValue(Context& inContext,TypedValue& inLambda);
static bool constant(const char *inName,Context& inContext,bool inOverwriteCheck);
static bool isInCStyleComment(Context& inContext);
static bool isInCppStyleComment(Context& inContext);

void InitDict_Word() {
	Install(new Word("_lit",WORD_FUNC {
		TypedValue *tv=(TypedValue *)(inContext.ip+1);
		inContext.DS.emplace_back(*tv);
		inContext.ip+=TvSize;
		NEXT;
	}));

	Install(new Word("_semis",WORD_FUNC {
		if(inContext.IS.size()<1) { return inContext.Error(E_IS_BROKEN); }
		inContext.ip=inContext.IS.back();
		inContext.IS.pop_back();
		NEXT;
	}));
	
	Install(new Word(":",WORD_FUNC {
		if(colon(":",inContext,true)==false) { return false; }
		NEXT;
	}));

	Install(new Word("::",WORD_FUNC {
		if(colon("::",inContext,false)==false) { return false; }
		NEXT;
	}));

	Install(new Word(";",WordLevel::Immediate,WORD_FUNC {
		inContext.Compile(std::string("_semis"));
		inContext.FinishNewWord();
		inContext.SetInterpretMode();
		NEXT;
	}));

	Install(new Word("immediate",WORD_FUNC {
		if(inContext.lastDefinedWord==NULL) {
			return inContext.Error(E_NO_LAST_DEFINED_WORD);
		}
		inContext.lastDefinedWord->level=WordLevel::Immediate;
		NEXT;
	}));

	Install(new Word("vocabulary",WORD_FUNC {
		inContext.DS.emplace_back(GetCurrentVocName());
		NEXT;
	}));

	Install(new Word("set-vocabulary",WORD_FUNC {
		if(inContext.DS.size()<1) { return inContext.Error(E_DS_IS_EMPTY); }
		TypedValue tos=Pop(inContext.DS);
		if(tos.dataType!=kTypeString) {
			return inContext.Error_InvalidType(E_TOS_STRING,tos);
		}
		SetCurrentVocName(*tos.stringPtr.get());
		NEXT;
	}));

	// usage: "constName" value const
	// string value --- 
	Install(new Word("const",WORD_FUNC {
		const char *name="const";
		if(constant(name,inContext,true)==false) {
			return false;
		}
		NEXT;	
	}));

	// usage: "constName" value const!
	// string value --- 
	Install(new Word("const!",WORD_FUNC {
		const char *name="const!";
		if(constant(name,inContext,false)==false) {
			return false;
		}
		NEXT;	
	}));

	// usage: "varName" var
	// string ---
	Install(new Word("var",WORD_FUNC {
		if(inContext.DS.size()<1) { return inContext.Error(E_DS_IS_EMPTY); }
		TypedValue tos=Pop(inContext.DS);
		if(tos.dataType!=kTypeString) {
			return inContext.Error_InvalidType(E_TOS_STRING,tos);
		}

		auto iter=Dict.find(*tos.stringPtr);
		if(iter!=Dict.end()) {
			return inContext.Error(E_ALREADY_DEFINED,*tos.stringPtr);
		}

		Word *newWord=new Word(tos.stringPtr.get());

		const int paramSize=2+TvSize*2+MutexSize;
		newWord->param=new const Word*[paramSize];
		
		const Word *lit=GetWordPtr(std::string("_lit"));
		if(lit==NULL) {
			return inContext.Error(E_CAN_NOT_FIND_THE_WORD,std::string("_lit"));
		}
		const Word *semis=GetWordPtr(std::string("_semis"));
		if(semis==NULL) {
			return inContext.Error(E_CAN_NOT_FIND_THE_WORD,std::string("_semis"));
		}

		const int storageIndex=1+TvSize+1;
		const int mutexIndex=1+TvSize+1+TvSize;
		newWord->param[0]=lit;
		new((TypedValue*)(newWord->param+1)) TypedValue(kTypeParamDest,
														newWord->param+storageIndex);
		newWord->param[1+TvSize]=semis;
		new((TypedValue*)(newWord->param+storageIndex)) TypedValue();	// <- storage
		new ((Mutex *)(newWord->param+mutexIndex)) Mutex();
		initMutex(*((Mutex *)(newWord->param+mutexIndex))); 

		Dict[newWord->shortName]=newWord;
		Dict[newWord->longName]=newWord;

		// define utility words.
		Word *newWordBackup=inContext.newWord;
			// define >$varName (ex: >$x)
			std::string toVarName((">$"+*tos.stringPtr.get()).c_str());
			Word *toVar=new Word(&toVarName);
			inContext.newWord=toVar;
			inContext.Compile(std::string(*tos.stringPtr.get()));
			inContext.Compile(std::string("store"));
			inContext.Compile(std::string("_semis"));
			inContext.FinishNewWord();
			Dict[toVar->shortName]=toVar;
			Dict[toVar->longName ]=toVar;

			// dfine $varName> (ex: $x>)
			std::string fromVarName((("$"+*tos.stringPtr.get())+">").c_str());
			Word *fromVar=new Word(&fromVarName);
			inContext.newWord=fromVar;
			inContext.Compile(std::string(*tos.stringPtr.get()));
			inContext.Compile(std::string("fetch"));
			inContext.Compile(std::string("_semis"));
			inContext.FinishNewWord();
			Dict[fromVar->shortName]=fromVar;
			Dict[fromVar->longName ]=fromVar;
		inContext.newWord=newWordBackup;
		NEXT;
	}));

	// usage: value var store
	// ex:
	//	"x" var
	//	100 x store
	//
	// value address ---
	// note: same as Forth's '!'
	Install(new Word("store",WORD_FUNC {
		if(inContext.DS.size()<2) {
			return inContext.Error(E_DS_AT_LEAST_2);
		}
		TypedValue tos=Pop(inContext.DS);
		if(tos.dataType!=kTypeParamDest) {
			return inContext.Error_InvalidType(E_TOS_PARAMDEST,tos);
		}
		
		Mutex *mutex=(Mutex *)(((Word**)tos.ipValue)+TvSize);
		Lock(*mutex);
			TypedValue second=Pop(inContext.DS);
			new((TypedValue*)tos.ipValue) TypedValue(second);
		Unlock(*mutex);	
		NEXT;
	}));

	// address --- value
	// same as Forth's '@'.
	Install(new Word("fetch",WORD_FUNC {
		if(inContext.DS.size()<1) { return inContext.Error(E_DS_IS_EMPTY); }
		TypedValue tos=Pop(inContext.DS);
		if(tos.dataType!=kTypeParamDest) {
			return inContext.Error_InvalidType(E_TOS_PARAMDEST,tos);
		}

		Mutex *mutex=(Mutex *)(((Word**)tos.ipValue)+TvSize);
		Lock(*mutex);
			inContext.DS.emplace_back(*(TypedValue*)tos.ipValue);
		Unlock(*mutex);
		NEXT;
	}));

	// left_curly
	Install(new Word("{",WordLevel::Level2,WORD_FUNC {
		if( inContext.IsInComment() ) { NEXT; }
		inContext.BeginNoNameWordBlock();
		NEXT;
	}));

	Install(new Word("}",WordLevel::Level2,WORD_FUNC {
		if( inContext.IsInComment() ) { NEXT; }
		inContext.Compile(std::string("_semis"));
		inContext.FinishNewWord();
		TypedValue tvWord(kTypeWord,inContext.newWord);
		if(inContext.EndNoNameWordBlock()==false) { return false; }
		switch(inContext.ExecutionThreshold) {
			case kInterpretLevel:
				inContext.DS.emplace_back(tvWord);
				break;
			case kCompileLevel:
				assert(inContext.newWord!=NULL);
				inContext.Compile(std::string("_lit"));
				if(compileValue(inContext,tvWord)==false) { return false; }
				break;
			case kSymbolLevel:
				assert(inContext.newWord!=NULL);
				if(compileValue(inContext,tvWord)==false) { return false; }
				break;
			default:
				inContext.Error(E_SYSTEM_ERROR);
				exit(-1);
		}
		NEXT;
	}));

	Install(new Word("${",WordLevel::Level2,WORD_FUNC {
		if( inContext.IsInComment() ) { NEXT; }
		inContext.BeginNoNameWordBlock();
		inContext.newWord->level=WordLevel::Immediate;
		NEXT;
	}));

	Install(new Word("{{",WordLevel::Level2,Dict["{"]->code));

	Install(new Word("}}",WordLevel::Level2,WORD_FUNC {
		if( inContext.IsInComment() ) { NEXT; }
		inContext.Compile(std::string("_semis"));
		inContext.FinishNewWord();
		TypedValue tvWord(kTypeWord,inContext.newWord);
		if(inContext.EndNoNameWordBlock()==false) { return false; }
		if(inContext.Exec(tvWord)==false) { return false; }
		NEXT;
	}));

	Install(new Word("/*",WordLevel::Level2,WORD_FUNC {
		if( isInCppStyleComment(inContext) ) { NEXT; }
		inContext.PushThreshold();
		inContext.PushNewWord();
		inContext.RS.emplace_back(kTypeMiscInt,kOPEN_C_STYLE_COMMENT);
		inContext.newWord=new Word(WordType::Normal);
		inContext.ExecutionThreshold=kSymbolLevel;
		NEXT;
	}));

	Install(new Word("*/",WordLevel::Level2,WORD_FUNC {
		if( isInCppStyleComment(inContext) ) { NEXT; }
		if(inContext.RS.size()<1) {
			return inContext.Error(E_C_STYLE_COMMENT_MISMATCH);
		}
		TypedValue& tvSyntax=ReadTOS(inContext.RS);
		if(tvSyntax.dataType==kTypeMiscInt
		  && tvSyntax.intValue==kOPEN_CPP_STYLE_ONE_LINE_COMMENT) {
			// do nothing in CPP style comment.
			NEXT;
		}
		if(tvSyntax.dataType!=kTypeMiscInt
		  || tvSyntax.intValue!=kOPEN_C_STYLE_COMMENT) {
			return inContext.Error(E_C_STYLE_COMMENT_MISMATCH);
		}
		Pop(inContext.RS);

		delete inContext.newWord->tmpParam;
		delete inContext.newWord;

		if(inContext.DS.size()<2) {
			return inContext.Error(E_DS_AT_LEAST_2);
		}
		TypedValue tvNW=Pop(inContext.DS);
		if(tvNW.dataType!=kTypeNewWord) {
			return inContext.Error_InvalidType(E_TOS_NEW_WORD,tvNW);
		}
 		inContext.newWord=(Word *)tvNW.wordPtr;

		TypedValue tvThreshold=Pop(inContext.DS);
		if(tvThreshold.dataType!=kTypeThreshold) {
			return inContext.Error_InvalidType(E_SECOND_THRESHOLD,tvThreshold);
		}
		inContext.ExecutionThreshold=tvThreshold.intValue;
		NEXT;
	}));

	Install(new Word("//",WordLevel::Level2,WORD_FUNC {
		if( isInCStyleComment(inContext) ) 	 { NEXT; }
		if( isInCppStyleComment(inContext) ) { NEXT; }
		inContext.PushThreshold();
		inContext.PushNewWord();
		inContext.RS.emplace_back(kTypeMiscInt,kOPEN_CPP_STYLE_ONE_LINE_COMMENT);
		inContext.newWord=new Word(WordType::Normal);
		inContext.ExecutionThreshold=kSymbolLevel;
		NEXT;
	}));

	Install(new Word(EOL_WORD,WordLevel::Level2,WORD_FUNC {
		if(inContext.RS.size()<1) { NEXT; }
		TypedValue& rsTos=ReadTOS(inContext.RS);
		if(rsTos.dataType==kTypeMiscInt
		  && rsTos.intValue==kOPEN_CPP_STYLE_ONE_LINE_COMMENT) {
			delete inContext.newWord->tmpParam;
			delete inContext.newWord;

			if(inContext.DS.size()<2) { return inContext.Error(E_DS_AT_LEAST_2); }
			TypedValue tvNW=Pop(inContext.DS);
			if(tvNW.dataType!=kTypeNewWord) {
				return inContext.Error_InvalidType(E_TOS_NEW_WORD,tvNW);
			}
	 		inContext.newWord=(Word *)tvNW.wordPtr;

			Pop(inContext.RS);	// already checked by ReadTOS(inContext.RS).

			TypedValue tvThreshold=Pop(inContext.DS);
			if(tvThreshold.dataType!=kTypeThreshold) {
				return inContext.Error_InvalidType(E_SECOND_THRESHOLD,tvThreshold);
			}
			inContext.ExecutionThreshold=tvThreshold.intValue;
		}
		NEXT;
	}));

	// (s -- wordPtr)
	Install(new Word(">word",WORD_FUNC {
		if(inContext.DS.size()<1) { return inContext.Error(E_DS_IS_EMPTY); }
		TypedValue tos=Pop(inContext.DS);
		if(tos.dataType!=kTypeString) {
			return inContext.Error_InvalidType(E_TOS_STRING,tos);
		}
		auto iter=Dict.find(*tos.stringPtr);
		if(iter==Dict.end()) {
			return inContext.Error(E_CAN_NOT_FIND_THE_WORD,*tos.stringPtr);
		}
		const Word *word=iter->second;
		inContext.DS.emplace_back(kTypeWord,word);
		NEXT;
	}));

	Install(new Word(">lit",WORD_FUNC {
		if(inContext.DS.size()<1) { return inContext.Error(E_DS_IS_EMPTY); }
		TypedValue tos=Pop(inContext.DS);
		inContext.Compile(std::string("_lit"));
		inContext.Compile(tos);
		NEXT;
	}));

	Install(new Word(">sym",WORD_FUNC {
		if(inContext.DS.size()<1) { return inContext.Error(E_DS_IS_EMPTY); }
		TypedValue tos=Pop(inContext.DS);
		inContext.Compile(tos);
		NEXT;
	}));

	// original-wordName aliaseName alias
	// ex: "dup" "case" alias
	Install(new Word("alias",WORD_FUNC {
		if(inContext.DS.size()<2) { return inContext.Error(E_DS_AT_LEAST_2); }
		TypedValue tvSrcWordName=Pop(inContext.DS);
		if(tvSrcWordName.dataType!=kTypeString) {
			return inContext.Error_InvalidType(E_SECOND_STRING,tvSrcWordName);
		}
		auto iter=Dict.find(*tvSrcWordName.stringPtr);
		if(iter==Dict.end()) {
			return inContext.Error(E_CAN_NOT_FIND_THE_WORD,*tvSrcWordName.stringPtr);
		}
		TypedValue tvNewWordName=Pop(inContext.DS);
		if(tvNewWordName.dataType!=kTypeString) {
			return inContext.Error_InvalidType(E_TOS_STRING,tvNewWordName);
		}
		const Word *srcWord=iter->second;
		const char *newWordName=strdup(tvNewWordName.stringPtr.get()->c_str());
		Word *newWord=new Word(newWordName,srcWord->level,srcWord->code);
		Install(newWord);
		newWord->param=srcWord->param;
		newWord->tmpParam=srcWord->tmpParam;
		NEXT;
	}));

	// wordPtr --- 
	Install(new Word("dump",WORD_FUNC {
		if(inContext.DS.size()<1) { return inContext.Error(E_DS_IS_EMPTY); }
		TypedValue tos=Pop(inContext.DS);
		if(tos.dataType!=kTypeWord && tos.dataType!=kTypeString) {
			return inContext.Error_InvalidType(E_TOS_WP,tos);
		}
		const Word *word=NULL;
		if(tos.dataType==kTypeWord) {
			word=tos.wordPtr;
		} else {
			auto iter=Dict.find(*tos.stringPtr);
			if(iter==Dict.end()) {
				return inContext.Error(E_CAN_NOT_FIND_THE_WORD,*tos.stringPtr);
			}
			word=iter->second;
		}
		if(word->isForgetable==false) {
			printf("the word '%s' is internal.\n",word->longName.c_str());
		} else {
			printf("the word '%s' is:\n",word->longName.c_str());
			const size_t n=word->tmpParam->size();
			for(size_t i=0; i<n; i++) {
				printf("[%zu] ",i);
				word->tmpParam->at(i).Dump();
			}
		}
		NEXT;
	}));
}

static bool colon(const char *inName,Context& inContext,bool inDefineShortend) {
	if(inContext.DS.size()<1) { return inContext.Error(E_DS_IS_EMPTY); }
	TypedValue tos=Pop(inContext.DS);
	if(tos.dataType!=kTypeString) { inContext.Error_InvalidType(E_TOS_STRING,tos); }
	Word *newWord=new Word(tos.stringPtr.get());
	if(Install(newWord,inDefineShortend)==false) {
		delete newWord;
		inContext.newWord=NULL;
		return false;
	}
	newWord->isForgetable=true;
	inContext.newWord=newWord;
	inContext.SetCompileMode();
	return true;
}

static bool compileValue(Context& inContext,TypedValue& inLambda) {
	if(inLambda.wordPtr->level==WordLevel::Immediate) {
		// the case of '${'
		if(inContext.Exec(inLambda)==false) { return false; }
		if(inContext.DS.size()<1) { return inContext.Error(E_DS_IS_EMPTY); }
		TypedValue tos=Pop(inContext.DS);
		inContext.Compile(tos);
	} else {
		inContext.Compile(inLambda);
	}
	return true;
}

static bool constant(const char *inName,Context& inContext,bool inOverwriteCheck) {
	if(inContext.DS.size()<2) { return inContext.Error(E_DS_AT_LEAST_2); }
	TypedValue tos=Pop(inContext.DS);
	TypedValue second=Pop(inContext.DS);
	if(second.dataType!=kTypeString) {
		return inContext.Error_InvalidType(E_SECOND_STRING,second);
	}

	if( inOverwriteCheck ) {
		auto iter=Dict.find(*second.stringPtr);
		if(iter!=Dict.end()) {
			return inContext.Error(E_ALREADY_DEFINED,*second.stringPtr);
		}
	}

	Word *newWord=new Word(second.stringPtr.get());
	Word *newWordBackup=inContext.newWord;
	inContext.newWord=newWord;
		inContext.Compile(std::string("_lit"));
		inContext.Compile(tos);
		inContext.Compile(std::string("_semis"));
		inContext.FinishNewWord();
	inContext.newWord=newWordBackup;
	Dict[newWord->shortName]=newWord;
	Dict[newWord->longName ]=newWord;
	return true;
}

static bool isInCStyleComment(Context& inContext) {
	if(inContext.RS.size()>0) {
		TypedValue& rsTos=ReadTOS(inContext.RS);
		if(rsTos.dataType==kTypeMiscInt && rsTos.intValue==kOPEN_C_STYLE_COMMENT) {
			return true;
		}
	}
	return false;
}

static bool isInCppStyleComment(Context& inContext) {
	if(inContext.RS.size()>0) {
		TypedValue& rsTos=ReadTOS(inContext.RS);
		if(rsTos.dataType==kTypeMiscInt
		  && rsTos.intValue==kOPEN_CPP_STYLE_ONE_LINE_COMMENT) {
			return true;
		}
	}
	return false;
}

