
#include <stdio.h>   // Input, output, and files
#include <time.h>    // Records the time of events

// GPIO code only runs on Raspberry Pi
#ifdef USE_GPIO
#include <gpiod.h>   // Raspberry Pi GPIO library

#define LED_PIN 17   // LED connected to GPIO 17

// Stores the connection to our LED
struct gpiod_line_request *led;

// Set up the LED as an output
int setupLED(void) {

    // Open the Raspberry Pi GPIO controller
    struct gpiod_chip *chip =
        gpiod_chip_open("/dev/gpiochip0");

    if (chip == NULL) {
        perror("Cannot open GPIO chip");
        return 0;
    }

    // Create settings for the GPIO pin
    struct gpiod_line_settings *settings =
        gpiod_line_settings_new();

    struct gpiod_line_config *config =
        gpiod_line_config_new();

    unsigned int pin = LED_PIN;

    // Make GPIO 17 an output, starting OFF
    int ok = (settings != NULL && config != NULL);

    if (ok)
        ok = (gpiod_line_settings_set_direction(
            settings, GPIOD_LINE_DIRECTION_OUTPUT) == 0);

    if (ok)
        ok = (gpiod_line_settings_set_output_value(
            settings, GPIOD_LINE_VALUE_INACTIVE) == 0);

    if (ok)
        ok = (gpiod_line_config_add_line_settings(
            config, &pin, 1, settings) == 0);

    // Request control of the LED pin
    if (ok)
        led = gpiod_chip_request_lines(chip, NULL, config);

    // Clean up temporary settings
    if (config)
        gpiod_line_config_free(config);

    if (settings)
        gpiod_line_settings_free(settings);

    gpiod_chip_close(chip);

    // Check whether GPIO setup worked
    if (led == NULL) {
        perror("Cannot set up LED");
        return 0;
    }

    return 1;
}
#endif

// Display room status and control the LED
void showStatus(int students, int capacity) {

    // Check whether the room is full
    int full = (students >= capacity);

    printf("\nStudents: %d/%d\n", students, capacity);

    if (full) {
        printf("Status: ROOM FULL\n");
        printf("LED: ON\n");
    } else {
        printf("Status: SPACE AVAILABLE\n");
        printf("LED: OFF\n");
    }

#ifdef USE_GPIO
    // Turn the physical LED ON or OFF
    if (gpiod_line_request_set_value(
            led, LED_PIN,
            full ? GPIOD_LINE_VALUE_ACTIVE
                 : GPIOD_LINE_VALUE_INACTIVE) < 0) {

        perror("Could not update LED");
    }
#endif
}

// Save successful events in a CSV file
void saveEvent(const char *event,
               int students, int capacity) {

    // Open the file in append mode
    FILE *file = fopen("data/occupancy_events.csv", "a");

    if (file == NULL) {
        perror("Could not open log file");
        return;
    }

    // Get the current time
    time_t now = time(NULL);

    // Save time, event, occupancy, and capacity
    fprintf(file, "%ld,%s,%d,%d\n",
            (long)now, event, students, capacity);

    fclose(file); // Close the file
}

// Main program
int main(void) {

    // Variables for tracking the room
    int students = 0;
    int capacity = 5;
    int choice;
    int newCapacity;

#ifdef USE_GPIO
    // Start the Raspberry Pi LED
    if (!setupLED())
        return 1;
#else
    // Display this message when testing on Mac
    printf("Mac simulation mode.\n");
#endif

    // Keep the program running
    while (1) {

        printf("\n===== SMART STUDY ROOM =====\n");

        // Display the current room status
        showStatus(students, capacity);

        // Display menu options
        printf("\n1. Student enters\n");
        printf("2. Student exits\n");
        printf("3. Display occupancy\n");
        printf("4. Change capacity\n");
        printf("5. Quit\n");

        printf("Choice: ");

        // Get user's choice
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Exiting.\n");
            break;
        }

        // Option 1: Student enters
        if (choice == 1) {

            // Allow entry if space is available
            if (students < capacity) {
                students++;
                printf("Student entered.\n");

                // Save the entry
                saveEvent("ENTER", students, capacity);

            } else {
                printf("Entry denied: room is full.\n");
            }
        }

        // Option 2: Student exits
        else if (choice == 2) {

            // Prevent negative occupancy
            if (students > 0) {
                students--;
                printf("Student exited.\n");

                // Save the exit
                saveEvent("EXIT", students, capacity);

            } else {
                printf("Exit denied: room is empty.\n");
            }
        }

        // Option 3: Display current occupancy
        else if (choice == 3) {
            showStatus(students, capacity);
        }

        // Option 4: Change maximum capacity
        else if (choice == 4) {

            printf("New capacity: ");

            if (scanf("%d", &newCapacity) != 1) {
                printf("Invalid input. Exiting.\n");
                break;
            }

            // Make sure the new capacity is valid
            if (newCapacity > 0 &&
                newCapacity >= students) {

                capacity = newCapacity;

                printf("Capacity changed to %d.\n",
                       capacity);

                // Save the capacity change
                saveEvent("CAPACITY_CHANGED",
                          students, capacity);

            } else {
                printf("Invalid capacity.\n");
            }
        }

        // Option 5: Quit the program
        else if (choice == 5) {
            printf("Goodbye!\n");
            break;
        }

        // Handle invalid menu options
        else {
            printf("Choose a number from 1 to 5.\n");
        }
    }

#ifdef USE_GPIO
    // Turn off LED when quitting
    gpiod_line_request_set_value(
        led, LED_PIN, GPIOD_LINE_VALUE_INACTIVE);

    // Release control of the GPIO pin
    gpiod_line_request_release(led);
#endif

    return 0; // End program
}

