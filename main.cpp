// Written By: Anu Panicker
// Github : the-mastercoder
// Insta : anupanicker.ig
// Dated : 14 - OCT - MMXIX
//Licence : MIT

#include <algorithm>
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>

#include "SFML/Audio.hpp"
#include "SFML/Graphics.hpp"

#define BACKBROUND_RGB sf::Color(73,95,105)
#define SNAKE_RGB sf::Color(192,192,192)
#define FOOD_RGB sf::Color(255,192,203)
#define SCOREBOARD_RGB sf::Color(192,188,101)

constexpr size_t winWidth  = 700;
constexpr size_t winHeight = 600;

bool checkInterection(const std::deque<sf::Vector2f>& snakeSegment, const sf::Vector2f& objectPosition, const size_t& objectFirstRadius, std::optional<size_t> objectSecondRadius) {
	if (objectSecondRadius == std::nullopt) {
		objectSecondRadius = objectFirstRadius;
	}
	auto checkForIntersection = [objectPosition,objectFirstRadius,objectSecondRadius](sf::Vector2f snakeSegment) ->bool {
		float distanceSquare = (objectPosition.x - snakeSegment.x) * (objectPosition.x - snakeSegment.x) +
			                   (objectPosition.y - snakeSegment.y) * (objectPosition.y - snakeSegment.y);
		float radiusSquare   = (objectFirstRadius + objectSecondRadius.value()) * (objectFirstRadius + objectSecondRadius.value());

		if (objectFirstRadius == objectSecondRadius) {
			radiusSquare -= 0.1f;
		}
		return(distanceSquare <= radiusSquare) ? true : false;
	};
	return std::any_of(snakeSegment.begin()+3,snakeSegment.end(),checkForIntersection);
}

std::pair<float, float> genetrateRandomCoordinates(const size_t objectRadius) {
	std::random_device seed{};
	std::mt19937 mtGenerator(seed());
	std::uniform_real_distribution<float> randomXaxis(objectRadius, ::winWidth - objectRadius);
	std::uniform_real_distribution<float> randomYaxis(objectRadius, ::winHeight - objectRadius);
	return{ randomXaxis(mtGenerator),randomYaxis(mtGenerator) };
}

class Snake : public sf::Drawable {

	void draw(sf::RenderTarget& target, sf::RenderStates states) const override{
	target.draw(this->snakeBody, states);
	}

public:
    size_t snakeLength = 3;
	const size_t snakeRadius = 12;
	const float snakeSpeed = 2 * snakeRadius;
	sf::CircleShape snakeBody;
	std::deque<sf::Vector2f> snakePosition;
	std::pair<sf::Vector2f, sf::Vector2f> snakeVelocity = { {0,0},{0,0} };
	const bool snakeFoodFeedback = false;
	bool snakeFoodStatus = false;
	bool selfCollision = false;
 
	std::unique_ptr<sf::SoundBuffer> eatSoundBuffer  = std::make_unique<sf::SoundBuffer>();
	std::unique_ptr<sf::SoundBuffer> failSoundBuffer = std::make_unique<sf::SoundBuffer>();
	sf::Sound eatingSound;

	Snake(){
	  snakeBody.setRadius(snakeRadius);
	  snakeBody.setFillColor(SNAKE_RGB);
	  auto [snakeXaxis, snakeYaxis] = genetrateRandomCoordinates(snakeRadius);
	  snakePosition.emplace_front(sf::Vector2f(snakeXaxis, snakeYaxis));
		while (snakeLength-- > 1) {
			if ([]()->float {std::random_device seed{}; std::mt19937 mtGenerator(seed());
				             std::uniform_real_distribution<float> random(0,1); return random(mtGenerator); } () > 0.5) {
				if (snakeYaxis - (4 * snakeRadius) > 0 ) {
					snakePosition.emplace_front(sf::Vector2f(snakePosition.front() - sf::Vector2f(0, 2 * snakeRadius)));
					if (snakePosition.size() == snakeLength) {
						snakeVelocity.first = { 0,-snakeSpeed };
					}
				}
				else if (snakeYaxis - (4 * snakeRadius) < 0) {
					snakePosition.emplace_front(sf::Vector2f(snakePosition.front() + sf::Vector2f(0, 2 * snakeRadius)));
					if (snakePosition.size() == snakeLength) {
						snakeVelocity.first = { 0,+snakeSpeed };
					}
				}
			}
			else{
				if (snakeXaxis - (4 * snakeRadius) > 0 ) {
					snakePosition.emplace_front(sf::Vector2f(snakePosition.front() - sf::Vector2f(2 * snakeRadius,0)));
					if (snakePosition.size() == snakeLength) {
						snakeVelocity.first = { -snakeSpeed,0 };
					}

				}
				else if (snakeXaxis - (4 * snakeRadius) < 0) {
					snakePosition.emplace_front(sf::Vector2f(snakePosition.front() + sf::Vector2f(+2 * snakeRadius, 0)));
					if (snakePosition.size() == snakeLength) {
						snakeVelocity.first = { +snakeSpeed,0 };
					}
				}
			}
		}
		snakeLength = 3;

		if (eatSoundBuffer->loadFromFile("eat.wav")) {
			eatingSound.setBuffer(*eatSoundBuffer);
		}
	}
	
