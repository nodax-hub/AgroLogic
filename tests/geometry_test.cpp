// tests/geometry_test.cpp
#include <mylib/geometry.h>

#include <gtest/gtest.h>
#include <optional>
#include <vector>

constexpr double kEps = 1e-12;

using mylib::Point;
using mylib::point_on_path;

// Вспомогательная заглушка polyline_lengths может быть уже реализована у вас.
// Тесты предполагают корректную работу polyline_lengths.

TEST(point_on_path_test, zero_distance_returns_first_point)
{
    std::vector<Point> pts{{0, 0}, {1, 0}, {2, 0}};
    auto p = point_on_path(pts, 0.0);
    EXPECT_EQ(p, Point(0, 0));
}

TEST(point_on_path_test, end_distance_returns_last_point)
{
    std::vector<Point> pts{{0, 0}, {1, 0}, {2, 0}};
    // Длина = 2
    auto p = point_on_path(pts, 2.0);
    EXPECT_EQ(p, Point(2, 0));
}

TEST(point_on_path_test, middle_interpolation_on_first_segment)
{
    std::vector<Point> pts{{0, 0}, {2, 0}, {2, 2}};
    // Длины: [0,2,4]
    auto p = point_on_path(pts, 1.0);
    EXPECT_EQ(p, Point(1.0, 0.0));
}

TEST(point_on_path_test, middle_interpolation_on_second_segment)
{
    std::vector<Point> pts{{0, 0}, {2, 0}, {2, 2}};
    // На расстоянии 3.0: вторая половина ломаной, t = (3-2)/(4-2)=0.5
    auto p = point_on_path(pts, 3.0);
    EXPECT_EQ(p, Point(2.0, 1.0));
}

TEST(point_on_path_test, zero_length_segments_are_skipped)
{
    std::vector<Point> pts{{0, 0}, {0, 0}, {2, 0}};
    // Первая "ступень" нулевая, общая длина = 2
    auto p = point_on_path(pts, 1.0);
    EXPECT_EQ(p, Point(1.0, 0.0));
}

TEST(point_on_path_test, negative_distance_throws)
{
    std::vector<Point> pts{{0, 0}, {1, 0}};
    EXPECT_THROW(point_on_path(pts, -0.1), std::invalid_argument);
}

TEST(point_on_path_test, distance_greater_than_length_throws)
{
    std::vector<Point> pts{{0, 0}, {1, 0}};
    EXPECT_THROW(point_on_path(pts, 2.0), std::invalid_argument);
}

TEST(point_on_path_test, empty_points_throws)
{
    std::vector<Point> pts;
    EXPECT_THROW(point_on_path(pts, 0.0), std::invalid_argument);
}

TEST(point_test, equality_exact)
{
    mylib::Point a{1.0, 2.0};
    mylib::Point b{1.0, 2.0};
    mylib::Point c{1.0, 2.0000000001}; // отличается
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

TEST(boundpoints_test, fields_are_accessible)
{
    mylib::BoundPoints bp{{0.0, 1.0}, {2.0, 3.0}};
    EXPECT_DOUBLE_EQ(bp.start.x, 0.0);
    EXPECT_DOUBLE_EQ(bp.start.y, 1.0);
    EXPECT_DOUBLE_EQ(bp.end.x, 2.0);
    EXPECT_DOUBLE_EQ(bp.end.y, 3.0);
}

TEST(geopoint_test, optional_altitude)
{
    mylib::GeoPoint g1{52.0, 5.0, std::nullopt};
    mylib::GeoPoint g2{52.0, 5.0, 12.3};
    EXPECT_FALSE(g1.alt.has_value());
    ASSERT_TRUE(g2.alt.has_value());
    EXPECT_DOUBLE_EQ(*g2.alt, 12.3);
}

TEST(dist_test, zero_and_simple)
{
    mylib::Point p{0.0, 0.0};
    EXPECT_DOUBLE_EQ(mylib::dist(p, p), 0.0);

    mylib::Point a{0.0, 0.0}, b{3.0, 4.0};
    EXPECT_DOUBLE_EQ(mylib::dist(a, b), 5.0);
}

TEST(dot_test, basic_cases)
{
    EXPECT_DOUBLE_EQ(mylib::dot(1.0, 0.0, 0.0, 1.0), 0.0); // ортогонально
    EXPECT_DOUBLE_EQ(mylib::dot(1.0, 2.0, 3.0, 4.0), 11.0);
    EXPECT_DOUBLE_EQ(mylib::dot(-1.0, 2.0, -3.0, 4.0), 11.0);
}

TEST(polyline_lengths_test, empty_and_single_point)
{
    // Пустая/одна точка: как в реализации — всегда начинается с 0.0
    std::vector<mylib::Point> empty;
    auto s1 = mylib::polyline_lengths(empty);
    ASSERT_EQ(s1.size(), 1u);
    EXPECT_DOUBLE_EQ(s1[0], 0.0);

    std::vector<mylib::Point> one{{1.0, 2.0}};
    auto s2 = mylib::polyline_lengths(one);
    ASSERT_EQ(s2.size(), 1u);
    EXPECT_DOUBLE_EQ(s2[0], 0.0);
}

TEST(polyline_lengths_test, cumulative_lengths)
{
    std::vector<mylib::Point> pts{
        {0.0, 0.0},
        {3.0, 4.0}, // +5
        {3.0, 0.0}  // +4
    };
    auto s = mylib::polyline_lengths(pts);
    ASSERT_EQ(s.size(), 3u);
    EXPECT_DOUBLE_EQ(s[0], 0.0);
    EXPECT_DOUBLE_EQ(s[1], 5.0);
    EXPECT_DOUBLE_EQ(s[2], 9.0);
}

TEST(polygon_test, removes_duplicate_last_vertex)
{
    std::vector<mylib::Point> verts{
        {0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {0.0, 0.0} // повтор первой
    };
    mylib::Polygon poly(verts);
    // Должен убрать последний дубль
    ASSERT_EQ(poly.vertices().size(), 3u);

    EXPECT_TRUE(poly.vertices().front() == mylib::Point(0.0, 0.0));
    EXPECT_TRUE(poly.vertices().back() == mylib::Point(0.0, 1.0));
}

TEST(polygon_test, area_triangle_and_square)
{
    // Прямоугольный треугольник с площадью 0.5
    mylib::Polygon tri({{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}});
    EXPECT_NEAR(tri.area(), 0.5, kEps);

    // Квадрат 2x2 с замкнутой последней вершиной
    mylib::Polygon sq({
        {0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}, {0.0, 0.0} // замыкание — должно быть удалено
    });
    EXPECT_NEAR(sq.area(), 4.0, kEps);
}

TEST(polygon_test, area_degenerate_less_than_three_vertices)
{
    mylib::Polygon p0({});
    mylib::Polygon p1({{0.0, 0.0}});
    mylib::Polygon p2({{0.0, 0.0}, {1.0, 1.0}});

    EXPECT_DOUBLE_EQ(p0.area(), 0.0);
    EXPECT_DOUBLE_EQ(p1.area(), 0.0);
    EXPECT_DOUBLE_EQ(p2.area(), 0.0);
}

TEST(polygon_test, area_colinear_points_zero)
{
    // Все точки на одной линии
    const mylib::Polygon colinear({{0.0, 0.0}, {1.0, 1.0}, {2.0, 2.0}, {3.0, 3.0}});
    EXPECT_NEAR(colinear.area(), 0.0, kEps);
}
