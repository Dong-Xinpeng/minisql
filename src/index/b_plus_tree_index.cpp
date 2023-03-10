#include "index/b_plus_tree_index.h"
#include "index/generic_key.h"

INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_INDEX_TYPE::BPlusTreeIndex(index_id_t index_id, IndexSchema *key_schema,
                                     BufferPoolManager *buffer_pool_manager)
        : Index(index_id, key_schema),
          comparator_(key_schema_),
          container_(index_id, buffer_pool_manager, comparator_) {

}

INDEX_TEMPLATE_ARGUMENTS
dberr_t BPLUSTREE_INDEX_TYPE::InsertEntry(const Row &key, RowId row_id, Transaction *txn) {
  ASSERT(row_id.Get() != INVALID_ROWID.Get(), "Invalid row id for index insert.");
  KeyType index_key;
  index_key.SerializeFromKey(key, key_schema_);

  bool status = container_.Insert(index_key, row_id, txn);

  if (!status) {
    return DB_FAILED;
  }
  return DB_SUCCESS;
}

INDEX_TEMPLATE_ARGUMENTS
dberr_t BPLUSTREE_INDEX_TYPE::RemoveEntry(const Row &key, RowId row_id, Transaction *txn) {
  KeyType index_key;
  index_key.SerializeFromKey(key, key_schema_);

  container_.Remove(index_key, txn);
  return DB_SUCCESS;
}

INDEX_TEMPLATE_ARGUMENTS
dberr_t BPLUSTREE_INDEX_TYPE::ScanKey(const Row &key, vector<RowId> &result, Transaction *txn) {
  KeyType index_key;
  index_key.SerializeFromKey(key, key_schema_);
  if (container_.GetValue(index_key, result, txn)) {
    return DB_SUCCESS;
  }
  return DB_KEY_NOT_FOUND;
}

INDEX_TEMPLATE_ARGUMENTS
dberr_t BPLUSTREE_INDEX_TYPE::ScanKey(const Row &key, const int8_t compareType, vector<RowId> &result, Transaction *txn) {
  result.clear();
  if (compareType & 0b0001) {
    // ==
    this->ScanKey(key, result, nullptr);
    assert(result.size() <= 1);
  }else{
    assert(compareType & 0b0010);
    KeyType indexKey;
    indexKey.SerializeFromKey(key, key_schema_);
    if (compareType & 0b0100) {
      // > / >=
      auto it = this->GetBeginIterator(key);
      auto it_end = this->GetEndIterator();
      if ( !(compareType & 0b1000) && it->first == indexKey ) {
        // >
        ++it;
      }
      while (it != it_end) {
        result.push_back(it->second);
        ++it;
      }
    }else{
      // < / <=
      auto it = this->GetBeginIterator();
      auto it_mid = this->GetBeginIterator(key);
      while (it != it_mid) {
        result.push_back(it->second);
        ++it;
      }
      if (compareType & 0b1000 && it->first == indexKey) {
        // <=
        result.push_back(it->second);
        ++it;
      }
    }
  }
  return DB_SUCCESS;
}

INDEX_TEMPLATE_ARGUMENTS
dberr_t BPLUSTREE_INDEX_TYPE::Destroy() {
  container_.Destroy();
  return DB_SUCCESS;
}

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE BPLUSTREE_INDEX_TYPE::GetBeginIterator() {
  return container_.Begin();
}

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE
BPLUSTREE_INDEX_TYPE::GetBeginIterator(const KeyType &key) {
  return container_.Begin(key);
}

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE
BPLUSTREE_INDEX_TYPE::GetBeginIterator(const Row &key) {
  KeyType index_key;
  index_key.SerializeFromKey(key, key_schema_);
  return container_.Begin(index_key);
}

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE BPLUSTREE_INDEX_TYPE::GetEndIterator() {
  return container_.End();
}

template
class BPlusTreeIndex<GenericKey<4>, RowId, GenericComparator<4>>;

template
class BPlusTreeIndex<GenericKey<8>, RowId, GenericComparator<8>>;

template
class BPlusTreeIndex<GenericKey<16>, RowId, GenericComparator<16>>;

template
class BPlusTreeIndex<GenericKey<32>, RowId, GenericComparator<32>>;

template
class BPlusTreeIndex<GenericKey<64>, RowId, GenericComparator<64>>;