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
} gameData;

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
    \param[in]  allowDuplicates Allows duplicate digits or not.
    \return                     .
*/
void generateSecret(const bool allowDuplicates = false)
{
    if(allowDuplicates)
    {
        for(size_t i = 0; i < SECRET_LENGTH; i++)
        {
            gameData.secret[i] = '0' + random(DIGIT_COUNT);
        }
    }
    else
    {
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
    if((x < 0 || x >= LCD_COLCOUNT) || (y < 0 || y >= LCD_ROWCOUNT))
    {
        return;
    }

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
    uint8_t x = LCD_COLCOUNT - numberLength(gameData.tries);
    if(x < 0)
    {
        return;
    }

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
    if((x < 0 || x >= LCD_COLCOUNT) || (y < 0 || y >= LCD_ROWCOUNT))
    {
        return;
    }

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

    lcdClear(0, 0, SECRET_LENGTH);
    lcdClear(0, 1, SECRET_LENGTH);
    lcdClear(LCD_COLCOUNT - STRLEN(MESSAGE_NEXTTRY), 0, STRLEN(MESSAGE_NEXTTRY));
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

    lcd.clear();

    generateSecret(false);

    gameData.tries = 0;
    lcdPrintTries();

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
    memset(gameData.hints, HINT_WRONG, sizeof(*gameData.hints) * SECRET_LENGTH);

    for(size_t i = 0; i < SECRET_LENGTH; i++)
    {
        for(size_t j = 0; j < SECRET_LENGTH; j++)
        {
            if(gameData.input[i] == gameData.secret[j])
            {
                if(i == j)
                {
                    gameData.hints[i] = HINT_CORRECT;
                    break;
                }
                else
                {
                    gameData.hints[i] = HINT_MISPLACED;
                }
            }
        }
    }
}

bool isSecretSolved(const char* const hints, const size_t n)
{
    for(size_t i = 0; i < n; i++)
    {
        if(hints[i] != HINT_CORRECT)
        {
            return false;
        }
    }

    return true;
}

void setup(void)
{
    Serial.begin(9600);
    Serial.println("Initializing...");

    randomSeed(analogRead(A0));             // Get seed from an unconnected analog port
    lcd.begin(LCD_COLCOUNT, LCD_ROWCOUNT);  // Initialize LCD

    Serial.println("Done.");
    Serial.flush();

    newGame();                            // Initialize game variables and LCD cursor
}

void loop(void)
{
    char key = keypad.getKey();
    if(key == NO_KEY)
    {
        return;
    }

    DebugPrint("Key: "); DebugPrint(key); DebugPrintLine("");

    if(gameData.state == GS_INPUT && (key >= '0' && key <= '9'))
    {
        gameData.input[gameData.inputCount] = key;
        gameData.inputCount++;
        lcd.print(key);

        if(gameData.inputCount >= SECRET_LENGTH)
        {
            gameData.tries++;
            lcdPrintTries();

            checkInput();
            lcdPrint(0, 1, gameData.hints, SECRET_LENGTH);
            DebugPrint("Input: "); DebugWrite(gameData.input, SECRET_LENGTH); DebugPrintLine("");
            DebugPrint("Hints: "); DebugWrite(gameData.hints, SECRET_LENGTH); DebugPrintLine("");

            if(isSecretSolved(gameData.hints, SECRET_LENGTH))
            {
                lcd.setCursor(LCD_COLCOUNT - STRLEN(MESSAGE_WIN), 0);
                lcd.print(MESSAGE_WIN);
                gameData.state = GS_GAMEOVER;

                DebugPrint("Player has won"); DebugPrintLine("");
            }
            else if(gameData.tries >= TRIES_MAX)
            {
                lcd.setCursor(LCD_COLCOUNT - STRLEN(MESSAGE_LOSE), 0);
                lcd.print(MESSAGE_LOSE);
                gameData.state = GS_GAMEOVER;

                DebugPrint("Player has lost"); DebugPrintLine("");
            }
            else
            {
                lcd.setCursor(LCD_COLCOUNT - STRLEN(MESSAGE_NEXTTRY), 0);
                lcd.print(MESSAGE_NEXTTRY);

                gameData.state = GS_ROUNDOVER;
            }
        }
    }
    else if(key == BUTTON_NEWROUND)
    {
        if(gameData.state == GS_INPUT || gameData.state == GS_ROUNDOVER)
        {
            resetInput();
        }
        else if(gameData.state == GS_GAMEOVER)
        {
            newGame();
        }
    }
    else if(key == BUTTON_NEWGAME)
    {
        newGame();
    }
}
