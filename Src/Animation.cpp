#include "../Include/Animation.h"
#include "../Include/Utilities.h"

// AnimationFilm
AnimationFilm::AnimationFilm(const std::string& _id) : id(_id) {}
AnimationFilm::AnimationFilm(Bitmap _bitmap, const std::vector<Rect>& _boxes, const std::string& _id) {
	bitmap = _bitmap;
	boxes = _boxes;
	id = _id;
}

unsigned char AnimationFilm::GetTotalFrames(void) const {
	return (unsigned char)boxes.size();
}

Bitmap AnimationFilm::GetBitmap(void) const {
	return bitmap;
}

auto AnimationFilm::GetId(void) const -> const std::string& {
	return id;
}

const Rect& AnimationFilm::GetFrameBox(unsigned char frameNo) const {
	assert(boxes.size() > frameNo);
	return boxes[frameNo];
}

void AnimationFilm::DisplayFrame(Bitmap dest, const Point& at, unsigned char frameNo) const {
	BitmapBlit(bitmap, GetFrameBox(frameNo), dest, at); // MaskedBlit
}

void AnimationFilm::SetBitmap(Bitmap b) {
	assert(!bitmap);
	bitmap = b;
}

void AnimationFilm::Append(const Rect& r) {
	boxes.push_back(r);
}

// AnimationFilmHolder
AnimationFilmHolder AnimationFilmHolder::holder;

auto AnimationFilmHolder::GetInstance(void) -> AnimationFilmHolder& {
	return holder;
}

// TODO(4u): set a parsing functor implemented externally to the class
// -1=error, 0=ended gracefully, else #chars read
int AnimationFilmHolder::ParseEntry(int startPos, const std::string& text, std::string& id, std::vector<Rect>& rects) {
	if (startPos >= text.size())
		return 0;

	char c;
	int charsread = 1;
	string sections[2]; // character with action, positions;
	int pos = 0;
	while ((c = text.at(startPos++)) != '$') {
		charsread++;

		if (c == ':') 
			pos++;
		else 
			sections[pos] += c;
	}

	id = sections[0];

	for (auto i : splitString(sections[1], ",")) {
		vector<string> coordinates = splitString(i, " ");

		int x = atoi(coordinates[0].c_str());
		int y = atoi(coordinates[1].c_str());
		int width = atoi(coordinates[2].c_str());
		int height = atoi(coordinates[3].c_str());
		rects.push_back(Rect{ x, y, width, height });
	}

	return charsread;
}


void AnimationFilmHolder::LoadAll(const std::string& text, const std::string& path) {
	int pos = 0;

	while (true) {
		std::string id;
		std::vector<Rect> rects;
		int i = ParseEntry(pos, text, id, rects);
		assert(i >= 0);
		if (!i) return;
		pos += i;
		assert(!GetFilm(id));
		films[id] = new AnimationFilm(bitmaps->Load(path), rects, id);
	}

}

AnimationFilmHolder::AnimationFilmHolder() {}
AnimationFilmHolder::~AnimationFilmHolder() { CleanUp(); }

void AnimationFilmHolder::CleanUp(void) {
	for (auto& i : films)
		delete (i.second);
	films.clear();
	//delete bitmaps;
}

auto AnimationFilmHolder::GetFilm(const std::string& id) -> const AnimationFilm* const {
	auto i = films.find(id);
	return i != films.end() ? i->second : nullptr;
}

// BitmapLoader
BitmapLoader::BitmapLoader(void) {}
BitmapLoader::~BitmapLoader() {
	CleanUp();
}

Bitmap BitmapLoader::GetBitmap(const std::string& path) const {
	auto i = bitmaps.find(path);
	return i != bitmaps.end() ? i->second : nullptr;
}

Bitmap BitmapLoader::Load(const std::string& path) {
	auto b = GetBitmap(path);
	if (!b) {
		bitmaps[path] = b = BitmapLoad(path);
		assert(b);
	}
	return b;
}

// prefer to massively clear bitmaps at the end than
// to destroy individual bitmaps during gameplay
void  BitmapLoader::CleanUp(void) {
	for (auto& i : bitmaps)
		BitmapDestroy(i.second);
	bitmaps.clear();
}

// Animation
Animation::Animation(const std::string& _id) : id(_id) {}
Animation::~Animation() {}

const std::string& Animation::GetId(void) {
	return id;
}

void Animation::SetId(const std::string& _id) {
	id = _id;
}

// MovingAnimation
MovingAnimation::MovingAnimation(const std::string& _id, unsigned _reps, int _dx, int _dy, unsigned _delay) :
	Animation(_id), reps(_reps), dx(_dx), dy(_dy), delay(_delay) {}

int MovingAnimation::GetDx(void) const {
	return dx;
}

MovingAnimation::Me& MovingAnimation::SetDx(int v) {
	dx = v;
	return *this;
}

int MovingAnimation::GetDy(void) const {
	return dy;
}

MovingAnimation::Me& MovingAnimation::SetDy(int v) {
	dy = v;
	return *this;
}

unsigned MovingAnimation::GetDelay(void) const {
	return delay;
}

MovingAnimation::Me& MovingAnimation::SetDelay(unsigned v) {
	delay = v;
	return *this;
}

unsigned MovingAnimation::GetReps(void) const {
	return reps;
}

MovingAnimation::Me& MovingAnimation::SetReps(unsigned n) {
	reps = n;
	return *this;
}

bool MovingAnimation::IsForever(void) const {
	return !reps;
}

MovingAnimation::Me& MovingAnimation::SetForever(void) {
	reps = 0;
	return *this;
}

Animation* MovingAnimation::Clone(void) const {
	return new MovingAnimation(id, reps, dx, dy, delay);
}

