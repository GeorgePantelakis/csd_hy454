#include "../Include/Destruction.h"

// DestructionManager
DestructionManager DestructionManager::singleton;

auto DestructionManager::Get(void) -> DestructionManager& {
	return singleton;
}

void DestructionManager::Register(LatelyDestroyable* d) {
	assert(!d->IsAlive());
	dead.push_back(d);
}

void DestructionManager::Commit(void) {
	for (auto* d : dead)d->Delete();
	dead.clear();
}

// LatelyDestroyable
LatelyDestroyable::~LatelyDestroyable() {
	assert(dying);
}

bool LatelyDestroyable::IsAlive(void) const {
	return alive;
}

void LatelyDestroyable::Destroy(void) {
	if (alive) {
		alive = false;
		DestructionManager::Get().Register(this);
	}
}

void LatelyDestroyable::Delete(void) {
	assert(!dying);
	dying = true;
	delete this;
}

// Recycler
/*static T* Recycled::top_and_pop(void) {
	auto* x = recycler.top();
	recycler.pop();
	return x;
}

template<class... Types>
static T* Recycled::New(Types... args) {
	if (recycler.empty())
		return new T(args...); // automatic propagation of any arguments!
	else
		return new (top_and_pop()) T(args...);// reusing ...
}

void Recycled::Delete(void) {
	this->~T();
	recycler.push(this);
}*/