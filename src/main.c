#include <stdio.h>
#include <time.h>
#include <pigpio.h>

#define LED_PIN 17

// Keep a simple record whenever someone enters or leaves.
void saveEvent(const char *event, int students)
{
    FILE *file = fopen("data/occupancy_events.csv", "a");

    if (file != NULL) {
        fprintf(file, "%ld,%s,%d\n", (long)time(NULL), event, students);
        fclose(file);
    }
}

int main(void)
{
    int students = 0;
    int roomLimit = 5;
    int choice;
    int newLimit;
    char input[20];

    if (gpioInitialise() < 0) {
        printf("Could not start GPIO.\n");
        return 1;
    }

    gpioSetMode(LED_PIN, PI_OUTPUT);
    gpioWrite(LED_PIN, 0);  // Start with the light off.

    while (1) {
        printf("\nStudents: %d/%d\n", students, roomLimit);
        printf("%s\n", students == roomLimit ? "ROOM FULL" : "SPACE AVAILABLE");
        printf("1. Student enters\n");
        printf("2. Student leaves\n");
        printf("3. Change room capacity\n");
        printf("4. Exit\n");
        printf("Choose: ");

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        if (sscanf(input, "%d", &choice) != 1) {
            printf("Please enter a number.\n");
            continue;
        }

        if (choice == 1) {
            if (students < roomLimit) {
                students++;
                saveEvent("entered", students);
            } else {
                printf("The room is already full.\n");
            }
        } else if (choice == 2) {
            if (students > 0) {
                students--;
                saveEvent("left", students);
            } else {
                printf("The room is already empty.\n");
            }
        } else if (choice == 3) {
            printf("New capacity: ");

            if (fgets(input, sizeof(input), stdin) == NULL)
                break;

            if (sscanf(input, "%d", &newLimit) == 1 &&
                newLimit > 0 && newLimit >= students) {
                roomLimit = newLimit;
                printf("Capacity changed to %d.\n", roomLimit);
            } else {
                printf("Capacity must be positive and at least %d.\n", students);
            }
        } else if (choice == 4) {
            break;
        } else {
            printf("Choose 1, 2, 3, or 4.\n");
        }

        // Light on only when every spot is taken.
        gpioWrite(LED_PIN, students == roomLimit);
    }

    gpioWrite(LED_PIN, 0);
    gpioTerminate();
    printf("Goodbye!\n");
    return 0;
}
