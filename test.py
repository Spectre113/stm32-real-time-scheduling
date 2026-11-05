n = int(input("Введите место: "))

if (n >=1 and n <= 54):
    if (n % 2 == 0):
        print("Верхнее")
    else:
        print("Нижнее")

    if (n >= 1 and n <= 36):
        print("Купе")
    else:
        print("Боковое")
else:
    print("Место неправильное")
    