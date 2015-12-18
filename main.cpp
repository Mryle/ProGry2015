#include <array>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <string>
#include <vector>

/*
@ - hero
# - wall
. - floor
##########
#........#
#........#
*/

const char Hero = '@';
const char Wall = '#';
const char Floor = '.';
const char Door = '+';

//const-correctness!

class MapGenerator;

class Creature {
public:
	int pos() const;

	void setPos(int position);

private:
	int pos_;
};

int Creature::pos() const
{
	return pos_;
}

void Creature::setPos(int position)
{
	pos_ = position;
}

class Player : public Creature {
};

class Level {
	friend class MapGenerator;
	friend std::istream & operator >>(std::istream &is, Level &level);
	friend std::ostream & operator <<(std::ostream &os, Level &level);
public:

	int cols() const;
	int rows() const;
	int heroStartPos() const;
	void display();
	bool readFromFile(const std::string &fileName);

	bool moveHero(int a, int b);

	bool canSee(int posA, int posB) const;
private:

	Level() = default;
	Level(char *data, int rows, int cols);

	int cols_;
	int rows_;
	int heroStartPos_;
	char *data_;
};

Level::Level(char *data, int rows, int cols)
	: cols_(cols), rows_(rows), data_(data)
{
}

int Level::cols() const
{
	return cols_;
}

int Level::rows() const
{
	return rows_;
}

int Level::heroStartPos() const
{
	return heroStartPos_;
}

void Level::display()
{
	for (int i = 0; i < rows_; ++i) {
		for (int j = 0; j < cols_; ++j)
			mvaddch(i, j, data_[i * cols_ + j]);
	}
}

bool Level::readFromFile(const std::string &fileName)
{
	std::ifstream fileStream(fileName);
	if (!fileStream.is_open()) {
		std::cerr << "Error opening file " << fileName << '\n';
		return false;
	}

	fileStream >> (*this);

	return true;
}

std::istream & operator >>(std::istream &is, Level &level)
{
	is >> level.rows_ >> level.cols_ >> level.heroStartPos_;

	is >> std::noskipws;
	level.data_ = new char[level.rows_ * level.cols_];

	char c;
	while (std::isspace(is.peek()))
		is >> c;

	for (int i = 0; i < level.rows_; ++i) {
		is.read(level.data_ + i * level.cols_, level.cols_);
		while (std::isspace(is.peek()))
			is >> c;
	}

	return is;
}

std::ostream & operator <<(std::ostream &os, Level &level)
{
	os << level.rows_ << " " << level.cols_ << " " << level.heroStartPos_ << '\n';
	for (int i = 0; i < level.rows_ ; ++i) {
		os.write(level.data_ + i * level.cols_, level.cols_);
		os << '\n';
	}

	return os;
}

bool Level::moveHero(int a, int b)
{
	if (data_[a] != Hero || data_[b] != Floor) //for now assuming walking on floor only
		return false;

	data_[a] = Floor;
	data_[b] = Hero;

	return true;
}

bool Level::canSee(int posA, int posB) const
{
	int aCol = posA % cols_;
	int aRow = posA / cols_;
	int bCol = posB % cols_;
	int bRow = posB / cols_;
	int deltaCol = bCol - aCol;
	int deltaRow = bRow - aRow;
	int deltaColSign = (deltaCol > 0) - (deltaCol < 0);
	int deltaRowSign = (deltaRow > 0) - (deltaRow < 0);
	std::vector<int> vec;

	if (deltaCol == 0)
		for (int row = aRow + deltaRowSign; row != bRow; row += deltaRowSign)
			vec.push_back((row * cols_) + aCol);
	else if (abs(deltaCol) >= abs(deltaRow))
		for (int col = deltaColSign; col + aCol != bCol; col += deltaColSign) {
			int calcRow = (2 * col * deltaRow + (deltaRowSign * deltaCol)) / (2 * deltaCol) + aRow;
			vec.push_back(calcRow * cols_ + aCol + col);
		}
	else
		for (int row = deltaRowSign; row + aRow != bRow; row += deltaRowSign) {
			int calcCol = (2 * row * deltaCol + (deltaColSign * deltaRow)) / (2 * deltaRow) + aCol;
			vec.push_back((row + aRow) * cols_ + calcCol);
		}

	for (int i : vec)
		if (data_[i] != Floor)
			return false;
	return true;
}

