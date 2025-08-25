// examples/geometry_example.cpp
#include <mylib/geometry.h>

#include <iostream>
#include <vector>

int main(int, char*[])
{
    using namespace mylib;

    // Работа с Point и dist
    Point a{0.0, 0.0};
    Point b{3.0, 4.0};
    std::cout << "dist(a, b) = " << dist(a, b) << std::endl; // 5.0

    // Скалярное произведение
    std::cout << "dot((1,2),(3,4)) = " << dot(1.0, 2.0, 3.0, 4.0) << std::endl; // 11.0

    // Polyline lengths
    std::vector<Point> pts{
        {0.0, 0.0},
        {3.0, 4.0}, // +5
        {3.0, 0.0}  // +4
    };
    auto lengths = polyline_lengths(pts);
    std::cout << "Polyline lengths: ";
    for (double s: lengths) {
        std::cout << s << " ";
    }
    std::cout << std::endl; // 0 5 9

    // Polygon
    Polygon tri({{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}});
    std::cout << "Triangle area = " << tri.area() << std::endl; // 0.5

    Polygon square({
        {0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}, {0.0, 0.0} // замыкание, будет удалено конструктором
    });
    std::cout << "Square area = " << square.area() << std::endl; // 4.0

    // BoundPoints
    BoundPoints bp{a, b};
    std::cout << "BoundPoints start=(" << bp.start.x << "," << bp.start.y << ")"
              << " end=(" << bp.end.x << "," << bp.end.y << ")" << std::endl;

    // GeoPoint
    GeoPoint g1{52.0, 5.0, std::nullopt};
    GeoPoint g2{52.0, 5.0, 100.0};
    std::cout << "GeoPoint g1: lat=" << g1.lat << " lon=" << g1.lon;
    if (g1.alt)
        std::cout << " alt=" << *g1.alt;
    std::cout << std::endl;

    std::cout << "GeoPoint g2: lat=" << g2.lat << " lon=" << g2.lon;
    if (g2.alt)
        std::cout << " alt=" << *g2.alt;
    std::cout << std::endl;


    // -------------------------------------------------- GEO TO XY--------------------------------------------------
    std::cout << "------------------------------------------------------------------------------------------------------" << std::endl;
    GeoToXYEquirectangular eq;
    GeoPoint center{37.618423, 55.751244}; // Москва (lon, lat)
    GeoPoint p{37.620000, 55.752000};

    std::cout << "GeoPoint p: lat=" << p.lat << " lon=" << p.lon;
    std::cout << "GeoPoint center: lat=" << center.lat << " lon=" << center.lon;

    Point xy = eq.geo_to_xy(center, p);
    std::cout  << "EQ Point: (" << xy.x << ", " << xy.y << ")" << std::endl;

    // AEQD/UTM при наличии PROJ:
    try {
        GeoToXYAeqd aeqd;
        auto xy2 = aeqd.geo_to_xy(center, p);
        std::cout  << "AEQD Point: (" << xy2.x << ", " << xy2.y << ")" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "AEQD недоступен: " << e.what() << std::endl;
    }

    try {
        GeoToXYUtm utm;
        auto xy3 = utm.geo_to_xy(center, p);
        std::cout  << "UTM Point: (" << xy3.x << ", " << xy3.y << ")" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "UTM недоступен: " << e.what() << std::endl;
    }

    return 0;
}
