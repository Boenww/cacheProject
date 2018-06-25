//========================================================//
//  cache.h                                               //
//  Header file for the Cache Simulator                   //
//                                                        //
//  Includes function prototypes and global variables     //
//  and defines for the cache simulator                   //
//========================================================//

#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include <stdlib.h>

//
// Student Information
//
extern const char *studentName;
extern const char *studentID;
extern const char *email;

//------------------------------------//
//          Global Defines            //
//------------------------------------//

#define TRUE 1
#define FALSE 0

#define ICACHE 0
#define DCACHE 1

#define L1CACHE 0
#define L2CACHE 1

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

extern uint32_t icacheSets;     // Number of sets in the I$
extern uint32_t icacheAssoc;    // Associativity of the I$
extern uint32_t icacheHitTime;  // Hit Time of the I$

extern uint32_t dcacheSets;     // Number of sets in the D$
extern uint32_t dcacheAssoc;    // Associativity of the D$
extern uint32_t dcacheHitTime;  // Hit Time of the D$

extern uint32_t l2cacheSets;    // Number of sets in the L2$
extern uint32_t l2cacheAssoc;   // Associativity of the L2$
extern uint32_t l2cacheHitTime; // Hit Time of the L2$
extern uint32_t inclusive;      // Indicates if the L2 is inclusive

extern uint32_t blocksize;      // Block/Line size
extern uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

extern uint64_t icacheRefs;       // I$ references
extern uint64_t icacheMisses;     // I$ misses
extern uint64_t icachePenalties;  // I$ penalties

extern uint64_t dcacheRefs;       // D$ references
extern uint64_t dcacheMisses;     // D$ misses
extern uint64_t dcachePenalties;  // D$ penalties

extern uint64_t l2cacheRefs;      // L2$ references
extern uint64_t l2cacheMisses;    // L2$ misses
extern uint64_t l2cachePenalties; // L2$ penalties

typedef struct Block_t {
    uint8_t valid;
    uint32_t lru;
    uint32_t tag;
    uint32_t index;
    uint32_t offset;
} Block;

typedef struct Cache_t {
    uint32_t setNum;
    uint32_t assoc;
    uint32_t hitTime;
    uint32_t blocksize;
    Block **blocks;
    uint8_t hierarchy;
} Cache;
//------------------------------------//
//      Cache Function Prototypes     //
//------------------------------------//

// Initialize the predictor
//
Cache *init_general_cache(uint32_t cacheSets, uint32_t cacheAssoc, uint32_t hitTime, uint8_t hierarchy);
void init_cache(void);

uint32_t get_index(uint32_t addr, Cache cache);
uint32_t get_tag(uint32_t addr, Cache cache);
uint32_t get_addr(uint32_t setIndex, uint32_t blockIndex, Cache cache);

uint8_t hit_miss(uint32_t addr, Cache *cache);
void inclusive_helper(uint32_t addr, Cache *cache);
void update_lru(uint32_t addr, Cache *cache);

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t icache_access(uint32_t addr);

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t dcache_access(uint32_t addr);

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t l2cache_access(uint32_t addr, uint8_t l1);

// Free memory
//
void free_cache(void);

#endif
