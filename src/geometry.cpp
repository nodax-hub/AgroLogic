// src/geometry.cpp
#include <mylib/geometry.h>

#include <cmath>
#include <utility>

namespace mylib {

double dist(const Point& a, const Point& b)
{
    return std::hypot(b.x - a.x, b.y - a.y);
}

double dot(double ax, double ay, double bx, double by)
{
    return ax * bx + ay * by;
}

std::vector<double> polyline_lengths(const std::vector<Point>& pts)
{
    std::vector<double> s;
    s.reserve(pts.empty() ? 1 : pts.size());
    s.push_back(0.0);
    for (std::size_t i = 1; i < pts.size(); ++i) {
        s.push_back(s.back() + dist(pts[i - 1], pts[i]));
    }
    return s;
}

Polygon::Polygon(std::vector<Point> vertices)
    : vertices_(std::move(vertices))
{
    if (vertices_.size() >= 2 && vertices_.front() == vertices_.back()) {
        vertices_.pop_back(); // убираем дублирующую последнюю вершину
    }
}

double Polygon::area() const noexcept
{
    const std::size_t n = vertices_.size();
    if (n < 3) return 0.0;

    long double s = 0.0L; // уменьшение накопления погрешности
    for (std::size_t i = 0; i < n; ++i) {
        const Point& p1 = vertices_[i];
        const Point& p2 = vertices_[(i + 1) % n];
        s += static_cast<long double>(p1.x) * p2.y
           - static_cast<long double>(p2.x) * p1.y;
    }
    return std::abs(static_cast<double>(s * 0.5L));
}

} // namespace mylib
