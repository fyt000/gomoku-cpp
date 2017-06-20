#pragma once

enum TType : unsigned char { EXACT, UPPER, LOWER, USELESS };

class TTEntry {
public:
    int value;
    TType type;
    int depth;
    TTEntry():depth(-1),type(USELESS){}
    TTEntry(int value, TType type, int depth) : value(value),type(type),depth(depth){}
};