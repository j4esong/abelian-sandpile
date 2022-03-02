
#include "sandpile.hpp"

Sandpile::Sandpile(int width, int height)
	: width(width), height(height), drops(0), capacity(0), center(true), currentDepth(-1), size(0)
{
	plate = std::vector<std::vector<int>>(width, std::vector<int>(height, 0));
	std::srand(time(0));
}

/*
advance simulation by one layer.
use two queues, one for the cells affected by a collapse and one to keep track of the depths of each affected cell.
each update, iterate over all of the affected cells of depth = currentDepth and process them.
use another queue to store the positions of the collapsing cells;
we have to clean those up in the next update for them to be shown on the current update.
*/
void Sandpile::update()
{
	if (affectedCells.size() == 0) {
		size = 0;
		currentDepth = 0;
		if (center)
			dropOne(width / 2, height / 2, 0, false);
		else
			dropOne(std::rand() % width, std::rand() % height, 0, false);
	}

	while (collapsingCells.size() > 0) {
		std::pair<int, int> pos = collapsingCells.front();
		plate[pos.first][pos.second] = plate[pos.first][pos.second] % 4;
		collapsingCells.pop();
	}

	while (affectedCells.size() > 0 && depths.front() == currentDepth) {

		std::pair<int, int> dim = affectedCells.front();
		int depth = depths.front();

		affectedCells.pop();
		depths.pop();

		int x = dim.first, y = dim.second;

		plate[x][y]++;

		if (plate[x][y] % 4 == 0) {
			//assume that all 4 will fall off, and then make it back up
			size += 4;
			capacity -= 4;
			collapsingCells.push({x, y});
			if (y != 0)
				dropOne(x, y - 1, depth + 1, true);
			if (y != height - 1)
				dropOne(x, y + 1, depth + 1, true);
			if (x != 0)
				dropOne(x - 1, y, depth + 1, true);
			if (x != width - 1)
				dropOne(x + 1, y, depth + 1, true);
		}
	}
	currentDepth = depths.front();
}

void Sandpile::fillRand()
{
	capacity = 0;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			plate[i][j] = std::rand() % 4;
			capacity += plate[i][j];
		}
	}
	resetQueues();
}

void Sandpile::fillValue(int n)
{
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
			plate[i][j] = n;
	capacity = width * height * n;
	resetQueues();
}

void Sandpile::resize()
{
	if (width > 1000 || height > 1000)
		throw std::invalid_argument("too large!");
	plate = std::vector<std::vector<int>>(width, std::vector<int>(height, 0));
	resetQueues();
}

void Sandpile::dropOne(int x, int y, int depth, bool collapse)
{
	affectedCells.push({x, y});
	depths.push(depth);
	if (collapse)
		size--;
	capacity++;
}

void Sandpile::resetQueues()
{
	std::queue<std::pair<int, int>>().swap(affectedCells);
	std::queue<std::pair<int, int>>().swap(collapsingCells);
	std::queue<int>().swap(depths);
}