class MapGenerator {
public:
	Level createLevel(int seed = -1)
	{
		if (seed == -1)
			srand(time(nullptr));
		else
			srand(seed);

		rows_ = 40;
		cols_ = 80;
		canvas_ = new char[rows_ * cols_];

		for(int i = 0; i < rows_; i++)
			for (int j = 0; j < cols_; j++)
				canvas_[i * cols_ + j] = Wall;

		int prevCol, prevRow, currRow, currCol;

		for (int i = 0; i < 10; i++) {
			prevRow = currRow;
			prevCol = currCol;

			currRow = rand() % (rows_ / 2 - 1);
			currRow = currRow * 2 + 1;
			currCol = rand() % (cols_ / 2 - 1);
			currCol = currRow * 2 + 1;
			int height = rand() % 7 + 3;
			int width = rand() % 7 + 3;
			createRectangularRoom(currRow, currCol, width, height);

			// Brutally add corridors

			int dir;
			if (i > 0) {
				dir = prevRow > currRow ? -1 : 1;

				// Horizontal
				while (prevRow != currRow) {
					prevRow += dir;
					int tmp = prevRow * cols_ + prevCol;
					if (canvas_[tmp] == Wall) {
						if (canvas_[tmp - 1] != Floor && canvas_[tmp + 1] != Floor)
							canvas_[tmp] = Corridor;
						else
							canvas_[tmp] = Floor;
					}
				}

				// Vertical
				dir = prevCol > currCol ? -1 : 1;
				while (prevCol != currCol) {
					prevCol += dir;
					int tmp = prevRow * cols_ + prevCol;
					if (canvas_[tmp] == Wall) {
						if (canvas_[tmp - cols_] != Floor && canvas_[tmp + cols_] != Floor)
							canvas_[tmp] = Corridor;
						else
							canvas_[tmp] = Floor;
					}
				}
			}
		}

		createDoors();

		clearCorridors();

		canvas_[prevRow * cols_ + prevCol] = Hero;
		Level result(canvas_, rows_, cols_);
		result.heroStartPos_ = prevRow * cols_ + prevCol;
		return result;
	}

private:

	static const char Corridor = 'C';

	char *canvas_;
	int rows_;
	int cols_;

	inline std::array<int, 8> directions() const
	{
		return std::array<int, 8> {{-1, 1, cols_, -cols_, 1-cols_, 1+cols_, -1-cols_, -1+cols_}};
	}
	inline std::array<int, 4> ortogonalDirections() const
	{
		return std::array<int, 4> {{-1, 1, cols_, -cols_}};
	}

	void createRectangularRoom(int posCol, int posRow, int width, int height)
	{
		width  = std::min(width, cols_ - posRow - 1);
		height = std::min(height, rows_ - posCol - 1);
		posCol = std::max(posCol, 1);
		posRow = std::max(posRow, 1);
		for (int i = posCol; i < posCol + height; ++i)
			for (int j = posRow; j < posRow + width; ++j)
				canvas_[i * cols_ + j] = Floor;
	}

	void createDoors()
	{
		std::vector<int> doors;
		std::vector<int> clear;
		for (int i = 0; i < cols_ * rows_; ++i) {
			int adjacent = 0;
			for (auto direction : ortogonalDirections()) {
				if (i + direction >= 0 && i + direction <= cols_ * rows_ && canvas_[i + direction] == Corridor)
					adjacent++;
			}

			if (canvas_[i] == Corridor) {
				if (adjacent == 1)
					doors.push_back(i);
				else if (adjacent == 0)
					clear.push_back(i);
			}
		}

		for (auto i : clear) {
			// Adjacent rooms with really short corridor
			if (canvas_[i + 1] == canvas_[i - 1])
				doors.push_back(i);
			else
				canvas_[i] = Floor;
		}

		for (auto i : doors) {
			// Additional check for corridors crossing in a room
			int adjacent = 0;
			for (auto direction : ortogonalDirections()) {
				if (canvas_[direction + i] == Floor || canvas_[direction + i] == Corridor)
					adjacent++;
			}

			if (adjacent < 3)
				canvas_[i] = Door;
		}
	}

	void clearCorridors()
	{
		for (int i = 0; i < cols_* rows_; ++i)
			if (canvas_[i] == Corridor)
				canvas_[i] = Floor;
	}
};

int main()
{
	initscr();
	noecho();
	MapGenerator gen;
	Level level = gen.createLevel();

	Player player;
	player.setPos(level.heroStartPos());

	std::cout << "Simple game loop, wsad - moving, q - quit\n";
	level.display();

	while (true) {
		char c;
		c = getch();
		int posOffset = 0;
		switch (c) {
			case 'q': {
				std::ofstream save("save.txt");
				save << level;
				endwin();
				return 0;
			}
			case 'w':
				posOffset = -level.cols();
				break;
			case 's':
				posOffset = +level.cols();
				break;
			case 'a':
				posOffset = -1;
				break;
			case 'd':
				posOffset = +1;
				break;
			case 't': //canSee test
				move(level.rows(), 0);
				refresh();
				if (level.canSee(level.heroStartPos(), player.pos()))
					printf("widać ze startu    \n");
				else
					printf("nie widać ze startu\n");
				break;
		}
		if (posOffset != 0) {
			if (level.moveHero(player.pos(), player.pos() + posOffset))
				player.setPos(player.pos() + posOffset);
			else
			;	//TODO not valid move
		}
		level.display();
	}
	endwin();
	return 0;
}
