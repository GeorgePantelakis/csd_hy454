#include "../Include/Animator.h"

// Animator
Animator::Animator(void) {
	AnimatorManager::GetSingleton().Register(this);
}

Animator::~Animator(void) {
	AnimatorManager::GetSingleton().Cancel(this);
}

void Animator::NotifyStopped(void) {
	AnimatorManager::GetSingleton().MarkAsSuspended(this);
	if (onFinish)
		(onFinish)(this);
}

void Animator::NotifyStarted(void) {
	AnimatorManager::GetSingleton().MarkAsRunning(this);
	if (onStart)
		(onStart)(this);
}

void Animator::NotifyAction(const Animation& anim) {
	if (onAction)
		(onAction)(this, anim);
}

void Animator::Finish(bool isForced) {
	if (!HasFinished()) {
		state = isForced ? ANIMATOR_STOPPED : ANIMATOR_FINISHED; 
		NotifyStopped();
	}
}

bool Animator::HasFinished(void) const {
	return state != ANIMATOR_RUNNING;
}

void Animator::Stop(void) {
	Finish(true);
}

void Animator::TimeShift(timestamp_t offset) {
	lastTime += offset;
}

void Animator::SetLastTime(timestamp_t time) {
	lastTime = time;
}


// MovingAnimator
void MovingAnimator::Progress(timestamp_t currTime) {
	while (currTime > lastTime && (currTime - lastTime) >= anim->GetDelay()) {
		lastTime += anim->GetDelay();
		NotifyAction(*anim);
		if (!anim->IsForever() && ++currRep == anim->GetReps()) {
			state = ANIMATOR_FINISHED;
			NotifyStopped();
			return;
		}
	}
}

Animation* MovingAnimator::GetAnim(void){
	return anim;
}

void MovingAnimator::Start(MovingAnimation* a, timestamp_t t) {
	anim = a;
	lastTime = t;
	state = ANIMATOR_RUNNING;
	currRep = 0;
	NotifyStarted();
}

void MovingAnimator::deleteCurrAnimation() {
	assert(anim);
	delete anim;
	anim = nullptr;
}

//Moving path animator
void MovingPathAnimator::Progress(timestamp_t currTime) { //plants never die
	while (currTime > lastTime && (currTime - lastTime) >= anim->GetPath().at(frame).delay) {
		lastTime += anim->GetPath().at(frame).delay;
		NotifyAction(*anim);
		//if (!anim->IsForever() && ++currRep == anim->GetReps()) {
		//	state = ANIMATOR_FINISHED;
		//	NotifyStopped();
		//}
	}
}

void MovingPathAnimator::deleteCurrAnimation() {
	assert(anim);
	delete anim;
	anim = nullptr;
}

Animation* MovingPathAnimator::GetAnim(void){
	return anim;
}

unsigned MovingPathAnimator::GetFrame(void) {
	return frame;
}

void MovingPathAnimator::Start(MovingPathAnimation* a, timestamp_t t) {
	anim = a;
	lastTime = t;
	state = ANIMATOR_RUNNING;
	NotifyStarted();
}

void MovingPathAnimator::nextFrame() {
	frame++;
	if (frame == anim->GetPath().size())
		frame = 0;
}

//Flash animator
void FlashAnimator::Progress(timestamp_t currTime) {
	unsigned delay = 0;
	if (anim->GetShowing() == true) {
		delay = anim->GetShowDeay();
		anim->SetShowing(false);
	}
	else {
		delay = anim->GetHideDeay();
		anim->SetShowing(true);
	}

	while (currTime > lastTime && (currTime - lastTime) >= delay) {
		lastTime += delay;
		NotifyAction(*anim);
		if (++currRep == anim->GetRepetitions()) {
			state = ANIMATOR_FINISHED;
			NotifyStopped();
			return;
		}
	}
}

void FlashAnimator::Start(FlashAnimation* a, timestamp_t t) {
	anim = a;
	lastTime = t;
	state = ANIMATOR_RUNNING;
	NotifyStarted();
}

void FlashAnimator::deleteCurrAnimation() {
	assert(anim);
	delete anim;
	anim = nullptr;
}

void FlashAnimator::ResetCurrRep() {
	currRep = 0;
}

Animation* FlashAnimator::GetAnim(void) {
	return anim;
}