	void updatePosition() {
		
		if (snakeVelocity.second != sf::Vector2f(0,0)) {
			if (!(snakeVelocity.first.x + snakeVelocity.second.x == 0 && snakeVelocity.first.y + snakeVelocity.second.y == 0)) {
				snakeVelocity.first = snakeVelocity.second;
				snakeVelocity.second = { 0,0 };
			}
		}

		auto updateBound = [](sf::Vector2f& snakeBound) -> void {
			if (snakeBound.x > winWidth) {
				snakeBound.x = 0;
			}
			else if (snakeBound.x < 0) {
				snakeBound.x = winWidth;
			}

			if (snakeBound.y > winHeight) {
				snakeBound.y = 0;
			}
			else if (snakeBound.y < 0) {
				snakeBound.y = winHeight;
			}
		}; std::for_each(snakePosition.begin(),snakePosition.end(),updateBound);

		if (checkInterection(snakePosition,snakePosition.front(),snakeRadius,std::nullopt)) {
			selfCollision = true;
		}
		else {
			snakePosition.emplace_front(sf::Vector2f(snakePosition.front() + snakeVelocity.first));
			if (!snakeFoodStatus) {
				snakePosition.pop_back();
			}
			else {
				snakeLength++;
				eatingSound.play();
			}
		}
	}
};

class Food :public sf::Drawable {

	const float foodRoation = 45.0f;

	void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(this->food, states);
	}

public:
	sf::RectangleShape food;
	sf::Vector2f foodPosition;
	const sf::Vector2f foodSize = { 12.5,12.5 };
	const size_t foodRadius = (foodSize.x + foodSize.y) / 2*sqrt(2);

	bool snakeAteFood = false;
	std::unique_ptr<Snake> snakeHWND = std::make_unique<Snake>();

	Food(Snake* snakeHWND):snakeHWND(snakeHWND){
		food.setSize(foodSize);
		food.setFillColor(FOOD_RGB);
		food.setRotation(foodRoation);
		do {
			auto [foodXaxis, foodYaxis] = genetrateRandomCoordinates((foodSize.x + foodSize.y)/2*sqrtf(2));
			foodPosition = sf::Vector2f(foodXaxis,foodYaxis);
			food.setPosition(foodPosition);
		} while (checkInterection(snakeHWND->snakePosition,foodPosition,snakeHWND->snakeRadius,foodRadius));

		//specialFood.setRadius(specialFoodRadius);
		//specialFood.setFillColor(SPECIALFOOD_RGB);
	}

	void ateBySnake() {
		if(food.getGlobalBounds().intersects(snakeHWND->snakeBody.getGlobalBounds())){
			do {
				auto [foodXaxis, foodYaxis] = genetrateRandomCoordinates((foodSize.x + foodSize.y)/2*sqrtf(2));
				foodPosition = sf::Vector2f(foodXaxis, foodYaxis);
				food.setPosition(foodPosition);
			} while (checkInterection(snakeHWND->snakePosition, foodPosition, snakeHWND->snakeRadius, foodRadius));
			snakeAteFood = true;
		}
	}

	explicit Food(const Food& masterObject) = delete;
	explicit Food(Food&& masterObject) noexcept(true) = delete;
    Food& operator = (const Food& masterObject) = delete;
	Food& operator = (Food&& masterObject) noexcept(true) = delete;

	~Food() = default;
};

class Scorecard:public sf::Drawable {

	sf::Font DIGITAL;

	sf::Text score;
	sf::Text highScore;
	sf::Text snakeLength;
	sf::Text level;

	sf::Vector2f scorePosition         = {winWidth - 150,winHeight - 590};
	sf::Vector2f highScorePosition     = {winWidth - 205,winHeight - 560};
	sf::Vector2f snakeLenghthPosition  = {winWidth - 242,winHeight - 530};
	sf::Vector2f levelPosition         = {winWidth - 150,winHeight - 500};

	const std::string scoreLabel       = "SCORE: ";
	const std::string highScoreLabel   = "HIGH SCORE: ";
	const std::string snakeLengthLabel = "SNAKE LENGTH: ";
	const std::string levelLabel       = "LEVEL: ";

	std::string scoreValue             = "0";
	std::string highScoreValue         = "0";
	std::string snakeLengthValue       = "0";
	std::string levelValue             = "1";

	const std::string dataFileName = "snake.txt";
	std::fstream snakeData;
	std::unique_ptr<Snake> snakeHWND = std::make_unique<Snake>();

	void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(this->score, states);
		target.draw(this->highScore, states);
		target.draw(this->snakeLength, states);
		target.draw(this->level, states);
	}

