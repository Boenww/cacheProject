//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

//
// TODO:Student Information
//
const char *studentName = "Bowen Zhang";
const char *studentID   = "";
const char *email       = "";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory in cycles

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties in cycles

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

//
//TODO: Add your Cache data structures here
//
Cache *icache, *dcache, *l2cache;

//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
Cache *
init_general_cache(uint32_t cacheSets, uint32_t cacheAssoc, uint32_t hitTime, uint8_t hierarchy)
{
    Cache *cache = malloc(sizeof(Cache));
    cache->setNum = cacheSets;
    cache->assoc = cacheAssoc;
    cache->hitTime = hitTime;
    cache->blocksize = blocksize;
    cache->blocks = malloc(cacheSets * sizeof(Block *));
    cache->hierarchy = hierarchy;
    
    for (int i = 0; i < cacheSets; i++) {
        cache->blocks[i] = malloc(cacheAssoc * sizeof(Block));
        for (int j = 0; j < cacheAssoc; j++) {
            cache->blocks[i][j].valid = FALSE;
            cache->blocks[i][j].lru = j;
            cache->blocks[i][j].tag = 0;
            cache->blocks[i][j].index = 0;
            cache->blocks[i][j].offset = 0;
        }
    }
    
    return cache;
}

void
init_cache()
{
    // Initialize cache stats
    icacheRefs        = 0;
    icacheMisses      = 0;
    icachePenalties   = 0;
    dcacheRefs        = 0;
    dcacheMisses      = 0;
    dcachePenalties   = 0;
    l2cacheRefs       = 0;
    l2cacheMisses     = 0;
    l2cachePenalties  = 0;
    
    //
    //TODO: Initialize Cache Simulator Data Structures
    //
    if (icacheSets) {icache = init_general_cache(icacheSets, icacheAssoc, icacheHitTime, L1CACHE);}
    if (dcacheSets) {dcache = init_general_cache(dcacheSets, dcacheAssoc, dcacheHitTime, L1CACHE);}
    if (l2cacheSets) {l2cache = init_general_cache(l2cacheSets, l2cacheAssoc, l2cacheHitTime, L2CACHE);}
}

uint32_t
get_index(uint32_t addr, Cache cache)
{
    uint8_t offsetBits = log2(cache.blocksize);
    uint8_t indexBits = log2(cache.setNum);
    return (addr >> offsetBits) & ((1 << indexBits) - 1);
}

uint32_t
get_tag(uint32_t addr, Cache cache)
{
    uint8_t offsetBits = log2(cache.blocksize);
    uint8_t indexBits = log2(cache.setNum);
    uint8_t tagBits = 32 - indexBits - offsetBits;
    return (addr >> (32 - tagBits)) & ((1 << tagBits) - 1);
}

uint32_t
get_addr(uint32_t setIndex, uint32_t blockIndex, Cache cache)
{
    uint8_t offsetBits = log2(cache.blocksize);
    uint8_t indexBits = log2(cache.setNum);
    return (cache.blocks[setIndex][blockIndex].tag << (indexBits + offsetBits)) | (cache.blocks[setIndex][blockIndex].index << offsetBits) | cache.blocks[setIndex][blockIndex].offset;
}

uint8_t
hit_miss(uint32_t addr, Cache *cache)
{
    uint32_t index = get_index(addr, *cache);
    uint32_t tag = get_tag(addr, *cache);
    uint8_t is_hit = FALSE;
    uint32_t hitIndex;
    
    for (int i = 0; i < cache->assoc; i++) {
        if (cache->blocks[index][i].valid == TRUE && cache->blocks[index][i].tag == tag) {
            hitIndex = i;
            is_hit = TRUE;
            break;
        }
    }

    if (is_hit) {
        for (int j = 0; j < cache->assoc; j++) {
            if (cache->blocks[index][j].valid == TRUE && cache->blocks[index][j].lru < cache->blocks[index][hitIndex].lru) {
                cache->blocks[index][j].lru++;
            }
        }
        cache->blocks[index][hitIndex].lru = 0;
    }
    return is_hit;
}

void
inclusive_helper(uint32_t addr, Cache *cache)
{
    uint32_t index = get_index(addr, *cache);
    uint32_t tag = get_tag(addr, *cache);
    
    for (int j = 0; j < cache->assoc; j++) {
        if (cache->blocks[index][j].tag == tag) {
            cache->blocks[index][j].valid = FALSE;
            return;
        }
    }
}

void
update_lru(uint32_t addr, Cache *cache)
{
    uint32_t index = get_index(addr, *cache);
    uint32_t tag = get_tag(addr, *cache);
    uint32_t replaceIndex;
    
    for (int i = 0; i < cache->assoc; i++) {
        if (cache->blocks[index][i].valid == FALSE) { //empty first
            replaceIndex = i;
            break;
        }
        if (cache->blocks[index][i].lru == cache->assoc - 1) {
            replaceIndex = i;
            break;
        }
    }
    
    for (int j = 0; j < cache->assoc; j++) {
        if (cache->blocks[index][j].lru < cache->assoc - 1) {
            cache->blocks[index][j].lru++;
        }
    }
    
    if (cache->hierarchy == L2CACHE && inclusive && cache->blocks[index][replaceIndex].valid) {
        uint32_t replaceAddr = get_addr(index, replaceIndex, *cache);
        inclusive_helper(replaceAddr, icache);
        inclusive_helper(replaceAddr, dcache);
    }
    
    cache->blocks[index][replaceIndex].valid = TRUE;
    cache->blocks[index][replaceIndex].tag = tag;
    cache->blocks[index][replaceIndex].index = index;
    cache->blocks[index][replaceIndex].lru = 0;
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
    //
    //TODO: Implement I$
    //
    if (icacheSets == 0) {
        return l2cache_access(addr, ICACHE);
    } else {
        icacheRefs++;
    }
    
    //Hit & Miss
    if (hit_miss(addr, icache)) {
        return icache->hitTime;
    } else {
        icacheMisses++;
        uint64_t penalty = l2cache_access(addr, ICACHE);
        icachePenalties += penalty;
        return icache->hitTime + penalty;
    }

}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
    //
    //TODO: Implement D$
    //
    if (dcacheSets == 0) {
        return l2cache_access(addr, DCACHE);
    } else {
        dcacheRefs++;
    }
    
    if (hit_miss(addr, dcache)) {
        return dcache->hitTime;
    } else {
        dcacheMisses++;
        uint64_t penalty = l2cache_access(addr, DCACHE);
        dcachePenalties += penalty;
        return dcache->hitTime + penalty;
    }
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr, uint8_t i_or_d)
{
    //
    //TODO: Implement L2$
    //
    if (l2cacheSets == 0) {
        return memspeed;
    } else {
        l2cacheRefs++;
    }
    
    if (hit_miss(addr, l2cache)) {
        if (!i_or_d && icacheSets) {
            update_lru(addr, icache);
        } else if (i_or_d && dcacheSets) {
            update_lru(addr, dcache);
        }
        return l2cache->hitTime;
    } else {
        update_lru(addr, l2cache);
        
        if (!i_or_d && icacheSets) {
            update_lru(addr, icache);
        } else if (i_or_d && dcacheSets) {
            update_lru(addr, dcache);
        }
        
        l2cacheMisses++;
        l2cachePenalties += memspeed;
        return l2cache->hitTime + memspeed;
    }
}

void
free_cache()
{
    free(icache);
    free(dcache);
    free(l2cache);
}
