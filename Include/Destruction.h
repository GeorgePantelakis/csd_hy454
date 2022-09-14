#pragma once

#include <list>
#include <stack>
#include "app.h"

class LatelyDestroyable;
class DestructionManager {
	std::list<LatelyDestroyable*> dead;
	static DestructionManager singleton;
public:
	void Register(LatelyDestroyable* d);
	void Commit(void);
	static auto Get(void)->DestructionManager&;
};

class LatelyDestroyable {
protected:
	friend class DestructionManager;
	bool alive = true;
	bool dying = false;
	virtual ~LatelyDestroyable();
	void Delete(void);
public:
	LatelyDestroyable(void) = default;

	bool IsAlive(void) const;
	void Destroy(void);
};

template<class T> class Recycled {// turn any class to green (recycle friendly)
protected:
	static std::stack<typename T*> recycler;
	static T* top_and_pop(void) {
		auto* x = recycler.top();
		recycler.pop();
		return x;
	}
public:
	template<class... Types> static T* New(Types... args) {
		if(recycler.empty())
			return new T(args...); // automatic propagation of any arguments!
		else
			return new (top_and_pop()) T(args...);// reusing ...
	}

	void Delete(void) {
		this->~T();
		recycler.push(this);
	}
};