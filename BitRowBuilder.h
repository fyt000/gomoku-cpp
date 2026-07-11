#pragma once

#include <cstdint>

namespace gomoku {

// The bit string starts with a sentinel 1, represents the evaluated player's
// stones with 1, and represents empty cells with 0. Opponent stones delimit
// a row and are not encoded. For example: ___O_ -> 100010 and __O_ -> 10010.
class BitRowBuilder {
 public:
  BitRowBuilder();
  explicit BitRowBuilder(std::uint16_t row);

  [[nodiscard]] std::uint16_t getRow() const;
  void reset();
  BitRowBuilder& appendCell(bool isSelfStone);
  BitRowBuilder& appendBit(int bit);
  [[nodiscard]] bool isSet(int index);
  void inplaceReverse();
  [[nodiscard]] int length();

  [[nodiscard]] static bool IsSet(std::uint16_t row, int index);
  [[nodiscard]] static std::uint16_t RowSplice(std::uint16_t row, int index);
  [[nodiscard]] static bool RowStartsWith(std::uint16_t row, std::uint16_t pattern);
  [[nodiscard]] static std::uint16_t GetReverse(std::uint16_t row);
  [[nodiscard]] static int LengthOf(std::uint16_t row);

 private:
  static constexpr std::uint16_t kEmptyRow = 1;

  std::uint16_t row = kEmptyRow;
  int len = 0;
  bool lenDirty = false;
};

}  // namespace gomoku
