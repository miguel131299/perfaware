#pragma once

#include "common/types.hpp"
#include <cstdio>

// ============================================================================
// Page size enum
// ============================================================================

enum class PageSize {
  Size4K = 12,
  Size2M = 21,
  Size1G = 30,
};

// ============================================================================
// Generic utilities (reusable across page sizes)
// ============================================================================

// Get page number given address and page shift (e.g., 12 for 4K, 21 for 2M, 30
// for 1G)
template <int PageShift> inline u64 getPageNumber(u64 addr) {
  return addr >> PageShift;
}

// Get base address of a page given page shift
template <int PageShift> inline u64 getPageBase(u64 addr) {
  u64 mask = (1ULL << PageShift) - 1;
  return addr & ~mask;
}

// Check if two addresses are in the same page
template <int PageShift> inline bool samePage(u64 addr1, u64 addr2) {
  return getPageBase<PageShift>(addr1) == getPageBase<PageShift>(addr2);
}

// Extract a field from address given shift and mask
inline u64 extractField(u64 addr, int shift, u64 mask) {
  return (addr >> shift) & mask;
}

// ============================================================================
// Generic page decomposition struct (works for all page sizes)
// ============================================================================

struct PageDecompose {
  u16 pml4;   // Page Map Level 4 (9 bits)
  u16 pdpt;   // Page Directory Pointer Table (9 bits)
  u16 pd;     // Page Directory (9 bits)
  u16 pt;     // Page Table (9 bits) - only used for 4K pages
  u32 offset; // Page Offset (12, 21, or 30 bits depending on page size)
};

// Decompose a 4KB page address
inline PageDecompose decompose4K(u64 addr) {
  return {
      (u16)extractField(addr, 39, 0x1ff), (u16)extractField(addr, 30, 0x1ff),
      (u16)extractField(addr, 21, 0x1ff), (u16)extractField(addr, 12, 0x1ff),
      (u32)extractField(addr, 0, 0xfff)};
}

// Decompose a 2MB page address
inline PageDecompose decompose2M(u64 addr) {
  return {(u16)extractField(addr, 39, 0x1ff),
          (u16)extractField(addr, 30, 0x1ff),
          (u16)extractField(addr, 21, 0x1ff),
          0, // Not used for 2M pages
          (u32)extractField(addr, 0, 0x1fffff)};
}

// Decompose a 1GB page address
inline PageDecompose decompose1G(u64 addr) {
  return {(u16)extractField(addr, 39, 0x1ff),
          (u16)extractField(addr, 30, 0x1ff),
          0, // Not used for 1G pages
          0, // Not used for 1G pages
          (u32)extractField(addr, 0, 0x3fffffff)};
}

// ============================================================================
// Utility functions
// ============================================================================

// Convenience aliases for common 4K operations
inline u64 getPageNumber4K(u64 addr) { return getPageNumber<12>(addr); }

inline u64 getPageBase4K(u64 addr) { return getPageBase<12>(addr); }

// Convenience aliases for common 2M operations
inline u64 getPageNumber2M(u64 addr) { return getPageNumber<21>(addr); }

inline u64 getPageBase2M(u64 addr) { return getPageBase<21>(addr); }

// Convenience aliases for common 1G operations
inline u64 getPageNumber1G(u64 addr) { return getPageNumber<30>(addr); }

inline u64 getPageBase1G(u64 addr) { return getPageBase<30>(addr); }

// Check if two addresses are in the same page (convenience functions)
inline bool samePageL3(u64 addr1, u64 addr2) {
  return samePage<12>(addr1, addr2);
}

inline bool samePageL2(u64 addr1, u64 addr2) {
  return samePage<21>(addr1, addr2);
}

inline bool samePageL1(u64 addr1, u64 addr2) {
  return samePage<30>(addr1, addr2);
}

// ============================================================================
// Print functions (table format)
// ============================================================================

inline void printAddressTableHeader() {
  printf("|Size| PML| PDT|  PD|  PT|       Offset|\n");
}

// Print a single address in table format
inline void printAddress(u64 addr, PageSize pageSize) {
  if (pageSize == PageSize::Size4K) {
    PageDecompose d = decompose4K(addr);
    printf("|4K  |%4u|%4u|%4u|%4u|%13u|\n", d.pml4, d.pdpt, d.pd, d.pt,
           d.offset);
  } else if (pageSize == PageSize::Size2M) {
    PageDecompose d = decompose2M(addr);
    printf("|2M  |%4u|%4u|%4u|    |%13u|\n", d.pml4, d.pdpt, d.pd, d.offset);
  } else if (pageSize == PageSize::Size1G) {
    PageDecompose d = decompose1G(addr);
    printf("|1G  |%4u|%4u|    |    |%13u|\n", d.pml4, d.pdpt, d.offset);
  }
}

// Print all three page sizes for a single address
inline void printAddressAll(u64 addr) {
  printAddress(addr, PageSize::Size4K);
  printAddress(addr, PageSize::Size2M);
  printAddress(addr, PageSize::Size1G);
}
