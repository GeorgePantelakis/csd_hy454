#include "../Include/BoundingArea.h"
#include "../Include/app.h"

// BoundingBox
bool BoundingBox::Intersects(const BoundingBox &box) const {
	return !(
		box.x2 < x1 || //at left
		x2 < box.x1 || //at right
		box.y2 < y1 || //above
		y2 < box.y1    //bellow
		);
}

bool BoundingBox::onTop(const BoundingBox& box) const {
	return !(
		box.y2 < y1  //above
		);
}

bool BoundingBox::Intersects(const BoundingCircle &circle) const {
	return circle.Intersects(*this);
}

bool BoundingBox::Intersects(const BoundingPolygon &poly) const {
	BoundingPolygon::Polygon points;
	points.push_back(BoundingPolygon::Point(x1, y1));
	points.push_back(BoundingPolygon::Point(x2, y2));
	BoundingPolygon selfPoly(points);

	return poly.Intersects(selfPoly);
}

bool BoundingBox::In(unsigned x, unsigned y) const {
	return x1 <= x && x <= x2 && y1 <= y && y <= y2;
}

BoundingBox::BoundingBox(unsigned _x1, unsigned _y1, unsigned _x2, unsigned _y2) :
	x1(_x1), y1(_y1), x2(_x2), y2(_y2) {}

bool BoundingBox::Intersects(const BoundingArea &area) const {
	return area.Intersects(*this);
}

BoundingArea* BoundingBox::Clone(void) const {
	return new BoundingBox(x1, y1, x2, y2);
}

void BoundingBox::move(int& dx, int& dy){
	x1 += dx;
	x2 += dx;
	y1 += dy;
	y2 += dy;
}

unsigned BoundingBox::getX1() const {
	return x1;
}

unsigned BoundingBox::getX2() const {
	return x2;
}

unsigned BoundingBox::getY1() const {
	return y1;
}

unsigned BoundingBox::getY2() const {
	return y2;
}

