#include <avr/io.h>
#include <util/delay.h>

#define PWM_PIN     PD5   // PWM output pin (OC1A)
#define PWM_FREQ    100   // PWM frequency in Hz

#define F_CPU 8000000UL   // Assuming an 8 MHz clock frequency

#define KEYPAD_PORT PORTA
#define KEYPAD_DDR  DDRA
#define KEYPAD_PIN  PINA

#define BUTTON_PIN  PD2   // ON/OFF button pin

// Keypad layout
const unsigned char KEYS[4][4] = {
	{'7', '8', '9', 'A'},
	{'4', '5', '6', 'B'},
	{'1', '2', '3', 'C'},
	{'*', '0', '#', 'D'}
};

void PWM_Init() {
	// Set PD5 (OC1A) as output pin
	DDRD |= (1 << PWM_PIN);
	
	// Set fast PWM mode with non-inverted output
	TCCR1A = (1 << COM1A1) | (1 << WGM11);
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
	// Set PWM frequency
	ICR1 = F_CPU / (2 * PWM_FREQ);
}

void PWM_SetDutyCycle(uint8_t dutyCycle) {
	// Calculate the OCR1A value based on the duty cycle
	uint16_t ocrValue = (dutyCycle / 100.0) * ICR1;

	// Set the new duty cycle value
	OCR1A = ocrValue;
}

void runMotorSpeed(int temp) {
	if (temp < 25) {
		// Run the motor slowly
		// Set PC0-PC3 to output the value 2 (binary: 0010)
		PORTC = (PORTC & 0xF0) | 0x02;
		// Set PC4-PC7 to output 0 (binary: 0000)
		PORTC &= 0x0F;
		PWM_SetDutyCycle(20);    // 10% duty cycle
		} else if (temp >= 25 && temp <= 40) {
		// Increase motor speed
		// Set PC0-PC3 to output the value 5 (binary: 0101)
		PORTC = (PORTC & 0xF0) | 0x05;
		// Set PC4-PC7 to output 0 (binary: 0000)
		PORTC &= 0x0F;

		PWM_SetDutyCycle(50);    // 50% duty cycle
		} else {
		// Run the motor full speed
		// Set PC0-PC3 to output the value 9 (binary: 1001)
		PORTC = (PORTC & 0xF0) | 0x09;

		// Set PC4-PC7 to output the value 9 (binary: 1001)
		PORTC = (PORTC & 0x0F) | 0x90;
		PWM_SetDutyCycle(100);    // 100% duty cycle
	}
}

// Function to initialize the keypad
void keypad_init() {
	// Set the keypad port as input
	KEYPAD_DDR = 0x0F;
	
	// Enable pull-up resistors
	KEYPAD_PORT = 0xFF;
}

// Function to read the keypad
char keypad_read() {
	// Loop through each column
	for (uint8_t col = 0; col < 4; col++) {
		// Set current column as output low
		KEYPAD_DDR = (0b00010000 << col);
		KEYPAD_PORT &= ~(0b00010000 << col);  // Set current column as input with pull-up
		
		// Delay for a short period for stabilization
		_delay_ms(1);
		
		// Loop through each row
		for (uint8_t row = 0; row < 4; row++) {
			// Check if the key in the current row and column is pressed
			if (!(KEYPAD_PIN & (0b00000001 << row))) {
				// Return the corresponding character from the KEYS array
				return KEYS[row][col];
			}
		}
		
		// Set current column as input high (disable pull-up)
		KEYPAD_PORT |= (0b00010000 << col);
	}
	
	// Return null character if no key is pressed
	return '\0';
}

// Function to check if a character is a digit
int is_digit(char c) {
	return (c >= '0' && c <= '9');
}

// Function to convert a digit character to its integer value
int char_to_int(char c) {
	return c - '0';
}
//disables the motor and the displays
void cleanUp(){
	// Disable everything and start from the beginning
	_delay_ms(1000);   // Delay for button debounce
	PORTB = 0x00;
	PWM_SetDutyCycle(0);  // Reset motor speed
	PORTC=0x00;
}
// Function to read the keypad and return a single-digit input
char get_single_digit() {
	while (1) {
		// Check the state of the ON/OFF button
		if ((PIND & (1 << BUTTON_PIN)) == 0) {
			// Button is pressed (OFF condition)
			cleanUp();  // Perform clean up and restart the code
		}

		// Read the keypad
		char key = keypad_read();

		// Check if a key is pressed and if it is a digit
		if (key != '\0' && is_digit(key)) {
			// Return the single-digit key input
			return key;
		}
	}
}

int main(void) {
	// Initialize the keypad
	keypad_init();
	// Set PORTB as key inputs
	DDRB = 0xFF;
	//set PORTC as output for duty cycle
	DDRC = 0xFF;
	// Initialize PWM
	PWM_Init();

	while (1) {
		// Get the first single-digit input
		char firstDigit = get_single_digit();
		

		// Wait for the second single-digit input
		char secondDigit;
		// Display the lower 4 bits of the input1 on PORTA
		PORTB = (PORTB & 0xF0) | (char_to_int(firstDigit) & 0x0F);

		_delay_ms(500);
		while (1) {
			// Check the state of the ON/OFF button
			if ((PIND & (1 << BUTTON_PIN)) == 0) {
				// Button is pressed (OFF condition)
				cleanUp();  // Perform clean up and restart the code
			}

			secondDigit = get_single_digit();
			if (secondDigit != '\0') {
				break;
			}
		}
		
		// Display the higher 4 bits of the variable on PORTA
		PORTB = (PORTB & 0x0F) | ((char_to_int(secondDigit) & 0x0F) << 4);
		
		_delay_ms(500);

		// Create the double-digit number
		int doubleDigit = char_to_int(firstDigit) * 10 + char_to_int(secondDigit);


		// Run the motor
		runMotorSpeed(doubleDigit);
	}

	return 0;
}
