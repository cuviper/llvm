//===-- BitVectorSet.h - A bit-vector representation of sets -----*- C++ -*--=//
//
// class BitVectorSet --
// 
// An implementation of the bit-vector representation of sets.
// Unlike vector<bool>, this allows much more efficient parallel set
// operations on bits, by using the bitset template .  The bitset template
// unfortunately can only represent sets with a size chosen at compile-time.
// We therefore use a vector of bitsets.  The maxmimum size of our sets
// (i.e., the size of the universal set) can be chosen at creation time.
//
// The size of each Bitset is defined by the macro WORDSIZE.
// 
// NOTE: The WORDSIZE macro should be made machine-dependent, in order to use
// 64-bit words or whatever gives most efficient Bitsets on each platform.
// 
// 
// External functions:
// 
// bool Disjoint(const BitSetVector& set1, const BitSetVector& set2):
//    Tests if two sets have an empty intersection.
//    This is more efficient than !(set1 & set2).any().
// 
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_BITVECTORSET_H
#define LLVM_SUPPORT_BITVECTORSET_H

#include <bitset>
#include <vector>
#include <functional>
#include <iostream>


#define WORDSIZE (32U)


class BitSetVector {
  typedef std::bitset<WORDSIZE> bitword;
  typedef bitword::reference reference;
  class iterator;

  std::vector<bitword> bitsetVec;
  unsigned maxSize;

  static unsigned NumWords(unsigned Size) { return (Size+WORDSIZE-1)/WORDSIZE;} 

  const bitword& getWord(unsigned i) const { return bitsetVec[i]; }
        bitword& getWord(unsigned i)       { return bitsetVec[i]; }

  friend bool Disjoint(const BitSetVector& set1,
                       const BitSetVector& set2);

  BitSetVector();                       // do not implement!

public:
  /// 
  /// Constructor: create a set of the maximum size maxSetSize.
  /// The set is initialized to empty.
  ///
  BitSetVector(unsigned maxSetSize)
    : bitsetVec(NumWords(maxSetSize)), maxSize(maxSetSize) { }

  /// size - Return the number of bits tracked by this bit vector...
  unsigned size() const { return maxSize; }

  /// 
  ///  Modifier methods: reset, set for entire set, operator[] for one element.
  ///  
  void reset() {
    for(std::vector<bitword>::iterator I=bitsetVec.begin(), E=bitsetVec.end();
        I != E; ++I)
      I->reset();
  }
  void set() {
    for(std::vector<bitword>::iterator I=bitsetVec.begin(), E=bitsetVec.end();
        I != E; ++I)
      I->set();
  }
  std::bitset<32>::reference operator[](unsigned n) {
    unsigned ndiv = n / WORDSIZE, nmod = n % WORDSIZE;
    assert(ndiv  < bitsetVec.size() && "BitSetVector: Bit number out of range");
    return bitsetVec[ndiv][nmod];
  }
  iterator begin() { return iterator::begin(*this); }
  iterator end()   { return iterator::end(*this);   } 

  /// 
  ///  Set membership operations: single element, any, none, count
  ///  
  bool test(unsigned n) const {
    unsigned ndiv = n / WORDSIZE, nmod = n % WORDSIZE;
    assert(ndiv  < bitsetVec.size() && "BitSetVector: Bit number out of range");
    return bitsetVec[ndiv].test(nmod);
  }
  bool any() const {
    for (unsigned i = 0; i < bitsetVec.size(); ++i)
      if (bitsetVec[i].any())
        return true;
    return false;
  }
  bool none() const {
    return ! any();
  }
  unsigned count() const {
    unsigned n = 0;
    for (unsigned i = 0; i < bitsetVec.size(); ++i)
      n += bitsetVec[i].count();
    return n;
  }

