			                  /*  Xonix Game By Mitul and Danyal */


#include<SFML/Graphics.hpp>
#include<time.h>
#include<cmath>
#include<iostream>
#include<fstream>
#include<SFML/Audio.hpp>

using namespace std;
using namespace sf;

//Game constants
const int M = 25;
const int N = 40;

int grid[M][N] = {0};
int ts = 18; // Tile size

	////////sounds////////////
SoundBuffer backgroundBuffer;
SoundBuffer captureTilesBuffer;
SoundBuffer bonusBuffer;
SoundBuffer powerupBuffer;
SoundBuffer gameOverBuffer;
Sound backgroundSound;
Sound captureTilesSound;
Sound bonusSound;
Sound powerupSound;
Sound gameOverSound;

	//////background/////////
Color defaultBackgroundColor = Color::Black; /////default odf the screen will be black
Color powerupBackgroundColor = Color(173,216,230); /////light color will appear when the power up happen


// Game state

Clock gameClock;
bool Game = true;
int level = 1;
int moveCount1 = 0;
int moveCount2 = 0;
bool buildingTiles1 = false;
bool buildingTiles2 = false;
float speedMultiplier = 2.0f;
float lastSpeedIncreaseTime = 0;
Clock speedIncreaseClock;
int enemyIncrement = 2;

//Scoring 
int score1 = 0;
int score2 = 0;
int tilesFilledInMove1 = 0;
int tilesFilledInMove2 = 0;
int bonusCounter1 = 0;
int bonusCounter2 = 0;
int bonusThreshold1 = 10;
int bonusThreshold2 = 10;
int bonusMultiplier1 = 2;
int bonusMultiplier2 = 2;

//Power-up
int powerUpsAvailable1 = 0;
int powerUpsAvailable2 = 0;
bool powerUpActive1 = false;
bool powerUpActive2 = false;
Clock powerUpClock1;
Clock powerUpClock2;
int nextPowerUpScore1 = 50;
int nextPowerUpScore2 = 50;

//Enemy patern
Clock patternClock;
bool patternsActivated = false;


// Const for different movements
const int LINEAR_PATTERN = 0;
const int SQUARE_PATTERN = 1;
const int CIRCULAR_PATTERN = 2;

struct Enemy
{
    int x,y,dx,dy;
    bool frozen;
    int pattern;
    float patternTimer;
    float patternPhase;
    int patternSize;
    float angularVelocity;
    int cornerCounter;
    int initialX, initialY;

    Enemy()
    {
        x = y = 300;
        dx = 4 - rand() % 8;
        dy = 4 - rand() % 8;
        frozen = false;
        pattern = LINEAR_PATTERN;

        // Initialize pattern parameters
        patternTimer = 0;
        patternPhase = 0;
        patternSize = 30 + rand() % 20; // Random size between 30-50 pixels
        angularVelocity = 0.05f + static_cast<float>(rand() % 10)/100.0f; // Random speed , circular motion
        cornerCounter = 0;
        initialX = initialY =0;
    }

    // Linear movement 
    void moveLinear()
    {
        x += dx * speedMultiplier;
        if(grid[y/ts][x/ts] == 1)
		{
            dx = -dx;
            x += dx * speedMultiplier;
        }
        y += dy * speedMultiplier;
        if(grid[y / ts][x / ts] == 1)
		{
            dy = -dy;
            y += dy * speedMultiplier;
        }
    }

    void moveSquare(float deltaTime)
    {
        // Update pattern timer
        patternTimer += deltaTime;

        // Changing direction at the corners of the square
        if(patternTimer>1.0f)
		{ 						// Change direction every second
            patternTimer = 0;
            cornerCounter =(cornerCounter + 1) % 4;

            // Set direction based on which corner we will heading to
            switch(cornerCounter)
			{
            	case 0:dx = 1; dy = 0;  break; // Right
            	case 1:dx = 0; dy = 1;  break; // Down
            	case 2:dx = -1; dy = 0; break; // Left
            	case 3:dx = 0; dy = -1; break; // Up
            }
        	}

        // Moving in current direction
        x += dx * speedMultiplier;
        y += dy * speedMultiplier;

        // Handling collisions with walls
        if(grid[y/ts][x/ts] == 1)
		{
            // Bouncing off walls
            if(dx != 0) dx = -dx;
            if(dy != 0) dy = -dy;

            // Moveing away from wall
            x += dx * speedMultiplier;
            y += dy * speedMultiplier;

            //Reset corner counter so we start a new square pattern
            cornerCounter = (cornerCounter + 2) % 4; // Jump to opposite direction
        }
    }

    // Circular pattern movement
    void moveCircular(float deltaTime)
    {
        // Update phase
        patternPhase += angularVelocity*deltaTime*10.0f;

        // Calculating new position using parametric equations of a circle
        // but also including a drift component to move across the screen
        float circleX = cos(patternPhase)*patternSize;
        float circleY = sin(patternPhase)*patternSize;

        // Calculate new position 
        int newX = initialX+circleX+(dx*patternTimer*speedMultiplier);
        int newY = initialY+circleY+(dy*patternTimer*speedMultiplier);

        // Check for collision with walls before moving
        if(newX / ts<1 || newX / ts >= N - 1 || newY / ts<1 || newY / ts >= M - 1 ||
            grid[newY / ts][newX / ts] == 1)
			{
            // Reset pattern start point and reverse direction
            initialX = x;
            initialY = y;
            dx = -dx;
            dy = -dy;
            patternTimer = 0;
        }
        else
		{
            // Move to new position
            x = newX;
            y = newY;
        }

        // Increase timer for drift effect
        patternTimer += deltaTime;

        // Reset timer periodically to prevent too much drift
        if(patternTimer > 5.0f)
		{
            initialX = x;
            initialY = y;
            patternTimer = 0;
        }
    }

    // Main move function that calls the appropriate movement pattern
    void move(float deltaTime)
    {
        if(frozen) return; // Don't move if frozen

        if(pattern == LINEAR_PATTERN)
		{
            moveLinear();
        }
        else if(pattern == SQUARE_PATTERN)
		{
            moveSquare(deltaTime);
        }
        else if(pattern == CIRCULAR_PATTERN)
		{
            moveCircular(deltaTime);
        }

        // Ensure enemy stays within bounds
        if(x < 0) x = 0; if(x > N * ts - 1) x = N * ts - 1;
        if(y < 0) y = 0; if(y > M * ts - 1) y = M * ts - 1;
    }

