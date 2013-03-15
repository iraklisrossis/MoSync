// WARNING: NOT THREAD SAFE
void putEvent(const MAEventNative&);

struct Functor {
	void (*func)(const Functor&);
};

// This one is thread safe, but asynchronous.
// Functor must be allocated with malloc(), as it will be free()d after func returns.
void executeInCoreThread(Functor*);
