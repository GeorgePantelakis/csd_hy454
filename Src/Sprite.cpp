#include "../Include/Sprite.h"
#include "../Include/Utilities.h"
extern struct game_params game_params;

// MotionQuantizer
MotionQuantizer& MotionQuantizer::SetRange(int h, int v) {
	horizMax = h, vertMax = v;
	used = true;
	return *this;
}

void MotionQuantizer::Move(const Rect& r, int* dx, int* dy) {
	if (!used)
		mover(r, dx, dy);
	else
		do {
			auto dxFinal = std::min(number_sign(*dx) * horizMax, *dx);
			auto dyFinal = std::min(number_sign(*dy) * vertMax, *dy);
			mover(r, &dxFinal, &dyFinal);
			if (!dxFinal) // X motion denied
				*dx = 0;
			else
				*dx -= dxFinal;
			if(!dyFinal)// Y motion denied
				*dy = 0;
			else
				*dy -= dyFinal;
		} while(*dx|| *dy);
}

// Clipper
template<typename Tfunc>
Clipper& Clipper::SetView(const Tfunc& f) {
	view = f;
	return *this;
}

bool Clipper::Clip(const Rect& r, const Rect& dpyArea, Point* dpyPos, Rect* clippedBox) const {
	Rect visibleArea;
	if (!clip_rect(r, view(), &visibleArea)) {
		clippedBox->w = clippedBox->h = 0;
		return false;
	} else {
		clippedBox->x = r.x - visibleArea.x;
		clippedBox->y = r.y - visibleArea.y;
		clippedBox->w = visibleArea.w;
		clippedBox->h = visibleArea.h;
		dpyPos->x = dpyArea.x + (visibleArea.x - view().x);
		dpyPos->y = dpyArea.y + (visibleArea.y - view().y);
		return true;
	}
}

// Sprite
Sprite::Sprite(int _x, int _y, const AnimationFilm* film, const std::string& _typeId) :
	x(_x), y(_y), currFilm(film), typeId(_typeId) {
	frameNo = currFilm->GetTotalFrames();
	SetFrame(0);
}

const Rect Sprite::GetBox(void) const {
	return { x, y, frameBox.w, frameBox.h };
}

void Sprite::SetBoxDimentions(int w = 16, int h = 16) {
	frameBox.w = w;
	frameBox.h = h;
}

Sprite& Sprite::Move(int dx, int dy) {
	if (directMotion) // apply unconditionally offsets!
		x += dx, y += dy;
	else {
		quantizer.Move(GetBox(), &dx, &dy);
		gravity.Check(GetBox());
	}
	if(boundingArea)
		boundingArea->move(dx, dy);
	return *this;
}


void Sprite::SetPos(int _x, int _y) {
	x = _x; y = _y;
}

void Sprite::SetZorder(unsigned z) {
	zorder = z;
}

unsigned Sprite::GetZorder(void) {
	return zorder;
}

void Sprite::SetAnimator(Animator * anim) {
	animator = anim;
}

Animator* Sprite::GetAnimator(void) {
	return animator;
}

const AnimationFilm* Sprite::GetCurrFilm() {
	return currFilm;
}

void Sprite::SetCurrFilm(const AnimationFilm* newFilm) {
	currFilm = newFilm;
}

void Sprite::NextFrame() {
	frameNo++;
	if (currFilm->GetTotalFrames() <= frameNo)
		frameNo = 0; //start over
}

void Sprite::SetFrame(unsigned char i) {
	if (i != frameNo) {
		assert(i < currFilm->GetTotalFrames());
		frameBox = currFilm->GetFrameBox(frameNo = i);
	}
}

unsigned char Sprite::GetFrame(void) const {
	return frameNo;
}

void Sprite::SetBoundingArea(const BoundingArea& area) {
	assert(!boundingArea);
	boundingArea = area.Clone();
}

