#include <algorithm>

template<unsigned bitsize>
Row<bitsize>::Row(BitVector x, BitVector y, bool rhs) : x(x), y(y), rhs(rhs) {
}

template<unsigned bitsize>
Row<bitsize> Row<bitsize>::GetPivotRow() {
  if (x) {
    BitVector xp = x;
    xp |= xp >> 1;
#if (bitsize > 2)
    xp |= xp >> 2;
    xp |= xp >> 4;
    xp |= xp >> 8;
    xp |= xp >> 16;
    xp |= xp >> 32;
#endif
    return Row<bitsize>(xp-(xp>>1), 0, 0);
  } else if (y) {
    BitVector yp = y;
    yp |= yp >> 1;
#if (bitsize > 2)
    yp |= yp >> 2;
    yp |= yp >> 4;
    yp |= yp >> 8;
    yp |= yp >> 16;
    yp |= yp >> 32;
#endif
    return Row<bitsize>(0, yp-(yp>>1), 0);
  } else {
    return Row<bitsize>(0, 0, rhs);
  }
}

template <unsigned bitsize>
bool Row<bitsize>::IsContradiction() {
  return !x && !y && rhs;
}

template <unsigned bitsize>
bool Row<bitsize>::IsEmpty() {
  return !x && !y && !rhs;
} 

template <unsigned bitsize>
bool Row<bitsize>::IsXSingleton() {
  return (x & (x-1)) == 0 && !y;
}

template <unsigned bitsize>
bool Row<bitsize>::IsYSingleton() {
  return (y & (y-1)) == 0 && !x;
}

template<unsigned bitsize>
bool Row<bitsize>::CommonVariableWith(const Row<bitsize>& other) {
  return (x & other.x) || (y & other.y);
}

template<unsigned bitsize>
Row<bitsize> operator^(const Row<bitsize>& left, const Row<bitsize>& right) {
  return Row<bitsize>(left.x ^ right.x, left.y ^ right.y, left.rhs ^ right.rhs);
}

template<unsigned bitsize>
Row<bitsize>& Row<bitsize>::operator^=(const Row<bitsize>& right) {
  x ^= right.x;
  y ^= right.y;
  rhs ^= right.rhs;
  return *this;
}
 
template<unsigned bitsize>
Row<bitsize> operator&(const Row<bitsize>& left, const Row<bitsize>& right) {
  return Row<bitsize>(left.x & right.x, left.y & right.y, left.rhs & right.rhs);
}

template<unsigned bitsize>
Row<bitsize>& Row<bitsize>::operator&=(const Row<bitsize>& right) {
  x &= right.x;
  y &= right.y;
  rhs &= right.rhs;
  return *this;
}

template<unsigned bitsize>
Row<bitsize> operator|(const Row<bitsize>& left, const Row<bitsize>& right) {
  return Row<bitsize>(left.x | right.x, left.y | right.y, left.rhs | right.rhs);
}

template<unsigned bitsize>
Row<bitsize>& Row<bitsize>::operator|=(const Row<bitsize>& right) {
  x |= right.x;
  y |= right.y;
  rhs |= right.rhs;
  return *this;
}

template<unsigned bitsize>
bool operator==(const Row<bitsize>& left, const Row<bitsize>& right) {
  return left.x == right.x && left.y == right.y && left.rhs == right.rhs;
}

template<unsigned bitsize>
std::ostream& operator<<(std::ostream& stream, const Row<bitsize>& row) {
  // prints in expected order (but not in memory order)
  //for (int xshift = (int)row.bitsize - 1; xshift >= 0; --xshift)
  for (unsigned xshift = 0; xshift < bitsize; ++xshift)
    stream << ((row.x >> xshift) & 1) << " ";
  stream << " ";
  //for (int yshift = (int)row.bitsize - 1; yshift >= 0; --yshift)
  for (unsigned yshift = 0; yshift < bitsize; ++yshift)
    stream << ((row.y >> yshift) & 1) << " ";
  stream << " " << row.rhs;
  return stream;
}