public:
	Scorecard(Snake* snakeHWND) :snakeHWND(snakeHWND) {

		if (DIGITAL.loadFromFile("digitalFont.ttf")) {
			score.setFont(DIGITAL);
			score.setFillColor(SCOREBOARD_RGB);
			score.setPosition(scorePosition);
			score.setString(scoreLabel);

			highScore.setFont(DIGITAL);
			highScore.setFillColor(SCOREBOARD_RGB);
			highScore.setPosition(highScorePosition);
			highScore.setString(highScoreLabel);

			snakeLength.setFont(DIGITAL);
			snakeLength.setFillColor(SCOREBOARD_RGB);
			snakeLength.setPosition(snakeLenghthPosition);
			snakeLength.setString(snakeLengthLabel);

			level.setFont(DIGITAL);
			level.setFillColor(SCOREBOARD_RGB);
			level.setPosition(levelPosition);
			level.setString(levelLabel);
		}
		
		if (!std::filesystem::exists(dataFileName)) {
			std::ofstream snakeTxt(dataFileName);
			snakeData.open(dataFileName);
			snakeData << "0";
			snakeData.close();
		}
		
		snakeData.open(dataFileName);
		if (snakeData.is_open()) {
			while (!snakeData.eof()) {
				std::getline(snakeData, highScoreValue);
			}
			snakeData.close();
		}
	}

	void update() {

         snakeLengthValue = std::to_string(snakeHWND->snakeLength);

         if (snakeHWND->snakeFoodStatus) {
		   switch (std::stoi(levelValue)) {
		   case 1: scoreValue = std::to_string(std::stoi(scoreValue) + 5);   break;
		   case 2: scoreValue = std::to_string(std::stoi(scoreValue) + 7);   break;
		   case 3: scoreValue = std::to_string(std::stoi(scoreValue) + 9);   break;
		   case 4: scoreValue = std::to_string(std::stoi(scoreValue) + 11);  break;
		   default: scoreValue = std::to_string(std::stoi(scoreValue) + 15); break;
		    }
 
		  if(snakeHWND->snakeLength % 25 == 0) {
			 levelValue = std::to_string(std::stoi(levelValue) + 1);
		    }

		  if (std::stoi(scoreValue) > std::stoi(highScoreValue)) {
			  highScoreValue.replace(highScoreValue.begin(),highScoreValue.end(),scoreValue);
		    }
		 }

		  if (snakeHWND->selfCollision) {

			  if (std::stoi(scoreValue) == std::stoi(highScoreValue)) {
			      snakeData.open(dataFileName, std::ios::trunc);
			      snakeData.close();
			      snakeData.open(dataFileName);
			  if (snakeData.is_open()) {
				  snakeData << scoreValue;
				  snakeData.close();
			    }
		      }
		  }

		score.setString(scoreLabel + scoreValue);
		highScore.setString(highScoreLabel + highScoreValue);
		snakeLength.setString(snakeLengthLabel + snakeLengthValue);
		level.setString(levelLabel + levelValue);
	}

	explicit Scorecard(const Scorecard& masterObject) = delete;
	explicit Scorecard(Scorecard&& masterObject) noexcept(true) = delete;
	Scorecard& operator = (const Scorecard& masterObject) = delete;
	Scorecard& operator = (Scorecard&& masterObject) noexcept(true) = delete;

	~Scorecard() = default;
};

int main() {
	sf::RenderWindow window(sf::VideoMode(::winWidth, ::winHeight), "Snake++", sf::Style::Titlebar | sf::Style::Close);
	sf::RectangleShape background(sf::Vector2f(::winWidth, ::winHeight));
	background.setFillColor(BACKBROUND_RGB);
	window.setFramerateLimit(11);

	Snake snake;
	Food food(&snake);
	Scorecard scorecard(&snake);

	while (window.isOpen()) {
		window.clear();
		window.draw(background);

		sf::Event event;
		while (window.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyPressed:
				switch (event.key.code) {
				case sf::Keyboard::Up:
				case sf::Keyboard::W:
					snake.snakeVelocity.second = { 0,-snake.snakeSpeed };
					break;

				case sf::Keyboard::Down:
				case sf::Keyboard::S:
					snake.snakeVelocity.second = { 0,+snake.snakeSpeed };
					break;

				case sf::Keyboard::Left:
				case sf::Keyboard::A:
					snake.snakeVelocity.second = { -snake.snakeSpeed,0 };
					break;

				case sf::Keyboard::Right:
				case sf::Keyboard::D:
					snake.snakeVelocity.second = { +snake.snakeSpeed,0 };
					break;
				}
				break;
			}
		}

		food.snakeAteFood = snake.snakeFoodFeedback;
		snake.updatePosition();
		food.ateBySnake();
		scorecard.update();
		snake.snakeFoodStatus = food.snakeAteFood;

		window.draw(food);
		for (auto reversePosition_it = snake.snakePosition.rbegin(); reversePosition_it != snake.snakePosition.rend(); ++reversePosition_it) {
			sf::Vector2f snakeCoordinate = *reversePosition_it;
			snake.snakeBody.setPosition(snakeCoordinate);
			window.draw(snake);
		}
		window.draw(scorecard);
		
		window.display();
	}
}
