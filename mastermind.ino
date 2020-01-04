/*! mastermind.ino
    Mastermind - Simple memory game
    \file   mastermind.ino
    \author albrdev (albrdev@gmail.com)
    \date   2019-12-28
*/

#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <stddef.h>

/*! NOP
    Defines 'no operation'.
    \def NOP
*/
#define NOP         ((void)0)

/*! ARRCNT()
    Calculates number of elements on a compile-time (statically allocated) array. This is calculated on compile-time and thus saves a small amount of runtime performance.
    \def ARRCNT()
    \param[in]  x   The array of which the count is to be calculated
    \return         The numbers of elements in the array
*/
#define ARRCNT(x)   (sizeof(x) / sizeof(*(x)))

/*! STRLEN()
    Compile-time version of strlen(). Should only be called on compile-time C-string constants.
    \def STRLEN()
    \param[in]  x   The string of which the length is to be calculated
    \return         The numbers of characters in the array (excluding the null termination character).
*/
#define STRLEN(x)   (ARRCNT(x) - 1)

#define DEBUG   // Uncomment for debug mode

#ifdef DEBUG
/*! DebugWrite()
    Debug mode version of Seria.write(). If 'DEBUG' is defined, this macro will call Serial.write() with the parameters supplied, otherwise no operation will occur.
    \def DebugWrite()
    \param[in]  x   The same as first parameter of Serial.write().
    \param[in]  n   The same as second parameter of Serial.write().
    \return         The same as Seria.write().
*/
#define DebugWrite(x, n)    Serial.write(x, n)

/*! DebugPrint()
    Debug mode version of Seria.print(). If 'DEBUG' is defined, this macro will call Serial.print() with the parameter(s) supplied, otherwise no operation will occur.
    \def DebugPrint()
    \param[in]  x   The same as first parameter(s) of Serial.print().
    \return         The same as Seria.print().
*/
#define DebugPrint(x)       Serial.print(x)

/*! DebugPrintLine()
    Debug mode version of Seria.println(). If 'DEBUG' is defined, this macro will call Serial.println() with the parameter(s) supplied, otherwise no operation will occur.
    \def DebugPrintLine()
    \param[in]  x   The same as first parameter(s) of Serial.println().
    \return         The same as Seria.println().
*/
#define DebugPrintLine(x)   Serial.println(x)
#else
#define DebugWrite(x, n)    NOP
#define DebugPrint(x)       NOP
#define DebugPrintLine(x)   NOP
#endif

#define KEYPAD_ROWCOUNT     4                                   /*!< The number of keypad rows. */
#define KEYPAD_COLCOUNT     4                                   /*!< The number of keypad columns. */
#define BUTTON_NEWROUND     '*'                                 /*!< Character tied to the keypad button resetting round/input. */
#define BUTTON_NEWGAME      '#'                                 /*!< Character tied to the keypad button resetting the game. */
const byte KEYPAD_KEYMAP[KEYPAD_ROWCOUNT][KEYPAD_COLCOUNT] =    /*!< Keypad keymap with characters tied to all the keypad buttons. Last column is not yet i use. */
{
    {'1',               '2',    '3',            'A'},
    {'4',               '5',    '6',            'B'},
    {'7',               '8',    '9',            'C'},
    {BUTTON_NEWROUND,   '0',    BUTTON_NEWGAME, 'D'}
};

byte ROW_PINS[KEYPAD_ROWCOUNT] = { 7, 6, 5, 4 };                                                /*!< Pin numbers mapped to keypad rows. */
byte COL_PINS[KEYPAD_COLCOUNT] = { 3, 2, A4, A5 };                                              /*!< Pin numbers mapped to keypad columns. */
Keypad keypad(makeKeymap(KEYPAD_KEYMAP), ROW_PINS, COL_PINS, KEYPAD_ROWCOUNT, KEYPAD_COLCOUNT); /*!< Keypad instance. */

#define LCD_ROWCOUNT    2                   /*!< The number of LCD rows. */
#define LCD_COLCOUNT    16                  /*!< The number of LCD columns. */
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);    /*!< LCD instance. */

#define LEDPIN_WIN  A2
#define LEDPIN_LOSE A3

#define SECRET_LENGTH   4   /*!< The length of the secret number combination a player should solve. */
#define HINT_WRONG      '-' /*!< The 'wrong' hint character. Displayed on the LCD when a guessed number is wrong. */
#define HINT_MISPLACED  '?' /*!< The 'misplaced' hint character. Displayed on the LCD when a guessed number is correct but in the wrong place. */
#define HINT_CORRECT    '!' /*!< The 'correct' hint character. Displayed on the LCD when a guessed number is correct and in the right place. */

