#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <stddef.h>

#define NOP         ((void)0)
#define ARRCNT(x)   (sizeof(x) / sizeof(*(x)))
#define STRLEN(x)   (ARRCNT(x) - 1)

#define DEBUG   // Uncomment for debug mode

#ifdef DEBUG
#define DebugWrite(x, n)    Serial.write(x, n)
#define DebugPrint(x)       Serial.print(x)
#define DebugPrintLine(x)   Serial.println(x)
#else
#define DebugWrite(x, n)    NOP
#define DebugPrint(x)       NOP
#define DebugPrintLine(x)   NOP
#endif

#define KEYPAD_ROWCOUNT     4
#define KEYPAD_COLCOUNT     4
#define BUTTON_NEWROUND     '*'
#define BUTTON_NEWGAME      '#'
const byte KEYPAD_KEYMAP[KEYPAD_ROWCOUNT][KEYPAD_COLCOUNT] =
{
    {'1',               '2',    '3',            'A'},
    {'4',               '5',    '6',            'B'},
    {'7',               '8',    '9',            'C'},
    {BUTTON_NEWROUND,   '0',    BUTTON_NEWGAME, 'D'}
};

byte ROW_PINS[KEYPAD_ROWCOUNT] = { 7, 6, 5, 4 };
byte COL_PINS[KEYPAD_COLCOUNT] = { 3, 2, A4, A5 };
Keypad keypad(makeKeymap(KEYPAD_KEYMAP), ROW_PINS, COL_PINS, KEYPAD_ROWCOUNT, KEYPAD_COLCOUNT);

#define LCD_ROWCOUNT    2
#define LCD_COLCOUNT    16
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

#define SECRET_LENGTH   4
#define HINT_WRONG      '-'
#define HINT_MISPLACED  '?'
#define HINT_CORRECT    '!'

#define TRIES_MAX       12

#define MESSAGE_WIN     "Winner"
#define MESSAGE_LOSE    "Loser"
#define MESSAGE_NEXTTRY "Try again"

typedef enum
{
    GS_INPUT,
    GS_ROUNDOVER,
    GS_GAMEOVER
} gamestate;

struct
{
    gamestate state;

    char secret[SECRET_LENGTH];
    char input[SECRET_LENGTH];
    char hints[SECRET_LENGTH];

    size_t inputCount;
    unsigned int tries;
} gameData;

#define DIGIT_COUNT  10
char randomDigitPool[DIGIT_COUNT] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

inline unsigned int numberLength(const unsigned int n)
{
    return (n > 0 ? (unsigned int)log10((double)n) : 0) + 1;
}

inline size_t randomNumber(const size_t n)
{
    return n > 0 ? random(n) : 0;
}

void shuffleSort(char* const list, const size_t n)
{
    size_t i;

    // Iterate backwards, swapping last element of each iteration with a random element of lower index
    for(i = n; i-- > 0;)
    {
        size_t z = randomNumber(i) + 1;
        char tmp = list[i];
        list[i] = list[z];
        list[z] = tmp;
    }
}

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

void checkInput(const char* const input, const char* const secret, char* const hints, const size_t n)
{
    memset(hints, HINT_WRONG, sizeof(*hints) * n);

    for(size_t i = 0; i < n; i++)
    {
        for(size_t j = 0; j < n; j++)
        {
            if(input[i] == secret[j])
            {
                if(i == j)
                {
                    hints[i] = HINT_CORRECT;
                    break;
                }
                else
                {
                    hints[i] = HINT_MISPLACED;
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

            checkInput(gameData.input, gameData.secret, gameData.hints, SECRET_LENGTH);
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
