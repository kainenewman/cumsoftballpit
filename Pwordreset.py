## Python ##

string_input = input()

data = input()

raw_password = " "

while not data == "Done":
    command = data.split()[0]

    if command == "TakeOdd":
        for index in range(len(string_input)):
            if not index % 2 == 0:
                raw_password += string_input[index]
        print(f"{raw_password}")

    elif command == "Cut":
        index = int(data.split()[1])
        length = int(data.split()[2])
        left_side = raw_password[:index]
        right_side = raw_password[index + length:]
        raw_password = left_side + right_side

        print(f"{raw_password}")


    elif command == "Substitute":
        pass

    data = input()