#define TRIES_MAX       12  /*!< The maximum number of tries before player loses. */

#define MESSAGE_WIN     "Winner"    /*!< The text displayed when a player wins a game. */
#define MESSAGE_LOSE    "Loser"     /*!< The text displayed when a player loses a game. */
#define MESSAGE_NEXTTRY "Try again" /*!< The text displayed when a player guessed wrong but still have guesses left. */

typedef enum
{
    GS_INPUT,       /*!< Game awaits input from player. */
    GS_ROUNDOVER,   /*!< Player has made a guess and game awaits a new round to be initialized (button press). */
    GS_GAMEOVER     /*!< Game is over, player may have won or lost. Game awaits a new game to be initialized (button press). */
} gamestate;

struct
{
    gamestate state;            /*!< The current state of the game. */

    char secret[SECRET_LENGTH]; /*!< Buffer containing the secret number. */
    char input[SECRET_LENGTH];  /*!< Buffer containing the current guessed number. */
    char hints[SECRET_LENGTH];  /*!< Buffer containing the hints of current guessed number. */

    size_t inputCount;          /*!< The number of characters currently typed in by the player. This is tied to the struct member 'input'. */
    unsigned int tries;         /*!< The current number of tries the player has made. */
} gameData;                     /*!< Anonymous struct containing game data. */

#define DIGIT_COUNT  10                                                                     /*!< Number of digits the secret number is containing/the player can use to solve the secret combination with. */
char randomDigitPool[DIGIT_COUNT] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };   /*!< Pool with digits to be uniquely randomized. */

/*! numberLength()
    Calculates the number of digits contained in the number (base 10).
    \fn numberLength()
    \param[in]  n   The number which length is to be calculated.
    \return         The length of the number.
*/
inline unsigned int numberLength(const unsigned int n)
{
    return (n > 0 ? (unsigned int)log10((double)n) : 0) + 1;
}

/*! randomNumber()
    Returns a random number from 0 to n - 1.
    \fn randomNumber()
    \param[in]  n   Maximum number to be randomized (exclusive). If this number is 0, then 0 will be returned, preventing a possible division by zero.
    \return         A random number between 0 and n - 1.
*/
inline size_t randomNumber(const size_t n)
{
    return n > 0 ? random(n) : 0;
}

/*! shuffleSort()
    Shuffles an array of characters. A call to 'randomSeed()' should be called beforehand.
    \fn shuffleSort()
    \param[in,out]  arr The array to be shuffled.
    \return             .
*/
void shuffleSort(char* const arr, const size_t n)
{
    size_t i;

    // Iterate backwards, swapping last element of each iteration with a random element of lower index
    for(i = n; i-- > 0;)
    {
        size_t z = randomNumber(i) + 1;
        char tmp = arr[i];
        arr[i] = arr[z];
        arr[z] = tmp;
    }
}

/*! generateSecret()
    Generates a secret number for the player to be solved. Result is stored in 'gameData.secret' buffer.
    \fn generateSecret()
    \param[in]  allowDuplicates Allow duplicate digits or not.
    \return                     .
*/
void generateSecret(const bool allowDuplicates)
{
    // Check if we allow duplicate digits.
    if(allowDuplicates)
    {
        for(size_t i = 0; i < SECRET_LENGTH; i++)
        {
            // Just use the random function to generate a number from 0 to 'DIGIT_COUNT' - 1 (i.e. 0 - 9). Then add it to the character '0' to generate a character between '0' and '9'.
            gameData.secret[i] = '0' + random(DIGIT_COUNT);
        }
    }
    else
    {
        // If we don't allow duplicate digits, then shuffle an array containing unique numbers and just pick the first 'SECRET_LENGTH' number of characters.
        shuffleSort(randomDigitPool, DIGIT_COUNT);
        for(size_t i = 0; i < SECRET_LENGTH; i++)
        {
            gameData.secret[i] = randomDigitPool[i];
        }
    }

    DebugPrint("Secret key generated: "); DebugWrite(gameData.secret, SECRET_LENGTH); DebugPrintLine("");
}

