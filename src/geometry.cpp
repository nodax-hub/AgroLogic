// src/geometry.cpp
#include <mylib/geometry.h>

#include <cmath>
#include <stdexcept>
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

Point point_on_path(const std::vector<Point>& pts, double distance)
{
    if (pts.empty()) {
        throw std::invalid_argument("Список точек пуст");
    }
    if (distance < 0.0) {
        throw std::invalid_argument("Дистанция не может быть отрицательной");
    }

    const std::vector<double> s = polyline_lengths(pts);
    const double length = s.back();

    if (distance > length) {
        throw std::invalid_argument("Дистанция больше длины траектории");
    }

    if (distance == 0.0) {
        return pts.front();
    }
    if (distance == length) {
        return pts.back();
    }

    // Найти индекс правой границы: s[i-1] <= distance < s[i]
    auto it = std::upper_bound(s.begin(), s.end(), distance);
    std::size_t i = static_cast<std::size_t>(it - s.begin());

    // Пролистываем нулевые сегменты: s может иметь одинаковые соседние значения
    while (i < s.size() && s[i] == s[i - 1]) {
        ++i;
    }

    if (i >= pts.size()) {
        // Теоретически недостижимо при проверках выше
        throw std::runtime_error("Не найден валидный сегмент (все оставшиеся сегменты нулевой длины)");
    }

    const Point& p1 = pts[i - 1];
    const Point& p2 = pts[i];
    const double seg_len = s[i] - s[i - 1]; // > 0 после пролистывания
    const double t = (distance - s[i - 1]) / seg_len;

    const double x_ = p1.x + t * (p2.x - p1.x);
    const double y_ = p1.y + t * (p2.y - p1.y);
    return Point{x_, y_};
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