/*void Sprite_MoveAction(Sprite* sprite, const MovingAnimation& anim) {
	sprite->Move(anim.GetDx(), anim.GetDy());
}

animator->SetOnAction([sprite](Animator* animator, constAnimation& anim) {
	Sprite_MoveAction(sprite, (constMovingAnimation&)anim);
	}
);*/

// FrameRangeAnimator
void FrameRangeAnimator::Progress(timestamp_t currTime) {
	while (currTime > lastTime && (currTime - lastTime) >= anim->GetDelay()) {
		if (currFrame == anim->GetEndFrame()) {
			assert(anim->IsForever() || currRep < anim->GetReps());
			currFrame = anim->GetStartFrame(); // flip to start
		}
		else
			++currFrame;
		lastTime += anim->GetDelay();
		NotifyAction(*anim);
		if (currFrame == anim->GetEndFrame())
			if (!anim->IsForever() && ++currRep == anim->GetReps()) {
				state = ANIMATOR_FINISHED;
				NotifyStopped();
				return;
			}
	}
}

void FrameRangeAnimator::deleteCurrAnimation() {
	assert(anim);
	delete anim;
	anim = nullptr;
}

unsigned FrameRangeAnimator::GetCurrFrame(void) const {
	return currFrame;
}

unsigned FrameRangeAnimator::GetCurrRep(void) const {
	return currRep;
}

void FrameRangeAnimator::Start(FrameRangeAnimation* a, timestamp_t t) {
	anim = a;
	lastTime = t;
	state = ANIMATOR_RUNNING;
	currFrame = anim->GetStartFrame();
	currRep = 0;
	NotifyStarted();
	NotifyAction(*anim);
}

Animation* FrameRangeAnimator::GetAnim(void) {
	return anim;
}

// TickAnimator
void TickAnimator::Progress(timestamp_t currTime) {
	if (!anim->IsDiscrete()) {
		elapsedTime = currTime - lastTime;
		lastTime = currTime;
		NotifyAction(*anim);
	}
	else
		while (currTime > lastTime && (currTime - lastTime) >= anim->GetDelay()) {
			lastTime += anim->GetDelay();
			NotifyAction(*anim);
			if (!anim->IsForever() && ++currRep == anim->GetReps()) {
				state = ANIMATOR_FINISHED;
				NotifyStopped();
				return;
			}
		}
}

unsigned TickAnimator::GetCurrRep(void) const {
	return currRep;
}

unsigned TickAnimator::GetElapsedTime(void) const {
	return elapsedTime;
}

float TickAnimator::GetElapsedTimeNormalised(void) const {
	return float(elapsedTime) / float(anim->GetDelay());
}

void TickAnimator::Start(const TickAnimation& a, timestamp_t t) {
	anim = (TickAnimation*)a.Clone();
	lastTime = t;
	state = ANIMATOR_RUNNING;
	currRep = 0;
	elapsedTime = 0;
	NotifyStarted();
}

// AnimatorManager
AnimatorManager AnimatorManager::singleton;

void AnimatorManager::Register(Animator* a) {
	assert(a->HasFinished());
	suspended.insert(a);
}

void AnimatorManager::Cancel(Animator* a) {
	assert(a->HasFinished());
	suspended.erase(a);
}

void AnimatorManager::CancelAndRemoveAll() {
	auto copied1(suspended);
	for (auto* a : copied1) {
		//if (a->GetAnim() != nullptr)
		//	a->deleteCurrAnimation();
		suspended.erase(a);
		a->Destroy();
	}

	auto copied2(running);
	for (auto* a : copied2) {
		if (a->GetAnim() != nullptr)
			a->deleteCurrAnimation();
		running.erase(a);
		a->Destroy();
	}

	//suspended.erase(a);
}

void AnimatorManager::MarkAsRunning(Animator* a) {
	assert(!a->HasFinished());
	suspended.erase(a);
	running.insert(a);
}

void AnimatorManager::MarkAsSuspended(Animator* a) {
	//assert(a->HasFinished());
	running.erase(a);
	suspended.insert(a);
}

void AnimatorManager::Progress(timestamp_t currTime) {
	auto copied(running);
	for (auto* a : copied)
		a->Progress(currTime);
}

auto AnimatorManager::GetSingleton(void) -> AnimatorManager& {
	return singleton;
}

auto AnimatorManager::GetSingletonConst(void) -> const AnimatorManager& {
	return singleton;
}

void AnimatorManager::TimeShift(unsigned dt) {
	for (auto* a : running)
		a->TimeShift(dt);
}