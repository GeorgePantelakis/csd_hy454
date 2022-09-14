#pragma once

#include <list>
#include <vector>

class BoundingBox;
class BoundingCircle;
class BoundingPolygon;

class BoundingArea {
public:
	virtual bool Intersects(const BoundingBox&) const = 0;
	virtual bool Intersects(const BoundingCircle&) const = 0;
	virtual bool Intersects(const BoundingPolygon&) const = 0;
	virtual bool In(unsigned, unsigned) const = 0;
	virtual bool Intersects(const BoundingArea&) const = 0;
	virtual BoundingArea* Clone(void) const = 0;
	virtual void move(int& dx, int& dy) = 0;
	~BoundingArea() {}
};

class BoundingBox: public BoundingArea {
private:
	int x1, y1, x2, y2;
public:
	BoundingBox(unsigned, unsigned, unsigned, unsigned);

	virtual bool Intersects(const BoundingBox&) const override;
	virtual bool Intersects(const BoundingCircle&) const override;
	virtual bool Intersects(const BoundingPolygon&) const override;
	virtual bool In(unsigned, unsigned) const;
	virtual bool Intersects(const BoundingArea&) const;
	virtual BoundingArea* Clone(void) const;
	virtual void move(int& dx, int& dy);

	unsigned getX1() const;
	unsigned getX2() const;
	unsigned getY1() const;
	unsigned getY2() const;
	unsigned getMaxDiagonal(void) const;
	void getCenter(unsigned&, unsigned&) const;
	unsigned getWidth() const;
	unsigned getHeight() const;

	bool onTop(const BoundingBox&) const;
};

class BoundingCircle : public BoundingArea {
private:
	unsigned x, y, r;
public:
	BoundingCircle(unsigned, unsigned, unsigned);

	virtual bool Intersects(const BoundingBox&) const override;
	virtual bool Intersects(const BoundingCircle&) const override;
	virtual bool Intersects(const BoundingPolygon&) const override;
	virtual bool In(unsigned, unsigned) const;
	virtual bool Intersects(const BoundingArea&) const;
	virtual BoundingArea* Clone(void) const;
	virtual void move(int& dx, int& dy);

	unsigned getX() const;
	unsigned getY() const;
	unsigned getR() const;
};

class BoundingPolygon : public BoundingArea {
public:
	struct Point {
		unsigned x, y;
		Point(void) : x(0), y(0) {}
		Point(unsigned _x, unsigned _y) : x(_x), y(_y) {}
		Point(const Point& other) : x(other.x), y(other.y) {}
	};
	typedef std::list<Point> Polygon;
protected:
	Polygon points;
public:
	BoundingPolygon(const Polygon&);

	virtual bool Intersects(const BoundingBox&) const override;
	virtual bool Intersects(const BoundingCircle&) const override;
	virtual bool Intersects(const BoundingPolygon&) const override;
	virtual bool In(unsigned, unsigned) const;
	virtual bool Intersects(const BoundingArea&) const;
	virtual BoundingArea* Clone(void) const;
	virtual void move(int& dx, int& dy);

	Polygon getPoints() const;
};