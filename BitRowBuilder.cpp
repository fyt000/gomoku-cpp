#include "BitRowBuilder.h"


BitRowBuilder::BitRowBuilder()
{
	row = EMPTYROW;
}

int BitRowBuilder::getRow()
{
	return row;
}

void BitRowBuilder::reset()
{
	row = EMPTYROW;
}

void BitRowBuilder::add(bool isEmpty)
{
	row <<= 1;
	row ^= (isEmpty ? 0 : 1);
}