    // Function to set the pattern
    void setPattern(int newPattern)
    {
        pattern = newPattern;

        // Reset pattern parameters
        patternTimer = 0;
        patternPhase = static_cast<float>(rand()) / RAND_MAX*2*3.14159f; // Random starting phase
        cornerCounter = rand() % 4; // Random starting corner

        // Store current position as initial position for the pattern
        initialX = x;
        initialY = y;
    }
};

// Function declarations
void drop(int y,int x);
void activatePowerUp1(Enemy a[],int enemyCount);
void activatePowerUp2(Enemy a[],int enemyCount);
void checkForPowerUp1();
void checkForPowerUp2();
void loadSound();
void updateEnemyPatterns(Enemy a[],int enemyCount);
void resetGame(Enemy a[],int enemyCount,int& x1,int& y1,int& x2,int& y2,bool& Game,int Mode);
void setupDifficulty(int LevelSelection,int& enemyCount,float& delay);
void addHighScore(int newScore,int newTime);
void displayMenu(RenderWindow& window,Font& font,bool& gameStart,int& LevelSelection,int& Mode);
void displaySelectLevel(RenderWindow& window,Font& font,int& LevelSelection);
void displayScoreBoard(RenderWindow& window,Font& font);
void PlayerMode(RenderWindow& window, Font& font, int& Mode);
void displayEndMenu(RenderWindow& window,Font& font,bool& Game,int score,bool& returnToMainMenu,
                   Enemy enemies[],int enemyCount,int& x1,int& y1, int& x2, int& y2, int Mode);

void drop(int y,int x)
{
    if(grid[y][x] == 0) grid[y][x] = -1;
    if(grid[y - 1][x] == 0) drop(y - 1, x);
    if(grid[y + 1][x] == 0) drop(y + 1, x);
    if(grid[y][x - 1] == 0) drop(y, x - 1);
    if(grid[y][x + 1] == 0) drop(y, x + 1);
 }

void activatePowerUp1(Enemy a[], int enemyCount)
{
    if(powerUpsAvailable1 > 0 && !powerUpActive1)
	{
        powerUpsAvailable1--;
        powerUpActive1 = true;
        powerUpClock1.restart();
        powerupSound.play();
        powerupSound.setVolume(70);
        for(int i = 0; i < enemyCount; i++)
		{
            a[i].frozen = true;
        }
    }
}

void activatePowerUp2(Enemy a[],int enemyCount)
{
    if(powerUpsAvailable2 > 0 && !powerUpActive2)
	{
        powerUpsAvailable2--;
        powerUpActive2 = true;
        powerUpClock2.restart();
        powerupSound.play();
        powerupSound.setVolume(70);
        for(int i = 0; i < enemyCount; i++)
		{
            a[i].frozen = true;
        }
    }
}

void checkForPowerUp1()
{
    if(score1 >= nextPowerUpScore1)
	{
        powerUpsAvailable1++;
        if(nextPowerUpScore1 < 70)
		{
            nextPowerUpScore1 += 20;
        } else
		{
            nextPowerUpScore1 += 30;
        }
    }
}

void checkForPowerUp2()
{
    if(score2 >= nextPowerUpScore2)
	{
        powerUpsAvailable2++;
        if(nextPowerUpScore2 < 70)
		{
            nextPowerUpScore2 += 20;
        } else{
            nextPowerUpScore2 += 30;
        }
    }
}

/////function for the sounds/////
void loadSound()
{
    if(!captureTilesBuffer.loadFromFile("sound/capturetile.mp3"))
	{
        cout<<"Error in loading the sound capturing the tiles."<<endl;
    }
    if(!bonusBuffer.loadFromFile("sound/bonus.mp3"))
	{
        cout<<"Error in loading the sound of bonus."<<endl;
    }
    if(!powerupBuffer.loadFromFile("sound/powerup.mp3"))
	{
        cout<<"Error in loading the sound of powerup."<<endl;
    }
    if(!gameOverBuffer.loadFromFile("sound/gameover.mp3"))
	{
        cout<<"Error in loading the sound gameover."<<endl;
    }
    if(!backgroundBuffer.loadFromFile("sound/backgroundmusic.mp3"))
	{
        cout<<"Error in loading the sound background music."<<endl;
    }
   
    captureTilesSound.setBuffer(captureTilesBuffer);
    bonusSound.setBuffer(bonusBuffer);
    backgroundSound.setBuffer(backgroundBuffer);
    gameOverSound.setBuffer(gameOverBuffer);
    powerupSound.setBuffer(powerupBuffer);

}


// updateing enemy patterns after 30 seconds
void updateEnemyPatterns(Enemy a[], int enemyCount)
{
    if(!patternsActivated && patternClock.getElapsedTime().asSeconds() >= 30.0f) {
        patternsActivated = true;

        // Set half of the enemies to square pattern and half to circular pattern
        for(int i = 0; i < enemyCount; i++)
		{
            if(i < enemyCount/2)
			{
                a[i].setPattern(SQUARE_PATTERN);
                cout<<"Enemy "<<i<<" now follows square pattern"<<endl;
            }
            else{
                a[i].setPattern(CIRCULAR_PATTERN);
                cout<<"Enemy "<<i<<" now follows circular pattern"<<endl;
            }
        }
    }
}

void resetGame(Enemy a[], int enemyCount, int& x1,int& y1,int& x2, int& y2,bool& Game,int Mode)
{
    for(int i = 1; i < M - 1; i++)
	{
        for(int j = 1; j < N - 1; j++)
		{
            grid[i][j] = 0;
        }
    }

    x1 = 10;
    y1 = 0;
    if(Mode == 1)
	{
        x2 = N - 11;
        y2 = M - 1;
    }
    Game = true;
    score1 = 0;
    moveCount1 = 0;
    buildingTiles1 = false;
    bonusCounter1 = 0;
    bonusThreshold1 = 10;
    bonusMultiplier1 = 2;
    powerUpsAvailable1 = 0;
    nextPowerUpScore1 = 50;
    powerUpActive1 = false;
    patternsActivated = false;
    speedMultiplier = 2.0f;
    score2 = 0 ;
    moveCount2 = 0 ;
    buildingTiles2 = false;
    bonusCounter2 = 0;
    bonusThreshold2 = 10;
    bonusMultiplier2 = 2;
    powerUpsAvailable2 = 0 ;
    nextPowerUpScore2 = 50 ;
    powerUpActive2 = false;

    // Reset all enemies to linear movement
    for(int i = 0; i < enemyCount; i++)
	{
        a[i] = Enemy(); // Create new enemies
        a[i].setPattern(LINEAR_PATTERN);
    }

    // Reset game timing variables
    gameClock.restart();
    patternClock.restart();
    speedIncreaseClock.restart();
    lastSpeedIncreaseTime = 0;
}



