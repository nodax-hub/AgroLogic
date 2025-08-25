// src/geometry.cpp
#include <mylib/geometry.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(MYLIB_WITH_PROJ)
#include <proj.h>
#endif

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
// ===================================================GEO2XY=========================================================

// ===== Equirectangular (сферическое приближение) =====
Point GeoToXYEquirectangular::geo_to_xy(const GeoPoint& center, const GeoPoint& geo_point) const
{
    const double lon0 = deg2rad(center.lon);
    const double lat0 = deg2rad(center.lat);
    const double lon  = deg2rad(geo_point.lon);
    const double lat  = deg2rad(geo_point.lat);

    const double x = R * (lon - lon0) * std::cos(lat0);
    const double y = R * (lat - lat0);
    return Point{x, y};
}

// ===== AEQD через PROJ (если доступно) =====
Point GeoToXYAeqd::geo_to_xy(const GeoPoint& center, const GeoPoint& geo_point) const
{
#ifndef MYLIB_WITH_PROJ
    (void)center;
    (void)geo_point;
    throw std::runtime_error("GeoToXYAeqd: требуется сборка с PROJ (определите MYLIB_WITH_PROJ и линкуйте libproj)");
#else
    // Минимальная обёртка над PROJ C-API
    // Требуются заголовки proj.h и линковка с -lproj
    PJ_CONTEXT* C = proj_context_create();
    if (!C) throw std::runtime_error("GeoToXYAeqd: proj_context_create() == nullptr");

    const std::string proj4 =
        std::string("+proj=aeqd +lat_0=") + std::to_string(center.lat) +
        " +lon_0=" + std::to_string(center.lon) +
        " +x_0=0 +y_0=0 +datum=WGS84 +units=m +no_defs";

    PJ* src = proj_create(C, "+proj=longlat +datum=WGS84 +no_defs");
    PJ* dst = proj_create(C, proj4.c_str());
    if (!src || !dst) {
        if (src) proj_destroy(src);
        if (dst) proj_destroy(dst);
        proj_context_destroy(C);
        throw std::runtime_error("GeoToXYAeqd: proj_create() failed");
    }

    PJ* P = proj_create_crs_to_crs_from_pj(C, src, dst, nullptr, nullptr);
    if (!P) {
        proj_destroy(src);
        proj_destroy(dst);
        proj_context_destroy(C);
        throw std::runtime_error("GeoToXYAeqd: proj_create_crs_to_crs_from_pj() failed");
    }

    // always_xy=true аналог: используем proj_normalize_for_visualization
    PJ* Pxy = proj_normalize_for_visualization(C, P);
    if (!Pxy) {
        proj_destroy(P);
        proj_destroy(src);
        proj_destroy(dst);
        proj_context_destroy(C);
        throw std::runtime_error("GeoToXYAeqd: proj_normalize_for_visualization() failed");
    }

    PJ_COORD in;
    in.lpzt.lam = deg2rad(geo_point.lon);
    in.lpzt.phi = deg2rad(geo_point.lat);
    in.lpzt.z = 0.0;
    in.lpzt.t = 0.0;

    PJ_COORD out = proj_trans(Pxy, PJ_FWD, in);

    proj_destroy(Pxy);
    proj_destroy(P);
    proj_destroy(src);
    proj_destroy(dst);
    proj_context_destroy(C);

    return Point{static_cast<double>(out.xy.x), static_cast<double>(out.xy.y)};
#endif
}

// ===== UTM через PROJ (если доступно) =====
int GeoToXYUtm::utm_zone_from_lon(double lon_deg)
{
    return static_cast<int>(std::floor((lon_deg + 180.0) / 6.0)) + 1;
}

Point GeoToXYUtm::geo_to_xy(const GeoPoint& center, const GeoPoint& geo_point) const
{
#ifndef MYLIB_WITH_PROJ
    (void)center;
    (void)geo_point;
    throw std::runtime_error("GeoToXYUtm: требуется сборка с PROJ (определите MYLIB_WITH_PROJ и линкуйте libproj)");
#else
    PJ_CONTEXT* C = proj_context_create();
    if (!C) throw std::runtime_error("GeoToXYUtm: proj_context_create() == nullptr");

    const int zone = utm_zone_from_lon(center.lon);
    const bool north = (center.lat >= 0.0);
    const int epsg = (north ? 32600 : 32700) + zone;

    // Источник: WGS84 lon/lat
    PJ* src = proj_create(C, "+proj=longlat +datum=WGS84 +no_defs");

    // Назначение: UTM по EPSG
    // std::string dst_str = std::string("+init=EPSG:") + std::to_string(epsg);
    // // Начиная с PROJ 6+ рекомендуется использовать "+proj=utm ..." — но init тоже поддерживается.
    // PJ* dst = proj_create(C, dst_str.c_str());

    // вместо "+init=EPSG:326xx/327xx"
    std::string dst_proj4 = std::string("+proj=utm +zone=") + std::to_string(zone)
                          + (center.lat >= 0.0 ? "" : " +south")
                          + " +datum=WGS84 +units=m +no_defs";
    PJ* dst = proj_create(C, dst_proj4.c_str());
    if (!src || !dst) {
        if (src) proj_destroy(src);
        if (dst) proj_destroy(dst);
        proj_context_destroy(C);
        throw std::runtime_error("GeoToXYUtm: proj_create() failed");
    }

    PJ* P = proj_create_crs_to_crs_from_pj(C, src, dst, nullptr, nullptr);
    if (!P) {
        proj_destroy(src);
        proj_destroy(dst);
        proj_context_destroy(C);
        throw std::runtime_error("GeoToXYUtm: proj_create_crs_to_crs_from_pj() failed");
    }

    PJ* Pxy = proj_normalize_for_visualization(C, P);
    if (!Pxy) {
        proj_destroy(P);
        proj_destroy(src);
        proj_destroy(dst);
        proj_context_destroy(C);
        throw std::runtime_error("GeoToXYUtm: proj_normalize_for_visualization() failed");
    }

    PJ_COORD in;
    in.lpzt.lam = deg2rad(geo_point.lon);
    in.lpzt.phi = deg2rad(geo_point.lat);
    in.lpzt.z = 0.0;
    in.lpzt.t = 0.0;

    PJ_COORD out = proj_trans(Pxy, PJ_FWD, in);

    proj_destroy(Pxy);
    proj_destroy(P);
    proj_destroy(src);
    proj_destroy(dst);
    proj_context_destroy(C);

    return Point{static_cast<double>(out.xy.x), static_cast<double>(out.xy.y)};
#endif
}

} // namespace mylib
