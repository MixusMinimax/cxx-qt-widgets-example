#pragma once

#include <array>
#include <cstdint>
#include <format>

namespace util
{
    namespace _format
    {
        template<typename T>
        concept _char = std::same_as<T, char> || std::same_as<T, wchar_t>;
    }

    struct uuid
    {
        std::array<std::uint8_t, 16> bytes{};

        constexpr std::uint8_t operator[](const std::size_t i) const noexcept { return bytes[i]; }
        constexpr std::uint8_t &operator[](const std::size_t i) noexcept { return bytes[i]; }
    };

} // namespace util

template<>
struct std::formatter<util::uuid>
{
    constexpr auto parse(std::format_parse_context &pc)
    {
        auto it = pc.begin();
        if (it != pc.end()) {
            switch (*it) {
                case 'X':
                    upper_case = true;
                case 'x':
                    ++it;
                    break;
                default:;
            }
        }
        return it;
    }

    template<typename FmtContext>
    auto format(util::uuid u, FmtContext &ctx) const
    {
        if (upper_case) {
            return std::format_to(
                ctx.out(),
                "{:02X}{:02X}{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}",
                u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], u[8], u[9], u[10], u[11], u[12], u[13], u[14], u[15]
            );
        }
        return std::format_to(
            ctx.out(),
            "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
            u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], u[8], u[9], u[10], u[11], u[12], u[13], u[14], u[15]
        );
    }

    bool upper_case{false};
};