void setupDifficulty(int LevelSelection,int& enemyCount, float& delay) {
    switch(LevelSelection)
	{
        case 0: // Easy
            level = 1;
            enemyCount = 2;
            delay = 0.07f;
            speedMultiplier = 2.0f;
            break;
        case 1: // Medium
            level = 2;
            enemyCount = 4;
            delay = 0.07f;
            speedMultiplier = 2.0f;
            break;
        case 2: // Hard
            level = 3;
            enemyCount = 6;
            delay = 0.07f;
            speedMultiplier = 2.0f;
            break;
        case 3: // Continuous
            level = 4;
            enemyCount = 2;
            delay = 0.07f;
            speedMultiplier = 2.0f;
            break;
    }
}



void PlayerMode(RenderWindow& window,Font& font,int& Mode)
{
    // For Background picture of Menu
    Texture backgroundTexture;
    if(!backgroundTexture.loadFromFile("images/background.png"))
	{
        cout<<"Error Loading background Image"<<endl;
    }
    Sprite backgroundsprite(backgroundTexture);

    Text title("Select Player Mode", font, 50);
    title.setFillColor(Color::White);
    title.setPosition(190, 80);
    title.setStyle(Text::Bold);

    Text onePlayer("Single Player", font, 35);
    onePlayer.setFillColor(Color::White);
    onePlayer.setPosition(250, 170);

    Text twoPlayer("Double Player", font, 35);
    twoPlayer.setFillColor(Color::White);
    twoPlayer.setPosition(250, 240);

	 // 0 for single player, 1 for double player
    int select = 0; 
    Color highlight = Color::Red;

    while(window.isOpen())
	{
        Event a;
        while(window.pollEvent(a))
		{
            if(a.type == Event::Closed)
			{
                window.close();
            }
            if(a.type == Event::KeyPressed)
			{
                if(a.key.code == Keyboard::Return)
				{
                    Mode = select;
                    return;
                }
                else if(a.key.code == Keyboard::Down)
				{
                    select = (select + 1) % 2;
                }
                else if(a.key.code == Keyboard::Up)
				{
                    select = (select + 1) % 2;
                }
            }
        }

        window.clear();
        window.draw(backgroundsprite);
        window.draw(title);

        if(select == 0) onePlayer.setFillColor(highlight);
        else onePlayer.setFillColor(Color::White);

        if(select == 1) twoPlayer.setFillColor(highlight);
        else twoPlayer.setFillColor(Color::White);

        window.draw(onePlayer);
        window.draw(twoPlayer);

        window.display();
    }
}
void addHighScore(int newScore,int newTime)
{
    const int MAX_SCORES = 5;
    int scores[MAX_SCORES] = {0};
    int times[MAX_SCORES] = {0};
    int count = 0;

    // Read existing scores
    ifstream fileIn("highscore.txt");
    if(fileIn)
	{
        while(count<MAX_SCORES&&fileIn>>scores[count]>>times[count])
        {
            count++;
        }
        fileIn.close();
    }

    // Check if new score should be added
    bool scoreAdded = false;
    for(int i = 0; i < MAX_SCORES; i++)
	{
        if(newScore > scores[i] || (newScore == scores[i] && newTime < times[i]))
		{
            // Shift lower scores down
            for(int j = MAX_SCORES - 1; j > i; j--)
			{
                scores[j] = scores[j - 1];
                times[j] = times[j - 1];
            }

            // Insert new score
            scores[i] = newScore;
            times[i] = newTime;
            scoreAdded = true;

            // If we were already at max scores, we only need count = MAX_SCORES
            if(count < MAX_SCORES) count++;
            break;
        }
    }

    // If score wasn't high enough to replace others
    if(!scoreAdded && count < MAX_SCORES)
	{
        scores[count] = newScore;
        times[count] = newTime;
        count++;
    }

    // Writing all scores back
    ofstream fileOut("highscore.txt");
    if(fileOut)
	{
        for(int i = 0; i < count; i++)
		{
            fileOut<<scores[i]<<" "<<times[i]<<endl;
        }
        fileOut.close();
    }
    else
	{
        cout<<"Could not open highscore.txt for writing"<<endl;
    }
}

