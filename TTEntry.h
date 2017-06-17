#pragma once

enum TType { EXACT, UPPER, LOWER };

class TTEntry {
public:
    int value;
    TType type;
    int depth;
    TTEntry(int value, TType type, int depth) : value(value),type(type),depth(depth){}
};