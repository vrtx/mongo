/// TEST JUNKYARD
/////////////////
class OtherClass;
class string;
template <typename Lang>
class BindingMgr;

class SomeClass {
public:
	int SomeMember(int a, const string& b, OtherClass *c) { return a; }
	static int SomeStatic(int a, const string& b, OtherClass *c) { return a; }
	void t1() {}
	void t2(int a) {}
	int t3() { return 1; }
	int ClassVar;
	float MemberVar;
};
/////////////////


// Wrappers
class MongoClass;
class MongoObject;
class MongoFunction;
class MongoVariable;

template <typename _MongoType>
class Wrapper : public _MongoType {

};

Wrapper <MongoFunction> 


class JS_V8 {
public:
	template <typename Ret, typename Arg1, typename Arg2, typename Arg3>
	static void Register() {}
};

class JS_SM {
public:
	template <typename Ret, typename Arg1, typename Arg2, typename Arg3>
	static void Register() {}
};


class Python {
public:
	template <typename Ret, typename Arg1, typename Arg2, typename Arg3>
	static void Register() {}
};

class Lua {
public:
	template <typename Ret, typename Arg1, typename Arg2, typename Arg3>
	static void Register() {}
};


template <typename Lang, typename Class>
class Wrapper {
public:
	Wrapper(const char* name) : _name(name) { }

	static void push(Class *obj) {
		// get lang class/prototype
		// push instance of class

	}

	// Register a static function
	template <typename Ret, typename Arg1, typename Arg2, typename Arg3>
	void RegisterFunc(Ret(*fn)(Arg1, Arg2, Arg3), const char* name) {

	}

	// Register a member function
	template <typename Ret, typename Arg1, typename Arg2,  typename Arg3>
	void RegisterFunc(Ret(Class::*fn)(Arg1, Arg2, Arg3), const char* name) { }


	template <typename Arg1>
	void RegisterFunc(void (*fn)(Arg1), const char* name) { }

	template <typename Arg1>
	void RegisterFunc(void (Class::*fn)(Arg1), const char* name) { }

	template <typename Ret>
	void RegisterFunc(Ret (*fn)(), const char* name) { }

	template <typename Ret>
	void RegisterFunc(Ret (Class::*fn)(), const char* name) { }


	template <typename T>
	void RegisterVariable(T Class::* addr, const char* name) { }

	template <typename T>
	void RegisterConst(const T Class::* addr, const char* name) { }

	// Set default GC method (MOVE TO PUSH())
	void setGCPolicy(int policy);

private:
	const char* _name;
};

template <typename Lang>
class BindingMgr {
public:
	template <typename Class>
	static Wrapper <Lang, Class>* RegisterClass(const char *name) {
		return new Wrapper <Lang, Class>(name);
	}

	template <typename Ret, typename Arg1, typename Arg2,  typename Arg3>
	void RegisterFunc(Ret(*fn)(Arg1, Arg2, Arg3), const char* name) { }

};


int main(int argc, char** argv) {

	// register a class with functions
	Wrapper<JS, SomeClass>* someWrapper = BindingMgr<JS>::RegisterClass<SomeClass>("SomeClass");
	someWrapper->RegisterFunc(&SomeClass::SomeStatic, "SomeStatic");
	someWrapper->RegisterFunc(&SomeClass::SomeMember, "SomeMember");
	someWrapper->RegisterFunc(&SomeClass::t1, "t1");
	someWrapper->RegisterFunc(&SomeClass::t2, "t2");
	someWrapper->RegisterFunc(&SomeClass::t3, "t3");
	someWrapper->RegisterVariable(&SomeClass::MemberVar, "MemberVar");
	someWrapper->RegisterVariable(&SomeClass::ClassVar, "ClassVar");
	someWrapper->RegisterConst(&SomeClass::ClassVar, "ClassVar");

	// register class with lang
	// push an object/function/variable to lang
	// lang alloc object
	// lang del object
	// lang gc object
	// lang call member func
	// fetch an object/function/variable from lang env/retval

	return 0;
}