// FrameRangeAnimation
FrameRangeAnimation::FrameRangeAnimation(const std::string& _id, unsigned s, unsigned e, unsigned r, int dx, int dy, int d):
	start(s), end(e), MovingAnimation(id, r, dx, dy, d) {}

unsigned FrameRangeAnimation::GetStartFrame(void) const {
	return start;
}

FrameRangeAnimation::Me& FrameRangeAnimation::SetStartFrame(unsigned v) {
	start = v;
	return *this;
}

unsigned FrameRangeAnimation::GetEndFrame(void) const {
	return end;
}

FrameRangeAnimation::Me& FrameRangeAnimation::SetEndFrame(unsigned v) {
	end = v;
	return *this;
}

FrameRangeAnimation::ExtraAction FrameRangeAnimation::GetChangeSpeed(void) const {
	return changeSpeed;
}

FrameRangeAnimation::Me& FrameRangeAnimation::SetChangeSpeed(FrameRangeAnimation::ExtraAction action) {
	changeSpeed = action;
	return *this;
}

Animation* FrameRangeAnimation::Clone(void) const {
	return new FrameRangeAnimation(id, start, end, GetReps(), GetDx(), GetDy(), GetDelay());
}

void FrameRangeAnimation::ChangeSpeed(int frameNo) {
	if (changeSpeed) {
		int new_dx, new_dy;
		new_dx = GetDx();
		new_dy = GetDy();
		changeSpeed(new_dx, new_dy, frameNo);
		SetDx(new_dx);
		SetDy(new_dy);
	}
}

// FrameListAnimation
FrameListAnimation::FrameListAnimation(const std::string& _id, const Frames& _frames, unsigned r, int dx, int dy, unsigned d, bool c) :
	frames(_frames), MovingAnimation(id, r, dx, dy, d) {}

const FrameListAnimation::Frames& FrameListAnimation::GetFrames(void) const {
	return frames;
}

void FrameListAnimation::SetFrames(const Frames& f) {
	frames = f;
}

Animation* FrameListAnimation::Clone(void) const {
	return new FrameListAnimation(id, frames, GetReps(), GetDx(), GetDy(), GetDelay(), true);
}

// MovingPathAnimation
MovingPathAnimation::MovingPathAnimation(const std::string& _id, const Path& _path) :
	path(_path), Animation(id) {}

const MovingPathAnimation::Path& MovingPathAnimation::GetPath(void) const {
	return path;
}

void MovingPathAnimation::SetPath(const Path& p) {
	path = p;
}

Animation* MovingPathAnimation::Clone(void) const {
	return new MovingPathAnimation(id, path);
}

// FlashAnimation
FlashAnimation::FlashAnimation(const std::string& _id, unsigned n, unsigned show, unsigned hide) :
	Animation(id), repetitions(n), hideDelay(hide), showDelay(show) {}

FlashAnimation::Me& FlashAnimation::SetRepetitions(unsigned n) {
	repetitions = n;
	return *this;
}

unsigned FlashAnimation::GetRepetitions(void) const {
	return repetitions;
}

FlashAnimation::Me& FlashAnimation::SetHideDeay(unsigned d) {
	hideDelay = d;
	return *this;
}

unsigned FlashAnimation::GetHideDeay(void) const {
	return hideDelay;
}

FlashAnimation::Me& FlashAnimation::SetShowDeay(unsigned d) {
	showDelay = d;
	return *this;
}

unsigned FlashAnimation::GetShowDeay(void) const {
	return showDelay;
}

void FlashAnimation::SetShowing(bool val) {
	showing = val;
}

bool FlashAnimation::GetShowing() {
	return showing;
}

Animation* FlashAnimation::Clone(void) const {
	return new FlashAnimation(id, repetitions, hideDelay, showDelay);
}

// ScrollAnimation
ScrollAnimation::ScrollAnimation(const std::string& _id, const Scroll& _scroll) :
	Animation(_id), scroll(_scroll) {}

const ScrollAnimation::Scroll& ScrollAnimation::GetScroll(void) const {
	return scroll;
}

void ScrollAnimation::SetScroll(const Scroll& p) {
	scroll = p;
}

Animation* ScrollAnimation::Clone(void) const {
	return new ScrollAnimation(id, scroll);
}

// TickAnimation
TickAnimation::TickAnimation(const std::string& _id, unsigned d, unsigned r, bool discrete) :
	Animation(id), delay(d), reps(r), isDiscrete(discrete) {
	assert(Inv());
}

bool TickAnimation::Inv(void) const {
	return isDiscrete || reps == 1;
}

unsigned TickAnimation::GetDelay(void) const {
	return delay;
}

TickAnimation::Me& TickAnimation::SetDelay(unsigned v) {
	delay = v;
	return *this;
}

unsigned TickAnimation::GetReps(void) const {
	return reps;
}

TickAnimation::Me& TickAnimation::SetReps(unsigned n) {
	reps = n;
	return *this;
}

bool TickAnimation::IsForever(void) const {
	return !reps;
}

TickAnimation::Me& TickAnimation::SetForever(void) {
	reps = 0;
	return *this;
}

bool TickAnimation::IsDiscrete(void) const {
	return isDiscrete;
}

Animation* TickAnimation::Clone(void) const {
	return new TickAnimation(id, delay, reps, isDiscrete);
}

void Animate(const AnimationFilm& film, const Point at) {
	uint64_t t = 0;
	for (unsigned char i = 0; i < film.GetTotalFrames(); )
		if (GetSystemTime() >= t) {
			t = GetSystemTime() + FRAME_DELAY; 
			while (!al_wait_for_vsync());//Vsync();
			BitmapClear(BitmapGetScreen(), Make24(0, 0, 0));
			film.DisplayFrame(BitmapGetScreen(), at, i++);
		}
}