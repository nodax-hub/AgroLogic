// include/mylib/geometry.h
#pragma once

#include <mylib/export.h>

#include <vector>
#include <optional>

namespace mylib {

struct MYLIB_EXPORT Point {
    double x{0.0};
    double y{0.0};
    constexpr Point() = default;
    constexpr Point(double x_, double y_) : x(x_), y(y_) {}
    bool operator==(const Point& other) const noexcept { return x == other.x && y == other.y; }
};
;

struct MYLIB_EXPORT BoundPoints {
    Point start;
    Point end;
};

struct MYLIB_EXPORT GeoPoint {
    double lat{0.0};
    double lon{0.0};
    std::optional<double> alt; // None -> std::nullopt
};

// dist(a, b) = hypot(b.x - a.x, b.y - a.y)
MYLIB_EXPORT double dist(const Point& a, const Point& b);

MYLIB_EXPORT double dot(double ax, double ay, double bx, double by);

/// Накопленная длина вдоль ломаной: s[i] — расстояние от начала до pts[i].
MYLIB_EXPORT std::vector<double> polyline_lengths(const std::vector<Point>& pts);

class MYLIB_EXPORT Polygon {
public:
    explicit Polygon(std::vector<Point> vertices);

    const std::vector<Point>& vertices() const noexcept { return vertices_; }

    double area() const noexcept;

private:
    std::vector<Point> vertices_;
};

} // namespace mylib