/*! lcdPrint()
    Prints text on the LCD at the specified coordinates.
    \fn lcdPrint()
    \param[in]  x   The x-coordinate of the LCD.
    \param[in]  y   The y-coordinate of the LCD.
    \param[in]  arr A buffer contianing characters to print.
    \param[in]  n   The number of characters to print.
    \return         .
*/
void lcdPrint(const uint8_t x, const uint8_t y, const char* const arr, const size_t n)
{
    // Boundary check.
    if((x < 0 || x >= LCD_COLCOUNT) || (y < 0 || y >= LCD_ROWCOUNT))
    {
        return;
    }

    // Set LCD cursor position and print buffer content.
    lcd.setCursor(x, y);
    for(size_t i = 0; i < n; i++)
    {
        lcd.print(arr[i]);
    }
}

/*! lcdPrintTries()
    Prints number of player tries on the LCD's lower right corner. The length of the number is calculated automatically.
    \fn lcdPrintTries()
    \param  N/A.
    \return .
*/
void lcdPrintTries(void)
{
    // Calculate position and check boundary.
    uint8_t x = LCD_COLCOUNT - numberLength(gameData.tries);
    if(x < 0)
    {
        return;
    }

    // Set LCD cursor position and print the number.
    lcd.setCursor(x, 1);
    lcd.print(gameData.tries);
}

/*! lcdClear()
    Clears characters on the LCD at the specified coordinates.
    \fn lcdClear()
    \param[in]  x   The x-coordinate of the LCD.
    \param[in]  y   The y-coordinate of the LCD.
    \param[in]  n   The number of characters to clear.
    \return         .
*/
void lcdClear(const uint8_t x, const uint8_t y, const size_t n)
{
    // Boundary check.
    if((x < 0 || x >= LCD_COLCOUNT) || (y < 0 || y >= LCD_ROWCOUNT))
    {
        return;
    }

    // Set LCD cursor position and clear characters (print spaces).
    lcd.setCursor(x, y);
    for(size_t i = 0; i < n; i++)
    {
        lcd.print(' ');
    }
}

/*! resetInput()
    Resets player input and LCD (i.e. starts a new round).
    \fn resetInput()
    \param  N/A.
    \return .
*/
void resetInput(void)
{
    #ifdef DEBUG
    if(gameData.state == GS_ROUNDOVER)
    {
        DebugPrint("New round: "); DebugPrint(gameData.tries); DebugPrint("/"); DebugPrint(TRIES_MAX); DebugPrintLine("");
    }
    else
    {
        DebugPrint("Reset input"); DebugPrintLine("");
    }
    #endif

    // Partially clear LCD.
    lcdClear(0, 0, SECRET_LENGTH);
    lcdClear(0, 1, SECRET_LENGTH);
    lcdClear(LCD_COLCOUNT - STRLEN(MESSAGE_NEXTTRY), 0, STRLEN(MESSAGE_NEXTTRY));

    // Reset LCD cursor, input count and state.
    lcd.setCursor(0, 0);
    gameData.inputCount = 0;
    gameData.state = GS_INPUT;
}

/*! newGame()
    Resets player input, LCD and game variabes (i.e. starts a new game).
    \fn newGame()
    \param  N/A.
    \return .
*/
void newGame(void)
{
    DebugPrintLine("");
    DebugPrint("New game"); DebugPrintLine("");

    // Clear entire LCD.
    lcd.clear();

    // Generate a new secret number.
    generateSecret(false);

    // Reset tries count and display 0.
    gameData.tries = 0;
    lcdPrintTries();

    digitalWrite(LEDPIN_WIN, LOW);
    digitalWrite(LEDPIN_LOSE, LOW);

    // Also reset input as it is also a new round.
    resetInput();
}

/*! checkInput()
    Checks player input and polulates 'gameData.hints' with correct/misplaced/wrong guesses.
    \fn checkInput()
    \param  N/A.
    \return .
*/
void checkInput(void)
{
    // Reset hints to 'HINT_WRONG'.
    memset(gameData.hints, HINT_WRONG, sizeof(*gameData.hints) * SECRET_LENGTH);

    // Loop player input.
    for(size_t i = 0; i < SECRET_LENGTH; i++)
    {
        // For every input character loop the secret number.
        for(size_t j = 0; j < SECRET_LENGTH; j++)
        {
            // Player has guessed right number.
            if(gameData.input[i] == gameData.secret[j])
            {
                if(i == j)
                {
                    // Player has also guessed the correct position of the number.
                    gameData.hints[i] = HINT_CORRECT;
                    // We don't want to check further because player has made the best guess and we dont want to 'overwrite' it with 'HINT_MISPLACED' in any next loops.
                    break;
                }
                else
                {
                    // Correct number, but in wrong position. Continue loop to check the if this number is also guessed in another position (perhaps with a 'HINT_CORRECT' guess).
                    gameData.hints[i] = HINT_MISPLACED;
                }
            }
        }
    }
}

