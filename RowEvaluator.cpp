#include <iostream>
#include "BitRowBuilder.h"
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

int maxPatLen = 0;

map<int, int> patternEvalMap1;
map<int, int> patternEvalMap2;

vector<int> rowEval1(1<<16,-1);
vector<int> rowEval2(1<<16,-1);

vector<int> initDP(int row) {
	int len = BitRowBuilder::LengthOf(row);
	vector<int> dp(len, -1);
	for (int i = 0; i < maxPatLen; i++) {
		dp.push_back(0);
	}
	return dp;
}

// optimize for a for loop version?
int rowDP(int row,vector<int>& dp,int idx,bool type) {
	int curMax = 0;
	BitRowBuilder rowBuilder(row);
	
	//assuming the shortest pattern has length 2
	if (rowBuilder.length() < 2) {
		dp[idx] = 0;
		return 0;
	}
	if (dp[idx] != -1)
		return dp[idx];

	if (!rowBuilder.isSet(0) && !rowBuilder.isSet(1)) {
		return rowDP(BitRowBuilder::RowSplice(row, 1), dp, idx + 1, type);
	}

	map<int, int>& evalMap = type ? patternEvalMap1 : patternEvalMap2;

	for (auto& kvp : evalMap) {
		int pattern = kvp.first;
		int patternEval = kvp.second;
		int patternLen = BitRowBuilder::LengthOf(pattern);
		if (BitRowBuilder::RowStartsWith(row, pattern)) {
			int curVal = patternEval +
				rowDP(BitRowBuilder::RowSplice(row, patternLen), dp, idx + patternLen, type);
			curMax = max(curVal, curMax);
		}
		int reversePattern = BitRowBuilder::GetReverse(pattern);
		if (BitRowBuilder::RowStartsWith(row, reversePattern)) {
			int curVal = patternEval +
				rowDP(BitRowBuilder::RowSplice(row, patternLen), dp, idx + patternLen, type);
			curMax = max(curVal, curMax);
		}
	}

	int curVal = rowDP(BitRowBuilder::RowSplice(row, 1), dp, idx + 1, type);
	curMax = max(curVal, curMax);

	dp[idx] = curMax;

	return curMax;
}



int genEvalForRow(int row, bool type) {

#define SAVERET(r) {type?rowEval1[row]=r:rowEval2[row]=r; return r;}

	if (row <= (1 << 5)) {
		SAVERET(0);
	}
	for (int i = 1; i < 17; i++) {
		if (row == (1 << i)){
			SAVERET(0);
		}
	}

	//reverse row has the same eval
	int revRow = BitRowBuilder::GetReverse(row);
	if (type) {
		if (rowEval1[revRow] != -1) {
			SAVERET(rowEval1[revRow]);
		}
	}
	if (!type) {
		if (rowEval2[revRow] != -1) {
			SAVERET(rowEval2[revRow]);
		}
	}

	auto forwardDP = initDP(row);
	auto backwardDP = initDP(revRow);

	int forwardEval = rowDP(row, forwardDP, 0, type);
	int backwardEval = rowDP(revRow, backwardDP, 0, type);

	int ret = max(forwardEval, backwardEval);

	SAVERET(ret);
}

void dump_result(string filename, vector<int> result) {
	ofstream fout(filename.c_str());
	for (auto item : result) {
		fout << item << endl;
	}
}


int main(int argc, char** argv) {
	
	if (argc != 4) {
		cerr << "pass in the path to pattern file and paths to place the dumps" << endl;
		return 0;
	}
	cout << "Evaluating..." << endl;
	ifstream fin(argv[1]);
	string pattern;
	int eval1;
	int eval2;
	string line;
	while (getline(fin,line)) {
		stringstream ss;
		ss << line;
		ss >> pattern;
		ss >> eval1;
		ss >> eval2;

		//add 1 so leading 0 don't get thrown away
		//because they do matter
		pattern = "1" + pattern;

		maxPatLen = max(maxPatLen, (int)pattern.size() - 1);

		int intPat = std::stoi(pattern, nullptr, 2);
		patternEvalMap1[intPat]= eval1;
		patternEvalMap2[intPat]= eval2;
	}

//	genEvalForRow(0b110011, true);
//	return 0;

	for (int i = 0; i < (1 << 16); i++) {
		genEvalForRow(i, true);
		genEvalForRow(i, false);
		//cout << i << " " << genEvalForRow(i, true) << " " << genEvalForRow(i, false) << endl;
	}

	dump_result(argv[2], rowEval1);
	dump_result(argv[3], rowEval2);

	cout << "Done" << endl;

	cin.get();
}