void displayScoreBoard(RenderWindow& window,Font& font)
{
    cout<<"showing score board....."<<endl;  
    // For Background picture of score board
    Texture backgroundTexture;
    if(!backgroundTexture.loadFromFile("images/background.png"))
	{
        cout<<"Error Loading background Image"<<endl;
    }
    Sprite backgroundsprite(backgroundTexture);

    Text title("SCORE BOARD", font, 50);
    title.setFillColor(Color::White);
    title.setPosition(190, 80);
    title.setStyle(Text::Bold);

    Text score("Score",font, 25);
    score.setFillColor(Color::Cyan);
    score.setPosition(250, 140);

    Text time("Time (s)",font, 25);
    time.setFillColor(Color::Cyan);
    time.setPosition(400, 140);

    const int max_score = 5;
    int scores[max_score] = {0};
    int scoreCount = 0;
    int times[max_score] = {0};

    ifstream FileIn("highscore.txt");
    if(!FileIn.is_open())
	{
        cout<<"Error opening highscore.txt for reading"<<endl;
    }
    else
    {
        while(scoreCount<max_score && FileIn>>scores[scoreCount]>>times[scoreCount])
		{
            scoreCount++;
        }
        FileIn.close();
    }

    Text scoreTexts[max_score];
    Text timeTexts[max_score];

    for(int i = 0; i < scoreCount; i++)
	{
        // score
        scoreTexts[i].setFont(font);
        scoreTexts[i].setString(to_string(i + 1)+" : "+ to_string(scores[i]));
        scoreTexts[i].setCharacterSize(35);
        scoreTexts[i].setFillColor(Color::White);
        scoreTexts[i].setPosition(250, 170 + i * 50);

        // Time
        timeTexts[i].setFont(font);
        timeTexts[i].setString(to_string(times[i]));
        timeTexts[i].setCharacterSize(30);
        timeTexts[i].setFillColor(Color::White);
        timeTexts[i].setPosition(400, 180 + i*40);
    }

    Text no_score("No high scores yet!",font, 35);
    no_score.setFillColor(Color::White);
    no_score.setPosition(220, 200);

    Text back("Press ESC to return",font, 30);
    back.setFillColor(Color::Magenta);
    back.setPosition(500, 410);




    while(window.isOpen())
	{
        Event a;
        while(window.pollEvent(a))
		{
            if(a.type == Event::Closed)
			{
                window.close();
            }
            if(a.type == Event::KeyPressed)
			{
                if(a.key.code == Keyboard::Escape)
				{

                    return;
                }

            }
        }


        window.clear();
        window.draw(backgroundsprite);
        window.draw(title);


        if(scoreCount > 0)
		{
            window.draw(score);
            window.draw(time);

            // Draw all score texts
            for(int i = 0; i < scoreCount;i++)
			{
                window.draw(scoreTexts[i]);
                window.draw(timeTexts[i]);
            }
        }
        else{
            window.draw(no_score);
        }

        window.draw(back);
        window.display();
    }
}

void displaySelectLevel(RenderWindow& window,Font& font, int& LevelSelection) {
    // For Background picture of Menu
    Texture backgroundTexture;
    if(!backgroundTexture.loadFromFile("images/background.png"))
	{
        cout<<"Error Loading background Image"<<endl;
    }
    Sprite backgroundsprite(backgroundTexture);

    Text title("Select Level",font, 50);
    title.setFillColor(Color::White);
    title.setPosition(190, 80);
    title.setStyle(Text::Bold);

    Text Easy("Easy", font, 35);
    Easy.setFillColor(Color::White);
    Easy.setPosition(250, 170);

    Text Medium("Medium", font, 35);
    Medium.setFillColor(Color::White);
    Medium.setPosition(250, 240);

    Text Hard("Hard", font, 35);
    Hard.setFillColor(Color::White);
    Hard.setPosition(250, 310);

    Text Continuous("Continuous Mode", font, 30);
    Continuous.setFillColor(Color::White);
    Continuous.setPosition(250, 380);
    Continuous.setStyle(Text::Bold);

	// 0 for Easy, 1 for Medium, 2 for Hard, 3 for Continuous
    int select = 0; 
    Color highlight = Color::Red;

    while(window.isOpen())
	{
        Event a;
        while(window.pollEvent(a))
		{
            if(a.type == Event::Closed)
			{
                window.close();
            }
            if(a.type == Event::KeyPressed)
			{
                if(a.key.code == Keyboard::Return)
				{
                    LevelSelection = select;
                    return;
                }
                else if(a.key.code == Keyboard::Down)
				{
                    select = (select + 1) % 4;
                }
                else if(a.key.code == Keyboard::Up)
				{
                    select = (select + 3) % 4 ;
                }
            }
        }

        window.clear();
        window.draw(backgroundsprite);
        window.draw(title);

        if(select == 0) Easy.setFillColor(highlight);
        else Easy.setFillColor(Color::White);

        if(select == 1) Medium.setFillColor(highlight);
        else Medium.setFillColor(Color::White);

        if(select == 2) Hard.setFillColor(highlight);
        else Hard.setFillColor(Color::White);

        if(select == 3) Continuous.setFillColor(highlight);
        else Continuous.setFillColor(Color::White);

        window.draw(Easy);
        window.draw(Medium);
        window.draw(Hard);
        window.draw(Continuous);

        window.display();
    }
}

void displayMenu(RenderWindow& window, Font& font,bool& gameStart, int& LevelSelection,int& Mode) {
    // For Background picture of Menu
    Texture backgroundTexture;
    if(!backgroundTexture.loadFromFile("images/background.png"))
	{
        cout<<"Error Loading background Image"<<endl;
    }
    Sprite backgroundsprite(backgroundTexture);

    Text title("XONIX GAME", font, 50);
    title.setFillColor(Color::Cyan);
    title.setPosition(190, 80);
    title.setStyle(Text::Bold);

    Text start("Start Game", font, 35);
    start.setFillColor(Color::White);
    start.setPosition(250, 170);

    Text selectLevel("Select Level", font, 35);
    selectLevel.setFillColor(Color::White);
    selectLevel.setPosition(250, 240);

    Text score_board("Score Board", font, 35);
    score_board.setFillColor(Color::White);
    score_board.setPosition(250, 310);

    Text madeBy("By Danyal and Mitul",font, 30);
    madeBy.setFillColor(Color::Magenta);
    madeBy.setPosition(420, 410);
    madeBy.setStyle(Text::Bold);

    int select = 0; // 0 for start, 1 for select level, 2 for exit
    Color highlight = Color::Red;

    while(window.isOpen())
	{
        Event a;
        while(window.pollEvent(a))
		{
            if(a.type == Event::Closed)
			{
                window.close();
            }
            if(a.type == Event::KeyPressed)
			{
                if(a.key.code == Keyboard::Return)
				{
                    if(select == 0)
					{ // Start Game
                        gameStart = true;
                        return;
                    }
                    else if(select == 1)
					{
                        displaySelectLevel(window, font, LevelSelection);
                        PlayerMode(window, font, Mode);
                    }
                    else if(select == 2)
					{
                        displayScoreBoard(window, font);
                        return;
                    }
                }
                else if(a.key.code == Keyboard::Down)
				{
                    select = (select + 1) % 3;
                }
                else if(a.key.code == Keyboard::Up)
				{
                    select = (select + 2) % 3;
                }
            }
        }

        window.clear();
        window.draw(backgroundsprite);
        window.draw(title);

        if(select == 0) start.setFillColor(highlight);
        else start.setFillColor(Color::White);

        if(select == 1) selectLevel.setFillColor(highlight);
        else selectLevel.setFillColor(Color::White);

        if(select == 2) score_board.setFillColor(highlight);
        else score_board.setFillColor(Color::White);

        window.draw(start);
        window.draw(selectLevel);
        window.draw(score_board);
        window.draw(madeBy);

        window.display();
    }
}

