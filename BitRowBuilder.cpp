#include "BitRowBuilder.h"

#ifdef _MSC_VER
#include <intrin.h>
typedef unsigned long DWORD;
#endif

BitRowBuilder::BitRowBuilder()
{
	row = EMPTYROW;
}

BitRowBuilder::BitRowBuilder(int r)
{
	row = r;
	lenDirty = true;
}

int BitRowBuilder::getRow()
{
	return row;
}

void BitRowBuilder::reset()
{
	row = EMPTYROW;
}

BitRowBuilder& BitRowBuilder::add(bool isEmpty)
{
	row <<= 1;
	row ^= (isEmpty ? 0 : 1);
	lenDirty = true;
	return (*this);
}

BitRowBuilder & BitRowBuilder::add(int b)
{
	row <<= 1;
	row ^= b;
	lenDirty = true;
	return (*this);
}

int BitRowBuilder::length()
{
	if (lenDirty) {
		len = LengthOf(row);
	}
	return len;
}

void BitRowBuilder::inplaceReverse()
{
	row = GetReverse(row);
}

bool BitRowBuilder::isSet(int idx)
{
	return (row & (1 << (length() - 1 - idx))) != 0;
}

bool BitRowBuilder::IsSet(int someRow, int idx)
{
	return (someRow & (1 << (LengthOf(someRow) - 1 - idx))) != 0;
}

int BitRowBuilder::RowSplice(int someRow, int idx)
{
	int startLen = LengthOf(someRow);
	someRow &= ~(1 << startLen);
	for (int i = 0; i < idx; i++) {
		someRow &= ~(1 << startLen - 1 - i);
	}
	someRow |= 1 << (startLen - idx);
	return someRow;
}

bool BitRowBuilder::RowStartsWith(int someRow, int p)
{
	int len1 = LengthOf(someRow) + 1;
	int len2 = LengthOf(p) + 1;
	if (len1 < len2)
		return false;
	return (someRow >> (len1 - len2)) == p;
}

int BitRowBuilder::GetReverse(int someRow)
{
	int rlen = LengthOf(someRow);
	int backwardRow = EMPTYROW;
	for (int i = 0; i < rlen; i++) {
		if ((someRow & (1 << i)) != 0) {
			backwardRow = (backwardRow << 1) ^ 1;
		}
		else {
			backwardRow = (backwardRow << 1);
		}
	}
	return backwardRow;
}

int BitRowBuilder::LengthOf(int anotherRow)
{
	if (anotherRow == 0)
		return 0;

	int minToOne = 0;
#ifdef _MSC_VER
	DWORD leadingZero = 0;
	_BitScanReverse(&leadingZero, anotherRow);
	minToOne = leadingZero;
#elif __GNUC__
	minToOne = __builtin_clz(anotherRow);
#else
	int v = anotherRow;
	while (v >>= 1) {
		minToOne++;
	}
#endif
	return minToOne;
}