void Sprite::ReplaceBoundingArea(BoundingArea* area) {
	assert(boundingArea);
	delete boundingArea;
	boundingArea = area;
}

void Sprite::SetBoundingArea(BoundingArea* area) {
	assert(!boundingArea);
	boundingArea = area;
}

const BoundingArea* Sprite::GetBoundingArea(void) {
	return boundingArea;
}

auto Sprite::GetTypeId(void) -> const std::string& {
	return typeId;
}

spritestate_t Sprite::GetStateId(void){
	return stateId;
}

void Sprite::SetStateId(spritestate_t id){
	stateId = id;
}

string Sprite::Get_Str_StateId(void) {
	return str_stateId;
}

void Sprite::Set_Str_StateId(string id) {
	str_stateId = id;
}


void Sprite::SetVisibility(bool v) {
	isVisible = v;
}

bool Sprite::IsVisible(void) const {
	return isVisible;
}

void Sprite::Display(Bitmap dest) {
	if (isVisible)
		GetCurrFilm()->DisplayFrame(dest, Point{GetBox().x, GetBox().y }, GetFrame());
}

const Sprite::Mover MakeSpriteGridLayerMover(GridLayer* gridLayer, Sprite* sprite) {
	return [gridLayer, sprite](const Rect& rect, int* dx, int* dy) {
		gridLayer->FilterGridMotion(sprite->GetBox(), dx, dy);
	};
}


extern bool isDead;

void Sprite::hit() {
	if (isHit || formStateId == INVINCIBLE_MARIO_SMALL || formStateId == INVINCIBLE_MARIO_SUPER) 
		return; //if sprite has alrteady been hit, dont hit it again :(
	
	if (formStateId == SUPER_MARIO) {
		formStateId = SMALL_MARIO;
		str_stateId = "Mario_small";
		SetBoxDimentions(16, 16);
		ReplaceBoundingArea(new BoundingBox(GetBox().x, GetBox().y, GetBox().x + GetBox().w, GetBox().y + GetBox().h));
		gravity.Check(GetBox());
	}
	else { //mario is small and gets hit->KILL HIM!
		isDead = true;
		//return; <- TODO: uncomment when we make the mario getting killed
	}
	
	FlashAnimator* animator = new FlashAnimator();
	AnimatorManager::GetSingleton().Register(animator);

	animator->SetOnStart([this](Animator* animator) {
		isHit = true;
	});

	animator->SetOnAction([this](Animator* animator, const Animation& anim) {
		isVisible = !isVisible;
		//Sprite_MoveAction(mario, (const MovingAnimation&)anim);
	});

	animator->SetOnFinish([this](Animator* animator) {
		((FlashAnimator*)animator)->deleteCurrAnimation();
		isHit = false;
		isVisible = true;
		AnimatorManager::GetSingleton().Cancel(animator);
		animator->Destroy();
	});

	animator->Start(new FlashAnimation("flash", 30, 100, 100), GetGameTime());
}

GravityHandler& Sprite::GetGravityHandler(void) {
	return gravity;
}

void Sprite::SetHasDirectMotion(bool v) {
	directMotion = v;
}

bool Sprite::GetHasDirectMotion(void) const {
	return directMotion;
}

char Sprite::GetSpeed() {
	return speed;
}

void Sprite::incSpeed(unsigned long time) {
	if (time - lastSpeedUpdate >= speedDelay && speedUpdatedTimes < game_params.mario_max_speed) {
		speed = ((speedUpdatedTimes + 1) * game_params.mario_max_speed) / game_params.mario_max_speed;
		lastSpeedUpdate = time;
		speedUpdatedTimes++;
	}
	else if (speedUpdatedTimes == game_params.mario_max_speed) {
		speed = game_params.mario_max_speed;
		SetStateId(RUNNING_STATE);
	}
}

void Sprite::resetSpeed() {
	speed = 0;
	speedUpdatedTimes = 0;
}

