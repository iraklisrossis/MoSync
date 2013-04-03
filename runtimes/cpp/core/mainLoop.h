namespace Base {
	class Syscall;

	void reloadProgram() GCCATTRIB(noreturn);
}

void mainLoop(Base::Syscall* syscall,
	const char* programFile, const char* resourceFile) GCCATTRIB(noreturn);
