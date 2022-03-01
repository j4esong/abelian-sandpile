
#ifndef SANDPILE_HPP
#define SANDPILE_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <vector>
#include <map>
#include <cstring>

class Sandpile
{
public:
	int width;
	int height;
	int drops;
	std::vector<std::vector<int>> plate;
	std::queue<std::pair<int, int>> affectedCells;
	std::queue<std::pair<int, int>> collapsingCells;
	std::queue<int> depths;
	bool center;
	Sandpile(int width, int height);
	void update();
	void fillRand();
	void fillValue(int n);
	void resize();
private:
	int currentDepth;
	int size;
	void dropOne(int x, int y, int depth, bool collapse);
	void resetQueues();
};

#endif