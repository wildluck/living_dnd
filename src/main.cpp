#include <format>
#include <print>

/* ---------------- Formatter for C++ classes ---------------- */
struct Vector2D {
    double x = 0.0;
    double y = 0.0;
};

template<>
struct std::formatter<Vector2D> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const Vector2D& p, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};
/* ---------------- End formatter ---------------- */

int main() {
    Vector2D a{2, 5};
    std::println("The point is: {}", a);
    return 0;
}
