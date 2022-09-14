#pragma once

#include "Animation.h"
#include "Animator.h"
#include "GravityHandler.h"
#include "States.h"
#include "BoundingArea.h"
#include <list>

class MotionQuantizer {
public:
	using Mover = std::function<void(const Rect& r, int* dx, int* dy)>;
protected:
	int horizMax = 0, vertMax = 0;
	Mover mover; // filters too
	bool used = false;
public:
	MotionQuantizer(void) = default;
	MotionQuantizer(const MotionQuantizer&) = default;

	MotionQuantizer& SetRange(int h, int v);
	template<typename Tfunc>
	MotionQuantizer& SetMover(const Tfunc& f);
	void Move (const Rect& r, int* dx, int* dy);
};

template<typename Tfunc>
MotionQuantizer& MotionQuantizer::SetMover(const Tfunc& f) {
	mover = f;
	return *this;
}

	
class Clipper {
public:
	using View = std::function<const Rect& (void)>;
private:
	View view;
public:
	Clipper(void) = default;
	Clipper(const Clipper&) = default;

	template<typename Tfunc>
	Clipper& SetView(const Tfunc& f);
	bool Clip(const Rect& r, const Rect& dpyArea, Point* dpyPos, Rect* clippedBox) const;
		
};

class Sprite {
public:
	using Mover = std::function<void(const Rect&, int* dx, int* dy)>;
protected:
	unsigned char frameNo = 0;
	Rect frameBox;
	int x = 0, y = 0;
	bool isVisible = true;
	bool isHit = false;
	const AnimationFilm* currFilm = nullptr;
	Animator* animator = nullptr;
	BoundingArea* boundingArea = nullptr;
	unsigned zorder = 0;
	std::string typeId;

	spritestate_t stateId = IDLE_STATE;
	string str_stateId = "Mario_small";
	spriteFormState_t formStateId;

	Mover mover;
	MotionQuantizer quantizer;

	bool directMotion = false;
	GravityHandler gravity;
	char speed = 0;
	unsigned long lastSpeedUpdate = 0;
	int speedDelay = 150;
	short int speedUpdatedTimes = 0;
public:
	bool lastMovedRight = false;
	bool won = false;

	Sprite(int _x, int _y, const AnimationFilm* film, const std::string& _typeId = "");

	template<typename Tfunc>
	void SetMover(const Tfunc& f);
	const Rect GetBox(void) const;
	void SetBoxDimentions(int w, int h);
	Sprite& Move(int dx, int dy);
	void SetPos(int _x, int _y);
	void SetZorder(unsigned z);
	unsigned GetZorder(void);

	const AnimationFilm* GetCurrFilm();
	void SetCurrFilm(const AnimationFilm* newFilm);  //slides doesn't do this
	Animator* GetAnimator(void);
	void SetAnimator(Animator*);
	void NextFrame();
	void SetFrame(unsigned char i);
	unsigned char GetFrame(void) const;
	void SetBoundingArea(const BoundingArea& area);
	void SetBoundingArea(BoundingArea* area);
	const BoundingArea* GetBoundingArea(void);
	void ReplaceBoundingArea(BoundingArea* area);
	
	auto GetTypeId(void) -> const std::string&;
	spritestate_t GetStateId(void);
	void SetStateId(spritestate_t);
	string Get_Str_StateId(void);
	void Set_Str_StateId(string state);

	void SetVisibility(bool v);
	bool IsVisible(void) const;
	bool CollisionCheck(Sprite* s) const;
	void Display(Bitmap dest);

	GravityHandler& GetGravityHandler(void);
	void SetHasDirectMotion(bool v);
	bool GetHasDirectMotion(void) const;

	char GetSpeed();
	void incSpeed(unsigned long time);
	void resetSpeed();
	void SetLastSpeedUpdate(unsigned long time);

	spriteFormState_t GetFormStateId(void);
	void SetFormStateId(spriteFormState_t);

	void hit();
};

template<typename Tfunc>
void Sprite::SetMover(const Tfunc& f) {
	quantizer.SetMover(f);
}

class CollisionChecker final{
	public:
		using Action = std::function<void(Sprite* s1, Sprite* s2)>;
		static CollisionChecker singleton;
	
	protected:
		using Entry = std::tuple<Sprite*, Sprite*, Action>;
		std::list<Entry> entries;
	
	public:
		template<typename T>
		void Register(Sprite* s1, Sprite* s2, const T& f);
		void Cancel(Sprite* s1, Sprite* s2);
		void CancelAll(Sprite* s1);
		void Check(void) const;
		void clear(void);
		static auto GetSingleton(void)->CollisionChecker&;
		static auto GetSingletonConst(void) -> const CollisionChecker&;
};

template<typename T>
void CollisionChecker::Register(Sprite* s1, Sprite* s2, const T& f) {
	entries.push_back(std::make_tuple(s1, s2, f));
}


class SpriteManager final {
	public:
		using SpriteList = std::list<Sprite*>;
		using TypeLists = std::map<std::string, SpriteList>;

	private:
		SpriteList dpyList;
		TypeLists types;
		static SpriteManager singleton;

	public:
		void Add(Sprite* s); // insert by ascending order
		void Remove(Sprite* s);
		void RemoveAll(void);
		auto GetDisplayList(void) -> const SpriteList&;
		auto GetTypeList(const std::string& typeId) -> const SpriteList&;
		static auto GetSingleton(void)->SpriteManager&;
		static auto GetSingletonConst(void) -> const SpriteManager&;
};


const Clipper MakeTileLayerClipper(TileLayer* layer);
const Sprite::Mover MakeSpriteGridLayerMover(GridLayer* gridLayer, Sprite* sprite);


template<typename Tnum>
int number_sign(Tnum x) {
	return x > 0 ? 1 : x < 0 ? -1 : 0;
}

template<class T>
bool clip_rect(T x, T y, T w, T h, T wx, T wy, T ww, T wh, T* cx, T* cy, T* cw, T* ch) {
	*cw = T(std::min(wx + ww, x + w)) - (*cx = T(std::max(wx, x)));
	*ch = T(std::min(wy + wh, y + h)) - (*cy = T(std::max(wy, y)));
	return *cw > 0 && *ch > 0;
}

bool clip_rect(const Rect& r, const Rect& area, Rect* result);
void PrepareSpriteGravityHandler(GridLayer* gridLayer, Sprite* sprite);