void displayEndMenu(RenderWindow& window,Font& font, bool& Game,int score1, int score2, bool& returnToMainMenu,
                   Enemy enemies[], int enemyCount,int& x1, int& y1,int& x2, int& y2, int Mode) {
    if(Mode == 0 && score1 > 0)
	{
        int gameTime = static_cast<int>(gameClock.getElapsedTime().asSeconds());
        addHighScore(score1, gameTime);
    } else if(Mode == 1) {
        int gameTime = static_cast<int>(gameClock.getElapsedTime().asSeconds());
        if (score1 > score2)
		{
            addHighScore(score1, gameTime);
        } else if(score2 > score1)
		{
            addHighScore(score2, gameTime);
        }
    }

    Texture backgroundTexture;
    if(!backgroundTexture.loadFromFile("images/background.png"))
	{
        cout<<"Error Loading background Image"<<endl;
    }
    Sprite backgroundsprite(backgroundTexture);

    Text title("GAME OVER", font, 50);
    title.setFillColor(Color::Cyan);
    title.setPosition(190, 80);
    title.setStyle(Text::Bold);

    Text your_score1("Player 1 Score: "+to_string(score1), font, 35);
    your_score1.setFillColor(Color::White);
    your_score1.setPosition(250, 150);

    Text your_score2("Player 2 Score: "+to_string(score2), font, 35);
    your_score2.setFillColor(Color::White);
    your_score2.setPosition(250, 190);

    Text winnerText;
    if(Mode == 1)
	{
        if(score1 > score2)
		{
            winnerText.setString("Player 1 Wins!");
        } else if(score2 > score1)
		{
            winnerText.setString("Player 2 Wins!");
        } else{
            winnerText.setString("It's a Tie!");
        }
        winnerText.setFont(font);
        winnerText.setCharacterSize(40);
        winnerText.setFillColor(Color::Magenta);
        winnerText.setPosition(250, 230);
    }

    Text restart("RESTART",font, 35);
    restart.setFillColor(Color::Cyan);
    restart.setPosition(250,(Mode == 1) ? 290 : 260);

    Text mainMenu("MAIN MENU", font, 35);
    mainMenu.setFillColor(Color::White);
    mainMenu.setPosition(250,(Mode == 1) ? 325 : 310);

    Text exit("QUIT",font, 30);
    exit.setFillColor(Color::White);
    exit.setPosition(250, (Mode == 1) ? 370 : 360);
    exit.setStyle(Text::Bold);

    int select = 0;
    Color highlight = Color::Red;

    while(window.isOpen())
	{
        Event event;
        while(window.pollEvent(event))
		{
            if(event.type == Event::Closed)
			{
                window.close();
            }
            if(event.type == Event::KeyPressed)
			{
                if(event.key.code == Keyboard::Return)
				{
                    if(select == 0)
					{
                        Game = true;
                        resetGame(enemies, enemyCount, x1, y1, x2, y2,Game, Mode);
                        return;
                    } else if(select == 1)
					{
                        returnToMainMenu = true;
                        return;
                    } else if(select == 2)
					{
                        window.close();
                        return;
                    }
                } else if(event.key.code == Keyboard::Down)
				{
                    select = (select + 1) % 3;
                } else if(event.key.code == Keyboard::Up)
				{
                    select = (select + 2) % 3;
                }
            }
        }

        window.clear();
        window.draw(backgroundsprite);
        window.draw(title);
        window.draw(your_score1);
        if(Mode == 1) {
            window.draw(your_score2);
            window.draw(winnerText);
        }

        if(select == 0) restart.setFillColor(highlight);
        else restart.setFillColor(Color::White);

        if(select == 1) mainMenu.setFillColor(highlight);
        else mainMenu.setFillColor(Color::White);

        if(select == 2) exit.setFillColor(highlight);
        else exit.setFillColor(Color::White);

        window.draw(restart);
        window.draw(mainMenu);
        window.draw(exit);
        window.display();
    }
}