void Sprite::SetLastSpeedUpdate(unsigned long time) {
	lastSpeedUpdate = time;
}

bool Sprite::CollisionCheck(Sprite* s) const {
	return boundingArea->Intersects(*s->GetBoundingArea());
}

spriteFormState_t Sprite::GetFormStateId(void) {
	return formStateId;
}

void Sprite::SetFormStateId(spriteFormState_t state) {
	formStateId = state;
}

// CollisionChecker
CollisionChecker CollisionChecker::singleton;

void CollisionChecker::Cancel(Sprite* s1, Sprite* s2) {
	auto i = std::find_if(entries.begin(), entries.end(), [s1, s2](const Entry& e) {
		return std::get<0>(e) == s1 && std::get<1>(e) == s2; // || std::get<0>(e) == s2 && std::get<1>(e) == s1;
	});
	entries.erase(i);
}

void CollisionChecker::CancelAll(Sprite* s1) {
	while (1) {
		auto i = std::find_if(entries.begin(), entries.end(), [s1](const Entry& e) {
			return std::get<0>(e) == s1 || std::get<1>(e) == s1;
		});
		if (i != entries.end())
			entries.erase(i);
		else
			break;
	}
}

void CollisionChecker::Check(void) const {
	std::list<Entry> entries_tmp = entries;
	for (auto& e : entries_tmp)
		if (std::get<0>(e)->CollisionCheck(std::get<1>(e)))
			std::get<2>(e)(std::get<0>(e), std::get<1>(e));
}

void CollisionChecker::clear(void) {
	entries.clear();
}

auto CollisionChecker::GetSingleton(void) -> CollisionChecker& {
	return singleton;
}

auto CollisionChecker::GetSingletonConst(void) -> const CollisionChecker& {
	return singleton;
}

// SpriteManager
SpriteManager SpriteManager::singleton;

void SpriteManager::Add(Sprite* s) {
	dpyList.push_back(s);
	dpyList.sort([](Sprite* s1, Sprite* s2) -> bool { return s1->GetZorder() < s2->GetZorder(); });
	
	types[s->GetTypeId()].push_back(s);
}

void SpriteManager::Remove(Sprite* s) {
	dpyList.remove(s);
	types[s->GetTypeId()].remove(s);
	delete s;
}

void SpriteManager::RemoveAll(void) {
	auto temp = dpyList;

	for (auto sprite : temp) {
		dpyList.remove(sprite);
		types[sprite->GetTypeId()].remove(sprite);
		delete sprite;
	}
}

auto SpriteManager::GetDisplayList(void) -> const SpriteList& {
	return dpyList;
}

auto SpriteManager::GetTypeList(const std::string& typeId) -> const SpriteList& {
	return types[typeId];
}

auto SpriteManager::GetSingleton(void) -> SpriteManager& {
	return singleton;
}

auto SpriteManager::GetSingletonConst(void) -> const SpriteManager& {
	return singleton;
}

// General functions
const Clipper MakeTileLayerClipper(TileLayer* layer) {
	return Clipper().SetView([layer](void) { return layer->GetViewWindow(); });
}

extern class TileLayer* action_layer;
void PrepareSpriteGravityHandler(GridLayer* gridLayer, Sprite* sprite) {
	sprite->GetGravityHandler().SetOnSolidGround([gridLayer, sprite](const Rect& r) {
		Rect posOnGrid{
			r.x + action_layer->GetViewWindow().x,
			r.y + action_layer->GetViewWindow().y,
			r.w,
			r.h,
		};
		
		return gridLayer->IsOnSolidGround(posOnGrid, sprite->GetStateId());
	});
}

bool clip_rect(const Rect& r, const Rect& area, Rect* result) {
	return clip_rect(r.x, r.y, r.w, r.h, area.x, area.y, area.w, area.h, &result->x, &result->y, &result->w, &result->h);
}