/*! isSolved()
    Checking 'gameData.hints' if all hints are correctly guessed (i.e. player has solved the secret number).
    \fn isSolved()
    \param  N/A.
    \return True if the entire number guessed is correct, false otherwise.
*/
bool isSolved(void)
{
    // Loop and check if all character are of type 'HINT_CORRECT'.
    for(size_t i = 0; i < SECRET_LENGTH; i++)
    {
        if(gameData.hints[i] != HINT_CORRECT)
        {
            return false;
        }
    }

    return true;
}

/*! setup()
    Arduino setup function. Initialized game/component instances. Called automatically internally.
    \fn setup()
    \param  N/A.
    \return .
*/
void setup(void)
{
    // Initialize 'Serial'
    Serial.begin(9600);
    Serial.println("Initializing...");

    // Get seed from an unconnected analog port
    randomSeed(analogRead(A0));
    // Initialize LCD
    lcd.begin(LCD_COLCOUNT, LCD_ROWCOUNT);

    pinMode(LEDPIN_WIN, OUTPUT);
    pinMode(LEDPIN_LOSE, OUTPUT);

    Serial.println("Done.");
    Serial.flush();

    // Initialize game variables and LCD cursor
    newGame();
}

/*! loop()
    Arduino main loop function. Game logic is running from here. Called automatically internally.
    \fn loop()
    \param  N/A.
    \return .
*/
void loop(void)
{
    // Get player input.
    char key = keypad.getKey();
    if(key == NO_KEY)
    {
        return;
    }

    DebugPrint("Key: "); DebugPrint(key); DebugPrintLine("");

    // If player is in input state and has typed in a digit.
    if(gameData.state == GS_INPUT && (key >= '0' && key <= '9'))
    {
        gameData.input[gameData.inputCount] = key;
        gameData.inputCount++;
        lcd.print(key);

        // If player has types in maximum digit guesses.
        if(gameData.inputCount >= SECRET_LENGTH)
        {
            // Another try has been made.
            gameData.tries++;
            lcdPrintTries();

            // Check player input to populate and print 'gameData.hints'.
            checkInput();
            lcdPrint(0, 1, gameData.hints, SECRET_LENGTH);
            DebugPrint("Input: "); DebugWrite(gameData.input, SECRET_LENGTH); DebugPrintLine("");
            DebugPrint("Hints: "); DebugWrite(gameData.hints, SECRET_LENGTH); DebugPrintLine("");

            // If player has solved the entire secret number.
            if(isSolved())
            {
                // Display win message.
                lcd.setCursor(LCD_COLCOUNT - STRLEN(MESSAGE_WIN), 0);
                lcd.print(MESSAGE_WIN);
                digitalWrite(LEDPIN_WIN, HIGH);
                gameData.state = GS_GAMEOVER;

                DebugPrint("Player has won"); DebugPrintLine("");
            }
            else if(gameData.tries >= TRIES_MAX)
            {
                // Display lose message.
                lcd.setCursor(LCD_COLCOUNT - STRLEN(MESSAGE_LOSE), 0);
                lcd.print(MESSAGE_LOSE);
                digitalWrite(LEDPIN_LOSE, HIGH);
                gameData.state = GS_GAMEOVER;

                DebugPrint("Player has lost"); DebugPrintLine("");
            }
            else
            {
                // Round over. Player can guess again.
                lcd.setCursor(LCD_COLCOUNT - STRLEN(MESSAGE_NEXTTRY), 0);
                lcd.print(MESSAGE_NEXTTRY);

                gameData.state = GS_ROUNDOVER;
            }
        }
    }
    // If button for new round was pressed.
    else if(key == BUTTON_NEWROUND)
    {
        // Intitiate new round/reset input.
        if(gameData.state == GS_INPUT || gameData.state == GS_ROUNDOVER)
        {
            resetInput();
        }
        // If game is over, initiate a new game instead (for conveniency).
        else if(gameData.state == GS_GAMEOVER)
        {
            newGame();
        }
    }
    // If button for new game was pressed, initiate a new game.
    else if(key == BUTTON_NEWGAME)
    {
        newGame();
    }
}
