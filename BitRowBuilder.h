#pragma once

class BitRowBuilder {
public:
	BitRowBuilder();
	int getRow();
	void reset();
	void add(bool isEmpty);

	//add the following to do sub row eval in this project
	//isSet
	//splice
	//length
	//startsWith
	//reverse
	//currently will use a python script to do sub row eval
	
private:
	const int EMPTYROW = 1;
	int row;
};