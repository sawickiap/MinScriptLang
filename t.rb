
#define print(...) __VA_ARGS__,

[
print(123)
print(-00444)
print(+0xaF250)
print(-0xFF)
print(0xAA00FF5544CD)
print(01.00)
print(10.5)
print(23.)
print(.25)
print(1e3)
print(1e+2)
print(.001e-1)
print(3.E+0)
print(2 + 6 - 3)
print(2 + 3 * 4)
print(2 - 10 / 2 + 7 % 3)
print(60 / 3 * 2)
print(2 + 3 * ((4)))
print((2 + 3) * 4)
]