int main()
{
    srand(time(0));

    RenderWindow window(VideoMode(N * ts, M * ts + 80),"Xonix Game!");
    window.setFramerateLimit(60);
   
    loadSound();
    backgroundSound.play();
    backgroundSound.setVolume(45);

    Texture t1, t2, t3;
    if(!t1.loadFromFile("images/tiles.png"))
	{
        cout<<"Error loading tiles image"<<endl;
        return -1;
    }
    if(!t2.loadFromFile("images/gameover.png"))
	{
        cout<<"Error loading gameover image"<<endl;
        return -1;
    }
    if(!t3.loadFromFile("images/enemy.png"))
	{
        cout<<"Error loading enemy image"<<endl;
        return -1;
    }

    Sprite sTile(t1), sGameover(t2), sEnemy(t3);
    sGameover.setPosition(100, 100);
    sEnemy.setOrigin(20, 20);

    int enemyCount = 4;
    Enemy a[10];

    // Player variables
    int x1 = 0, y1 = 0, dx1 = 0, dy1 = 0;
    int x2 = N-1, y2 = M-1, dx2 = 0, dy2 = 0;
   
    float timer = 0, delay = 0.07;
    Clock clock;
    Clock speedIncreaseClock;
    int lastSpeedIncreaseTime = 0;

    Font myfont;
    if(!myfont.loadFromFile("Fonts/BebasNeue-Regular.ttf"))
	{
        cout<<"Error loading font"<<endl;
        return -1;
    }

    // Text elements
    Text T_Level;
    T_Level.setFont(myfont);
    T_Level.setCharacterSize(30);
    T_Level.setFillColor(Color::Yellow);
    T_Level.setPosition(210, M * ts + 8);

    Text T_Score1;
    T_Score1.setFont(myfont);
    T_Score1.setCharacterSize(36);
    T_Score1.setFillColor(Color::White);
    T_Score1.setPosition(18, M * ts + 8);

    Text T_Moves1;
    T_Moves1.setFont(myfont);
    T_Moves1.setCharacterSize(30);
    T_Moves1.setFillColor(Color::Cyan);
    T_Moves1.setPosition(18, M * ts + 40);

    Text T_Score2;
    T_Score2.setFont(myfont);
    T_Score2.setCharacterSize(36);
    T_Score2.setFillColor(Color::White);
    T_Score2.setPosition(520, M * ts + 8);

    Text T_Moves2;
    T_Moves2.setFont(myfont);
    T_Moves2.setCharacterSize(30);
    T_Moves2.setFillColor(Color::Cyan);
    T_Moves2.setPosition(520, M * ts + 40);

    Text T_Timer;
    T_Timer.setFont(myfont);
    T_Timer.setCharacterSize(30);
    T_Timer.setFillColor(Color::Blue);
    T_Timer.setPosition(437, M * ts + 8);

    Text T_Speed;
    T_Speed.setFont(myfont);
    T_Speed.setCharacterSize(30);
    T_Speed.setFillColor(Color::Red);
    T_Speed.setPosition(312, M * ts + 8);

    Text T_PowerUps1;
    T_PowerUps1.setFont(myfont);
    T_PowerUps1.setCharacterSize(30);
    T_PowerUps1.setFillColor(Color::Green);
    T_PowerUps1.setPosition(110, M * ts + 40);

    Text T_PowerUps2;
    T_PowerUps2.setFont(myfont);
    T_PowerUps2.setCharacterSize(30);
    T_PowerUps2.setFillColor(Color::Green);
    T_PowerUps2.setPosition(620, M * ts + 40);

    Text T_PowerUpActive;
    T_PowerUpActive.setFont(myfont);
    T_PowerUpActive.setCharacterSize(30);
    T_PowerUpActive.setFillColor(Color::Cyan);
    T_PowerUpActive.setPosition(315, M * ts + 40);



    // Menu variables
    bool gameStart = false;
    int LevelSelection = 1;
    int Mode = 0;
    bool returnToMainMenu = false;
    bool player1Dead = false;
    bool player2Dead = false;

    while(window.isOpen())
	{
        if(!gameStart)
		{
            displayMenu(window, myfont, gameStart, LevelSelection, Mode);

            if(!window.isOpen())
                return 0;

            if(!gameStart)
                continue;

            setupDifficulty(LevelSelection, enemyCount, delay);
            Game = true;
            returnToMainMenu = false;
            player1Dead = false;
            player2Dead = false;

            for(int i = 0; i < M; i++)
			{
                for(int j = 0; j < N; j++)
				{
                    grid[i][j] = 0;
                    if(i == 0 || j == 0 || i == M - 1 || j == N - 1)
                        grid[i][j] = 1;
                }
            }

            x1 = 10;
            y1 = 0;
            dx1 = dy1 = 0;
           
            if(Mode == 1)
			{
                x2 = N - 11;
                y2 = M - 1;
                dx2 = dy2 = 0;
            }

            for(int i = 0; i < enemyCount; i++)
			{
                a[i] = Enemy();
            }

            gameClock.restart();
            patternClock.restart();
            speedIncreaseClock.restart();
            speedMultiplier = 2.0f;
            lastSpeedIncreaseTime = 0;

            patternsActivated = false;
            score1= 0;
            moveCount1= 0;
            buildingTiles1= false;
            bonusCounter1= 0;
            bonusThreshold1= 10;
            bonusMultiplier1= 2;
            powerUpsAvailable1= 0;
            nextPowerUpScore1= 50;
            powerUpActive1= false;

            score2= 0;
            moveCount2= 0;
            buildingTiles2= false;
            bonusCounter2= 0;
            bonusThreshold2= 10;
            bonusMultiplier2= 2;
            powerUpsAvailable2= 0;
            nextPowerUpScore2= 50;
            powerUpActive2= false;
           
        }

        while(window.isOpen() && gameStart && !returnToMainMenu)
		{
            float time = clock.getElapsedTime().asSeconds();
            clock.restart();
            timer += time;

            int elapsedSeconds = static_cast<int>(gameClock.getElapsedTime().asSeconds());
            updateEnemyPatterns(a,enemyCount);

            if(elapsedSeconds >= lastSpeedIncreaseTime + 20)
			{
                lastSpeedIncreaseTime = elapsedSeconds;
                if(LevelSelection == 3)
				{
                    enemyCount += 2;
                }
                speedMultiplier += 1.0f;
            }

            if(powerUpActive1 && powerUpClock1.getElapsedTime().asSeconds() >= 3.0f)
			{
                powerUpActive1 = false;
                for(int i = 0; i < enemyCount; i++)
				{
                    a[i].frozen = false;
                }
            }

            if(powerUpActive2 && powerUpClock2.getElapsedTime().asSeconds() >= 3.0f)
			{
                powerUpActive2 = false;
                for(int i = 0; i<enemyCount; i++)
				{
                    a[i].frozen = false;
                }
            }

            Event e;
            while(window.pollEvent(e))
			{
                if(e.type == Event::Closed)
                    window.close();

                if(e.type == Event::KeyPressed)
				{
                    if (e.key.code == Keyboard::Escape)
					{
                        for(int i = 1; i < M - 1; i++)
                            for(int j = 1; j < N - 1; j++)
                                grid[i][j] = 0;

                        x1 = 10;
                        y1 = 0;
                        Game = true;
                        score1 = 0;
                        moveCount1 = 0;
                        buildingTiles1 = false;
                        bonusCounter1 = 0;
                        bonusThreshold1 = 10;
                        bonusMultiplier1 = 2;
                        powerUpsAvailable1 = 0;
                        nextPowerUpScore1 = 50;
                        powerUpActive1 = false;
                        patternsActivated = false;
                        x2 = 30;
                        y2 = 25;
                        score2 = 0;
                        moveCount2 = 0;
                        buildingTiles2 = false;
                        bonusCounter2 = 0;
                        bonusThreshold2 = 10;
                        bonusMultiplier2 = 2;
                        powerUpsAvailable2 = 0;
                        nextPowerUpScore2 = 50;
                        powerUpActive2 = false;

                        // Reset to linear 
                        for(int i = 0; i < enemyCount; i++)
						{
                            a[i].setPattern(LINEAR_PATTERN);
                        }

                        // Reset game timing variables
                        gameClock.restart();
                        patternClock.restart();
                        speedIncreaseClock.restart();
                        speedMultiplier = 2.0f;
                        lastSpeedIncreaseTime = 0;
                    }
                    if(e.key.code == Keyboard::Space && Game && !player1Dead)
					{
                        if(powerUpsAvailable1 > 0 && !powerUpActive1)
						{
                            activatePowerUp1(a, enemyCount);
                        }
                    }
                    if(e.key.code == Keyboard::F && Game && Mode == 1 && !player2Dead)
					{
                        if(powerUpsAvailable2 > 0 && !powerUpActive2)
						{
                            activatePowerUp2(a, enemyCount);
                        }
                    }
                }
            }

            // Player 1 movement
            if(Game && !player1Dead)
			{
                if(Keyboard::isKeyPressed(Keyboard::Left)) {dx1 = -1; dy1 = 0;}
                if(Keyboard::isKeyPressed(Keyboard::Right)) {dx1 = 1; dy1 = 0;}
                if(Keyboard::isKeyPressed(Keyboard::Up)) {dx1 = 0; dy1 = -1;}
                if(Keyboard::isKeyPressed(Keyboard::Down)) {dx1 = 0; dy1 = 1;}
            }

            // only in 2p mode
            if(Game && Mode == 1 && !player2Dead)
			{
                if(Keyboard::isKeyPressed(Keyboard::A)) { dx2 = -1; dy2 = 0;}
                if(Keyboard::isKeyPressed(Keyboard::D)) { dx2 = 1; dy2 = 0;}
                if(Keyboard::isKeyPressed(Keyboard::W)) { dx2 = 0; dy2 = -1;}
                if(Keyboard::isKeyPressed(Keyboard::S)) { dx2 = 0; dy2 = 1;}
            }

            // Game over 
            if(!Game)
			{
                static bool gameOverPlayed = false;
                if(!gameOverPlayed)
				{
                    gameOverSound.play();
                    gameOverSound.setVolume(130);
                    gameOverPlayed = true;
                }
               
                displayEndMenu(window, myfont,Game, score1,score2, returnToMainMenu, a, enemyCount,x1, y1, x2, y2,Mode);
               
                if(Game)
				{
                    gameOverPlayed = false;
                    player1Dead = false;  
                    if(Mode == 1) player2Dead = false;  
                     continue;
                }

                if(returnToMainMenu)
				{
                    gameStart = false;
                    gameOverPlayed = false;
                    break;
                }

                if(Game)
				{
                    continue;
                }
            }

            // player movements update
            if(Game && timer > delay)
			{
                // Player 1 movement
                if(!player1Dead)
				{
                    x1 += dx1;
                    y1 += dy1;

                    if(x1 < 0) x1 = 0; if(x1 > N - 1) x1 = N - 1;
                    if(y1 < 0) y1 = 0; if(y1 > M - 1) y1 = M - 1;

                    // Check collisions with player 2 or their constructing tiles
                    if(Mode == 1 && !player2Dead)
					{
                        if(x1 == x2 && y1 == y2 && (buildingTiles1 || buildingTiles2))
						{
                            player1Dead = true;
                            player2Dead = true;
                        } else if(grid[y1][x1] == 2 && buildingTiles2)
						{
                            player1Dead = true;
                        }
                    }

                    if(grid[y1][x1] == 2)
					{
                        player1Dead = true;
                    }

                    if(grid[y1][x1] == 0 && !buildingTiles1)
					{
                        buildingTiles1 = true;
                        tilesFilledInMove1 = 0;
                        moveCount1++;
                    }

                    if(grid[y1][x1] == 0)
					{
                        grid[y1][x1] = 2;
                        tilesFilledInMove1++;
                        if(tilesFilledInMove1 % 5 == 0)
						{
                            captureTilesSound.setVolume(60);
                            captureTilesSound.play();
                        }
                    }
                }

                // Player 2 movement 
                if(Mode == 1 && !player2Dead)
				{
                    x2 += dx2;
                    y2 += dy2;

                    if(x2 < 0) x2 = 0; if(x2 > N - 1) x2 = N - 1;
                    if(y2 < 0) y2 = 0; if(y2 > M - 1) y2 = M - 1;

                    // Check collisions with player 1 
                    if(!player1Dead)
					{
                        if(x2 == x1 && y2 == y1 && (buildingTiles2 || buildingTiles1))
						{
                            player1Dead = true;
                            player2Dead = true;
                        }else if(grid[y2][x2] == 2 && buildingTiles1)
						{
                            player2Dead = true;
                        }
                    }

                    if (grid[y2][x2] == 2) {
                        player2Dead = true;
                    }

                    if(grid[y2][x2] == 0 && !buildingTiles2)
					{
                        buildingTiles2 = true;
                        tilesFilledInMove2 = 0;
                        moveCount2++;
                    }

                    if(grid[y2][x2] == 0)
					{
                        grid[y2][x2] = 2;
                        tilesFilledInMove2++;
                        if(tilesFilledInMove2 % 5 == 0)
						{
                            captureTilesSound.setVolume(60);
                            captureTilesSound.play();
                        }
                    }
                }

                timer = 0;
            }

            // Check if both players are dead
            if(Mode == 1 && player1Dead && player2Dead)
			{
                Game = false;
                
            }

            // Update enemy movements
            for(int i = 0; i < enemyCount; i++) a[i].move(time);

            // Handle player returning to border
            bool player1Returned = false;
            bool player2Returned = false;

            if(!player1Dead && grid[y1][x1] == 1 && (dx1 != 0 || dy1 != 0))
			{
                dx1 = dy1 = 0;
                player1Returned = true;

                int filledTiles = tilesFilledInMove1;
                if(buildingTiles1 && filledTiles > bonusThreshold1){
                    score1 += filledTiles * bonusMultiplier1;
                    bonusSound.play();
                    bonusSound.setVolume(70);
                    bonusCounter1++;
                    if(bonusCounter1 >= 5)
					{
                        bonusThreshold1 = 5;
                        bonusMultiplier1 = 4;
                    }else if(bonusCounter1 >= 3)
					{
                        bonusThreshold1 = 5;
                        bonusMultiplier1 = 2;
                    }
                }else if(buildingTiles1)
				{
                    score1 += filledTiles;
                }
                checkForPowerUp1();
                if(buildingTiles1)
				{
                    buildingTiles1 = false;
                    tilesFilledInMove1 = 0;
                }
            }

            if(Mode == 1 && !player2Dead && grid[y2][x2] == 1 && (dx2 != 0 || dy2 != 0))
			{
                dx2 = dy2 = 0;
                player2Returned = true;

                int filledTiles = tilesFilledInMove2;
                if(buildingTiles2 && filledTiles > bonusThreshold2)
				{
                    score2 += filledTiles * bonusMultiplier2;
                    bonusSound.play();
                    bonusSound.setVolume(70);
                    bonusCounter2++;
                    if(bonusCounter2 >= 5)
					{
                        bonusThreshold2 = 5;
                        bonusMultiplier2 = 4;
                    } else if(bonusCounter2 >= 3)
					{
                        bonusThreshold2 = 5;
                        bonusMultiplier2 = 2;
                    }
                }else if(buildingTiles2)
				{
                    score2 += filledTiles;
                }
                checkForPowerUp2();
                if(buildingTiles2)
				{
                    buildingTiles2 = false;
                    tilesFilledInMove2 = 0;
                }
            }

            // update the grid
            // if player returns to border
            if(player1Returned || player2Returned)
			{
                for(int i = 0; i < enemyCount; i++)
                    drop(a[i].y / ts, a[i].x / ts);

                for(int i = 0; i < M; i++)
				{
                    for(int j = 0; j < N; j++)
					{
                        if(grid[i][j] == -1) grid[i][j] = 0;
                        else if(grid[i][j] != 2) grid[i][j] = 1;
                        if(grid[i][j] == 2) grid[i][j] = 1;
                    }
                }
            }

            // Checking if enemies hit player trails
            for(int i = 0; i < enemyCount; i++)
			{
                if(grid[a[i].y / ts][a[i].x / ts] == 2)
				{
                    // If enemy hits any trail tile belonging to player 1
                    if(!player1Dead && buildingTiles1)
					{
                        player1Dead = true;
                    }
                    // If enemy hits any trail tile belonging to player 2
                    if(Mode == 1 && !player2Dead && buildingTiles2)
					{
                        player2Dead = true;
                    }
                }
            }

            // Draw everything
            if(powerUpActive1||powerUpActive2)
			{
                window.clear(powerupBackgroundColor);
            } else{
                window.clear(defaultBackgroundColor);
            }

            // Draw grid tiles
            for(int i = 0; i < M; i++)
			{
                for(int j = 0; j < N; j++)
				{
                    if(grid[i][j] == 0) continue;
                    if(grid[i][j] == 1) sTile.setTextureRect(IntRect(0, 0, ts, ts));
                    if(grid[i][j] == 2) sTile.setTextureRect(IntRect(54, 0, ts, ts));
                    sTile.setPosition(j*ts,i*ts);
                    window.draw(sTile);
                }
            }

            // Draw player 1 (if alive)
            if(!player1Dead)
			{
                sTile.setTextureRect(IntRect(36, 0, ts, ts));
                sTile.setPosition(x1 * ts, y1 * ts);
                window.draw(sTile);
            }

            // Draw player 2 (if alive)
            if(Mode == 1 && !player2Dead)
			{
                sTile.setTextureRect(IntRect(18, 0, ts, ts)); // Diff color for player 2
                sTile.setPosition(x2 * ts, y2 * ts);
                window.draw(sTile);
            }

            // Draw enemies
            for(int i = 0; i < enemyCount; i++)
			{
                if(a[i].frozen)
				{
                    sEnemy.setColor(Color::White);
                } else
				{
                    if(a[i].pattern == LINEAR_PATTERN)
					{
                        sEnemy.setColor(Color::White);
                    } else if(a[i].pattern == SQUARE_PATTERN)
					{
                        sEnemy.setColor(Color::White);
                    } else if(a[i].pattern == CIRCULAR_PATTERN)
					{
                        sEnemy.setColor(Color::White);
                    }
                }

                if(!a[i].frozen)
				{
                    sEnemy.rotate(5);
                }

                sEnemy.setPosition(a[i].x, a[i].y);
                window.draw(sEnemy);
            }

            // game over will appear  if game is over
            if((Mode == 0 && player1Dead) || (Mode == 1 && player1Dead && player2Dead))
			{
                Game = false;
            }

            // Draw score bar
            RectangleShape scoreBar(Vector2f(N * ts, 80));
            scoreBar.setFillColor(Color(0, 0, 0, 200));
            scoreBar.setPosition(0, M * ts);
            window.draw(scoreBar);

            // Update text elements
            T_Level.setString("Level: "+to_string(level));
            T_Score1.setString("P1: "+to_string(score1));
            T_Moves1.setString("Moves: "+to_string(moveCount1));
            T_Timer.setString("Time: "+to_string(elapsedSeconds));
            T_Speed.setString("Speed: "+to_string(int(speedMultiplier * 100)));
            T_PowerUps1.setString("Power: "+to_string(powerUpsAvailable1));

            if(Mode == 1)
			{
                T_Score2.setString("P2: "+to_string(score2));
                T_Moves2.setString("Moves: "+to_string(moveCount2));
                T_PowerUps2.setString("Power: "+to_string(powerUpsAvailable2));
            }

            // Update bonus and power-up displays
            //T_Bonus.setString("Bonus: x" + std::to_string(bonusMultiplier));
            // Update pattern status display


            if(powerUpActive1)
			{
                float timeLeft = 3.0f - powerUpClock1.getElapsedTime().asSeconds();
                T_PowerUpActive.setString("FREEZE: " + std::to_string(int(timeLeft + 0.5f)) + "s");
            }else if(powerUpActive2)
			{
                float timeLeft = 3.0f - powerUpClock2.getElapsedTime().asSeconds();
                T_PowerUpActive.setString("FREEZE: " + std::to_string(int(timeLeft + 0.5f)) + "s");
            }
            else{
                T_PowerUpActive.setString("");
            }


            // Draw all text elements
            window.draw(T_Level);
            window.draw(T_Score1);
            window.draw(T_Moves1);
            window.draw(T_Timer);
            window.draw(T_Speed);
            window.draw(T_PowerUps1);
           window.draw(T_PowerUpActive);
           
            if(Mode == 1)
			{
                window.draw(T_Score2);
                window.draw(T_Moves2);
                window.draw(T_PowerUps2);
            }

            window.display();
        }
    }

    return 0;
}

