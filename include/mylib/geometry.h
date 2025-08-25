// include/mylib/geometry.h
#pragma once

#include <mylib/export.h>

#include <optional>
#include <vector>

namespace mylib {

constexpr double kPI = 3.141592653589793238462643383279502884;
constexpr double kEarthRadius = 6378137.0; // м, WGS84 экваториальный

inline double deg2rad(double deg) noexcept
{
    return deg * (kPI / 180.0);
}

inline double rad2deg(double rad) noexcept
{
    return rad * (180.0 / kPI);
}

struct MYLIB_EXPORT Point {
    double x{0.0};
    double y{0.0};
    constexpr Point() = default;
    constexpr Point(double x_, double y_): x(x_), y(y_) { }
    bool operator==(const Point& other) const noexcept { return x == other.x && y == other.y; }
};

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

MYLIB_EXPORT Point point_on_path(const std::vector<Point>& pts, double distance);

class MYLIB_EXPORT Polygon {
public:
    explicit Polygon(std::vector<Point> vertices);

    [[nodiscard]] const std::vector<Point>& vertices() const noexcept { return vertices_; }

    [[nodiscard]] double area() const noexcept;

private:
    std::vector<Point> vertices_;
};

// ==== Общий интерфейс ====
class MYLIB_EXPORT IGeoPointToXY {
public:
    virtual ~IGeoPointToXY() = default;

    // Преобразование: (центр-проекции, геоточка) -> метрические XY
    [[nodiscard]] virtual Point geo_to_xy(const GeoPoint& center, const GeoPoint& geo_point) const = 0;
};

// ==== Реализации ====

class MYLIB_EXPORT GeoToXYEquirectangular final: public IGeoPointToXY {
public:
    // Радиус сферы (WGS84 экваториальный радиус, как в Python)
    static constexpr double R = 6378137.0;
    [[nodiscard]] Point geo_to_xy(const GeoPoint& center, const GeoPoint& geo_point) const override;
};

class MYLIB_EXPORT GeoToXYAeqd final: public IGeoPointToXY {
public:
    // Требует PROJ при MYLIB_WITH_PROJ, иначе бросает исключение.
    [[nodiscard]] Point geo_to_xy(const GeoPoint& center, const GeoPoint& geo_point) const override;
};

class MYLIB_EXPORT GeoToXYUtm final: public IGeoPointToXY {
public:
    // Вспомогательное: номер UTM-зоны по долготе
    static int utm_zone_from_lon(double lon_deg);
    // Требует PROJ при MYLIB_WITH_PROJ, иначе бросает исключение.
    [[nodiscard]] Point geo_to_xy(const GeoPoint& center, const GeoPoint& geo_point) const override;
};

} // namespace mylib
