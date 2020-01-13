#ifndef SYNCHRONIZED_HPP
#define SYNCHRONIZED_HPP

#include <mutex>

namespace eptg {

template<typename T, class Mutex>
class synchronized_proxy
{
	template<typename X, class Y>
	friend class synchronized;

private:
	synchronized_proxy();
	synchronized_proxy(const synchronized_proxy &);
	synchronized_proxy & operator=(const synchronized_proxy &);
	synchronized_proxy & operator=(synchronized_proxy &&);

	synchronized_proxy(Mutex & m, T & obj)
		:lock(m)
		,t(&obj)
	{}
	synchronized_proxy(Mutex & m, T & obj, int)
		:lock(m, std::try_to_lock)
		,t((lock)?&obj:nullptr)
	{}

public:
	synchronized_proxy(synchronized_proxy && other)
		:lock(*other.lock.mutex(), std::adopt_lock)
		,t(std::move(other.t))
	{
		other.t = nullptr;
	}

	explicit operator bool() const { return (bool)lock; }

	const T * operator->() const { return t; }
		  T * operator->()       { return t; }

	const T & operator*() const { return *t; }
		  T & operator*()       { return *t; }

private:
	std::unique_lock<Mutex> lock;
	T * t;
};


template<typename T, class Mutex=std::recursive_mutex>
class synchronized
{
public:
	// Convenience typefed for subclasses to use
	typedef T SynchronizedObject;

	synchronized_proxy<T,Mutex> GetSynchronizedProxy() {
		return synchronized_proxy<T,Mutex>(mutex, t);
	}
	synchronized_proxy<T,Mutex> TryGetSynchronizedProxy() {
		return synchronized_proxy<T,Mutex>(mutex, t, 0);
	}

protected:
	T t;
	Mutex mutex;
};

} // namespace

#endif // include guard