  /// 
  ///  Set operations: intersection, union, disjoint union, complement.
  ///  
  BitSetVector operator& (const BitSetVector& set2) const {
    assert(maxSize == set2.maxSize && "Illegal intersection");
    BitSetVector result(maxSize);
    for (unsigned i = 0; i < bitsetVec.size(); ++i)
      result.getWord(i) = getWord(i) & set2.getWord(i);
    return result;
  }
  BitSetVector operator| (const BitSetVector& set2) const {
    assert(maxSize == set2.maxSize && "Illegal intersection");
    BitSetVector result(maxSize);
    for (unsigned i = 0; i < bitsetVec.size(); ++i)
      result.getWord(i) = getWord(i) | set2.getWord(i);
    return result;
  }
  BitSetVector operator^ (const BitSetVector& set2) const {
    assert(maxSize == set2.maxSize && "Illegal intersection");
    BitSetVector result(maxSize);
    for (unsigned i = 0; i < bitsetVec.size(); ++i)
      result.getWord(i) = getWord(i) ^ set2.getWord(i);
    return result;
  }
  BitSetVector operator~ () const {
    BitSetVector result(maxSize);
    for (unsigned i = 0; i < bitsetVec.size(); ++i)
      (result.getWord(i) = getWord(i)).flip();
    return result;
  }

  /// 
  ///  Printing and debugging support
  ///  
  void print(std::ostream &O) const;
  void dump() const { print(std::cerr); }

public:
  // 
  // An iterator to enumerate the bits in a BitSetVector.
  // Eventually, this needs to inherit from bidirectional_iterator.
  // But this iterator may not be as useful as I once thought and
  // may just go away.
  // 
  class iterator {
    unsigned   currentBit;
    unsigned   currentWord;
    BitSetVector& bitvec;
    iterator(unsigned B, unsigned W, BitSetVector _bitvec)
      : currentBit(B), currentWord(W), bitvec(_bitvec) { }
  public:
    iterator(BitSetVector& _bitvec)
      : currentBit(0), currentWord(0), bitvec(_bitvec) { }
    iterator(const iterator& I)
      : currentBit(I.currentBit),currentWord(I.currentWord),bitvec(I.bitvec) { }
    iterator& operator=(const iterator& I) {
      currentWord == I.currentWord;
      currentBit == I.currentBit;
      bitvec = I.bitvec;
      return *this;
    }

    // Increment and decrement operators (pre and post)
    iterator& operator++() {
      if (++currentBit == WORDSIZE)
        { currentBit = 0; if (currentWord < bitvec.maxSize) ++currentWord; }
      return *this;
    }
    iterator& operator--() {
      if (currentBit == 0) {
        currentBit = WORDSIZE-1;
        currentWord = (currentWord == 0)? bitvec.maxSize : --currentWord;
      }
      else
        --currentBit;
      return *this;
    }
    iterator operator++(int) { iterator copy(*this); ++*this; return copy; }
    iterator operator--(int) { iterator copy(*this); --*this; return copy; }

    // Dereferencing operators
    reference operator*() {
      assert(currentWord < bitvec.maxSize &&
             "Dereferencing iterator past the end of a BitSetVector");
      return bitvec.getWord(currentWord)[currentBit];
    }

    // Comparison operator
    bool operator==(const iterator& I) {
      return (&I.bitvec == &bitvec &&
              I.currentWord == currentWord && I.currentBit == currentBit);
    }

  protected:
    static iterator begin(BitSetVector& _bitvec) { return iterator(_bitvec); }
    static iterator end(BitSetVector& _bitvec)   { return iterator(0,
                                                    _bitvec.maxSize, _bitvec); }
    friend class BitSetVector;
  };
};


inline void BitSetVector::print(std::ostream& O) const
{
  for (std::vector<bitword>::const_iterator
         I=bitsetVec.begin(), E=bitsetVec.end(); I != E; ++I)
    O << "<" << (*I) << ">" << (I+1 == E? "\n" : ", ");
}

inline std::ostream& operator<< (std::ostream& O, const BitSetVector& bset)
{
  bset.print(O);
  return O;
};


///
/// Optimized versions of fundamental comparison operations
/// 
inline bool Disjoint(const BitSetVector& set1,
                     const BitSetVector& set2)
{
  assert(set1.size() == set2.size() && "Illegal intersection");
  for (unsigned i = 0; i < set1.bitsetVec.size(); ++i)
    if ((set1.getWord(i) & set2.getWord(i)).any())
      return false;
  return true;
}

#endif
