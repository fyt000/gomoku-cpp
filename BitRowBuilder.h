#pragma once

namespace gomoku {

// The bit string starts with a sentinel 1, represents the evaluated player's
// stones with 1, and represents empty cells with 0. Opponent stones delimit
// a row and are not encoded. For example: ___O_ -> 100010 and __O_ -> 10010.
class BitRowBuilder {
 public:
  BitRowBuilder();
  explicit BitRowBuilder(int row);

  [[nodiscard]] int getRow() const;
  void reset();
  BitRowBuilder& add(bool isEmpty);
  BitRowBuilder& add(int bit);
  [[nodiscard]] bool isSet(int index);
  void inplaceReverse();
  [[nodiscard]] int length();

  [[nodiscard]] static bool IsSet(int row, int index);
  [[nodiscard]] static int RowSplice(int row, int index);
  [[nodiscard]] static bool RowStartsWith(int row, int pattern);
  [[nodiscard]] static int GetReverse(int row);
  [[nodiscard]] static int LengthOf(int row);

 private:
  static constexpr int kEmptyRow = 1;

  int row = kEmptyRow;
  int len = 0;
  bool lenDirty = false;
};

}  // namespace gomoku
