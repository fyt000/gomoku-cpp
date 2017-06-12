#pragma once

//to get the length of the row
//the bit string starts with '1' as empty
// eg. ___O_ -> 100010 
// __O_ -> 10010
class BitRowBuilder {
public:
	BitRowBuilder();
	BitRowBuilder(int r);
	int getRow();
	void reset();
	BitRowBuilder& add(bool isEmpty);
	BitRowBuilder& add(int b);
	bool isSet(int idx);
	void inplaceReverse();
	int length();


	static bool IsSet(int someRow, int idx);
	static int RowSplice(int someRow, int idx);
	static bool RowStartsWith(int someRow, int p);
	static int GetReverse(int someRow);
	static int LengthOf(int anotherRow);

	

private:
	static const int EMPTYROW = 1;
	int row;
	int len;
	bool lenDirty = false;
};