unsigned BoundingBox::getMaxDiagonal(void) const {
	return sqrt((double)(x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

void BoundingBox::getCenter(unsigned &cx, unsigned &cy) const {
	cx = abs((long)(x2 - x1));
	cy = abs((long)(y2 - y1));
}

unsigned BoundingBox::getWidth() const {
	return x2 - x1;
}

unsigned BoundingBox::getHeight() const {
	return y2 - y1;
}

// BoundingCircle
bool BoundingCircle::Intersects(const BoundingBox& box) const {
	float closestX = (x < box.getX1() ? box.getX1() : (x > box.getX2() ? box.getX2() : x));
	float closestY = (y < box.getY1() ? box.getY1() : (y > box.getY2() ? box.getY2() : y));
	float dx = closestX - x;
	float dy = closestY - y;

	return (dx * dx + dy * dy) <= r * r;
}

bool BoundingCircle::Intersects(const BoundingCircle& circle) const {
	return sqrt(((double)(x - circle.x) * (x - circle.x)) + ((double)(y - circle.y) * (y - circle.y))) < ((double)r + circle.r);
}

static bool pointCircle(float px, float py, const BoundingCircle& circle) {
	float distX = px - circle.getX();
	float distY = py - circle.getY();
	float distance = sqrt((distX * distX) + (distY * distY));

	return distance <= circle.getR();
}

static bool linePoint(float x1, float y1, float x2, float y2, const BoundingPolygon::Point& p) {
	float d1 = sqrt((p.x - x1) * (p.x - x1) + (p.y - y1) * (p.y - y1));
	float d2 = sqrt((p.x - x2) * (p.x - x2) + (p.y - y2) * (p.y - y2));

	float lineLen = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));

	float buffer = 0.1;

	return d1 + d2 >= lineLen - buffer && d1 + d2 <= lineLen + buffer;
}

static bool lineCircle(float x1, float y1, float x2, float y2, const BoundingCircle& circle) {
	bool inside1 = pointCircle(x1, y1, circle);
	bool inside2 = pointCircle(x2, y2, circle);
	if (inside1 || inside2) return true;

	float distX = x1 - x2;
	float distY = y1 - y2;
	float len = sqrt((distX * distX) + (distY * distY));

	float dot = (((circle.getX() - x1) * (x2 - x1)) + ((circle.getY() - y1) * (y2 - y1))) / pow(len, 2);

	float closestX = x1 + (dot * (x2 - x1));
	float closestY = y1 + (dot * (y2 - y1));

	bool onSegment = linePoint(x1, y1, x2, y2, { (unsigned int)closestX, (unsigned int)closestY });
	if (!onSegment) return false;

	distX = closestX - circle.getX();
	distY = closestY - circle.getY();
	float distance = sqrt((distX * distX) + (distY * distY));

	return distance <= circle.getR();
}

static bool polygonPoint(const BoundingPolygon::Polygon& points, const BoundingPolygon::Point& p) {
	bool collision = false;

	std::vector<BoundingPolygon::Point> vpoints = std::vector<BoundingPolygon::Point>(points.begin(), points.end());

	int next = 0;
	for (int current = 0; current < vpoints.size(); current++) {
		next = current + 1;
		if (next == points.size()) next = 0;

		BoundingPolygon::Point vc = vpoints[current];
		BoundingPolygon::Point vn = vpoints[next];

		if (((vc.y > p.y && vn.y < p.y) || (vc.y < p.y && vn.y > p.y)) &&
			(p.x < (vn.x - vc.x) * (p.y - vc.y) / (vn.y - vc.y) + vc.x)) {
			collision = !collision;
		}
	}
	return collision;
}

bool BoundingCircle::Intersects(const BoundingPolygon& poly) const {
	// same as BoundingBox but for N lines
	BoundingPolygon::Polygon points = poly.getPoints();
	std::vector<BoundingPolygon::Point> vpoints = std::vector<BoundingPolygon::Point>(points.begin(), points.end());

	int next = 0;
	for (int current = 0; current < vpoints.size(); current++) {
		next = current + 1;
		if (next == vpoints.size()) next = 0;

		BoundingPolygon::Point vc = vpoints[current];
		BoundingPolygon::Point vn = vpoints[next];

		bool collision = lineCircle(vc.x, vc.y, vn.x, vn.y, *this);
		if (collision) return true;
	}
	return false;
}

bool BoundingCircle::In(unsigned _x, unsigned _y) const {
	return sqrt((double)(x - _x) * (x - _x) + (y - _y) * (y - _y)) <= r;
}

BoundingCircle::BoundingCircle(unsigned _x, unsigned _y, unsigned _r) :
	x(_x), y(_y), r(_r){}

bool BoundingCircle::Intersects(const BoundingArea& area) const {
	return area.Intersects(*this);
}

BoundingArea* BoundingCircle::Clone(void) const {
	return new BoundingCircle(x, y, r);
}

void BoundingCircle::move(int& dx, int& dy) {
	x += dx;
	y += dy;
}

unsigned BoundingCircle::getX() const {
	return x;
}

unsigned BoundingCircle::getY() const {
	return y;
}

unsigned BoundingCircle::getR() const {
	return r;
}

// BoundingPolygon
bool BoundingPolygon::Intersects(const BoundingPolygon& poly) const {
	BoundingPolygon::Polygon _points = poly.getPoints();
	std::vector<BoundingPolygon::Point> more_points = points.size() > poly.getPoints().size() ?
		std::vector<BoundingPolygon::Point>(points.begin(), points.end()) : std::vector<BoundingPolygon::Point>(_points.begin(), _points.end());
	std::vector<BoundingPolygon::Point> less_points = points.size() <= poly.getPoints().size() ?
		std::vector<BoundingPolygon::Point>(points.begin(), points.end()) : std::vector<BoundingPolygon::Point>(_points.begin(), _points.end());
	
	bool intersects = false;
	for (int i = 0; i < less_points.size() - 1; ++i)
		intersects = intersects
			|| more_points[i + 1].x < less_points[i].x
			|| less_points[i + 1].x < more_points[i].x
			|| more_points[i + 1].y < less_points[i].y
			|| less_points[i + 1].y < more_points[i].y;
	return intersects;
}

bool BoundingPolygon::Intersects(const BoundingCircle& circle) const {
	return circle.Intersects(*this);
}

bool BoundingPolygon::Intersects(const BoundingBox& box) const {
	return box.Intersects(*this);
}

bool BoundingPolygon::In(unsigned x, unsigned y) const {
	std::vector<BoundingPolygon::Point> vpoints = std::vector<BoundingPolygon::Point>(points.begin(), points.end());
	bool in = true;
	for (int i = 0; i < vpoints.size() - 1; ++i)
		in = in && vpoints[i].x <= x && x <= vpoints[i + 1].x && vpoints[i].y <= y && y <= vpoints[i + 1].y;
	return in;
}

BoundingPolygon::BoundingPolygon(const Polygon &_points) :
	points(_points) {}

bool BoundingPolygon::Intersects(const BoundingArea& area) const {
	return area.Intersects(*this);
}

BoundingArea* BoundingPolygon::Clone(void) const {
	return new BoundingPolygon(points);
}

void BoundingPolygon::move(int& dx, int& dy) {
	for (auto onePoint : points) {
		onePoint.x += dx;
		onePoint.y += dy;
	}
}

BoundingPolygon::Polygon BoundingPolygon::getPoints() const {
	return points;
}