
## Python ##

command = input()
percentage_of_cinema_room_fullness = 0
total_count_of_tickets_for_all_movies = 0
student_tickets = 0
standard_tickets = 0
kids_tickets = 0


while command != "Finish":
    free_seats_in_cinema = int(input())
    percentage_of_cinema_room_fullness = 0
    total_tickets_bought = 0

    for i in range(0, free_seats_in_cinema):
        type_of_ticket = input()
        if type_of_ticket == "End" or type_of_ticket == "Finish":
            break
        if type_of_ticket == "student":
            student_tickets += 1
            total_tickets_bought += 1
        if type_of_ticket == "standard":
            standard_tickets += 1
            total_tickets_bought += 1
        if type_of_ticket == "kid":
            kids_tickets += 1
            total_tickets_bought += 1

        total_count_of_tickets_for_all_movies += 1
        percentage_of_cinema_room_fullness = (total_tickets_bought / free_seats_in_cinema) * 100
    print(f"{command} - {percentage_of_cinema_room_fullness:.2f}% full.")

    command = input()

percentage_student_tickets = (student_tickets / total_count_of_tickets_for_all_movies) * 100
percentage_standard_tickets = (standard_tickets / total_count_of_tickets_for_all_movies) * 100
percentage_kids_tickets = (kids_tickets / total_count_of_tickets_for_all_movies) * 100
print(f"Total tickets: {total_count_of_tickets_for_all_movies}")
print(f"{percentage_student_tickets:.2f}% student tickets.")
print(f"{percentage_standard_tickets:.2f}% standard tickets.")
print(f"{percentage_kids_tickets:.2f}% kids tickets.")
