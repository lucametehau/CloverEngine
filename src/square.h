#pragma once
#include <cstdint>

class Square
{
  private:
    uint8_t sq;

  public:
    constexpr Square() = default;
    constexpr Square(uint8_t sq) : sq(sq)
    {
    }
    constexpr Square(uint8_t rank, uint8_t file) : sq(rank * 8 + file)
    {
    }

    constexpr operator int() const
    {
        return sq;
    }

    constexpr Square mirror(const bool side) const
    {
        return sq ^ (56 * !side);
    };

    Square operator+(const Square &other) const
    {
        return sq + other.sq;
    }
    Square operator-(const Square &other) const
    {
        return sq - other.sq;
    }
    Square operator+(const int &other) const
    {
        return sq + other;
    }
    Square operator-(const int &other) const
    {
        return sq - other;
    }

    Square &operator+=(const Square &other)
    {
        sq += other.sq;
        return *this;
    }
    Square &operator-=(const Square &other)
    {
        sq -= other.sq;
        return *this;
    }
    Square &operator++()
    {
        sq++;
        return *this;
    }
    Square operator++(int)
    {
        Square temp = *this;
        sq++;
        return temp;
    }
};

namespace Squares
{
constexpr Square A1 = Square(0);
constexpr Square B1 = Square(1);
constexpr Square C1 = Square(2);
constexpr Square D1 = Square(3);
constexpr Square E1 = Square(4);
constexpr Square F1 = Square(5);
constexpr Square G1 = Square(6);
constexpr Square H1 = Square(7);
constexpr Square A2 = Square(8);
constexpr Square B2 = Square(9);
constexpr Square C2 = Square(10);
constexpr Square D2 = Square(11);
constexpr Square E2 = Square(12);
constexpr Square F2 = Square(13);
constexpr Square G2 = Square(14);
constexpr Square H2 = Square(15);
constexpr Square A3 = Square(16);
constexpr Square B3 = Square(17);
constexpr Square C3 = Square(18);
constexpr Square D3 = Square(19);
constexpr Square E3 = Square(20);
constexpr Square F3 = Square(21);
constexpr Square G3 = Square(22);
constexpr Square H3 = Square(23);
constexpr Square A4 = Square(24);
constexpr Square B4 = Square(25);
constexpr Square C4 = Square(26);
constexpr Square D4 = Square(27);
constexpr Square E4 = Square(28);
constexpr Square F4 = Square(29);
constexpr Square G4 = Square(30);
constexpr Square H4 = Square(31);
constexpr Square A5 = Square(32);
constexpr Square B5 = Square(33);
constexpr Square C5 = Square(34);
constexpr Square D5 = Square(35);
constexpr Square E5 = Square(36);
constexpr Square F5 = Square(37);
constexpr Square G5 = Square(38);
constexpr Square H5 = Square(39);
constexpr Square A6 = Square(40);
constexpr Square B6 = Square(41);
constexpr Square C6 = Square(42);
constexpr Square D6 = Square(43);
constexpr Square E6 = Square(44);
constexpr Square F6 = Square(45);
constexpr Square G6 = Square(46);
constexpr Square H6 = Square(47);
constexpr Square A7 = Square(48);
constexpr Square B7 = Square(49);
constexpr Square C7 = Square(50);
constexpr Square D7 = Square(51);
constexpr Square E7 = Square(52);
constexpr Square F7 = Square(53);
constexpr Square G7 = Square(54);
constexpr Square H7 = Square(55);
constexpr Square A8 = Square(56);
constexpr Square B8 = Square(57);
constexpr Square C8 = Square(58);
constexpr Square D8 = Square(59);
constexpr Square E8 = Square(60);
constexpr Square F8 = Square(61);
constexpr Square G8 = Square(62);
constexpr Square H8 = Square(63);
}; // namespace Squares

constexpr Square NO_SQUARE = Square(64);