#ifndef MINISQL_LRU_REPLACER_H
#define MINISQL_LRU_REPLACER_H

#include <list>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "buffer/replacer.h"
#include "common/config.h"

using namespace std;

/**
 * LRUReplacer implements the Least Recently Used replacement policy.
 */
class LRUReplacer : public Replacer {
public:
  /**
   * Create a new LRUReplacer.
   * @param num_pages the maximum number of pages the LRUReplacer will be required to store
   */
  explicit LRUReplacer(size_t num_pages);

  /**
   * Destroys the LRUReplacer.
   */
  ~LRUReplacer() override;

  bool Victim(frame_id_t *frame_id) override;

  void Pin(frame_id_t frame_id) override;

  void Unpin(frame_id_t frame_id) override;

  size_t Size() override;

private:
  list<frame_id_t> lru_list;
  unordered_map<frame_id_t,list<frame_id_t>::iterator> hash_map;
  // add your own private member variables here
};

class ClockReplacer:public Replacer{
public:
  explicit ClockReplacer(size_t num_pages);

  ~ClockReplacer() override;

  bool Victim(frame_id_t *frame_id) override;

  void Pin(frame_id_t frame_id) override;

  void Unpin(frame_id_t frame_id) override;

  size_t Size() override;

private:
  // list<frame_id_t> clock_list;
  // unordered_map<frame_id_t,list<frame_id_t>::iterator> clock_map;
  // unordered_map<frame_id_t,bool> use_bit;
  // list<frame_id_t>::iterator iter;
  
  list<pair<frame_id_t, bool>> clock_list;
  list<pair<frame_id_t, bool>>::iterator clock_hand;
  unordered_map<frame_id_t, list<pair<frame_id_t, bool>>::iterator> clock_map;
};
#endif  // MINISQL_LRU_REPLACER_H