//-----------------------------------------------------------------------------

template<unsigned bitsize>
LinearStep<bitsize>::LinearStep() {
}

template<unsigned bitsize>
LinearStep<bitsize>::LinearStep(std::function<BitVector(BitVector)> fun) {
  Initialize(fun);
}

template<unsigned bitsize>
void LinearStep<bitsize>::Initialize(std::function<BitVector(BitVector)> fun) {
  rows.clear();
  rows.reserve(bitsize);
  for (unsigned i = 0; i < bitsize; ++i)
    rows.emplace_back(1ULL << i, fun(1ULL << i), 0); // lower triangle version
}

template<unsigned bitsize>
bool LinearStep<bitsize>::AddMasks(Mask& x, Mask& y) {
  BitVector care = x.caremask.care;
  BitVector pat = 1;
  for (unsigned xshift = 0; xshift < bitsize; ++xshift, pat <<= 1) {
    if (!(care & ((~0ULL) << xshift)))
      break;
    if (care & pat)
      if (!AddRow(Row<bitsize>(pat, 0ULL, (pat & x.caremask.canbe1) != 0)))
        return false;
  }
  care = y.caremask.care;
  pat = 1;
  for (unsigned yshift = 0; yshift < bitsize; ++yshift, pat <<= 1) {
    if (!(care & ((~0ULL) << yshift)))
      break;
    if (care & pat)
      if (!AddRow(Row<bitsize>(0ULL, pat, (pat & y.caremask.canbe1) != 0)))
        return false;
  }
  return true;
}

template<unsigned bitsize>
bool LinearStep<bitsize>::AddRow(const Row<bitsize>& row) {
  // assumes that only one variable is set!!
  for (Row<bitsize>& other : rows) { // maybe optimize via pivots
    if (other.CommonVariableWith(row)) {
      other ^= row;
      if (other.IsContradiction()) {
        return false;
      } else if (other.IsEmpty()) {
        other = rows.back();
        rows.pop_back();
        return true;
      } else {
        Row<bitsize> pivotrow = other.GetPivotRow();
        for (Row<bitsize>& third : rows)
          if (&third != &other && third.CommonVariableWith(pivotrow))
            third ^= other;
      }
    }
  }
  return true;
}

template<unsigned bitsize>
bool LinearStep<bitsize>::ExtractMasks(Mask& x, Mask& y) {
  // deletes information from system!!
  for (Row<bitsize>& row : rows) {
    if (row.IsXSingleton()) {
      if (x.caremask.care & row.x) {
        if (((x.caremask.canbe1 & row.x) != 0) != row.rhs)
          return false;
      } else {
        x.caremask.care |= row.x;
        x.caremask.canbe1 &= (~0ULL ^ (row.x * (BitVector)(1-row.rhs)));
      }
      row = rows.back();
      rows.pop_back();
    } else if (row.IsYSingleton()) {
      if (y.caremask.care & row.y) {
        if (((y.caremask.canbe1 & row.y) != 0) != row.rhs)
          return false;
      } else {
        y.caremask.care |= row.y;
        y.caremask.canbe1 &= (~0ULL ^ (row.y * (BitVector)(1-row.rhs)));
      }
      row = rows.back();
      rows.pop_back();
    }
  }
  x.init_bitmasks();
  y.init_bitmasks();
  return true;
}

template<unsigned bitsize>
std::ostream& operator<<(std::ostream& stream, const LinearStep<bitsize>& sys) {
  for (const Row<bitsize>& row : sys.rows)
    stream << row << std::endl;
  return stream;
}

template <unsigned bitsize>
bool LinearStep<bitsize>::Update(Mask& x, Mask& y) {
  if (AddMasks(x, y))
    return ExtractMasks(x, y);
  return false;
}

